#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <BSPMap.h>
#include <Render/GLContext.h>
#include <Render/VertexArrayObject.h>
#include <Render/BufferObject.h>
#include <Render/Texture.h>
#include <Render/SharedTextureMgr.h>
#include <PlainShaderProgram.h>

namespace LambdaCore
{
    class BSPRender
    {
    private:
        struct VertexData
        {
            glm::vec3 mPos;            
        };

        struct FaceBatch
        {
            uint32_t mStartVertex;
            uint32_t mNumVertexes;
            const BSPMap::BSPFace* mFace;
        };

    public:
        // TODO: singleton pool with texture manager
        BSPRender(const BSPMapPtr& map, const Commons::Render::SharedTextureMgrPtr& textureManager);
        
    public:
        void render(const Commons::Render::CameraPtr& camera);

    private:
        static void CalcUV(const BSPMap::BSPTextureInfo& texInfo, VertexData& data, uint32_t width, uint32_t height);

        void drawLeaf(const BSPMap::BSPLeaf& leaf);
        void drawFace(const BSPMap::BSPFace& face);

    private:
        BSPMapPtr m_map;
        Commons::Render::SharedTextureMgrPtr mTexMgr;

        Commons::Render::VertexArrayObject mVao;
        Commons::Render::BufferObject mVertexes;

        // Visible faces
        std::vector<bool> mVisFaces;

        // Output render data for rendering face
        // TODO: textures
        std::vector<VertexData> mVertexData;
        std::vector<FaceBatch> mFaceBatches;

        PlainShaderProgram mShader;

        std::vector<Commons::Render::SharedTexturePtr> mTextures;
    };
}