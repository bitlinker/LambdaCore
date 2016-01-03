#include <cassert>
#include <limits>
#include <BSPRender.h>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>
#include <Render/AABB.h>
#include <Render/VertexArrayObject.h>
#include <Render/BufferObject.h>
#include <Render/ScopeBind.h>
#include <BSPShaderProgram.h>
#include <Logger/Log.h>

using namespace Commons;
using namespace Commons::Render;

namespace LambdaCore
{  
    static const int32_t LIGHTMAP_SAMPLE_SIZE = 16;

    static const uint32_t TEXTURE_SAMPLER_DIFFUSE = 0;
    static const uint32_t TEXTURE_SAMPLER_LIGHTMAP = 1;

    static const glm::mat4 STATIC_MAP_TRANSFORM = glm::rotate(90.F, -1.F, 0.F, 0.F);

    // TODO: animation - in render
    /*if it begins by * it will be animated like lava or water.
    if it begins by + then it will be animated with frames, and the second letter of the name will be the frame number.Those numbers begin at 0, and go up to 9 at the maximum.
    if if begins with sky if will double scroll like a sky.
    Beware that sky textures are made of two distinct parts.*/

    BSPRender::BSPRender(const BSPMapPtr& map, const Commons::Render::SharedTextureMgrPtr& textureManager)
        : mLightmapMgr(512, 64, 1, LightmapMgr::InterpolationNONE, 1.F)
        , mMap(map)
        , mTexMgr(textureManager) // TODO: common
        , mVao()
        , mVertexVBO(GL_ARRAY_BUFFER)
        , mVisLeafs()
        , mVisFaces(map->mFaces.size())
        , mVertIndices()
        , mFaceBatches()
        , mShader()
        , mTextures()
        , mFaceData()
        // TODO: init members
    {
        // Preinit vis leaves
        mVisLeafs.resize(map->mLeafs.size());

        // Load textures:
        // TODO: func
       

        // Calc face data:
        // TODO: func
        

        initFaceData();
        initVBOs();
        initTextures();
        initLightmaps();

        // TODO: func, in map?
        mModelData.resize(mMap->mModels.size());        
        mModelData[0].mIsVisible = true;
        mModelData[1].mIsVisible = true;
    }

    void BSPRender::initVBOs()
    {
        mVertexVBO.setData(sizeof(glm::vec3) * mMap->mVertices.size(), &mMap->mVertices[0], GL_STATIC_DRAW);
    }

    void BSPRender::initTextures()
    {
        auto it = mMap->mMipTextures.begin();
        const auto itEnd = mMap->mMipTextures.end();
        mTextures.reserve(mMap->mMipTextures.size());
        for (; it != itEnd; ++it)
        {
            SharedTexturePtr tex = mTexMgr->getTexture(it->mName);
            mTextures.push_back(tex);
        }
    }

    void BSPRender::calcFaceSize(const BSPMap::BSPFace& face, glm::vec2& mins, glm::vec2& maxs)
    {
        mins.x = mins.y = std::numeric_limits<float>::max();
        maxs.x = maxs.y = std::numeric_limits<float>::lowest();
        
        glm::vec3 *pVertex = nullptr;
        const BSPMap::BSPTextureInfo& texInfo = mMap->mTexInfo[face.mTextureInfo];
        for (uint32_t i = 0; i < face.mNumEdges; ++i)
        {
            int32_t edge = mMap->mSurfEdges[face.mFirstEdge + i];
            if (edge >= 0)
            {
                pVertex = &mMap->mVertices[mMap->mEdges[edge].mVertex[0]];
            }
            else
            {
                pVertex = &mMap->mVertices[mMap->mEdges[-edge].mVertex[1]];
            }

            glm::vec2 value;
            value.x = (glm::dot(texInfo.mS, *pVertex) + texInfo.mSShift);
            value.y = (glm::dot(texInfo.mT, *pVertex) + texInfo.mTShift);
            for (uint32_t j = 0; j < 2; ++j)
            {
                if (value[j] < mins[j])
                    mins[j] = value[j];
                if (value[j] > maxs[j])
                    maxs[j] = value[j];
            }
        }
    }

