#include "RenderingShader.h"

Shader::Shader::Shader(const char* vertexCode, const char* fragmentCode)
{
    int success;
    char infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexCode, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Error compiling vertex shader: " << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Error compiling fragment shader: " << infoLog << std::endl;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);

    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::Shader::Use() const
{
    glUseProgram(ID);
}

/***
 * Gets the location of a uniform variable with cacheing.
 */
GLint Shader::Shader::GetUniformLocation(const std::string &name) const
{
    // Return uniform if cached
    auto it = uniformLocationCache.find(name);
    if (it != uniformLocationCache.end()) {
        return it->second;
    }

    // Search for uniform location
    GLint location = glGetUniformLocation(ID, name.c_str());
    if (location == -1) {
        std::cerr << "Warning: Uniform '" << name << "' not found in shader program." << std::endl;
    }
    uniformLocationCache[name] = location;
    return location;
}

void Shader::Shader::SetBool(const std::string &name, bool value) const
{
    glUniform1i(this->GetUniformLocation(name.c_str()), (int)value);
}

void Shader::Shader::SetInt(const std::string &name, int value) const
{
    glUniform1i(this->GetUniformLocation(name.c_str()), value);
}

void Shader::Shader::SetFloat(const std::string &name, float value) const
{
    glUniform1f(this->GetUniformLocation(name.c_str()), value);
}
