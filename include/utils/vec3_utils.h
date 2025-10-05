#pragma once

#include <math.h>
#include <cuda_runtime.h>
#include <curand_kernel.h>
#include "math/defines.h"
#include "math/vec3.h"


__device__ __host__ __forceinline__ vec3 lerp(const vec3& a, const vec3& b, const float& t) {
    return a * (1.0f - t) + b * t;
}

__device__ __host__ __forceinline__ float lerp(const float& a, const float& b, const float& t) {
    return a * (1.0f - t) + b * t;
}

__device__ inline vec3 random_vec_on_disk(curandStatePhilox4_32_10_t *state) {
    float4 rand4 = curand_uniform4(state);
    float u = rand4.x * 2.0f - 1.0f;
    float v = rand4.y * 2.0f - 1.0f;

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

__device__ inline vec3 unit_vec3_on_hemisphere(curandStatePhilox4_32_10_t *state){
    // give us an uniformly distributed random unit 3d vector on a hemisphere on the xy plane 
    // uses Malley's cosine weighted sampling technique
    vec3 v = random_vec_on_disk(state);
    float z = sqrtf(fmaxf(0.0f, 1.0f - v.x * v.x - v.y * v.y));
    return vec3(v.x, v.y, z);
}

__device__ inline void orthonormal_basis_frisvad(const vec3& normal, vec3& tangent, vec3& bitangent) {
    /* 
        tangent: X axis
        bitangent: Y axis
        normal: Z axis
    */
    if (normal.z < - 0.999f) {
        tangent.setXYZ(0.0f, -1.0f, 0.0f);
        bitangent.setXYZ(-1.0f, 0.0f, 0.0f);
    }
    else {
        float a = 1.0f / (1.0f + normal.z);
        float b = - normal.x * normal.y * a;
        tangent.setXYZ(1.0f - normal.x * normal.x * a, b, - normal.x);
        bitangent.setXYZ(b, 1.0f - normal.y * normal.y * a, - normal.y);
    }
}

//! orthonormal basis by frisvad
__device__ inline vec3 rotate_to_align_hemiZ_to_normal(const vec3& v, const vec3& normal) {
    // the v vector is a uniformly sampled unit vector on the +Z hemisphere
    // we need to transform it to the orthonormal basis of the normal on the surface
    vec3 up = fabsf(normal.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangent = up.cross(normal).normalize();
    vec3 bitangent = normal.cross(tangent);

    return v.x * tangent + v.y * bitangent + v.z * normal;
}


__device__ vec3 scatter_along(vec3 normal, curandStatePhilox4_32_10_t *state) {
    vec3 dir = unit_vec3_on_hemisphere(state);
    dir = rotate_to_align_hemiZ_to_normal(dir, normal);
    return dir;
}

__host__ bool is_zero_vector(float x, float y, float z) {
    return x == 0.0f && y == 0.0f && z == 0.0f;
}




// __host__ __device__ 
// float fresnel(const vec3& r, const vec3& n, float ior1, float ior2) {
//     float r0 = (ior1 - ior2) / (ior1 + ior2);
//     r0 = r0 * r0;

//     // Use the cosine of the incident angle.
//     float cosI = fminf(-r.dot(n), 1.0f);

//     // If exiting a dense medium, check for Total Internal Reflection
//     if (ior1 > ior2) {
//         float ratio = ior1 / ior2;
//         float sinT2 = ratio * ratio * (1.0f - cosI * cosI);
//         if (sinT2 > 1.0f) {
//             return 1.0f; // TIR means 100% reflection
//         }
//         // When exiting, the formula is more accurate using the transmitted angle's cosine
//         float cosT = sqrtf(1.0f - sinT2);
//         float x = 1.0f - cosT;
//         return r0 + (1.0f - r0) * x * x * x * x * x;
//     }

//     // Otherwise, use the standard Schlick approximation with the incident angle
//     float x = 1.0f - cosI;
//     return r0 + (1.0f - r0) * x * x * x * x * x;
// }

__host__ __device__ float fresnel(float& cosine, float& ior1, float& ior2) {
    float x = 1 - cosine;
    float r0 = (ior1 - ior2) / (ior1 + ior2);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * x * x * x * x * x;
}



