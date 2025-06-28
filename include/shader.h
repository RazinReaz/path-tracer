#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader
{
public:
    unsigned int ID;
    Shader(const char *vertexPath, const char *fragmentPath);

    void use();
    void del();

    // utility uniform functions
    void setBool(const std::string &uniformVarName, bool value) const;
    void setInt(const std::string &uniformVarName, int value) const;
    void setFloat(const std::string &uniformVarName, float value) const;
    void setMat4(const std::string &uniformVarName, glm::mat4 mat) const;
    void setVec3(const std::string &uniformVarName, const float x, const float y, const float z) const;
    void setVec3(const std::string &uniformVarName, const glm::vec3 vect) const;

};