#pragma once

#include <math.h>
#include <iostream>

class vec3
{
private:
    __host__ __device__ float inverse_sqrt(const float number) const;

public:
    union
    {
        struct
        {
            float x, y, z, w;
        };
        struct
        {
            float r, g, b, a;
        };
        float data[4];
    };
    __host__ __device__ inline vec3(float x = 0, float y = 0, float z = 0);
    __host__ __device__ __forceinline__ void setXYZ(float x, float y, float z);
    __host__ __device__ __forceinline__ float length() const;
    __host__ __device__ __forceinline__ float length_squared() const;
    __host__ __device__ __forceinline__ vec3 normalize() const;
    __host__ __device__ __forceinline__ void normalize_self();
    __host__ __device__ __forceinline__ vec3 cross(const vec3 &v) const;
    __host__ __device__ __forceinline__ float dot(const vec3 &v) const;
    __host__ __device__ __forceinline__ vec3 rotate(float angle, const vec3 &axis);
    __host__ __device__ __forceinline__ vec3 scale(float sx, float sy, float sz);
    __host__ __device__ __forceinline__ vec3 scale(float s);
    __host__ __device__ __forceinline__ bool near_zero() const;
    // void something before scale()
    __host__ __device__ __forceinline__ vec3 operator+(const vec3 &v) const;
    __host__ __device__ __forceinline__ vec3 operator-(const vec3 &v) const;
    __host__ __device__ __forceinline__ vec3 operator*(const float& scalar) const;
    __host__ __device__ __forceinline__ vec3 operator*(const vec3 &v) const;
    __host__ __device__ __forceinline__ vec3 operator/(const float scalar) const;
    __host__ __device__ __forceinline__ vec3 operator/(const vec3 &v) const;
    __host__ __device__ __forceinline__ vec3& operator=(const vec3 &v);
    __host__ __device__ __forceinline__ vec3 operator+=(const vec3 &v);
    __host__ __device__ __forceinline__ vec3 operator-=(const vec3 &v);
    __host__ __device__ __forceinline__ vec3 operator*=(const vec3 &v);
    __host__ __device__ __forceinline__ vec3 operator-() const;
};

///////////////////////////////
//     old implementation    //
///////////////////////////////

// __host__ __device__ float vec3::inverse_sqrt(const float number) const
// {
//     {
//         //! don't ask. watch https://www.youtube.com/watch?v=p8u_k2LIZyo
//         const float threehalfs = 1.5F;
//         float x2 = number * 0.5F;
//         float y = number;
//         long i = *(long *)&y;
//         i = 0x5f3759df - (i >> 1);
//         y = *(float *)&i;
//         y = y * (threehalfs - (x2 * y * y));
//         y = y * ( threehalfs - ( x2 * y * y ) );
//         return y;
//     }
// }

__host__ __device__ float vec3::inverse_sqrt(const float x) const
{
#if defined(__CUDA_ARCH__)
    // Device code: use fast CUDA intrinsic
    return rsqrtf(x);
#else
    // Host code: use standard math
    return 1.0f / sqrtf(x);
#endif
}

__host__ __device__ inline vec3::vec3(float x, float y, float z)
    : x(x), y(y), z(z), w(1)
{
}

__host__ __device__ __forceinline__ void vec3::setXYZ(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = 1;
}

__host__ __device__ __forceinline__ float vec3::length() const
{
    return ::sqrtf(x * x + y * y + z * z);
}

__host__ __device__ __forceinline__ float vec3::length_squared() const
{
    return x * x + y * y + z * z;
}

__host__ __device__ __forceinline__ vec3 vec3::normalize() const
{
    float l = inverse_sqrt(x * x + y * y + z * z);
    return vec3(x * l, y * l, z * l);
}

__host__ __device__ __forceinline__ void vec3::normalize_self()
{
    float l = inverse_sqrt(x * x + y * y + z * z);
    x *= l;
    y *= l;
    z *= l;
}

