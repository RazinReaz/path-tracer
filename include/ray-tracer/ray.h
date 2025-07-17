#pragma once

#include "math/vec3.h"
#include <ostream>

typedef struct Info
{
    bool hit;
    double t;
    vec3 norm;
} Info;

class Ray {
private:
public:
    __host__ __device__
    Ray();
    __host__ __device__
    Ray(const vec3 &origin, const vec3& direction);
    __host__ __device__ void set_hit(const float &distance, const vec3 &normal);
    __host__ friend std::ostream& operator<<(std::ostream& os, const Ray& ray);
private:
public:
    vec3 origin;
    vec3 direction;
    Info info;
};

__host__
inline std::ostream& operator<<(std::ostream& os, const Ray& ray) {
    os << "Ray(origin: " << ray.origin << ", direction: " << ray.direction << ")";
    return os;
}

__host__ __device__
Ray::Ray()
    : origin(0.0f, 0.0f, 0.0f), direction(0.0f, 0.0f, 0.0f)
{
    info.hit = false;
    info.t = 10000.0;
    info.norm = vec3(0.0f, 0.0f, 0.0f);
}

__host__ __device__
Ray::Ray(const vec3 &orig, const vec3 &dir)
    : origin(orig), direction(dir.normalize())
{
    info.hit = false;
    info.t = 10000.0;
    info.norm = vec3(0.0f, 0.0f, 0.0f);
}

__host__ __device__
void
Ray::set_hit(const float &distance, const vec3 &normal)
{
    if (distance < 0 || distance > this->info.t)
        return;

    this->info.t = distance;
    this->info.hit = true;
    this->info.norm = normal;
}