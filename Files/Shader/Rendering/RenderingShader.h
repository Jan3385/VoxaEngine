#pragma once

#include "Shader/Shader.h"

namespace Shader{
    class RenderShader : public Shader::Shader {
    public:
        RenderShader() = default;
        RenderShader(const char* shaderPathName);
        RenderShader(const char* vertexPath, const char* fragmentPath, std::string shaderName);
        ~RenderShader() = default;
    private:
        static const std::string SHADER_EXTENSION_VERT;
        static const std::string SHADER_EXTENSION_FRAG;
        static const std::string SHADER_DIRECTORY;
    };
}