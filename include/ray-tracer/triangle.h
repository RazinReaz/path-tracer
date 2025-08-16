#pragma once

#include "math/vec3.h"
#include "ray.h"

class Triangle {
private:
__host__ __device__ vec3 interpolate_norm(const float u, const float v);

public:
    vec3 va, vb, vc; //vertices
    vec3 na, nb, nc; //normals
    int material_index;
    __host__ __device__
    Triangle(const vec3 &va, const vec3 &vb, const vec3 &vc, const vec3 &na, const vec3 &nb, const vec3 &nc, int material_index);
    
    __host__ __device__
    void calculate_hit_by(Ray& ray);
};

__host__ __device__
Triangle::Triangle( const vec3 &va, const vec3 &vb, const vec3 &vc,
                    const vec3 &na, const vec3 &nb, const vec3 &nc,
                    int material_index
                )
    : va(va), vb(vb), vc(vc), na(na), nb(nb), nc(nc), material_index(material_index)
{}

__host__ __device__
vec3
Triangle::interpolate_norm(const float u, const float v)
{
    vec3 norm = (1 - u - v) * na 
    + u * nb 
    + v * nc;
    norm.normalize_self();
    return norm;
}

__host__ __device__
void
Triangle::calculate_hit_by(Ray &ray)
{
    // moller trumbore algorithm

    vec3 e1 = vb - va;
    vec3 e2 = vc - va;
    
    vec3 p = ray.direction.cross(e2);
    float det = p.dot(e1);
    if (det < 1e-5) 
        return;
    
    vec3 ao = ray.origin - va;
    float u = p.dot(ao);
    if (u < 0.0 || u > det)
        return;
    
    vec3 q = ao.cross(e1);
    float v = q.dot(ray.direction);
    if (v < 0.0 || u + v > det)
        return;
    
    float distance = q.dot(e2);
    if (distance < 0)
        return;
    float inv_det = 1.0f / det;
    u *= inv_det;
    v *= inv_det;
    distance *= inv_det;

    vec3 normal = interpolate_norm(u, v);
    float dot = ray.direction.dot(normal);
    if (dot > 0) normal = -1 * normal;
    ray.set_hit(distance, normal, material_index);
    return;
}
