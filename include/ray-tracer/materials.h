#pragma once

#include <curand_kernel.h>

#include "math/vec3.h"
#include "utils/vec3_utils.h"
#include "ray-tracer/ray.h"

enum class MaterialType {
    LAMBERTIAN,
    METAL,
    EMISSIVE,
};


typedef struct Material {
    MaterialType type;
    vec3 albedo;
} Material;


__device__
void scatter_ray(Ray ray, curandState_t *state)
{
    //! Razin maybe too many new vec3s
    vec3 normal = ray.info.norm;
    
    vec3 neworigin = ray.origin + ray.direction * ray.info.t + normal * 0.001f; // offset to avoid self-intersection
    vec3 newdir = unit_vec3_on_hemisphere(state);
    newdir = rotate_to_align_hemiZ_to_normal(newdir, normal);
    
    ray.set_origin_and_direction(neworigin, newdir);
}

__device__ 
Ray bounce(Ray &ray, Material &material, curandState_t *state)
{
    switch (material.type) {
        case MaterialType::LAMBERTIAN:
            scatter_ray(ray, state);
            break;
        default:
            // For now, we only handle Lambertian materials
            // Other materials can be added later
            break;
    }
    return ray;
}
