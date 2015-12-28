#include <cassert>
#include <BSPRender.h>
#include <Exception/Exception.h>
#include <Common/StringUtils.h>
#include <Render/AABB.h>
#include <Render/VertexArrayObject.h>
#include <Render/BufferObject.h>
#include <Render/ScopeBind.h>
#include <PlainShaderProgram.h>

#include <Common/Singleton.h>

using namespace Commons;
using namespace Commons::Render;

namespace LambdaCore
{  
    static const int32_t LIGHTMAP_SAMPLE_SIZE = 16;

    // TODO: animation - in render
    /*if it begins by * it will be animated like lava or water.
    if it begins by + then it will be animated with frames, and the second letter of the name will be the frame number.Those numbers begin at 0, and go up to 9 at the maximum.
    if if begins with sky if will double scroll like a sky.
    Beware that sky textures are made of two distinct parts.*/

    BSPRender::BSPRender(const BSPMapPtr& map, const Commons::Render::SharedTextureMgrPtr& textureManager)
        : m_map(map)
        , mTexMgr(textureManager)
        , mVao()
        , mVertexes(GL_ARRAY_BUFFER)
        , mVisFaces(map->mFaces.size())
        , mVertexData()
        , mFaceBatches()
        , mShader()
        , mTextures()
        , mFaceData()
        // TODO: init members
    {
        // Load textures:
        // TODO: func
        {
            auto it = map->mMipTextures.begin();
            auto itEnd = map->mMipTextures.end();
            for (; it != itEnd; ++it)
            {                
                SharedTexturePtr tex = mTexMgr->getTexture(it->mName);
                mTextures.push_back(tex);
            }
        }

        // Calc face data:
        // TODO: func
        {            
            mFaceData.resize(m_map->mFaces.size());
            for (uint32_t fc = 0; fc < m_map->mFaces.size(); ++fc)
            {
                const BSPMap::BSPFace& face = m_map->mFaces[fc]; // TODO: func for faces
                FaceData& faceData = mFaceData[fc];

                // TODO: in map class?
                float mins[2], maxs[2];
                mins[0] = mins[1] = 999999;
                maxs[0] = maxs[1] = -99999;
                
                glm::vec3 *pVertex = nullptr;                

                const BSPMap::BSPTextureInfo& texInfo = m_map->mTexInfo[face.mTextureInfo];
                for (uint32_t i = 0; i < face.mNumEdges; ++i)
                {
                    int32_t edge = m_map->mSurfEdges[face.mFirstEdge + i];
                    if (edge >= 0)
                    {
                        pVertex = &m_map->mVertices[m_map->mEdges[edge].mVertex[0]];
                    }
                    else
                    {
                        pVertex = &m_map->mVertices[m_map->mEdges[-edge].mVertex[1]];
                    }

                    glm::vec2 value;
                    value.x = (glm::dot(texInfo.mS, *pVertex) + texInfo.mSShift);
                    value.y = (glm::dot(texInfo.mT, *pVertex) + texInfo.mTShift);
                    for (uint32_t j = 0; j < 2; ++j)
                    {                   
                        // TODO: std min/max
                        if (value[j] < mins[j])
                            mins[j] = value[j];
                        if (value[j] > maxs[j])
                            maxs[j] = value[j];
                    }
                }

                // Calc the integer extents:
                glm::i32vec2 bmins, bmaxs;
                for (uint32_t i = 0; i < 2; i++)
                {
                    bmins[i] = floor(mins[i] / LIGHTMAP_SAMPLE_SIZE);
                    bmaxs[i] = ceil(maxs[i] / LIGHTMAP_SAMPLE_SIZE);

                    faceData.mMins[i] = bmins[i] * LIGHTMAP_SAMPLE_SIZE;
                    faceData.mExtents[i] = (bmaxs[i] - bmins[i]) * LIGHTMAP_SAMPLE_SIZE;
                    //if (!(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */)
                    //  Sys_Error("Bad surface extents");
                }

                // Calc lightmap texture
                // TODO: check if face has zero size
                glm::i32vec2 lightmapSize = (faceData.mExtents / LIGHTMAP_SAMPLE_SIZE) + 1;

                // TODO: combine in texture atlas
                if (face.mLightmapOffset >= 0)
                {                    
                    // TODO: remove then lightmap packaging will be done
                    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                    TexturePtr tex(new Texture());

                    // TODO: 4 lightmaps....
                    //MAXLIGHTMAPS
                    tex->setTexData2d(0, GL_RGB, lightmapSize.x, lightmapSize.y, GL_RGB, GL_UNSIGNED_BYTE, &m_map->mLightmaps[face.mLightmapOffset]);
                    faceData.mLightmapTex = tex;                    
                    tex->setMagFilter(GL_LINEAR);
                    tex->setMinFilter(GL_LINEAR); // TODO: enums
                }
            }            
        }

        initVBOs();
    }

