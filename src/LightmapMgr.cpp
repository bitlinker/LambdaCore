#include <cassert>
#include <LightmapMgr.h>
#include <Logger/Log.h>

using namespace Commons::Render;

namespace LambdaCore
{
    LightmapMgr::Block::Block(const LightmapMgr* mgr)
        : mMgr(mgr)
        , mLightmap(new Texture())
        , mAllocVBorder(mgr->mBlockWidth)
    {
        // TODO: default lightmap color???
        mLightmap->setFilters(Texture::FILTER_LINEAR, Texture::FILTER_LINEAR);
        mLightmap->setWrap(Texture::WRAP_CLAMP, Texture::WRAP_CLAMP);
        mLightmap->setData(0, Texture::FORMAT_RGB, mgr->mBlockWidth, mgr->mBlockHeight, nullptr); // Prealloc texture data
    }

    LightmapMgr::LightmapMgr(uint32_t blockSize, uint32_t numBlocks)
        : mBlockWidth(blockSize)
        , mBlockHeight(blockSize)
        , mBlocks()
        , mPadding(1)
    {
        // TODO: prealloc blocks?
    }

    LightmapMgr::~LightmapMgr()
    {
    }

    const LightmapMgr::Block* LightmapMgr::calcPlaceForLightmap(uint32_t width, uint32_t height, uint32_t& x, uint32_t& y)
    {
        // TODO: check if w & h > block here?...
        assert(width <= mBlockWidth);
        assert(height <= mBlockHeight);

        uint16_t btmLevel, topLevel;
        uint32_t w1, w2;
        
        for (uint32_t blockIndex = 0; ; ++blockIndex) // TODO: iterator?
        {
            // Add new block if needed
            if (blockIndex == mBlocks.size())
            {
                mBlocks.push_back(Block(this));
            }

            Block* curBlock = &mBlocks[blockIndex];
            btmLevel = static_cast<uint16_t>(mBlockHeight);
            for (w1 = 0; w1 < mBlockWidth - width; ++w1)
            {
                topLevel = 0;
                for (w2 = 0; w2 < width; ++w2)
                {
                    uint16_t curLevel = curBlock->mAllocVBorder[w1 + w2]; // TODO: max block = 256!
                    if (curLevel >= btmLevel)
                        break;
                    if (curLevel >= topLevel)
                        topLevel = curLevel;
                }

                if (w2 == width) // Valid horizontal location
                {
                    x = w1;
                    y = btmLevel = topLevel;
                }
            }
            
            if (btmLevel <= mBlockHeight - height) // Vertical space is also valid
            {
                std::fill(curBlock->mAllocVBorder.begin(), curBlock->mAllocVBorder.end(), btmLevel + height);
                return curBlock;
            }
        }
    }

    LightmapMgr::Lightmap LightmapMgr::allocLightmap(uint32_t width, uint32_t height, const uint8_t* data, uint32_t magFactor)
    {
        uint32_t x, y;
        const Block* blockPtr = calcPlaceForLightmap(width + 2 * mPadding, height + 2 * mPadding, x, y);
        x += mPadding;
        y += mPadding;
        // TODO: padding color?
        blockPtr->mLightmap->setSubData(0, Texture::FORMAT_RGB, x, y, width, height, data);

        Lightmap result;
        result.mTex = blockPtr->mLightmap.get();
        result.mX = x;
        result.mY = y;
        result.mW = mBlockWidth;
        result.mH = mBlockHeight;
        result.mMagFactor = magFactor;
        return result;
    }

    void LightmapMgr::clear() 
    {
        mBlocks.clear();
    }
}