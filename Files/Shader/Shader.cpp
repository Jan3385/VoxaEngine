#include "Shader.h"

#include <stdexcept>
#include <iostream>

GLuint Shader::Shader::activeShaderID = 0;

void Shader::Shader::Use() const
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[OnUSE-" << this->name << "] GL error: [" << err << "]" << std::endl;
    }

    if (ID == 0)
        throw std::runtime_error("Shader program with ID 0 is not valid.");
    
    if(activeShaderID == ID) {
        return; // Already using this shader
    }

    Shader::activeShaderID = ID;
    glUseProgram(ID);
}

Shader::Shader::~Shader()
{
    if (ID != 0) {
        glDeleteProgram(ID);
        ID = 0; // Reset ID to prevent accidental use after deletion
    }
}

/// @brief Unsets the active shader cache
/// Use this when you cannot guarantee that the shader cache will be valid (calling a shader elsewhere than in Shader::Use())
void Shader::Shader::UnsetActiveShaderCache()
{
    Shader::activeShaderID = 0;
}


/***
 * Gets the location of a uniform variable with caching.
 */
GLint Shader::Shader::GetUniformLocation(const std::string &name)
{
    // Return uniform if cached
    auto it = uniformLocationCache.find(name);
    if (it != uniformLocationCache.end()) {
        return it->second;
    }

    // Search for uniform location
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
    }
    uniformLocationCache[name] = location;
    return location;
}

void Shader::Shader::SetBool(const std::string &name, bool value)
{
    glUniform1i(this->GetUniformLocation(name.c_str()), (int)value);
}

void Shader::Shader::SetUnsignedInt(const std::string &name, unsigned int value)
{
    glUniform1ui(this->GetUniformLocation(name.c_str()), value);
}

void Shader::Shader::SetInt(const std::string &name, int value)
{
    glUniform1i(this->GetUniformLocation(name.c_str()), value);
}

void Shader::Shader::SetFloat(const std::string &name, float value)
{
    glUniform1f(this->GetUniformLocation(name.c_str()), value);
}

void Shader::Shader::SetMat4(const std::string &name, glm::mat4 value)
{
    glUniformMatrix4fv(this->GetUniformLocation(name.c_str()), 1, GL_FALSE, &value[0][0]);
}

void Shader::Shader::SetVec2(const std::string &name, glm::vec2 value)
{
    glUniform2fv(this->GetUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::Shader::SetVec3(const std::string &name, glm::vec3 value)
{
    glUniform3fv(this->GetUniformLocation(name.c_str()), 1, &value[0]);
}

void Shader::Shader::SetVec4(const std::string &name, glm::vec4 value)
{
    glUniform4fv(this->GetUniformLocation(name.c_str()), 1, &value[0]);
}

GLuint Shader::Shader::CompileShader(const char *shaderSource, const std::string &shaderName, GLenum shaderType)
{
    GLuint shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID, 1, &shaderSource, NULL);
    glCompileShader(shaderID);

    GLint success;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetShaderInfoLog(shaderID, 512, NULL, infoLog);
        std::cerr << "Error compiling " << shaderName << ": " << infoLog << std::endl;
    }

    return shaderID;
}
