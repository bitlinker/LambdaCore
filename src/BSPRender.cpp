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
    // TODO: move
    class TextureManager : public Commons::Singleton<TextureManager>
    {
    public:
        TexturePtr loadTexture(const std::string& name)
        {
            TexturePtr tex = std::make_shared<Texture>(); // TODO: create in context
            ScopeBind texBind(*tex.get());
            tex->setMagFilter(GL_LINEAR);
            tex->setMinFilter(GL_LINEAR);
            uint8_t data[256 * 256 * 3];
            tex->setTexData2d(0, GL_RGB, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, data);
            return tex;
        }
    private:
    };


    BSPRender::BSPRender(const BSPMapPtr& map)
        : m_map(map)
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
            // TODO: load them
            //auto tex = TextureManager::getInstance().loadTexture(it->mName);
            //mTextures.push_back(tex);
        }
    }

    void BSPRender::render(const CameraPtr& camera)
    {
        // Clear visible faces flags
        std::fill(mVisFaces.begin(), mVisFaces.end(), false);

        // TODO: not clear, optimize
        mVertexData.clear();

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
            ScopeBind bindVertexes(mVertexes);
            mVertexes.setData(sizeof(VertexData) * mVertexData.size(), &mVertexData[0], GL_DYNAMIC_DRAW);
        }



        {
            mShader.use();
            glm::mat4 camMatrix = camera->getProjection() * camera->getModelview();
            mShader.setMVP(camMatrix);
            // TODO
            //mTextures[0]->bind();
            mShader.setTex1Sampler(0);            

            ScopeBind bind(mVao);

            glEnableVertexAttribArray(0); // Vertexes
            glEnableVertexAttribArray(1); // Normals
            glEnableVertexAttribArray(2); // UVs            
            mVertexes.bind();
            
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), NULL);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, sizeof(VertexData), (void*)(NULL + sizeof(glm::vec3))); // TODO: better                     
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)(NULL + 2 * sizeof(glm::vec3))); // TODO: better

            //glDrawArrays(GL_LINES, 0, mVertexData.size()); // TODO: check
            glDrawArrays(GL_TRIANGLE_STRIP, 0, mVertexData.size()); // TODO: check
        }

    }

    void BSPRender::drawLeaf(const BSPMap::BSPLeaf& leaf)
    {
        //leaf.mContents; // TODO: check if transparent?
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

    void BSPRender::CalcUV(const BSPMap::BSPTextureInfo& texInfo, VertexData& data)
    {
        data.mUV.x = glm::dot(texInfo.mS, data.mPos) + texInfo.mSShift;
        data.mUV.y = glm::dot(texInfo.mT, data.mPos) + texInfo.mTShift;
    }

    void BSPRender::drawFace(const BSPMap::BSPFace& face)
    {
        const BSPMap::BSPPlane& plane = m_map->mPlanes[face.mPlane];
        const BSPMap::BSPTextureInfo& texInfo = m_map->mTexInfo[face.mTextureInfo];

        // TODO: calc UV in shader?
        // TODO: set texture and lightmap        
        face.mLightmapOffset;
        m_map->mMipTextures[texInfo.mMiptex];        

        VertexData vertexData;
        vertexData.mNormal = face.mPlaneSide == 0 ? plane.mNormal : -plane.mNormal;
        for (int16_t i = 0; i < face.mNumEdges; ++i)
        {
            int32_t edgeIndex = m_map->mSurfEdges[face.mFirstEdge + i]; // TODO: invert support
            if (edgeIndex > 0)
            {
                const BSPMap::BSPEdge& edge = m_map->mEdges[edgeIndex]; // TODO: optimize, copypaste
                vertexData.mPos = m_map->mVertices[edge.mVertex[0]];
                CalcUV(texInfo, vertexData);
                mVertexData.push_back(vertexData);

                vertexData.mPos = m_map->mVertices[edge.mVertex[1]];
                CalcUV(texInfo, vertexData);
                mVertexData.push_back(vertexData);
            }
            else
            {
                const BSPMap::BSPEdge& edge = m_map->mEdges[-edgeIndex - 1]; // TODO: check -1
                vertexData.mPos = m_map->mVertices[edge.mVertex[1]];
                CalcUV(texInfo, vertexData);
                mVertexData.push_back(vertexData);

                vertexData.mPos = m_map->mVertices[edge.mVertex[0]];
                CalcUV(texInfo, vertexData);
                mVertexData.push_back(vertexData);
            }
        }
    }
}