    void BSPRender::initVBOs()
    {
        // TODO
    }

    void genLightmapTex()
    {
        // TODO
    }

    void BSPRender::render(const CameraPtr& camera)
    {
        // Clear visible faces flags
        std::fill(mVisFaces.begin(), mVisFaces.end(), false);

        // TODO: not clear, optimize
        mVertexData.clear();
        // TODO: pool
        mFaceBatches.clear();

        // Check for visible leafs        
        int32_t camLeaf = m_map->getPointLeaf(camera->getTranslation());
        for (int32_t i = 0; i < m_map->mLeafs.size(); ++i)
        {
            if (m_map->isLeafVisible(camLeaf, i))
            {
                const BSPMap::BSPLeaf& leaf = m_map->mLeafs[i];
                AABB aabb(leaf.mMins, leaf.mMaxs);
                if (camera->isInFrustum(aabb))
                {
                    drawLeaf(leaf);
                }
            }
        }
        
        // Render the results
        {
            // TODO: load all vertexes in buffer statically
            ScopeBind bindVertexes(mVertexes); // TODO: static
            mVertexes.setData(sizeof(VertexData) * mVertexData.size(), &mVertexData[0], GL_STATIC_DRAW);

            ScopeBind bind(mVao);

            mShader.use(); // TODO: bind?

            mShader.setTexDiffuseSampler(0); // TODO: constants
            mShader.setTexLightmapSampler(1);

            uint32_t vertexAttributeLocation = mShader.getVertexPositionLocation();

            // TODO: disable wrapper?
            glEnableVertexAttribArray(vertexAttributeLocation); // Vertexes
            mVertexes.bind();

            glVertexAttribPointer(vertexAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), NULL);

            glm::mat4 camMatrix = camera->getProjection() * camera->getModelview();
            glm::mat4 STATIC_MAP_TRANSFORM = glm::rotate(90.F, -1.F, 0.F, 0.F);

            // TODO: models transformations
            mShader.setMVP(camMatrix * STATIC_MAP_TRANSFORM);

            auto it = mFaceBatches.begin();
            const auto itEnd = mFaceBatches.end();
            while (it != itEnd)
            {
                // TODO: funcs
                // Configure lighting:
                const BSPMap::BSPPlane& plane = m_map->mPlanes[it->mFace->mPlane];
                glm::vec3 normal = it->mFace->mPlaneSide == 0 ? plane.mNormal : -plane.mNormal;
                mShader.setNormal(normal);

                //face.mTypelight // TODO: lighting types
                float lightness = it->mFace->mBaselight / 255.F;
                mShader.setLightness(lightness);
                
                // Configure texture mapping
                const BSPMap::BSPTextureInfo& texInfo = m_map->mTexInfo[it->mFace->mTextureInfo];
                const BSPMap::BSPMipTex& tex = m_map->mMipTextures[texInfo.mMiptex];
                
                // TODO: bind wrapper to minimize overhead
                glActiveTexture(GL_TEXTURE0 + 0);
                
                auto ltmShared = mTextures[texInfo.mMiptex];
                if (ltmShared)
                {
                    ltmShared->get().get()->bind();
                }
                                
                // Lightmap:
                glActiveTexture(GL_TEXTURE0 + 1); // TODO: consts here?                                
                const TexturePtr& lightmap = mFaceData[it->mFaceIndex].mLightmapTex; // TODO: use correct index
                if (lightmap)
                {
                    lightmap->bind();                
                }
                else
                {
                    ::glBindTexture(GL_TEXTURE_2D, 0); // TODO
                }

                // TODO: pack this in single matrix?
                mShader.setTextureMapping(
                    texInfo.mS,
                    texInfo.mT,

                    glm::vec2(texInfo.mSShift, texInfo.mTShift),
                    glm::vec2(tex.mWidth, tex.mHeight),
                    
                    glm::vec2(texInfo.mSShift - mFaceData[it->mFaceIndex].mMins.x + (LIGHTMAP_SAMPLE_SIZE >> 1),
                        texInfo.mTShift - mFaceData[it->mFaceIndex].mMins.y + (LIGHTMAP_SAMPLE_SIZE >> 1)
                        ),
                    glm::vec2(
                        ((mFaceData[it->mFaceIndex].mExtents.x / LIGHTMAP_SAMPLE_SIZE) + 1) * LIGHTMAP_SAMPLE_SIZE,
                        ((mFaceData[it->mFaceIndex].mExtents.y / LIGHTMAP_SAMPLE_SIZE) + 1) * LIGHTMAP_SAMPLE_SIZE
                        )
                );
                
                // TODO: combine lm in atlas
                // TODO: static vertex data
                // TODO: draw models independently
                
                glDrawArrays(GL_TRIANGLE_FAN, it->mStartVertex, it->mNumVertexes);

                ++it;
            }
        }
    }

    void BSPRender::drawLeaf(const BSPMap::BSPLeaf& leaf)
    {     
        if (leaf.mContents == BSPMap::CONTENTS_CLIP)
            return;
        if (leaf.mContents == BSPMap::CONTENTS_SKY)
            return;
        //leaf.mContents; // TODO: check for other content

        for (uint16_t i = 0; i < leaf.mNumMarkSurfaces; ++i)
        {
            uint16_t faceIndex = m_map->mMarkSurfaces[leaf.mFirstMarkSurface + i];
            if (!mVisFaces[faceIndex])
            {
                drawFace(faceIndex); // TODO: sorting
                mVisFaces[faceIndex] = true;
            }
        }
    }

    void BSPRender::CalcUV(const BSPMap::BSPTextureInfo& texInfo, VertexData& data, uint32_t width, uint32_t height)
    {
        //data.mUV.x = (glm::dot(texInfo.mS, data.mPos) + texInfo.mSShift) / width;
        //data.mUV.y = (glm::dot(texInfo.mT, data.mPos) + texInfo.mTShift) / height;
    }

    void BSPRender::drawFace(uint32_t faceIndex)
    {
        const BSPMap::BSPFace& face = m_map->mFaces[faceIndex];
        FaceBatch batch;

        const BSPMap::BSPTextureInfo& texInfo = m_map->mTexInfo[face.mTextureInfo];

        // TODO: calc UV in shader?
        // TODO: set texture and lightmap        
        //face.mLightmapOffset;
        const BSPMap::BSPMipTex& tex = m_map->mMipTextures[texInfo.mMiptex];
        batch.mFace = &face;
        // TODO
        batch.mFaceIndex = faceIndex;
        batch.mStartVertex = mVertexData.size();
        // HACK: quick fix
        if (strncmp(tex.mName, "sky", 3) == 0)
        {
            return;
        }

        VertexData vertexData;
        

        uint16_t startVert = -1;
        uint16_t lastVert = -1;
        for (int16_t i = 0; i < face.mNumEdges; ++i)
        {
            uint16_t edgeVerts[2];
            int32_t edgeIndex = m_map->mSurfEdges[face.mFirstEdge + i];
            if (edgeIndex > 0)
            {
                const BSPMap::BSPEdge& edge = m_map->mEdges[edgeIndex];
                edgeVerts[0] = edge.mVertex[0];
                edgeVerts[1] = edge.mVertex[1];
            }
            else
            {
                const BSPMap::BSPEdge& edge = m_map->mEdges[-edgeIndex];
                edgeVerts[0] = edge.mVertex[1];
                edgeVerts[1] = edge.mVertex[0];
            }

            if (i == 0) // First edge, start triangle
            {
                vertexData.mPos = m_map->mVertices[edgeVerts[0]];
                //CalcUV(texInfo, vertexData, tex.mWidth, tex.mHeight);
                mVertexData.push_back(vertexData);
                startVert = edgeVerts[0];
            }
            else
            {
                assert(edgeVerts[0] == lastVert);
            }

            vertexData.mPos = m_map->mVertices[edgeVerts[1]];
            //CalcUV(texInfo, vertexData, tex.mWidth, tex.mHeight);
            mVertexData.push_back(vertexData);

            lastVert = edgeVerts[1];
        }

        assert(lastVert == startVert);
        // TODO: close poly if needed

        batch.mNumVertexes = mVertexData.size() - batch.mStartVertex;
        mFaceBatches.push_back(batch);
    }
}