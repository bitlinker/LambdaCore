#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <BSPMap.h>
#include <Render/GLContext.h>
#include <Render/VertexArrayObject.h>
#include <Render/BufferObject.h>
#include <Render/Texture.h>
#include <Render/SharedTextureMgr.h>
#include <BSPShaderProgram.h>
#include <LightmapMgr.h>

namespace LambdaCore
{
    class BSPRender
    {
    private:
        struct VertexData
        {
            uint32_t mVertIndex;
        };

        struct FaceBatch
        {
            uint32_t mStartIndex;
            uint32_t mNumIndexes;
            const BSPMap::BSPFace* mFace;
            uint32_t mFaceIndex;
        };

        struct FaceData
        {
            glm::i32vec2 mMins; // TODO: if needed
            glm::i32vec2 mExtents;
            LightmapMgr::Lightmap mLightmap;
        };

    public:
        // TODO: singleton pool with texture manager
        BSPRender(const BSPMapPtr& map, const Commons::Render::SharedTextureMgrPtr& textureManager);
        
    public:
        void render(const Commons::Render::CameraPtr& camera);

    private:

        void initVBOs();

        void drawLeaf(const BSPMap::BSPLeaf& leaf);
        void drawFace(uint32_t faceIndex);

    private:
        LightmapMgr mLightmapMgr;
        BSPMapPtr mMap;
        Commons::Render::SharedTextureMgrPtr mTexMgr;

        Commons::Render::VertexArrayObject mVao;
        Commons::Render::BufferObject mVertexVBO;

        // Visible faces
        std::vector<bool> mVisFaces;

        // Output render data for rendering face
        std::vector<uint16_t> mVertIndices;
        std::vector<FaceBatch> mFaceBatches;

        BSPShaderProgram mShader;

        std::vector<Commons::Render::SharedTexturePtr> mTextures;

        std::vector<FaceData> mFaceData;
    };
}