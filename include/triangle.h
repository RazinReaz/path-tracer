#pragma once

#include "ray.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Triangle {
private:
    glm::vec3 va, vb, vc; //vertices
    glm::vec3 na, nb, nc; //normals
    glm::vec3 interpolate_norm(const float u, const float v);

public:
    Triangle(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc,
             const glm::vec3 &na, const glm::vec3 &nb, const glm::vec3 &nc);
    void calculate_hit_by(Ray& ray);
};