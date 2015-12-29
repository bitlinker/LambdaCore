#include <MipTexLoader.h>
#include <Render/ScopeBind.h>
#include <vector>

using namespace Commons;
using namespace Commons::Render;

namespace LambdaCore
{
    // TODO: same in BSP
    static const uint32_t HL_MAXTEXTURENAME = 16;
    static const uint32_t HL_MIPLEVELS = 4;

    struct MipTex
    {
        char mName[HL_MAXTEXTURENAME];
        uint32_t mWidth;
        uint32_t mHeight;
        uint32_t mOffsets[HL_MIPLEVELS];
    };

    MipTexLoader::MipTexLoader(const WADPtr& wad)
        : mWad(wad)
    {
    }

    MipTexLoader::~MipTexLoader()
    {
    }

    // TODO: pass GL context, create texture there
    void MipTexLoader::deserializeTexture(const Commons::MemoryStreamPtr& stream, bool isTransparent, Commons::Render::TexturePtr texture)
    {
        ScopeBind texBind(*texture);
        texture->setFilters(Texture::FILTER_LINEAR_MIPMAP_LINEAR, Texture::FILTER_LINEAR);
        texture->setMipLevels(0, HL_MIPLEVELS - 1);
        texture->setWrap(Texture::WRAP_REPEAT, Texture::WRAP_REPEAT);

        MipTex header;
        stream->read(&header, sizeof(MipTex));

        struct MipLevel
        {
            const uint8_t* pOrigData;
            uint32_t width;
            uint32_t height;
        };

        MipLevel mipLevels[HL_MIPLEVELS];
        
        uint32_t width = header.mWidth;
        uint32_t height = header.mHeight;

        for (uint32_t mipLevel = 0; mipLevel < HL_MIPLEVELS; ++mipLevel)
        {
            MipLevel& level = mipLevels[mipLevel];
            level.pOrigData = static_cast<const uint8_t*>(stream->getCurConstPtr());
            level.width = width;
            level.height = height;
            stream->seek(width * height, IOStream::ORIGIN_CUR);
            width >>= 1;
            height >>= 1;
        }

        // Padding
        uint16_t u1 = 0;
        stream->read(&u1, sizeof(uint16_t)); // Always 1. Flags?

        // Palette, RGB.
        const uint8_t *palPtr = static_cast<const uint8_t*>(stream->getCurConstPtr());

        // Upload texture data
        uint32_t bpp = isTransparent ? 4 : 3;
        std::vector<uint8_t> levelData(header.mWidth * header.mHeight * bpp);
        for (uint32_t mipLevel = 0; mipLevel < HL_MIPLEVELS; ++mipLevel)
        {
            MipLevel& level = mipLevels[mipLevel];
            for (uint32_t i = 0; i < level.width * level.height; ++i)
            {
                uint8_t* dstPtr = &levelData[bpp * i];
                const uint8_t src = level.pOrigData[i];
                if (isTransparent) 
                {
                    if (src == 255) // Last color in palette is transparent
                    {
                        for (uint32_t j = 0; j < bpp; ++j)
                            *dstPtr++ = 0;
                    }
                    else
                    {
                        for (uint32_t j = 0; j < bpp - 1; ++j)
                            *dstPtr++ = palPtr[3 * src + j];
                        *dstPtr++ = 255;
                    }                    
                }
                else
                {
                    for (uint32_t j = 0; j < bpp; ++j)
                        *dstPtr++ = palPtr[3 * src + j];
                }
            }
            texture->setData(mipLevel, isTransparent ? Texture::FORMAT_RGBA : Texture::FORMAT_RGB, level.width, level.height, &levelData[0]);
        }
    }

    bool MipTexLoader::loadTexture(const std::string& name, const void* extra, const Commons::Render::TexturePtr& texture)
    {
        MemoryStreamPtr stream = mWad->getEntryStream(name);
        if (stream)
        {
            bool isTransparent = (name[0] == '{');
            // TODO: catch exceptions in case of errors
            deserializeTexture(stream, isTransparent, texture);
            return true;
        }
        return false;
    }
}

