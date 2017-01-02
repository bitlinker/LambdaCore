#include <cassert>
#include <LightmapMgr.h>
#include <Logger/Log.h>
#include <Imaging/BilinearInterpolator.h>
#include <Imaging/BicubicInterpolator.h>

using namespace Commons::Render;

namespace LambdaCore
{
    static const uint32_t LIGHTMAP_CHANNELS = 3;

    LightmapMgr::Block::Block(const LightmapMgr* mgr)
        : mMgr(mgr)
        , mLightmap(new Texture())
        , mAllocVBorder(mgr->mBlockWidth)
    {
        mLightmap->setFilters(Texture::FILTER_LINEAR, Texture::FILTER_LINEAR);
        mLightmap->setWrap(Texture::WRAP_CLAMP_TO_EDGE, Texture::WRAP_CLAMP_TO_EDGE); // TODO: Makes no sense, map is always clamped by design
        mLightmap->setData(0, Texture::FORMAT_RGB, mgr->mBlockWidth, mgr->mBlockHeight, nullptr); // Preallocate texture data
    }

    LightmapMgr::LightmapMgr(uint32_t blockSize, uint32_t numBlocks, uint32_t padding, EInterpolation interpolation, float scaleFactor)
        : mBlockWidth(blockSize)
        , mBlockHeight(blockSize)
        , mBlocks()
        , mPadding(padding)
        , mInterpolator()
        , mScaleFactor(scaleFactor)
    {
        mBlocks.reserve(numBlocks);
        for (uint32_t i = 0; i < numBlocks; ++i)
        {
            mBlocks.push_back(Block(this));
        }

        switch (interpolation)
        {
        case InterpolationNONE:
            assert(mScaleFactor == 1.F);
            break;
        case InterpolationBilinear:
            mInterpolator.reset(new Commons::Imaging::BilinearInterpolator());
            break;
        case InterpolationBicubic:
            mInterpolator.reset(new Commons::Imaging::BicubicInterpolator());
            break;
        default:
            assert(0 && "Unknown interpolation type");
        }
    }

    LightmapMgr::~LightmapMgr()
    {
    }

    const LightmapMgr::Block* LightmapMgr::calcPlaceForLightmap(uint32_t width, uint32_t height, uint32_t& x, uint32_t& y)
    {
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
                    uint16_t curLevel = curBlock->mAllocVBorder[w1 + w2];
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

    LightmapMgr::Lightmap LightmapMgr::allocLightmap(uint32_t width, uint32_t height, const uint8_t* data)
    {        
        Lightmap result;

        // Interpolate image
        Commons::Imaging::ImageInfo srcInfo(width, height, LIGHTMAP_CHANNELS);
        Commons::Imaging::ImageInfo dstInfo;
        std::vector<uint8_t> dstData;
        const uint8_t *pData;        
        if (mInterpolator && mScaleFactor != 1.F)
        {
            dstInfo = Commons::Imaging::ImageInfo(width * mScaleFactor, height * mScaleFactor, srcInfo.getNumChannels());
            dstData.resize(dstInfo.getImageSize());
            if (!mInterpolator->interpolateImage(data, srcInfo, &dstData[0], dstInfo))
            {
                LOG_ERROR("Can't interpolate lightmap");
                return result;
            }
            pData = &dstData[0];
        }
        else
        {
            dstInfo = srcInfo;
            pData = data;
        }

        // Find place in blocks
        uint32_t x, y;
        const Block* blockPtr = calcPlaceForLightmap(dstInfo.getWidth() + 2 * mPadding, dstInfo.getHeight() + 2 * mPadding, x, y);
        x += mPadding;
        y += mPadding;

        // Upload
        // TODO: set padding color?
        blockPtr->mLightmap->setSubData(0, Texture::FORMAT_RGB, x, y, dstInfo.getWidth(), dstInfo.getHeight(), pData);

        result.mTex = blockPtr->mLightmap.get();
        result.mOffset = glm::u32vec2(x, y);
        result.mSize = glm::u32vec2(mBlockWidth, mBlockHeight);
        result.mScaleFactor = glm::vec2((float)dstInfo.getWidth() / srcInfo.getWidth(), (float)dstInfo.getHeight() / srcInfo.getHeight());
        return result;
    }

    void LightmapMgr::saveBlocksCache(const std::string& path)
    {
        // TODO: save; debug interpolation issues

    }

    void LightmapMgr::clear() 
    {
        mBlocks.clear();
    }
}