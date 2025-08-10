#pragma once

#include "math/vec3.h"
#include "ray.h"

class BoundingBox {
public:

    __host__ __device__ 
    BoundingBox(const vec3 &min, const vec3 &max);
    __host__ __device__
    bool intersected_by(const Ray& ray);

private:
    vec3 corners[2];
}

__host__ __device__
BoundingBox::BoundingBox(const vec3 &min, const vec3 &max)
{
    this->corners[0] = min;
    this->corners[1] = max;
}

__host__ __device__
bool BoundingBox::intersected_by(const Ray& ray, float &tNear) {
    float tmin = 0.0f, tmax = INFINITY;
    for (int d = 0; d < 3; d++)
    {
        bool sign = ray.inv_direction.data[d] < 0.0f;
        float bmin = corners[sign].data[d];
        float bmax = corners[!sign].data[d];

        float dmin = (bmin - ray.origin.data[d]) * (ray.inv_direction.data[d]);
        float dmax = (bmax - ray.origin.data[d]) * (ray.inv_direction.data[d]);

        tmin = fmaxf(dmin, tmin);
        tmax = fminf(dmax, tmax);
    }
    if (tmin < tmax) {
        tNear = tmin;
    }
    return tmin < tmax;
};
