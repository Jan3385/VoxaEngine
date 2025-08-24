#include "GLVertexArray.h"

Shader::GLVertexArray::GLVertexArray()
{
    glGenVertexArrays(1, &ID);
}
Shader::GLVertexArray::GLVertexArray(std::string name)
{
    glGenVertexArrays(1, &ID);
    this->name = name;
    this->Bind();
    glObjectLabel(GL_VERTEX_ARRAY, ID, -1, this->name.c_str());
    this->Unbind();
}
Shader::GLVertexArray::~GLVertexArray()
{
    if(this->ID != 0) {
        glDeleteVertexArrays(1, &ID);
        ID = 0;
    }
}

void Shader::GLVertexArray::Bind() const
{
    glBindVertexArray(ID);
}

void Shader::GLVertexArray::Unbind() const
{
    glBindVertexArray(0);
}
