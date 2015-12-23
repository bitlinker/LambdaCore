#pragma once
#include <stdint.h>
#include <Streams/IOStream.h>
#include <Streams/MemoryStream.h>
#include <Render/ITextureLoader.h>
#include <WAD.h>

namespace LambdaCore
{
    class MipTexLoader : public Commons::NonCopyable, public Commons::Render::ITextureLoader
    {
    public:
        MipTexLoader(const WADPtr& wad);
        virtual ~MipTexLoader();

        virtual bool loadTexture(const std::string& name, const void* extra, const Commons::Render::TexturePtr& texture) override;
    private:
        void deserializeTexture(const Commons::MemoryStreamPtr& stream, bool isTransparent, Commons::Render::TexturePtr texture);

    private:
        WADPtr mWad; // TODO: multiple WADs, map built-in textures
    };
    typedef std::shared_ptr<MipTexLoader> MipTexLoaderPtr;
}