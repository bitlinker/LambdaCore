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

    // TODO: pass gl context, create texture there
    void MipTexLoader::deserializeTexture(const Commons::MemoryStreamPtr& stream, bool isTransparent, Commons::Render::TexturePtr texture)
    {
        ScopeBind texBind(*texture);
        texture->setMagFilter(GL_LINEAR);
        texture->setMinFilter(GL_LINEAR_MIPMAP_LINEAR);

        // TODO: set clamping???        
        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_REPEAT);

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
        uint16_t u1 = 0;
        stream->read(&u1, sizeof(uint16_t)); // Always 1. Flags?
        
        const uint8_t *palPtr = static_cast<const uint8_t*>(stream->getCurConstPtr());
        
        // TODO: no alpha if texture have no transparent pixels
        // TODO: transparent textures starts with '{'
        //if (isTransparent)

        // Upload texture data
        std::vector<uint8_t> levelData(header.mWidth * header.mHeight * 4);
        for (int mipLevel = 0; mipLevel < 4; ++mipLevel)
        {
            MipLevel& level = mipLevels[mipLevel];            
            for (uint32_t i = 0; i < level.width * level.height; ++i)
            {
                const uint8_t src = level.pOrigData[i];
                levelData[4 * i + 0] = palPtr[3 * src + 0];     // R 
                levelData[4 * i + 1] = palPtr[3 * src + 1];     // G
                levelData[4 * i + 2] = palPtr[3 * src + 2];     // B

                // TODO: optimize. They are fucking insane. Why blue is transparent color key?
                //if (palPtr[3 * src + 2] == 255 && palPtr[3 * src + 1] == 0 && palPtr[3 * src + 0] == 0)
                if (isTransparent && src == 255)
                {
                    // TODO: set color to nearest pixels
                    levelData[4 * i + 0] = 0;
                    levelData[4 * i + 1] = 0;
                    levelData[4 * i + 2] = 0;
                    levelData[4 * i + 3] = 0;
                }
                else
                {
                    levelData[4 * i + 3] = 255;
                }
            }
            texture->setTexData2d(mipLevel, GL_RGBA, level.width, level.height, GL_RGBA, GL_UNSIGNED_BYTE, &levelData[0]);
        }
        // TODO: more mip levels if need?     
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

