#include "triangle.h"

Triangle::Triangle(const glm::vec3 &va, const glm::vec3 &vb, const glm::vec3 &vc,
         const glm::vec3 &na, const glm::vec3 &nb, const glm::vec3 &nc)
    : va(va), vb(vb), vc(vc), na(na), nb(nb), nc(nc)
    {
    }

glm::vec3
Triangle::interpolate_norm(const float u, const float v) {
    glm::vec3 norm = (1 - u - v) * this.na 
    + u * nb 
    + v * nc;
    return glm::normalize(norm); 
}

void
Triangle::calculate_hit_by(Ray& ray) {
    // moller trumbore algorithm

    glm::vec3 e1 = vb - va;
    glm::vec3 e2 = vc - va;
    
    glm::vec3 p = glm::cross(ray.direction, e2);
    float det = glm::dot(p, e1);
    if (det < 1e-5) 
        return;
    
    glm::vec3 ao = ray.origin - va;
    float u = glm::dot(p, ao);
    if (u < 0.0 || u > det)
        return;
    
    glm::vec3 q = glm::cross(ao, e1);
    float v = glm::dot(q, ray.direction);
    if (v < 0.0 || u + v > det)
        return;
    
    float distance = glm::dot(q, e2);
    if (distance < 0)
        return;
    float inv_det = 1.0 / det;
    u *= inv_det;
    v *= inv_det;
    distance *= inv_det;

    glm::vec3 normal = interpolate_norm(u, v);
    ray.set_hit(distance, normal);
    return;
}
