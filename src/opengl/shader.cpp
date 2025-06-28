#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include <string>
#include <stdexcept>

std::string readFile(const char *filePath)
{
    // Check if the file path is valid
    if (filePath == nullptr || std::string(filePath).empty())
    {
        std::cerr << "Error: File path is null or empty." << std::endl;
        return "";
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file: " << filePath << std::endl;
        return "";
    }

    // Check for file read errors
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (file.fail())
    {
        std::cerr << "Error: Failed to read file: " << filePath << std::endl;
        file.close();
        return "";
    }

    file.close();
    return buffer.str();
}

Shader::Shader(const char *vertexPath, const char *fragmentPath)
{
    //retrieve the vertex/fragment source code from filePath
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);
    
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();
    
    int success;
    char infoLog[512];
    
    // vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    
    // shader program
    ID = glCreateProgram();
    glAttachShader(ID, vertexShader);
    glAttachShader(ID, fragmentShader);
    glLinkProgram(ID);
    glGetProgramiv(ID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(ID, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::use()
{
    glUseProgram(ID);
}
void Shader::del()
{
    glDeleteProgram(ID);
}

void Shader::setBool(const std::string &uniformVarName, bool value) const
{
    int location = glGetUniformLocation(ID, uniformVarName.c_str());
    if (location == -1)
    {
        std::cerr << "ERROR::SHADER::UNIFORM::BOOL::NOT_FOUND\n" << uniformVarName << std::endl;
    }
    glUniform1i(location, (int)value);
}

void Shader::setInt(const std::string &uniformVarName, int value) const
{
    int location = glGetUniformLocation(ID, uniformVarName.c_str());
    if (location == -1)
    {
        std::cerr << "ERROR::SHADER::UNIFORM::INT::NOT_FOUND\n" << uniformVarName << std::endl;
    }
    glUniform1i(location, value);
}

void Shader::setFloat(const std::string &uniformVarName, float value) const
{
    int location = glGetUniformLocation(ID, uniformVarName.c_str());
    if (location == -1)
    {
        std::cerr << "ERROR::SHADER::UNIFORM::FLOAT::NOT_FOUND\n" << uniformVarName << std::endl;
    }
    glUniform1f(location, value);
}

void Shader::setMat4(const std::string &uniformVarName, glm::mat4 mat) const
{
    int location = glGetUniformLocation(ID, uniformVarName.c_str());
    if (location == -1)
    {
        std::cerr << "ERROR::SHADER::UNIFORM::MATRIX::NOT_FOUND\n" << uniformVarName << std::endl;
    }
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const std::string &uniformVarName, const float x, const float y, const float z) const
{
    int location = glGetUniformLocation(ID, uniformVarName.c_str());
    if (location == -1)
    {
        std::cerr << "ERROR::SHADER::UNIFORM::VECTOR::NOT_FOUND\n" << uniformVarName << std::endl;
    }
    glUniform3fv(location, 1, glm::value_ptr(glm::vec3(x, y, z)));
}

void Shader::setVec3(const std::string &uniformVarName, const glm::vec3 vect) const
{
    int location = glGetUniformLocation(ID, uniformVarName.c_str());
    if (location == -1)
    {
        std::cerr << "ERROR::SHADER::UNIFORM::VECTOR::NOT_FOUND\n" << uniformVarName << std::endl;
    }
    glUniform3fv(location, 1, glm::value_ptr(vect));
}