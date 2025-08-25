#pragma once

#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace Shader{
    /// @brief Base class for OpenGL buffer objects
    class GLBufferBase {
    public:
        virtual ~GLBufferBase() = default;

        virtual void Bind() const = 0;
        static void Unbind();
    };

    /// @brief OpenGL buffer object wrapper
    /// @tparam T Data type stored in the buffer
    /// @tparam Target OpenGL buffer target (e.g., GL_ARRAY_BUFFER)
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

        bool IsInitialized() const { return ID != 0; }

        void Bind() const;
        static void Unbind();

        void SetData(const T& data, GLenum usage);
        void SetData(const T* data, GLuint size, GLenum usage);
        void SetData(const std::vector<T>& data, GLenum usage);
        void UpdateData(GLuint offset, const std::vector<T>& data) const;
        void UpdateData(GLuint offset, const T* data, GLuint size) const;

        template <GLenum otherTarget>
        void UploadBufferIn(GLuint copyOffset, GLuint writeOffset, GLBuffer<T, otherTarget>& buffer, GLuint size) const;

        void BindBufferBase(GLuint binding) const;

        void ClearBuffer();
        void ClearBuffer(const GLuint newSize, GLenum usage);

        void SetSize(GLuint size, bool silenceWarnings = false);
        GLuint GetSize() const { return bufferSize; };

        T* ReadBuffer() const;
        T* ReadBuffer(GLuint size) const;

        template<typename, GLenum>
        friend class GLBuffer;

    private:
        GLuint ID;
        GLint bufferSize = 0;
        std::string name;
    };
}

#include "GLBuffer.inl"