#include <cassert>
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

inline unsigned char getpixel(const uint8_t* src, int32_t srcWidth, int32_t srcHeight, int32_t x, int32_t y, int32_t numChannels, int32_t channel)
{
    // Border color
    //if (x < 0 || y < 0 || x >= srcWidth || y >= srcHeight)
    //    return 128;

    // Clamp sampler implementation
    if (x < 0)
        x = 0;

    if (x >= srcWidth)
        x = srcWidth - 1;

    if (y < 0)
        y = 0;

    if (y >= srcHeight)
        y = srcHeight - 1;

    
    return src[(x + y * srcWidth) * 3 + channel];
}

// TODO: move
static void BicubicInterpolate(const uint8_t *src, int32_t srcWidth, int32_t srcHeight, int32_t numChannels, uint8_t *dst, int32_t dstWidth, int32_t dstHeight)
{
    const float tx = float(srcWidth) / dstWidth;
    const float ty = float(srcHeight) / dstHeight;
    const int32_t srcStride = srcWidth * numChannels;
    const int32_t dstStride = dstWidth * numChannels;

    float C[5] = { 0 };

    for (int32_t i = 0; i < dstHeight; ++i)
    {
        for (int32_t j = 0; j < dstWidth; ++j)
        {
            const int32_t x = uint32_t(tx * j);
            const int32_t y = uint32_t(ty * i);
            const float dx = tx * j - x;
            const float dy = ty * i - y;

            for (int32_t k = 0; k < numChannels; ++k)
            {
                for (int32_t jj = 0; jj < 4; ++jj)
                {
                    const int32_t z = y - 1 + jj;
                    float a0 = getpixel(src, srcWidth, srcHeight, x, z, numChannels, k);
                    float d0 = getpixel(src, srcWidth, srcHeight, x - 1, z, numChannels, k) - a0;
                    float d2 = getpixel(src, srcWidth, srcHeight, x + 1, z, numChannels, k) - a0;
                    float d3 = getpixel(src, srcWidth, srcHeight, x + 2, z, numChannels, k) - a0;
                    float a1 = -1.0 / 3 * d0 + d2 - 1.0 / 6 * d3;
                    float a2 = 1.0 / 2 * d0 + 1.0 / 2 * d2;
                    float a3 = -1.0 / 6 * d0 - 1.0 / 2 * d2 + 1.0 / 6 * d3;
                    C[jj] = a0 + a1 * dx + a2 * dx * dx + a3 * dx * dx * dx;

                    d0 = C[0] - C[1];
                    d2 = C[2] - C[1];
                    d3 = C[3] - C[1];
                    a0 = C[1];
                    a1 = -1.0 / 3 * d0 + d2 - 1.0 / 6 * d3;
                    a2 = 1.0 / 2 * d0 + 1.0 / 2 * d2;
                    a3 = -1.0 / 6 * d0 - 1.0 / 2 * d2 + 1.0 / 6 * d3;
                    float value = a0 + a1 * dy + a2 * dy * dy + a3 * dy * dy * dy;

                    if (value > 255.F)
                        value = 255.F;

                    if (value < 0.F)
                        value = 0.F;

                    dst[i * dstStride + j * numChannels + k] = (uint8_t)(value);
                }
            }
        }
    }
}


