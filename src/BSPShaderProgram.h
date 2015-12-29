#pragma once
#include <Render/ShaderProgram.h>

namespace LambdaCore
{    
    class BSPShaderProgram : public Commons::Render::ShaderProgram
    {
    public:
        BSPShaderProgram();
        virtual ~BSPShaderProgram();

        void setMVP(const glm::mat4& matrix);
        void setNormal(const glm::vec3& normal);
        void setTexDiffuseSampler(int32_t sampler);
        void setTexLightmapSampler(int32_t sampler);
        void setLightness(float lightness);
        void setTextureMapping(const glm::vec3& vecS, const glm::vec3& vecT, const glm::vec2& texOffset, const glm::vec2& texSize, const glm::vec2& lmOffset, const glm::vec2& lmSize);

        uint32_t getVertexPositionLocation() const { return m_aVertexPos; }

    private:
        void init();

    private:
        uint32_t m_aVertexPos;

        uint32_t m_uMVP;
        uint32_t m_uNormal;
        uint32_t m_uTexDiffuse;
        uint32_t m_uTexLightmap;
        uint32_t m_uLightness;

        uint32_t m_uTexMatrix;
        uint32_t m_uTexOffset;
        uint32_t m_uTexSize;
        uint32_t m_uLmOffset;
        uint32_t m_uLmSize;
    };

    typedef std::shared_ptr<BSPShaderProgram> PlainShaderProgramPtr;
}