    void BSPRender::initFaceData() // TODO: bsp utils?
    {
        mFaceData.resize(mMap->mFaces.size());
        for (uint32_t fc = 0; fc < mMap->mFaces.size(); ++fc)
        {
            const BSPMap::BSPFace& face = mMap->mFaces[fc];
            FaceData& faceData = mFaceData[fc];

            glm::vec2 mins, maxs;
            calcFaceSize(face, mins, maxs);

            // Calculate face integer extents for ligthmap:
            glm::i32vec2 bmins, bmaxs;
            for (uint32_t i = 0; i < 2; i++)
            {
                bmins[i] = (int32_t)floor(mins[i] / LIGHTMAP_SAMPLE_SIZE);
                bmaxs[i] = (int32_t)ceil(maxs[i] / LIGHTMAP_SAMPLE_SIZE);

                faceData.mLightmapMins[i] = bmins[i] * LIGHTMAP_SAMPLE_SIZE;
                faceData.mLightmapExtents[i] = (bmaxs[i] - bmins[i]) * LIGHTMAP_SAMPLE_SIZE;
            }
        }
    }

    void BSPRender::initLightmaps()
    {
        for (uint32_t fc = 0; fc < mMap->mFaces.size(); ++fc)
        {
            const BSPMap::BSPFace& face = mMap->mFaces[fc];
            FaceData& faceData = mFaceData[fc];
            
            // Update lightmap texture
            glm::i32vec2 lightmapSize = (faceData.mLightmapExtents / LIGHTMAP_SAMPLE_SIZE) + 1;
            if (face.mLightmapOffset >= 0 && faceData.mLightmapExtents.x > 0 && faceData.mLightmapExtents.y > 0)
            {
                // TODO: store extents data in lightmap?
                faceData.mLightmap = mLightmapMgr.allocLightmap(lightmapSize.x, lightmapSize.y, &mMap->mLightmaps[face.mLightmapOffset]);
            }
        }
    }

