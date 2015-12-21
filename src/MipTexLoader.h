#pragma once
#include <stdint.h>
#include <Streams/IOStream.h>
#include <Streams/MemoryStream.h>
#include <Render/Texture.h>

namespace LambdaCore
{
    class MipTexLoader // TODO: public interface
    {
    public:        
        void loadTexture(const Commons::IOStreamPtr& stream, Commons::Render::TexturePtr texture);
        void loadTexture(const Commons::MemoryStreamPtr& stream, Commons::Render::TexturePtr texture);    
    };
}