static void BilinearInterpolate(const uint8_t* src, int32_t srcWidth, int32_t srcHeight, int32_t numChannels, uint8_t *dst, int32_t dstWidth, int32_t dstHeight)
{
    int32_t a, b, c, d, x, y, index;
    float tx = (float)(srcWidth - 1) / dstWidth;
    float ty = (float)(srcHeight - 1) / dstHeight;
    float x_diff, y_diff;
    const int32_t srcStride = srcWidth * numChannels;
    const int32_t dstStride = dstWidth * numChannels;

    for (int32_t i = 0; i < dstHeight; i++)
    {
        for (int32_t j = 0; j < dstWidth; j++)
        {
            x = (int32_t)(tx * j);
            y = (int32_t)(ty * i);

            x_diff = ((tx * j) - x);
            y_diff = ((ty * i) - y);

            index = y * srcStride + x * numChannels;
            a = (int32_t)index;
            b = (int32_t)(index + numChannels);
            c = (int32_t)(index + srcStride);
            d = (int32_t)(index + srcStride + numChannels);

            for (int32_t k = 0; k < numChannels; k++)
            {
                dst[i * dstStride + j * numChannels + k] =
                    src[a + k] * (1.F - x_diff) * (1.F - y_diff)
                    + src[b + k] * (1.F - y_diff) * (x_diff)
                    +src[c + k] * (y_diff)* (1.F - x_diff)
                    + src[d + k] * (y_diff)* (x_diff);
            }
        }
    }
}


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
        : mLightmapMgr(256, 128)
        , mMap(map)
        , mTexMgr(textureManager)
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
            mFaceData.resize(mMap->mFaces.size());
            for (uint32_t fc = 0; fc < mMap->mFaces.size(); ++fc)
            {
                const BSPMap::BSPFace& face = mMap->mFaces[fc]; // TODO: func for faces
                FaceData& faceData = mFaceData[fc];

                // TODO: in map class?
                float mins[2], maxs[2];
                mins[0] = mins[1] = 999999;
                maxs[0] = maxs[1] = -99999;
                
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
                    bmins[i] = (int32_t)floor(mins[i] / LIGHTMAP_SAMPLE_SIZE);
                    bmaxs[i] = (int32_t)ceil(maxs[i] / LIGHTMAP_SAMPLE_SIZE);

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
                    // TODO: interpolation in lightmap manager
                    /*uint32_t LIGHTMAP_MAG_FACTOR = 4;
                    uint32_t newWidth = lightmapSize.x * LIGHTMAP_MAG_FACTOR;
                    uint32_t newHeight = lightmapSize.y * LIGHTMAP_MAG_FACTOR;
                    std::vector<uint8_t> newLightmap(newWidth * newHeight * 3);
                    BicubicInterpolate(&mMap->mLightmaps[face.mLightmapOffset], lightmapSize.x, lightmapSize.y, 3, &newLightmap[0], newWidth, newHeight);
                    faceData.mLightmap = mLightmapMgr.allocLightmap(newWidth, newHeight, &newLightmap[0], LIGHTMAP_MAG_FACTOR);*/

                    faceData.mLightmap = mLightmapMgr.allocLightmap(lightmapSize.x, lightmapSize.y, &mMap->mLightmaps[face.mLightmapOffset], 1);
                }
            }            
        }

        initVBOs();

        // TODO: func
        mModelData.resize(mMap->mModels.size());
        
        mModelData[0].mIsVisible = true;
        mModelData[1].mIsVisible = true;
    }

    void BSPRender::initVBOs()
    {
        mVertexVBO.setData(sizeof(glm::vec3) * mMap->mVertices.size(), &mMap->mVertices[0], GL_STATIC_DRAW);
        // TODO: index buffer?
    }

    void genLightmapTex()
    {
        // TODO
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
            uint32_t lightmapSample = LIGHTMAP_SAMPLE_SIZE / lightmap.mMagFactor;

            // TODO: pack this in single matrix, optimize                
            mShader.setTextureMapping(
                texInfo.mS,
                texInfo.mT,

                glm::vec2(texInfo.mSShift, texInfo.mTShift),
                glm::vec2(tex.mWidth, tex.mHeight),

                glm::vec2(texInfo.mSShift - mFaceData[it->mFaceIndex].mMins.x + (lightmapSample >> 1) + lightmap.mX * lightmapSample,
                    texInfo.mTShift - mFaceData[it->mFaceIndex].mMins.y + (lightmapSample >> 1) + lightmap.mY * lightmapSample
                    ),
                glm::vec2(
                    lightmap.mW * lightmapSample,
                    lightmap.mH * lightmapSample
                    )
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
}