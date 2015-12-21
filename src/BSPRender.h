#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <BSPMap.h>
#include <Render/GLContext.h>
#include <Render/VertexArrayObject.h>
#include <Render/BufferObject.h>
#include <Render/Texture.h>
#include <PlainShaderProgram.h>

namespace LambdaCore
{
    class BSPRender
    {
    private:
        struct VertexData
        {
            glm::vec3 mPos;
            glm::vec3 mNormal;
            glm::vec2 mUV;
        };

    public:
        BSPRender(const BSPMapPtr& map);
        
    public:
        void render(const Commons::Render::CameraPtr& camera);

    private:
        static void CalcUV(const BSPMap::BSPTextureInfo& texInfo, VertexData& data);

        void drawLeaf(const BSPMap::BSPLeaf& leaf);
        void drawFace(const BSPMap::BSPFace& face);

    private:
        BSPMapPtr m_map;
        Commons::Render::VertexArrayObject mVao;
        Commons::Render::BufferObject mVertexes;

        // Visible faces
        std::vector<bool> mVisFaces;

        // Output render data for rendering face
        // TODO: textures
        std::vector<VertexData> mVertexData;

        PlainShaderProgram mShader;

        std::vector<Commons::Render::TexturePtr> mTextures;
    };
}