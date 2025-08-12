#pragma once

#include <unordered_map>
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Shader
{
    class Shader{
    public:
        // Shader program or Compute shader based on context
        GLuint ID = 0;

        Shader() = default;
        ~Shader();

        // disable copy and move semantics
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = delete;
        Shader& operator=(Shader&&) = delete;

        static void UnsetActiveShaderCache();
        void Use() const;

        GLint GetUniformLocation(const std::string &name);

        // Uniform setters
        void SetBool(const std::string &name, bool value);
        void SetUnsignedInt(const std::string &name, unsigned int value);
        void SetInt(const std::string &name, int value);
        void SetFloat(const std::string &name, float value);
        void SetMat4(const std::string &name, glm::mat4 value);
        void SetVec2(const std::string &name, glm::vec2 value);
        void SetVec3(const std::string &name, glm::vec3 value);
        void SetVec4(const std::string &name, glm::vec4 value);
    protected:
        GLuint CompileShader(const char* shaderSource, const std::string& shaderName, GLenum shaderType);
    private:
        std::unordered_map<std::string, GLint> uniformLocationCache;
        static GLuint activeShaderID;
    };
}
