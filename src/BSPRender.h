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
        class FaceBatch
        {
        public:
            uint32_t mStartIndex;
            uint32_t mNumIndexes;
            const BSPMap::BSPFace* mFace;
            uint32_t mFaceIndex;
        };

        class FaceData
        {
        public:
            glm::i32vec2 mLightmapMins; // TODO: if needed
            glm::i32vec2 mLightmapExtents;
            LightmapMgr::Lightmap mLightmap;
        };

        class ModelData
        {
        public:
            ModelData() : mIsVisible(false) {}

        public:
            glm::mat4 mTranslation;
            bool mIsVisible;
        };

    public:
        // TODO: singleton pool with texture manager
        BSPRender(const BSPMapPtr& map, const Commons::Render::SharedTextureMgrPtr& textureManager);
        
    public:
        void render(const Commons::Render::CameraPtr& camera);

    private:
        void initFaceData();
        void initVBOs();
        void initTextures();
        void initLightmaps();

        void calcFaceSize(const BSPMap::BSPFace& face, glm::vec2& mins, glm::vec2& maxs);

        void drawLeaf(const BSPMap::BSPLeaf& leaf);
        void drawFace(uint32_t faceIndex);

        void renderModel(const glm::mat4& matrix);

        // TODO: to another class:
        void traceRay(const glm::vec3 pos, const glm::vec3 normal);

    private:
        LightmapMgr mLightmapMgr;
        BSPMapPtr mMap;
        Commons::Render::SharedTextureMgrPtr mTexMgr;

        Commons::Render::VertexArrayObject mVao;
        Commons::Render::BufferObject mVertexVBO;

        // Visible leaves
        std::vector<bool> mVisLeafs;

        // Visible faces
        std::vector<bool> mVisFaces;

        // Output render data for rendering face
        std::vector<uint16_t> mVertIndices;
        std::vector<FaceBatch> mFaceBatches;

        BSPShaderProgram mShader;

        // Precomputed data for map rendering & state adjusting
        std::vector<Commons::Render::SharedTexturePtr> mTextures;
        std::vector<FaceData> mFaceData;
        std::vector<ModelData> mModelData; // TODO: move to map itself?
    };
}