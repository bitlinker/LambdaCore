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

    void MipTexLoader::loadTexture(const Commons::IOStreamPtr& stream, Commons::Render::TexturePtr texture)
    {
        Commons::MemoryStreamPtr memoryStream = std::dynamic_pointer_cast<Commons::MemoryStream>(stream);
        if (!memoryStream)
        {
            uint32_t size = stream->size();
            memoryStream.reset(new MemoryStream(size));
            stream->read(memoryStream->getPtr(), size);
        }
        loadTexture(memoryStream, texture);
    }

    // TODO: pass gl context, create texture there
    void MipTexLoader::loadTexture(const Commons::MemoryStreamPtr& stream, Commons::Render::TexturePtr texture)
    {
        ScopeBind texBind(*texture);
        texture->setMagFilter(GL_LINEAR);
        texture->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);
        // TODO: set clamping???

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

        for (int mipLevel = 0; mipLevel < 4; ++mipLevel)
        {
            MipLevel& level = mipLevels[mipLevel];
            level.pOrigData = static_cast<const uint8_t*>(stream->getCurConstPtr());
            level.width = width;
            level.height = height;
            stream->seek(width * height, IOStream::ORIGIN_CUR);
            width >>= 1;
            height >>= 1;
        }

        // Palette, RGB. Transparent color is 0? TODO: check
        const uint8_t *palPtr = static_cast<const uint8_t*>(stream->getCurConstPtr());
        
        // Upload texture data
        std::vector<uint8_t> levelData(header.mWidth * header.mHeight * 4);
        for (int mipLevel = 0; mipLevel < 4; ++mipLevel)
        {
            MipLevel& level = mipLevels[mipLevel];            
            for (uint32_t i = 0; i < level.width * level.height; ++i)
            {
                const uint8_t src = level.pOrigData[i];
                levelData[4 * i + 0] = palPtr[3 * src + 2];     // R
                levelData[4 * i + 1] = palPtr[3 * src + 1];     // G
                levelData[4 * i + 2] = palPtr[3 * src + 0];     // B
                levelData[4 * i + 3] = src == 0 ? 0 : 255;      // A
            }
            texture->setTexData2d(mipLevel, GL_RGBA, level.width, level.height, GL_RGBA, GL_UNSIGNED_BYTE, &levelData[0]);
        }
        // TODO: more mip levels if need?     
    }
}

