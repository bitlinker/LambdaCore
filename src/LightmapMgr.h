#pragma once
#include <glm/glm.hpp>
#include <stdint.h>
#include <vector>
#include <Common/NonCopyable.h>
#include <Render/GLContext.h>

namespace LambdaCore
{
    class LightmapMgr : public Commons::NonCopyable
    {
    public:
        class Block
        {
        public:
            Block(const LightmapMgr* mgr);

        private:
            std::vector<uint8_t> mData;
            std::vector<uint8_t> mAllocVBorder;
            Commons::Render::TexturePtr mLightmap;
            const LightmapMgr *mMgr;
        };

    public:
        LightmapMgr(uint32_t blockSize, uint32_t numBlocks);
        ~LightmapMgr();

        Block* allocLightmap(uint32_t width, uint32_t height, const uint8_t* data, uint32_t& x, uint32_t y);

    private:
        uint32_t mBlockWidth;
        uint32_t mBlockHeight;
        uint32_t mNumBlocks;
        std::vector<Block> mBlocks;
    };
}