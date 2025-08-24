namespace Shader {
    template<typename AttribType, typename T>
    void GLVertexArray::AddAttribute(GLuint layoutIndex, GLint size, const GLBuffer<T, GL_ARRAY_BUFFER>& buffer, 
        GLboolean normalized, size_t offset, GLuint divisor) const{
        
        buffer.Bind();
        glEnableVertexAttribArray(layoutIndex);

        size_t stride = std::is_class<T>::value ? sizeof(T) : 0;

        glVertexAttribPointer(layoutIndex, size, GLType<AttribType>::value, normalized, stride, (void*)offset);

        if (divisor > 0)
            glVertexAttribDivisor(layoutIndex, divisor);
    }

    template<typename AttribType, typename T>
    void GLVertexArray::AddIntAttribute(GLuint layoutIndex, GLint size, const GLBuffer<T, GL_ARRAY_BUFFER>& buffer, 
        size_t offset, GLuint divisor) const{

        buffer.Bind();
        glEnableVertexAttribArray(layoutIndex);

        size_t stride = std::is_class<T>::value ? sizeof(T) : 0;

        glVertexAttribIPointer(layoutIndex, size, GLType<AttribType>::value, stride, (void*)offset);

        if (divisor > 0)
            glVertexAttribDivisor(layoutIndex, divisor);
    }
}