#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <vector>
#include <Common/NonCopyable.h>
#include <Render/GLContext.h>
#include <Render/Texture.h>

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
                , mX(0), mY(0)
                , mW(0), mH(0)
                , mMagFactor(1)
            {
            }

        public:
            Commons::Render::Texture* mTex;
            uint32_t mX, mY;
            uint32_t mW, mH;
            uint32_t mMagFactor;
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
        LightmapMgr(uint32_t blockSize, uint32_t numBlocks);
        ~LightmapMgr();

        Lightmap allocLightmap(uint32_t width, uint32_t height, const uint8_t* data, uint32_t magFactor);
        void clear();

    private:
        const LightmapMgr::Block* calcPlaceForLightmap(uint32_t width, uint32_t height, uint32_t& x, uint32_t& y);
    private:
        uint32_t mBlockWidth;
        uint32_t mBlockHeight;
        uint32_t mNumBlocks;
        std::vector<Block> mBlocks;
        uint32_t mPadding;
    };
}