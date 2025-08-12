#include "ComputeShader.h"

#include <fstream>
#include <sstream>

const std::string Shader::ComputeShader::shaderDirectory = "Shaders/ComputeShaders/";

Shader::ComputeShader::ComputeShader(const char *shaderPathName)
    : ComputeShader(
        (shaderPathName + std::string(".comp")).c_str(),
        std::string(shaderPathName)) { }

Shader::ComputeShader::ComputeShader(const char *computePath, std::string shaderName)
{
    std::string printShaderName = shaderName.empty() ? "[Unnamed Compute Shader] " : "[" + shaderName + "] ";

    std::string trueComputePathStr = shaderDirectory + std::string(computePath);
    const char *trueComputePath = trueComputePathStr.c_str();

    // Load shader from file
    std::string computeCode;
    
    std::ifstream computeFile;
    computeFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try{
        computeFile.open(trueComputePath);
        std::stringstream computeStream;

        computeStream << computeFile.rdbuf();
        
        computeCode = computeStream.str();
    }
    catch (const std::ifstream::failure& e) {
        std::cerr << printShaderName << "Error reading compute shader file: " << e.what() << std::endl;
    }

    const char *computeCodeCStr = computeCode.c_str();

    // Compile loaded shader
    GLuint computeShader = this->CompileShader(computeCodeCStr, printShaderName, GL_COMPUTE_SHADER);

    // Create shader program
    GLint success;
    ID = glCreateProgram();
    glAttachShader(ID, computeShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[512];
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << printShaderName << "Error linking compute shader program: " << infoLog << std::endl;
        ID = 0; // Prevent use of invalid shader
    }

    glDeleteShader(computeShader);
}

void Shader::ComputeShader::Run(GLuint workGroupX, GLuint workGroupY, GLuint workGroupZ) const
{
    this->Use();

    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Shader::ComputeShader::BindBufferAt(GLuint bindingPoint, GLuint buffer)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffer);
}
