#include "RenderingShader.h"

#include <iostream>
#include <fstream>
#include <sstream>

const std::string Shader::Shader::shaderDirectory = "Shaders/";
GLuint Shader::Shader::activeShaderID = 0;

Shader::Shader::Shader(const char* shaderPathName)
    : Shader(
        (shaderPathName + std::string(".vert")).c_str(),
        (shaderPathName + std::string(".frag")).c_str(),
        std::string(shaderPathName)) { }

Shader::Shader::Shader(const char* vertexPath, const char* fragmentPath, std::string shaderName)
{
    std::string printShaderName = shaderName.empty() ? "[Unnamed Shader] " : "[" + shaderName + "] ";

    std::string trueVertexPathStr = shaderDirectory + std::string(vertexPath);
    std::string trueFragmentPathStr = shaderDirectory + std::string(fragmentPath);
    const char* trueVertexPath = trueVertexPathStr.c_str();
    const char* trueFragmentPath = trueFragmentPathStr.c_str();

    // Load shader from files
    std::string vertexCode;
    std::string fragmentCode;

    std::ifstream vertexFile;
    vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    std::ifstream fragmentFile;
    fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        vertexFile.open(trueVertexPath);
        fragmentFile.open(trueFragmentPath);

        std::stringstream vertexStream, fragmentStream;

        vertexStream << vertexFile.rdbuf();
        fragmentStream << fragmentFile.rdbuf();

        vertexCode = vertexStream.str();
        fragmentCode = fragmentStream.str();
    }
    catch (const std::ifstream::failure& e) {
        std::cerr << printShaderName << "Error reading shader files: " << e.what() << std::endl;
    }

    const char* vertexCodeCStr = vertexCode.c_str();
    const char* fragmentCodeCStr = fragmentCode.c_str();

    // Compile loaded shader
    int success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexCodeCStr, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << printShaderName << "Error compiling vertex shader: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentCodeCStr, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << printShaderName << "Error compiling fragment shader: " << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << printShaderName << "Error linking shader program: " << infoLog << std::endl;
        ID = 0; // Prevent use of invalid shader
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
