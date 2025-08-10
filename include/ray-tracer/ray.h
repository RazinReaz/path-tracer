#pragma once

#include "math/vec3.h"
#include <ostream>


typedef struct Info
{
    bool hit;
    double t;
    vec3 norm;
    int mat_idx;
} Info;

class Ray {
private:
public:
    // __host__ __device__ Ray();
    __host__ __device__ Ray(const vec3 &origin, const vec3& direction);
    __host__ __device__ void set_hit(const float &distance, const vec3 &normal, const int& material_index);
    __host__ __device__ void set_origin_and_direction(const vec3& orig, const vec3& dir);
    __host__ __device__ void reset_hit();
private:
public:
    vec3 origin;
    vec3 direction;
    vec3 inv_direction;
    Info info;
};

// __host__ __device__
// Ray::Ray()
//     : origin(0.0f, 0.0f, 0.0f), direction(0.0f, 0.0f, 0.0f)
// {
//     info.hit = false;
//     info.t = 10000.0;
//     info.norm = vec3(0.0f, 0.0f, 0.0f);
//     info.mat_idx = -1;
// }

__host__ __device__
Ray::Ray(const vec3 &orig, const vec3 &dir)
    : origin(orig), direction(dir.normalize())
{
    inv_direction = vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
    info.hit = false;
    info.t = 10000.0;
    info.norm = vec3(0.0f, 0.0f, 0.0f);
    info.mat_idx = -1;
}

__host__ __device__
void
Ray::set_hit(const float &distance, const vec3 &normal, const int &material_index)
{
    if (distance < 0 || distance > this->info.t)
        return;

    this->info.t = distance;
    this->info.hit = true;
    this->info.norm = normal;
    this->info.mat_idx = material_index;
}

__host__ __device__ 
void 
Ray::set_origin_and_direction(const vec3& orig, const vec3& dir){
    this->origin = orig;
    this->direction = dir;
    this->inv_direction = vec3(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);
}

__host__ __device__
void
Ray::reset_hit(){
    info.hit = false;
    info.t = 10000.0;
    info.norm = vec3(0.0f, 0.0f, 0.0f);
    info.mat_idx = -1;
}

