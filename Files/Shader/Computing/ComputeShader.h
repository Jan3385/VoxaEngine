#pragma once

#include "Shader/Shader.h"
#include <cstring>
#include <iostream>

namespace Shader{
    class ComputeShader : public Shader::Shader {
    public:
        ComputeShader() = default;
        ComputeShader(const char* shaderPathName);
        ComputeShader(const char* computePath, std::string shaderName);
        ~ComputeShader() = default;

        void Run(GLuint workGroupX, GLuint workGroupY, GLuint workGroupZ) const;

        static void BindBufferAt(GLuint bindingPoint, GLuint buffer);

        template<typename T>
        static void UploadDataToBuffer(GLuint buffer, const T* data, size_t size);

        template<typename T>
        static T *ReadDataFromBuffer(GLuint buffer, size_t size);
    private:
        static const std::string shaderDirectory;
    };
}

template <typename T>
void Shader::ComputeShader::UploadDataToBuffer(GLuint buffer, const T *data, size_t size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size * sizeof(T), data, GL_STATIC_DRAW);
}

/**
 * Reads data from the output buffer and returns a pointer to an array of type T.
 * @attention The caller is responsible for deleting the returned pointer!
 * @tparam T The type of the elements to read from the buffer.
 * @param buffer The buffer to read data from.
 * @param size The number of elements to read from the buffer.
 * @return A pointer to an array of type T containing the data read from the output buffer.
 */
template<typename T>
T *Shader::ComputeShader::ReadDataFromBuffer(GLuint buffer, size_t size)
{
    if(buffer == 0) {
        throw std::runtime_error("Cannot read from a NULL compute shader buffer");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);

    T* data = static_cast<T*>(glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY));
    if(!data) {
        throw std::runtime_error("Failed to map compute shader buffer for reading data (NULLPTR)");
    }

    T* result = new T[size];
    std::memcpy(result, data, size * sizeof(T));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    return result;
}