__host__ __device__ __forceinline__ vec3 vec3::cross(const vec3 &v) const
{
    return vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}

__host__ __device__ __forceinline__ float vec3::dot(const vec3 &v) const
{
    return x * v.x + y * v.y + z * v.z;
}

__host__ __device__ __forceinline__ vec3 vec3::rotate(float angle, const vec3 &axis)
{
    // rotate vector about axis by angle
    const float epsilon = 1e-09f;
    // Rodrigues' rotation formula
    vec3 v = *this;
    vec3 a = axis.normalize();
    float cosa = ::cosf(angle) > epsilon ? ::cosf(angle) : 0;
    vec3 v_rot = v * cosa + a.cross(v) * ::sinf(angle) + a * (a.dot(v)) * (1 - cosa);
    return v_rot;
}

__host__ __device__ __forceinline__ bool vec3::near_zero() const
{
    const float s = 1e-8f;
    return (::fabsf(x) < s) && (::fabsf(y) < s) && (::fabsf(z) < s);
}

__host__ __device__ __forceinline__ vec3 vec3::operator+(const vec3 &v) const
{
    return vec3(x + v.x, y + v.y, z + v.z);
}

__host__ __device__ __forceinline__ vec3 vec3::operator-(const vec3 &v) const
{
    return vec3(x - v.x, y - v.y, z - v.z);
}

__host__ __device__ __forceinline__ vec3 vec3::operator*(const float& scalar) const
{
    return vec3(x * scalar, y * scalar, z * scalar);
}
// this is not a member function
__host__ __device__ __forceinline__ vec3 operator*(const float& scalar, const vec3 &v)
{
    return vec3(v.x * scalar, v.y * scalar, v.z * scalar);
}

__host__ __device__ __forceinline__ vec3 vec3::operator*(const vec3 &v) const
{
    return vec3(x * v.x, y * v.y, z * v.z);
}

__host__ __device__ __forceinline__ vec3 vec3::operator/(const float scalar) const
{
    return vec3(x / scalar, y / scalar, z / scalar);
}

__host__ __device__ __forceinline__ vec3 vec3::operator/(const vec3 &v) const
{
    return vec3(x / v.x, y / v.y, z / v.z);
}

__host__ __device__ __forceinline__ vec3 &vec3::operator=(const vec3 &v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    return *this;
}

__host__ __device__ __forceinline__ vec3 vec3::operator+=(const vec3 &v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}

__host__ __device__ __forceinline__ vec3 vec3::operator-=(const vec3 &v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

__host__ __device__ __forceinline__ vec3 vec3::operator*=(const vec3 &v)
{
    x *= v.x;
    y *= v.y;
    z *= v.z;
    return *this;
}

__host__ __device__ __forceinline__ vec3 vec3::operator-() const{
    return vec3(-x, -y, -z);
}

__host__ __device__ __forceinline__ vec3 vec3::scale(float sx, float sy, float sz)
{
    x *= sx;
    y *= sy;
    z *= sz;
    return *this;
}
__host__ __device__ __forceinline__ vec3 vec3::scale(float s)
{
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

// ostream overload for printing vec3
inline std::ostream &operator<<(std::ostream &os, const vec3 &v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

inline __host__ __device__ vec3 reflect(const vec3& R, const vec3& N) {
    return R - 2 * R.dot(N) * N;
}
inline __host__ __device__ vec3 refract(const vec3& R, const vec3& N, const float& ior1, const float& ior2) {
    // R has the direction from camera to surface (only for the first bounce)
    float cosI = fminf(-R.dot(N), 1.0f);
    float ratio = ior1 / ior2;
    float sinT2 = ratio * ratio * (1.0f - cosI * cosI);    
    vec3 r_out_parallel = -N * sqrtf(1.0f - sinT2);
    vec3 r_out_perp = ratio * (R + cosI * N);
    return r_out_parallel + r_out_perp;
}

