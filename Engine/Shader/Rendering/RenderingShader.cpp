#include "RenderingShader.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "Debug/Logger.h"

using namespace Shader;

const std::string RenderShader::SHADER_EXTENSION_VERT = ".vert";
const std::string RenderShader::SHADER_EXTENSION_FRAG = ".frag";
const std::string RenderShader::SHADER_DIRECTORY = Shader::SHADER_DEFAULT_DIRECTORY + "RenderShaders/";

RenderShader::RenderShader(const char* shaderPathName)
    : RenderShader(
        (shaderPathName + SHADER_EXTENSION_VERT).c_str(),
        (shaderPathName + SHADER_EXTENSION_FRAG).c_str(),
        std::string(shaderPathName)) { }

RenderShader::RenderShader(const char* vertexPath, const char* fragmentPath, std::string shaderName)
{
    this->name = shaderName.empty() ? "Unnamed Render Shader" : shaderName;
    std::string printShaderName = shaderName.empty() ? "[Unnamed Render Shader] " : "[" + shaderName + "] ";

    std::string trueVertexPathStr = SHADER_DIRECTORY + std::string(vertexPath);
    std::string trueFragmentPathStr = SHADER_DIRECTORY + std::string(fragmentPath);
    const char* trueVertexPath = trueVertexPathStr.c_str();
    const char* trueFragmentPath = trueFragmentPathStr.c_str();

    // Load shader from files
    std::string vertexCode = this->LoadFileWithShaderPreprocessor(trueVertexPath, printShaderName);

    this->preprocessorAtFirstLine = true;   //reset preprocessor to compile another shader
    std::string fragmentCode = this->LoadFileWithShaderPreprocessor(trueFragmentPath, printShaderName);

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
        Debug::LogError(printShaderName + "Error linking rendering shader program: " + infoLog);
        ID = 0; // Prevent use of invalid shader
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}