    void BSPRender::renderModel(const glm::mat4& matrix)
    {        
        // TODO: sort leafs or faces?
        // Some statistic:
        uint32_t numDiffuseTextureBinds = 0;
        uint32_t numLightmapTextureBinds = 0;
        uint32_t numDIPCalls = 0;
        uint32_t numVertices = 0;

        // VAO state
        ScopeBind bind(mVao);

        mShader.use(); // TODO: bind?

        mShader.setMVP(matrix);

        mShader.setTexDiffuseSampler(TEXTURE_SAMPLER_DIFFUSE);
        mShader.setTexLightmapSampler(TEXTURE_SAMPLER_LIGHTMAP);

        uint32_t vertexAttributeLocation = mShader.getVertexPositionLocation();

        // TODO: disable wrapper?
        glEnableVertexAttribArray(vertexAttributeLocation); // Vertexes
        mVertexVBO.bind();
        ::glVertexAttribPointer(vertexAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BufferObject::BUFFER_OFFSET(0));

        // Texture bind caching
        Commons::Render::SharedTexturePtr prevTexDiffuse;
        Commons::Render::Texture* prevTexLightmap = nullptr;

        auto it = mFaceBatches.begin();
        const auto itEnd = mFaceBatches.end();
        while (it != itEnd)
        {
            // Configure lighting:
            const BSPMap::BSPPlane& plane = mMap->mPlanes[it->mFace->mPlane];
            glm::vec3 normal = it->mFace->mPlaneSide == 0 ? plane.mNormal : -plane.mNormal;
            mShader.setNormal(normal);

            //face.mTypelight // TODO: lighting types
            float lightness = it->mFace->mBaselight / 255.F;
            mShader.setLightness(lightness);

            // Configure texture mapping
            const BSPMap::BSPTextureInfo& texInfo = mMap->mTexInfo[it->mFace->mTextureInfo];
            const BSPMap::BSPMipTex& tex = mMap->mMipTextures[texInfo.mMiptex];

            // TODO: bind wrapper to minimize overhead
            glActiveTexture(GL_TEXTURE0 + TEXTURE_SAMPLER_DIFFUSE);

            auto texDiffuse = mTextures[texInfo.mMiptex];
            if (prevTexDiffuse != texDiffuse)
            {
                if (texDiffuse)
                {
                    texDiffuse->get().get()->bind();
                }
                else
                {
                    ::glBindTexture(GL_TEXTURE_2D, 0); // TODO
                }
                prevTexDiffuse = texDiffuse;
                ++numDiffuseTextureBinds;
            }

            // Lightmap:
            glActiveTexture(GL_TEXTURE0 + TEXTURE_SAMPLER_LIGHTMAP);

            const LightmapMgr::Lightmap& lightmap = mFaceData[it->mFaceIndex].mLightmap; // TODO: use correct index
            if (prevTexLightmap != lightmap.mTex)
            {
                if (lightmap.mTex)
                {
                    lightmap.mTex->bind();
                }
                else
                {
                    ::glBindTexture(GL_TEXTURE_2D, 0); // TODO
                }
                prevTexLightmap = lightmap.mTex;
                ++numLightmapTextureBinds;
            }

            // TODO: float?
            glm::vec2 lightmapSampleSize(LIGHTMAP_SAMPLE_SIZE / lightmap.mScaleFactor.x, LIGHTMAP_SAMPLE_SIZE / lightmap.mScaleFactor.y);
            
            // TODO: pack this in single matrix, optimize                
            mShader.setTextureMapping(
                texInfo.mS,
                texInfo.mT,

                glm::vec2(texInfo.mSShift, texInfo.mTShift),
                glm::vec2(tex.mWidth, tex.mHeight),

                glm::vec2(texInfo.mSShift - mFaceData[it->mFaceIndex].mLightmapMins.x + (lightmap.mOffset.x + 0.5F) * lightmapSampleSize.x,
                    texInfo.mTShift - mFaceData[it->mFaceIndex].mLightmapMins.y + (lightmap.mOffset.y + 0.5F) * lightmapSampleSize.y
                    ),                
                glm::vec2(lightmapSampleSize.x * lightmap.mSize.x, lightmapSampleSize.y * lightmap.mSize.y)
                );

            ::glDrawElements(GL_TRIANGLE_FAN, it->mNumIndexes, GL_UNSIGNED_SHORT, &mVertIndices[it->mStartIndex]);

            ++numDIPCalls;
            numVertices += it->mNumIndexes;

            ++it;
        }
    }

    void BSPRender::render(const CameraPtr& camera)
    {
        // TODO: cleared by PVS
        //std::fill(mVisLeafs.begin(), mVisLeafs.end(), false);

        // Clear visible faces flags
        std::fill(mVisFaces.begin(), mVisFaces.end(), false);

        // TODO: not clear, optimize
        mVertIndices.clear();

        // Check for visible leafs
        // TODO: in bsp renderer
        glm::vec4 pos = glm::vec4(camera->getTranslation(), 1.F);
        pos = glm::inverse(STATIC_MAP_TRANSFORM) * pos;
        printf("Pos: %f, %f, %f\n", pos.x, pos.y, pos.z);

        glm::mat4 camMatrix = camera->getMatrix() * STATIC_MAP_TRANSFORM;
        Frustum frustum(camMatrix); // Camera frustum in BSP map space

        //int32_t camLeaf = mMap->getPointLeaf(glm::vec3(pos));
        //LOG_DEBUG("Leaf: %d", camLeaf);

        //mMap->fillVisLeafs(camLeaf, mVisLeafs);

        // DBG: move models a little
        //{
        //    auto it = mModelData.begin();
        //    auto itEnd = mModelData.end();
        //    for (; it != itEnd; ++it)
        //    {
        //        it->mIsVisible = true;
        //        if (it != mModelData.begin())
        //            it->mTranslation = glm::rotate(15 * sin(GetTickCount() * 0.001F), glm::vec3(0, 1, 0));
        //    }
        //}


        // Cull visible leafs:
        uint32_t cnt = 0, totalCnt = 0;
        for (int32_t i = 0; i < (int32_t)mMap->mLeafs.size(); ++i)
        {
            //if (mVisLeafs[i])
            {
                const BSPMap::BSPLeaf& leaf = mMap->mLeafs[i];
                AABB aabb(leaf.mMins, leaf.mMaxs);
                if (frustum.isInFrustum(aabb))
                {
                    drawLeaf(leaf);
                    ++cnt;
                }
                ++totalCnt;
            }
        }
        //printf("Vis leafs: %d, total: %d\n", cnt, totalCnt);

        // Draw all models using currently visible leafs:
        auto itMdl = mModelData.begin();
        auto itMdlEnd = mModelData.end();
        for (uint32_t mdlIndex = 0; mdlIndex < mMap->mModels.size(); ++mdlIndex)
        {
            const BSPMap::BSPModel& mdl = mMap->mModels[mdlIndex];
            const ModelData& mdlData = mModelData[mdlIndex];

            if (!mdlData.mIsVisible)
            {
                continue;
            }

            // TODO: pool
            mFaceBatches.clear();

            for (int32_t faceIndex = 0; faceIndex < mdl.mNumFaces; ++faceIndex)
            {                
                int32_t index = mdl.mFirstFace + faceIndex;
                if (mVisFaces[index])
                {
                    drawFace(index);
                }
            }          
            
            // Calculate model matrix based on origin & translation
            glm::mat4 mdlMatrix = camMatrix * glm::translate(mdl.mOrigin) * mdlData.mTranslation * glm::translate(-mdl.mOrigin);
            renderModel(mdlMatrix);
        }        
    }

