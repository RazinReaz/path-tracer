#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

typedef struct Info
{
    bool hit;
    double t;
    glm::vec3 norm;
} Info;

class Ray {
private:
public:
    Ray();
    Ray(const glm::vec3 &origin, const glm::vec3& direction);
    void set_hit(const float& distance, const glm::vec3& normal);
private:
public:
    glm::vec3 origin;
    glm::vec3 direction;
    Info info;
};