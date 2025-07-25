#pragma once

#include <curand_kernel.h>

#include "math/vec3.h"
#include "utils/vec3_utils.h"
#include "ray-tracer/ray.h"

enum MaterialType {
    LAMBERTIAN,
    METAL,
    EMISSIVE,
};


typedef struct Material {
    enum MaterialType type;
    vec3 albedo;
    vec3 emission;
} Material;


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
    switch (material.type) {
        case LAMBERTIAN:
            scatter_ray(ray, state);
            break;
        default:
            // For now, we only handle Lambertian materials
            // Other materials can be added later
            ray.set_origin_and_direction(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f));
            break;
    }
    return ray;
}
