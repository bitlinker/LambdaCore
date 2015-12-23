#include <Render/RenderCommon.h>
#include <PlainShaderProgram.h>
#include <Logger/Log.h>
#include <Exception/exception.h>

namespace LambdaCore
{
    static const std::string FRAGMENT_SHADER =
        "#version 400\n"
        "out vec4 frag_colour;"
        "uniform sampler2D u_tex1;"
        "uniform float u_lightness;"
        "in vec2 v_uv;"
        "in vec3 v_normal;"
        "void main () {"
        "  frag_colour =  + texture(u_tex1, v_uv);"
        "  frag_colour.r *=  u_lightness;"
        "  frag_colour.g *=  u_lightness;"
        "  frag_colour.b *=  u_lightness;"
        "  frag_colour.a = frag_colour.a > 0.5 ? 1.0 : 0.0;" // Alpha test
        //"  frag_colour = vec4(v_normal[0], v_normal[1], 1.0, 1.0);"
        //"  frag_colour = vec4(1.0, 1.0, 0.0, 1.0);"
        "}";

    static const std::string VERTEX_SHADER =
        "#version 400\n"
        //"layout(location = 0) in vec3 vp;"
        "in vec3 vp;"
        "uniform mat4 m_MVP;"
        "uniform vec3 u_normal;"
        "uniform vec3 u_vecS;"
        "uniform vec3 u_vecT;"
        "uniform vec2 u_distST;"
        "uniform vec2 u_texSize;"
        "out vec2 v_uv;"
        "out vec3 v_normal;"
        "void main () {"
        "  v_uv.x = (dot(u_vecS, vp) + u_distST.x) / u_texSize.x;"
        "  v_uv.y = (dot(u_vecT, vp) + u_distST.y) / u_texSize.y;"
        "  gl_Position = m_MVP * vec4(vp, 1.0);"
        "  v_normal = (m_MVP * vec4(u_normal, 0.0)).xyz;"
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

        //uint32_t a = m_shader->getNumAttributes();
        //uint32_t un = m_shader->getNumUniforms();

        // Attributes:
        m_aVertexPos = getAttributeLocation("vp");

        // TODO: helpers...
        // Uniforms:
        m_uMVP = getUniformLocation("m_MVP");
        m_uNormal = getUniformLocation("u_normal");
        m_uTex1 = getUniformLocation("u_tex1");
        m_uLightness = getUniformLocation("u_lightness");

        m_uVecS = getUniformLocation("u_vecS");
        m_uVecT = getUniformLocation("u_vecT");
        m_uDistST = getUniformLocation("u_distST");
        m_uTexSize = getUniformLocation("u_texSize");

        setUniformSampler2D(m_uTex1, 0);
    }

    void PlainShaderProgram::setMVP(const glm::mat4& matrix)
    {
        setUniformMatrix(m_uMVP, matrix);
    }

    void PlainShaderProgram::setNormal(const glm::vec3& normal)
    {
        setUniformVec3(m_uNormal, normal);
    }

    void PlainShaderProgram::setTex1Sampler(uint32_t sampler)
    {
        setUniformSampler2D(m_uTex1, sampler);
    }

    void PlainShaderProgram::setLightness(float lightness)
    {
        setUniformFloat(m_uLightness, lightness);
    }

    void PlainShaderProgram::setTextureMapping(const glm::vec3& vecS, const glm::vec3& vecT, const glm::vec2& distST, const glm::vec2& texSize)
    {
        setUniformVec3(m_uVecS, vecS);
        setUniformVec3(m_uVecT, vecT);
        setUniformVec2(m_uDistST, distST);
        setUniformVec2(m_uTexSize, texSize);
    }
}