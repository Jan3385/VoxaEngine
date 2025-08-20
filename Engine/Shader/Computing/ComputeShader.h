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
        static void BindAtomicCounterAt(GLuint bindingPoint, GLuint atomicCounter);

        template<typename T>
        static void UploadDataToBuffer(GLuint buffer, const T* data, size_t size);

        template<typename T>
        static T *ReadDataFromBuffer(GLuint buffer, size_t size);

         template<typename T>
        static T ReadDataFromAtomicCounter(GLuint counter);
    private:
        static const std::string SHADER_EXTENSION;
        static const std::string SHADER_DIRECTORY;
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
    if(size == 0)
        return nullptr;

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

template <typename T>
inline T Shader::ComputeShader::ReadDataFromAtomicCounter(GLuint counter)
{
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter);
    T result = T();
    glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(T), &result);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
    return result;
}
