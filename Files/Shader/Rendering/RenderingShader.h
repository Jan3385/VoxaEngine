#pragma once

#include <unordered_map>
#include <glew.h>

namespace Shader{
    class Shader{
    public:
        GLuint ID;
        Shader() = default;
        Shader(const char* vertexCode, const char* fragmentCode);

        void Use() const;

        GLint GetUniformLocation(const std::string &name) const;

        // Uniform setters
        void SetBool(const std::string &name, bool value) const;
        void SetInt(const std::string &name, int value) const;
        void SetFloat(const std::string &name, float value) const;

    private:
        std::unordered_map<std::string, GLint> uniformLocationCache;
    };
}