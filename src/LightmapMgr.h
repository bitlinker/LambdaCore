#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <vector>

#include <Common/NonCopyable.h>

#include <Render/GLContext.h>
#include <Render/Texture.h>

#include <Imaging/IInterpolator.h>

namespace LambdaCore
{
    class LightmapMgr : public Commons::NonCopyable
    {
    public:
        class Lightmap
        {
        public:
            Lightmap()
                : mTex(nullptr)
                , mOffset()
                , mSize()
                , mScaleFactor(1.F)
            {
            }

        public:
            Commons::Render::Texture* mTex;            
            glm::u32vec2 mOffset;
            glm::u32vec2 mSize;
            glm::vec2 mScaleFactor;
        };

        class Block
        {
        public:
            Block(const LightmapMgr* mgr);

        public: // TODO: private?
            const LightmapMgr *mMgr;
            Commons::Render::TexturePtr mLightmap;
            std::vector<uint16_t> mAllocVBorder;
        };

    public:
        enum EInterpolation
        {
            InterpolationNONE,
            InterpolationBilinear,
            InterpolationBicubic,
        };

    public:
        LightmapMgr(uint32_t blockSize, uint32_t numBlocks, uint32_t padding, EInterpolation interpolation, float scaleFactor);
        ~LightmapMgr();

        Lightmap allocLightmap(uint32_t width, uint32_t height, const uint8_t* data);
        void clear();

        void saveBlocksCache(const std::string& path);

    private:
        const LightmapMgr::Block* calcPlaceForLightmap(uint32_t width, uint32_t height, uint32_t& x, uint32_t& y);
    private:
        uint32_t mBlockWidth;
        uint32_t mBlockHeight;
        uint32_t mNumBlocks;
        std::vector<Block> mBlocks;
        uint32_t mPadding;
        EInterpolation mInterpolation;
        Commons::Imaging::IInterpolatorPtr mInterpolator;
        float mScaleFactor;
    };
}