//! THIS CODE IS AI WRITTEN because I didn't want to redo the same vec3 code
#pragma once
#include <math.h>
#include <cuda_runtime.h> // for float3, make_float3

// ================================
// Constructors
// ================================
__host__ __device__ inline float3 make_vec3(float x=0, float y=0, float z=0) {
    return make_float3(x, y, z);
}

// ================================
// Basic operators
// ================================
__host__ __device__ inline float3 operator+(const float3& a, const float3& b) {
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}
__host__ __device__ inline float3 operator-(const float3& a, const float3& b) {
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}
__host__ __device__ inline float3 operator-(const float3& v) {
    return make_float3(-v.x, -v.y, -v.z);
}
__host__ __device__ inline float3 operator*(const float3& a, float s) {
    return make_float3(a.x * s, a.y * s, a.z * s);
}
__host__ __device__ inline float3 operator*(float s, const float3& a) {
    return make_float3(a.x * s, a.y * s, a.z * s);
}
__host__ __device__ inline float3 operator*(const float3& a, const float3& b) {
    return make_float3(a.x * b.x, a.y * b.y, a.z * b.z);
}
__host__ __device__ inline float3 operator/(const float3& a, float s) {
    float inv = 1.0f / s;
    return make_float3(a.x * inv, a.y * inv, a.z * inv);
}
__host__ __device__ inline float3 operator/(const float3& a, const float3& b) {
    return make_float3(a.x / b.x, a.y / b.y, a.z / b.z);
}

// ================================
// Compound operators
// ================================
__host__ __device__ inline float3& operator+=(float3& a, const float3& b) {
    a.x += b.x; a.y += b.y; a.z += b.z; return a;
}
__host__ __device__ inline float3& operator-=(float3& a, const float3& b) {
    a.x -= b.x; a.y -= b.y; a.z -= b.z; return a;
}
__host__ __device__ inline float3& operator*=(float3& a, const float3& b) {
    a.x *= b.x; a.y *= b.y; a.z *= b.z; return a;
}
__host__ __device__ inline float3& operator*=(float3& a, float s) {
    a.x *= s; a.y *= s; a.z *= s; return a;
}
__host__ __device__ inline float3& operator/=(float3& a, float s) {
    float inv = 1.0f / s;
    a.x *= inv; a.y *= inv; a.z *= inv; return a;
}

// ================================
// Vector functions
// ================================
__host__ __device__ inline float dot(const float3& a, const float3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

__host__ __device__ inline float3 cross(const float3& a, const float3& b) {
    return make_float3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

__host__ __device__ inline float length(const float3& v) {
    return sqrtf(dot(v, v));
}

__host__ __device__ inline float length_squared(const float3& v) {
    return dot(v, v);
}

__host__ __device__ inline float3 normalize(const float3& v) {
#if defined(__CUDA_ARCH__)
    float invLen = rsqrtf(dot(v, v));
#else
    float invLen = 1.0f / sqrtf(dot(v, v));
#endif
    return v * invLen;
}

__host__ __device__ inline bool near_zero(const float3& v) {
    const float s = 1e-8f;
    return (fabsf(v.x) < s) && (fabsf(v.y) < s) && (fabsf(v.z) < s);
}

// ================================
// Debug print (host only)
// ================================
inline std::ostream& operator<<(std::ostream& os, const float3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
