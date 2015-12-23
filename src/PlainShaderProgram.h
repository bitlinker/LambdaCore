#pragma once
#include <Render/ShaderProgram.h>

namespace LambdaCore
{    
    class PlainShaderProgram : public Commons::Render::ShaderProgram
    {
    public:
        PlainShaderProgram();
        virtual ~PlainShaderProgram();

        void setMVP(const glm::mat4& matrix);
        void setNormal(const glm::vec3& normal);
        void setTex1Sampler(uint32_t sampler);
        void setLightness(float lightness);
        void setTextureMapping(const glm::vec3& vecS, const glm::vec3& vecT, const glm::vec2& distST, const glm::vec2& texSize);

        uint32_t getVertexPositionLocation() const { return m_aVertexPos; }

    private:
        void init();

    private:
        uint32_t m_aVertexPos;

        uint32_t m_uMVP;
        uint32_t m_uNormal;
        uint32_t m_uTex1;
        uint32_t m_uLightness;

        uint32_t m_uVecS;
        uint32_t m_uVecT;
        uint32_t m_uDistST;
        uint32_t m_uTexSize;
    };

    typedef std::shared_ptr<PlainShaderProgram> PlainShaderProgramPtr;
}
