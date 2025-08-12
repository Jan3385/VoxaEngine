#include "RenderingShader.h"

#include <iostream>
#include <fstream>
#include <sstream>

const std::string Shader::RenderShader::shaderDirectory = "Shaders/RenderShaders/";

Shader::RenderShader::RenderShader(const char* shaderPathName)
    : RenderShader(
        (shaderPathName + std::string(".vert")).c_str(),
        (shaderPathName + std::string(".frag")).c_str(),
        std::string(shaderPathName)) { }

Shader::RenderShader::RenderShader(const char* vertexPath, const char* fragmentPath, std::string shaderName)
{
    std::string printShaderName = shaderName.empty() ? "[Unnamed Render Shader] " : "[" + shaderName + "] ";

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
        std::cerr << printShaderName << "Error reading rendering shader files: " << e.what() << std::endl;
    }

    const char* vertexCodeCStr = vertexCode.c_str();
    const char* fragmentCodeCStr = fragmentCode.c_str();

    // Compile loaded shader
    GLuint vertexShader = this->CompileShader(vertexCodeCStr, printShaderName, GL_VERTEX_SHADER);
    GLuint fragmentShader = this->CompileShader(fragmentCodeCStr, printShaderName, GL_FRAGMENT_SHADER);

    // Create shader program
    GLint success;
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << printShaderName << "Error linking rendering shader program: " << infoLog << std::endl;
        ID = 0; // Prevent use of invalid shader
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
