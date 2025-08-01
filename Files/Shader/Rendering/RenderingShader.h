#pragma once

#include <unordered_map>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

namespace Shader{
    class Shader{
    public:
        GLuint ID;
        Shader() = default;
        Shader(const char* vertexCode, const char* fragmentCode);

        static void UnsetActiveShaderCache();

        void Use() const;

        GLint GetUniformLocation(const std::string &name);

        // Uniform setters
        void SetBool(const std::string &name, bool value);
        void SetInt(const std::string &name, int value);
        void SetFloat(const std::string &name, float value);
        void SetMat4(const std::string &name, glm::mat4 value);
        void SetVec2(const std::string &name, glm::vec2 value);
        void SetVec3(const std::string &name, glm::vec3 value);
        void SetVec4(const std::string &name, glm::vec4 value);

    private:
        std::unordered_map<std::string, GLint> uniformLocationCache;
        static GLuint activeShaderID;
    };
}