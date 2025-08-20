#include "ComputeShader.h"

#include <fstream>
#include <sstream>

using namespace Shader;

const std::string ComputeShader::SHADER_EXTENSION = ".comp";
const std::string ComputeShader::SHADER_DIRECTORY = Shader::SHADER_DEFAULT_DIRECTORY + "ComputeShaders/";

ComputeShader::ComputeShader(const char *shaderPathName)
    : ComputeShader(
        (shaderPathName + SHADER_EXTENSION).c_str(),
        std::string(shaderPathName)) { }

ComputeShader::ComputeShader(const char *computePath, std::string shaderName)
{
    this->name = shaderName.empty() ? "Unnamed Compute Shader" : shaderName;
    std::string printShaderName = shaderName.empty() ? "[Unnamed Compute Shader] " : "[" + shaderName + "] ";

    std::string trueComputePath = SHADER_DIRECTORY + std::string(computePath);

    // Load shader from file
    std::string computeCode = this->LoadFileWithShaderPreprocessor(trueComputePath, printShaderName);

    // Compile loaded shader
    const char *computeCodeCStr = computeCode.c_str();
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

void ComputeShader::Run(GLuint workGroupX, GLuint workGroupY, GLuint workGroupZ) const
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[PRE-" << this->name << "] GL error: [" << err << "]" << std::endl;
    }

    this->Use();

    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[" << this->name << "] GL error: [" << err << "]" << std::endl;
    }
}

void ComputeShader::BindBufferAt(GLuint bindingPoint, GLuint buffer)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buffer);
}

void ComputeShader::BindAtomicCounterAt(GLuint bindingPoint, GLuint atomicCounter)
{
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, bindingPoint, atomicCounter);
}
