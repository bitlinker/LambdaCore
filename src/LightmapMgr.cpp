#include <cassert>
#include <LightmapMgr.h>
#include <Logger/Log.h>

namespace LambdaCore
{   
    LightmapMgr::Block::Block(const LightmapMgr* mgr)
        : mMgr(mgr)
    {
        mMgr->mBlockHeight;
    }

    LightmapMgr::LightmapMgr(uint32_t blockSize, uint32_t numBlocks)
        : mBlockWidth(blockSize)
        , mBlockHeight(blockSize)
        , mBlocks(numBlocks, this)
    {
    }

    LightmapMgr::~LightmapMgr()
    {
    }

    LightmapMgr::Block* LightmapMgr::allocLightmap(uint32_t width, uint32_t height, const uint8_t* data, uint32_t& x, uint32_t y)
    {
        assert(width <= mBlockWidth);
        assert(height <= mBlockHeight);

        return nullptr;
    }
}