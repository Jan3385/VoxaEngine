#include <iostream>
#include <cstring>
#include "GLVertexArray.h"
#include "GLBuffer.h"

namespace Shader{

template<typename T, GLenum Target>
GLBuffer<T, Target>::GLBuffer() {
    this->ID = 0;
}

template <typename T, GLenum Target>
inline GLBuffer<T, Target>::GLBuffer(std::string name)
{
    glGenBuffers(1, &ID);
    this->name = name;
    this->Bind();
    glObjectLabel(GL_BUFFER, ID, -1, this->name.c_str());
    this->Unbind();
}
template <typename T, GLenum Target>
inline GLBuffer<T, Target>::~GLBuffer()
{
    glDeleteBuffers(1, &ID);
    ID = 0;
}

template <typename T, GLenum Target>
inline GLBuffer<T, Target>::GLBuffer(GLBuffer &&other) noexcept
    : ID(other.ID), bufferSize(other.bufferSize), name(std::move(other.name)) 
{
    other.ID = 0;
    other.bufferSize = 0;
    other.name.clear();
}

template <typename T, GLenum Target>
inline GLBuffer<T, Target> &GLBuffer<T, Target>::operator=(GLBuffer<T, Target> &&other) noexcept
{
    if (this != &other) {
        glDeleteBuffers(1, &ID);
        ID = other.ID;
        bufferSize = other.bufferSize;
        name = std::move(other.name);
        other.ID = 0;
        other.bufferSize = 0;
        other.name.clear();
    }
    return *this;
}

template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::Bind() const
{
    if(ID == 0){
        throw std::runtime_error("Attempt to bind uninitialized GLBuffer: " + this->name);
    }

    glBindBuffer(Target, ID);
}

template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::Unbind() const
{
    glBindBuffer(Target, 0);
}

/// @brief Sets a variable for the buffer object (Causes reallocation)
/// @param data Variable to set
/// @param usage GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW,..
template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::SetData(const T &data, GLenum usage)
{
    this->Bind();
    glBufferData(Target, sizeof(T), &data, usage);
    this->bufferSize = 1;
}

/// @brief Sets a data array (Causes reallocation)
/// @param data Array to set
/// @param size Size of the array
/// @param usage GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW,..
template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::SetData(const T *data, GLuint size, GLenum usage)
{
    this->Bind();
    glBufferData(Target, size * sizeof(T), data, usage);
    this->bufferSize = size;
}

/// @brief Sets a data vector (Causes reallocation)
/// @param data Vector to set
/// @param usage GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW,..
template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::SetData(const std::vector<T> &data, GLenum usage)
{
    this->Bind();
    glBufferData(Target, data.size() * sizeof(T), data.data(), usage);
    this->bufferSize = static_cast<GLint>(data.size());
}

/// @brief Updates a portion of the buffer with new data
/// @param offset Offset in the buffer to update
/// @param data New data vector to update
template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::UpdateData(GLuint offset, const std::vector<T> &data) const
{
    if (offset + data.size() > this->bufferSize) {
        std::cerr << "[" << this->name << "] GLBuffer::UpdateData VEC - Error: Attempt to update buffer data out of bounds!" << std::endl;
    }

    this->Bind();
    glBufferSubData(Target, offset * sizeof(T), data.size() * sizeof(T), data.data());
}

/// @brief Updates a portion of the buffer with new data
/// @param offset Offset in the buffer to update
/// @param data New data array to update
/// @param size Size of the array
template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::UpdateData(GLuint offset, const T *data, GLuint size) const
{
    if (offset + size > static_cast<GLuint>(this->bufferSize)) {
        std::cerr << "[" << this->name << "] GLBuffer::UpdateData ARR - Error: Attempt to update buffer data out of bounds!" << std::endl;
    }

    this->Bind();
    glBufferSubData(Target, offset * sizeof(T), size * sizeof(T), data);
}

template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::BindBufferBase(GLuint binding) const
{
    if(ID == 0){
        throw std::runtime_error("Attempt to bind base uninitialized GLBuffer: " + this->name);
    }

    glBindBufferBase(Target, binding, ID);
}

template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::ClearBuffer()
{
    if(ID == 0){
        throw std::runtime_error("Attempt to clear uninitialized GLBuffer: " + this->name);
    }
    this->Bind();

    #ifdef GL_VERSION_4_3
    if constexpr (std::is_same_v<T, float>) {
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, nullptr);
    }else{
        void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        if (ptr) {
            memset(ptr, 0, this->bufferSize * sizeof(T));
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
    }
    #else
    // Fallback - map and memset to zero
    this->Bind();
    void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    if (ptr) {
        memset(ptr, 0, size * sizeof(T));
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }
    #endif
}

template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::ClearBuffer(const GLuint newSize, GLenum usage)
{
    this->Bind();
    if(this->bufferSize == static_cast<GLint>(newSize))
        this->ClearBuffer();
    else{
        std::vector<T> emptyData(newSize);
        this->SetData(emptyData, usage);
    }
}
/// @brief Sets the size variable inside the object
/// @warning Does not reallocate the buffer. ! Only use this when you know what you are doing !
/// @param size New size (in number of elements) of the buffer
/// @param silenceWarnings suppresses warnings at undefined or dangerous behaviour
template <typename T, GLenum Target>
inline void GLBuffer<T, Target>::SetSize(GLuint size, bool silenceWarnings)
{
    if(!silenceWarnings) {
        if (size == 0)
            std::cerr << "[" << this->name << "] GLBuffer::SetSize - Warning: Setting buffer size to 0!" << std::endl;
        
        if(size > static_cast<GLuint>(this->bufferSize))
            std::cerr << "[" << this->name << "] GLBuffer::SetSize - Warning: New size is larger than current buffer size! Without reallocation this may lead to undefined behaviour!" << std::endl;
    }

    this->bufferSize = size;
}

/// @brief Reads the buffer data
/// @warning Pointer needs to be deleted after use (Memory leak!)
/// @return Pointer to the buffer data
template <typename T, GLenum Target>
inline T *GLBuffer<T, Target>::ReadBuffer() const
{
    this->Bind();

    T* data = static_cast<T*>(glMapBuffer(Target, GL_READ_ONLY));
    if (!data) {
        std::cerr << "[" << this->name << "] GLBuffer::ReadBuffer - Error: Failed to map buffer to client address space for reading!" << std::endl;
        return nullptr;
    }

    T* result = new T[this->bufferSize];
    std::memcpy(result, data, this->bufferSize * sizeof(T));
    glUnmapBuffer(Target);
    return result;
}

template <typename T, GLenum Target>
inline T *GLBuffer<T, Target>::ReadBuffer(GLuint size) const
{
    if(this->bufferSize < static_cast<GLint>(size)){
        std::cerr << "[" << this->name << "] GLBuffer::ReadBuffer - Warning: Requested size is larger than current buffer size! Returning only available data." << std::endl;
        size = this->bufferSize;
    }

    this->Bind();

    T* data = static_cast<T*>(glMapBuffer(Target, GL_READ_ONLY));
    if (!data) {
        std::cerr << "[" << this->name << "] GLBuffer::ReadBuffer - Error: Failed to map buffer to client address space for reading!" << std::endl;
        return nullptr;
    }

    T* result = new T[size];
    std::memcpy(result, data, size * sizeof(T));
    glUnmapBuffer(Target);
    return result;
}
}