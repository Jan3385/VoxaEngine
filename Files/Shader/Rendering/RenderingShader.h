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
        static const std::string shaderDirectory;
    };
}