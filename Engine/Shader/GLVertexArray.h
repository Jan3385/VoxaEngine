#pragma once

#include "Shader/Buffer/GLBuffer.h"

namespace Shader{
    template<typename T>
    struct GLType;

    template<> struct GLType<float>                 { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<int>                   { static constexpr GLenum value = GL_INT; };
    template<> struct GLType<unsigned int>          { static constexpr GLenum value = GL_UNSIGNED_INT; };
    template<> struct GLType<short int>             { static constexpr GLenum value = GL_SHORT; };
    template<> struct GLType<unsigned short int>    { static constexpr GLenum value = GL_UNSIGNED_SHORT; };
    template<> struct GLType<char>                  { static constexpr GLenum value = GL_BYTE; };
    template<> struct GLType<unsigned char>         { static constexpr GLenum value = GL_UNSIGNED_BYTE; };
    template<> struct GLType<glm::vec2>             { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<glm::vec3>             { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<glm::vec4>             { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<glm::ivec2>            { static constexpr GLenum value = GL_INT; };
    template<> struct GLType<glm::ivec3>            { static constexpr GLenum value = GL_INT; };
    template<> struct GLType<glm::ivec4>            { static constexpr GLenum value = GL_INT; };
    template<> struct GLType<glm::mat2>             { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<glm::mat3>             { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<glm::mat4>             { static constexpr GLenum value = GL_FLOAT; };
    template<> struct GLType<double>                { static constexpr GLenum value = GL_DOUBLE; };
    template<> struct GLType<glm::dvec2>            { static constexpr GLenum value = GL_DOUBLE; };
    template<> struct GLType<glm::dvec3>            { static constexpr GLenum value = GL_DOUBLE; };
    template<> struct GLType<glm::dvec4>            { static constexpr GLenum value = GL_DOUBLE; };
    template<> struct GLType<glm::umat2x2>          { static constexpr GLenum value = GL_UNSIGNED_INT; };
    template<> struct GLType<glm::umat3x3>          { static constexpr GLenum value = GL_UNSIGNED_INT; };
    template<> struct GLType<glm::umat4x4>          { static constexpr GLenum value = GL_UNSIGNED_INT; };
    template<> struct GLType<glm::bvec2>            { static constexpr GLenum value = GL_BOOL; };
    template<> struct GLType<glm::bvec3>            { static constexpr GLenum value = GL_BOOL; };
    template<> struct GLType<glm::bvec4>            { static constexpr GLenum value = GL_BOOL; };

    /// @brief OpenGL Vertex Array Object wrapper
    class GLVertexArray {
    public:
        GLVertexArray();
        GLVertexArray(std::string name);
        ~GLVertexArray();

        void Bind() const;
        void Unbind() const;

        //Disable copy
        GLVertexArray(const GLVertexArray&) = delete;
        GLVertexArray& operator=(const GLVertexArray&) = delete;
        //Enable move
        GLVertexArray(GLVertexArray&& other) noexcept
            : ID(other.ID), name(std::move(other.name)) {
            other.ID = 0;
            other.name.clear();
        }
        GLVertexArray& operator=(GLVertexArray&& other) noexcept {
            if (this != &other) {
                glDeleteVertexArrays(1, &ID);
                ID = other.ID;
                name = std::move(other.name);
                other.ID = 0;
                other.name.clear();
            }
            return *this;
        }

        template<typename AttribType, typename T>
        void AddAttribute(GLuint layoutIndex, GLint size, const GLBuffer<T, GL_ARRAY_BUFFER>& buffer, GLboolean normalized, size_t offset, GLuint divisor = 0) const;
        template<typename AttribType, typename T>
        void AddIntAttribute(GLuint layoutIndex, GLint size, const GLBuffer<T, GL_ARRAY_BUFFER>& buffer, size_t offset, GLuint divisor = 0) const;

    private:
        GLuint ID;
        std::string name;
    };
}

#include "Shader/GLVertexArray.inl"