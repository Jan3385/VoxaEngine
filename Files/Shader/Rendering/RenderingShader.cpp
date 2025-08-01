#include "RenderingShader.h"

#include <iostream>

GLuint Shader::Shader::activeShaderID = 0;

Shader::Shader::Shader(const char* vertexCode, const char* fragmentCode)
{
    int success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexCode, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Error compiling vertex shader: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Error compiling fragment shader: " << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

/// @brief Unsets the active shader cache
/// Use this when you cannot guarantee that the shader cache will be valid (calling a shader elsewhere than in Shader::Use())
void Shader::Shader::UnsetActiveShaderCache()
{
    activeShaderID = 0;
}

void Shader::Shader::Use() const
{
    if (ID == 0)
        throw std::runtime_error("Shader program with ID 0 is not valid.");
    
    if(activeShaderID == ID) {
        return; // Already using this shader
    }

    Shader::activeShaderID = ID;
    glUseProgram(ID);
}

/***
 * Gets the location of a uniform variable with cacheing.
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
