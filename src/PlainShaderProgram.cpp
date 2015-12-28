#include <Render/RenderCommon.h>
#include <PlainShaderProgram.h>
#include <Logger/Log.h>
#include <Exception/exception.h>

namespace LambdaCore
{
    static const std::string FRAGMENT_SHADER =
        "#version 400\n"
        "out vec4 frag_colour;"
        "uniform sampler2D u_texDiffuse;"
        "uniform sampler2D u_texLightmap;"
        "uniform float u_lightness;"
        "in vec2 v_uvTexture;"
        "in vec2 v_uvLightmap;"
        "in vec3 v_normal;"
        "void main () {"
        "  vec4 color = texture(u_texDiffuse, v_uvTexture);"
        "  vec4 lightmap = texture(u_texLightmap, v_uvLightmap);"
        "  vec4 value = color * lightmap * u_lightness * 1.2 + 0.1;"
        "  frag_colour.rgb = value.rgb;"
        "  frag_colour.a = color.a;"
        "  frag_colour.a = frag_colour.a > 0.5 ? 1.0 : 0.0;" // Alpha test        
        "}";

    static const std::string VERTEX_SHADER =
        "#version 400\n"
        //"layout(location = 0) in vec3 vp;"
        "in vec3 a_vp;"
        "uniform mat4 u_MVP;"
        "uniform vec3 u_normal;"

        "uniform mat3 u_texMatrix;"
        "uniform vec2 u_texOffset;"
        "uniform vec2 u_texSize;"
        "uniform vec2 u_lmOffset;"
        "uniform vec2 u_lmSize;"

        "out vec2 v_uvTexture;"
        "out vec2 v_uvLightmap;"
        "out vec3 v_normal;"

        "void main () {"
        "  vec3 uv3 = a_vp * u_texMatrix;"
        "  v_uvTexture = (uv3.xy + u_texOffset) / u_texSize;"
        "  v_uvLightmap = (uv3.xy + u_lmOffset) / u_lmSize;"
        "  gl_Position = u_MVP * vec4(a_vp, 1.0);"
        "  v_normal = (u_MVP * vec4(u_normal, 0.0)).xyz;"
        "}";

    PlainShaderProgram::PlainShaderProgram()
        : ShaderProgram()
    {
        init();
    }

    PlainShaderProgram::~PlainShaderProgram()
    {
    }

    static void WriteShaderLog(const std::string& log)
    {
        if (!log.empty())
        {
            LOG_WARN(log.c_str());
        }
    }

    void PlainShaderProgram::init()
    {
        Commons::Render::ShaderPtr vs(new Commons::Render::Shader(GL_VERTEX_SHADER));
        vs->setSource(VERTEX_SHADER);
        LOG_INFO("Compiling plain vertex shader...");
        bool isCompiled = vs->compile();
        std::string log;
        vs->getInfoLog(log);
        WriteShaderLog(log);
        if (!isCompiled)
            throw Commons::GLException("Can't compile vertex shader. See log for details");

        Commons::Render::ShaderPtr fs(new Commons::Render::Shader(GL_FRAGMENT_SHADER));
        fs->setSource(FRAGMENT_SHADER);
        isCompiled = fs->compile();
        fs->getInfoLog(log);
        WriteShaderLog(log);
        if (!isCompiled)
            throw Commons::GLException("Can't compile fragment shader. See log for details");

        // Link
        attach(fs);
        attach(vs);

        bool isLinked = link();
        getInfoLog(log);
        WriteShaderLog(log);
        if (!isLinked)
            throw Commons::GLException("Can't link shader program. See log for details");

        bool isValidated = validate();
        getInfoLog(log);
        WriteShaderLog(log);
        if (!isValidated)
            throw Commons::GLException("Can't validate shader program. See log for details");

        // Attributes:
        m_aVertexPos = getAttributeLocation("a_vp");

        // TODO: helpers...
        // Uniforms:
        m_uMVP = getUniformLocation("u_MVP");
        m_uNormal = getUniformLocation("u_normal");
        m_uTexDiffuse = getUniformLocation("u_texDiffuse");
        m_uTexLightmap = getUniformLocation("u_texLightmap");
        m_uLightness = getUniformLocation("u_lightness");

        m_uTexMatrix = getUniformLocation("u_texMatrix");
        m_uTexOffset = getUniformLocation("u_texOffset");
        m_uTexSize = getUniformLocation("u_texSize");
        m_uLmOffset = getUniformLocation("u_lmOffset");
        m_uLmSize = getUniformLocation("u_lmSize");
    }

    void PlainShaderProgram::setMVP(const glm::mat4& matrix)
    {
        setUniformMat4(m_uMVP, matrix);
    }

    void PlainShaderProgram::setNormal(const glm::vec3& normal)
    {
        setUniformVec3(m_uNormal, normal);
    }

    void PlainShaderProgram::setTexDiffuseSampler(int32_t sampler)
    {
        setUniformSampler2D(m_uTexDiffuse, sampler);
    }

    void PlainShaderProgram::setTexLightmapSampler(int32_t sampler)
    {
        setUniformSampler2D(m_uTexLightmap, sampler);
    }

    void PlainShaderProgram::setLightness(float lightness)
    {
        setUniformFloat(m_uLightness, lightness);
    }

    void PlainShaderProgram::setTextureMapping(const glm::vec3& vecS, const glm::vec3& vecT, const glm::vec2& texOffset, const glm::vec2& texSize, const glm::vec2& lmOffset, const glm::vec2& lmSize)
    {
        glm::mat3 texMat(vecS, vecT, glm::vec3());
        setUniformMat3(m_uTexMatrix, texMat);
        setUniformVec2(m_uTexOffset, texOffset);
        setUniformVec2(m_uTexSize, texSize);
        setUniformVec2(m_uLmOffset, lmOffset);
        setUniformVec2(m_uLmSize, lmSize);
    }    
}