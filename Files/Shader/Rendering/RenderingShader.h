#pragma once

#include <unordered_map>
#include <glew.h>
#include <glm/glm.hpp>
#include <string>

namespace Shader{
    class Shader{
    public:
        GLuint ID;
        Shader() = default;
        Shader(const char* vertexCode, const char* fragmentCode);

        void Use() const;

        GLint GetUniformLocation(const std::string &name);

        // Uniform setters
        void SetBool(const std::string &name, bool value);
        void SetInt(const std::string &name, int value);
        void SetFloat(const std::string &name, float value);
        void SetMat4(const std::string &name, glm::mat4 value);

    private:
        std::unordered_map<std::string, GLint> uniformLocationCache;
    };
}