    void BSPRender::drawLeaf(const BSPMap::BSPLeaf& leaf)
    {     
        /*if (leaf.mContents == BSPMap::CONTENTS_CLIP)
            return;
        if (leaf.mContents == BSPMap::CONTENTS_SKY)
            return;*/
        //leaf.mContents; // TODO: check for other content

        for (uint16_t i = 0; i < leaf.mNumMarkSurfaces; ++i)
        {
            uint16_t faceIndex = mMap->mMarkSurfaces[leaf.mFirstMarkSurface + i];
            if (!mVisFaces[faceIndex])
            {
                mVisFaces[faceIndex] = true;
            }
        }
    }

    void BSPRender::drawFace(uint32_t faceIndex)
    {
        const BSPMap::BSPFace& face = mMap->mFaces[faceIndex];
        FaceBatch batch;

        const BSPMap::BSPTextureInfo& texInfo = mMap->mTexInfo[face.mTextureInfo];

        const BSPMap::BSPMipTex& tex = mMap->mMipTextures[texInfo.mMiptex];
        batch.mFace = &face;


        // TODO
        batch.mFaceIndex = faceIndex;
        batch.mStartIndex = mVertIndices.size();

        // HACK: quick fix
        if (strncmp(tex.mName, "sky", 3) == 0)
        {
            return;
        }    

        uint16_t startVert = -1;
        uint16_t lastVert = -1;
        for (int16_t i = 0; i < face.mNumEdges; ++i)
        {
            uint16_t edgeVerts[2];
            int32_t edgeIndex = mMap->mSurfEdges[face.mFirstEdge + i];
            if (edgeIndex > 0)
            {
                const BSPMap::BSPEdge& edge = mMap->mEdges[edgeIndex];
                edgeVerts[0] = edge.mVertex[0];
                edgeVerts[1] = edge.mVertex[1];
            }
            else
            {
                const BSPMap::BSPEdge& edge = mMap->mEdges[-edgeIndex];
                edgeVerts[0] = edge.mVertex[1];
                edgeVerts[1] = edge.mVertex[0];
            }

            if (i == 0) // First edge, start triangle
            {
                uint16_t vert = edgeVerts[0];
                startVert = vert;
                mVertIndices.push_back(vert);                
            }
            else
            {
                assert(edgeVerts[0] == lastVert);
            }

            uint16_t vert = edgeVerts[1];
            lastVert = vert;
            mVertIndices.push_back(vert); // TODO: add not push
        }

        assert(lastVert == startVert);
        // TODO: close poly if needed

        batch.mNumIndexes = mVertIndices.size() - batch.mStartIndex;
        mFaceBatches.push_back(batch);
    }

    void BSPRender::traceRay(const glm::vec3 pos, const glm::vec3 normal)
    {
        // TODO
    }
}