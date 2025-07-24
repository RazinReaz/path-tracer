#pragma once

#include <math.h>
#include <curand_kernel.h>
#include "math/defines.h"
#include "math/vec3.h"

__device__ inline vec3 random_vec_on_disk(curandState_t *state) {
    float u = curand_uniform(state) * 2.0f - 1.0f;
    float v = curand_uniform(state) * 2.0f - 1.0f;

    if (u == 0.0f && v == 0.0f) return vec3(0.0f, 0.0f, 0.0f);

    float theta, r;
    if (fabsf(u) > fabsf(v))
    {
        r = u;
        theta = PIover4 * (v / u);
    }
    else
    {
        r = v;
        theta = PIover2 - PIover4 * (u / v);
    }
    return vec3(r * cosf(theta), r * sinf(theta), 0.0f);
}

__device__ inline vec3 unit_vec3_on_hemisphere(curandState_t *state){
    // give us an uniformly distributed random unit 3d vector on a hemisphere on the xy plane 
    // uses Malley's cosine weighted sampling technique
    vec3 v = random_vec_on_disk(state);
    float z = sqrtf(fmaxf(0.0f, 1.0f - v.x * v.x - v.y * v.y));
    return vec3(v.x, v.y, z);
}


__device__ inline vec3 rotate_to_align_hemiZ_to_normal(const vec3& v, const vec3& normal) {
    // the v vector is a uniformly sampled unit vector on the +Z hemisphere
    // we need to transform it to the orthonormal basis of the normal on the surface
    vec3 up = fabsf(normal.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangent = up.cross(normal).normalize();
    vec3 bitangent = normal.cross(tangent);

    return v.x * tangent + v.y * bitangent + v.z * normal;
}


__device__ vec3 scatter_along(vec3 normal, curandState_t *state) {
    vec3 dir = unit_vec3_on_hemisphere(state);
    dir = rotate_to_align_hemiZ_to_normal(dir, normal);
    return dir;
}