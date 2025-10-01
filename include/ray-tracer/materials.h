#pragma once

#include <curand_kernel.h>
#include <iostream>

#include "math/vec3.h"
#include "utils/vec3_utils.h"
#include "ray-tracer/ray.h"

enum MaterialType {
    DIFFUSE,
    SPECULAR,
};


typedef struct Material {
    enum MaterialType type;
    vec3 albedo;
    vec3 emission;
    float ior;
    float roughness;
    float metalness;
} Material;



inline std::ostream& operator<<(std::ostream& os, const Material& mat) {
    os << "Material(";
    switch (mat.type) {
        case DIFFUSE: os << "DIFFUSE"; break;
        case SPECULAR:   os << "SPECULAR"; break;
        default:         os << "UNKNOWN"; break;
    }
    os << ", albedo: (" << mat.albedo.x << ", " << mat.albedo.y << ", " << mat.albedo.z << ")";
    os << ", emission: (" << mat.emission.x << ", " << mat.emission.y << ", " << mat.emission.z << ")";
    os << ", ior: " << mat.ior;
    os << ", roughness:" << mat.roughness;
    os << ", metalness: " << mat.metalness;
    os << ") size: " << sizeof(Material);
    return os;
}



__device__
void scatter_ray(Ray& ray, curandState_t *state)
{
    // //! ChatGPT maybe too many new vec3s
    vec3 normal = ray.info.norm;
    
    vec3 neworigin = ray.origin + ray.direction * ray.info.t + normal * 0.001f; // offset to avoid self-intersection
    vec3 newdir = unit_vec3_on_hemisphere(state);
    newdir = rotate_to_align_hemiZ_to_normal(newdir, normal);
    
    ray.set_origin_and_direction(neworigin, newdir);
    ray.reset_hit();
}

__device__ 
Ray bounce(Ray &ray, Material &material, curandState_t *state)
{
    // handles DIFFUSE and METALLIC materials
    switch (material.type) {
        case MaterialType::DIFFUSE:
            scatter_ray(ray, state);
            break;
        default:
            // For now, we only handle diffuse materials
            // Other materials can be added later
            break;
    }
    return ray;
}
