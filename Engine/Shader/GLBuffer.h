#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Shader{
    class GLBufferBase {
    public:
        virtual ~GLBufferBase() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
    };

    template<typename T, GLenum Target>
    class GLBuffer : public GLBufferBase {
    public:
        GLBuffer();
        GLBuffer(std::string name);
        ~GLBuffer();

        // Disable copy
        GLBuffer(const GLBuffer&) = delete;
        GLBuffer& operator=(const GLBuffer&) = delete;
        // Movable
        GLBuffer(GLBuffer&& other) noexcept;
        GLBuffer& operator=(GLBuffer&& other) noexcept;

        void Bind() const;
        void Unbind() const;

        void SetData(const T& data, GLenum usage);
        void SetData(const T* data, GLuint size, GLenum usage);
        void SetData(const std::vector<T>& data, GLenum usage);
        void UpdateData(GLuint offset, const std::vector<T>& data) const;
        void UpdateData(GLuint offset, const T* data, GLuint size) const;

        void BindBufferBase(GLuint binding) const;

        void ClearBuffer();
        void ClearBuffer(const GLuint newSize, GLenum usage);

        void SetSize(GLuint size, bool silenceWarnings = false);
        GLuint GetSize() const { return bufferSize; };

        T* ReadBuffer() const;
        T* ReadBuffer(GLuint size) const;

    private:
        GLuint ID;
        GLint bufferSize = 0;
        std::string name;
    };
}

#include "GLBuffer.inl"