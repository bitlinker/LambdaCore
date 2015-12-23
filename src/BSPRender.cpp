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
        , mShader()
        // TODO: init members
    {
        // Load textures:
        auto it = map->mMipTextures.begin();
        auto itEnd = map->mMipTextures.end();
        for (; it != itEnd; ++it)
        {
            SharedTexturePtr tex = mTexMgr->getTexture(it->mName);
            mTextures.push_back(tex);
        }
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
            mVertexes.setData(sizeof(VertexData) * mVertexData.size(), &mVertexData[0], GL_DYNAMIC_DRAW);

            ScopeBind bind(mVao);

            mShader.use(); // TODO: bind?

            uint32_t vertexAttributeLocation = mShader.getVertexPositionLocation();

            // TODO: disable wrapper?
            glEnableVertexAttribArray(vertexAttributeLocation); // Vertexes
            mVertexes.bind();

            glVertexAttribPointer(vertexAttributeLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), NULL);


            glm::mat4 camMatrix = camera->getProjection() * camera->getModelview();
            glm::mat4 STATIC_MAP_TRANSFORM = glm::rotate(90.F, -1.F, 0.F, 0.F);

            // TODO: models transformations
            mShader.setMVP(camMatrix * STATIC_MAP_TRANSFORM);
            mShader.setTex1Sampler(0);

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
                ScopeBind texBind(*mTextures[texInfo.mMiptex]->get().get()); 

                // TODO: pack this in single matrix?
                mShader.setTextureMapping(
                    texInfo.mS,
                    texInfo.mT,
                    glm::vec2(texInfo.mSShift, texInfo.mTShift),
                    glm::vec2(tex.mWidth, tex.mHeight)
                );

                // configure lightmap:
                // TODO
                // 1. Precalc extents for every face
                // 2. Preload lightmap textures with size according to extents (combine in atlas for performance?)
                
                glDrawArrays(GL_TRIANGLE_FAN, it->mStartVertex, it->mNumVertexes); // TODO: check

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
                const BSPMap::BSPFace& face = m_map->mFaces[faceIndex];
                drawFace(face); // TODO: sorting
                mVisFaces[faceIndex] = true;
            }
        }
    }

    void BSPRender::CalcUV(const BSPMap::BSPTextureInfo& texInfo, VertexData& data, uint32_t width, uint32_t height)
    {
        //data.mUV.x = (glm::dot(texInfo.mS, data.mPos) + texInfo.mSShift) / width;
        //data.mUV.y = (glm::dot(texInfo.mT, data.mPos) + texInfo.mTShift) / height;
    }

    void BSPRender::drawFace(const BSPMap::BSPFace& face)
    {
        FaceBatch batch;


        const BSPMap::BSPTextureInfo& texInfo = m_map->mTexInfo[face.mTextureInfo];

        // TODO: calc UV in shader?
        // TODO: set texture and lightmap        
        //face.mLightmapOffset;
        const BSPMap::BSPMipTex& tex = m_map->mMipTextures[texInfo.mMiptex];
        batch.mFace = &face;
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