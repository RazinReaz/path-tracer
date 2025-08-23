#pragma once

#include <cfloat> // for FLT_MAX
#include "utils/cuda_utils.h"
#include "math/vec3.h"
#include "ray.h"
#include "triangle.h"

#define EPSILON 1e-6f


class BoundingBox
{
public:
    __host__ __device__ BoundingBox();
    __host__ __device__ BoundingBox(const vec3 &min, const vec3 &max);
    __host__ __device__ BoundingBox& operator=(const BoundingBox &other);
    __host__ __device__ float intersection_distance(const Ray &ray);
    __host__ __device__ void grow(const Triangle &tri);
    __host__ __device__ void grow(const BoundingBox &other);

    float corners[2][3];
private:
    __host__ __device__ void fixCorners();
};

__host__ __device__ void BoundingBox::fixCorners() {
    if (corners[0][0] == corners[1][0]){
        corners[0][0] -= EPSILON;
        corners[1][0] += EPSILON;
    }
    if (corners[0][1] == corners[1][1]){
        corners[0][1] -= EPSILON;
        corners[1][1] += EPSILON;
    }
    if (corners[0][2] == corners[1][2]){
        corners[0][2] -= EPSILON;
        corners[1][2] += EPSILON;
    }
}

__host__ __device__
BoundingBox::BoundingBox()
{
    corners[0][0] = FLT_MAX - EPSILON;
    corners[0][1] = FLT_MAX - EPSILON;
    corners[0][2] = FLT_MAX - EPSILON;
    corners[1][0] = -FLT_MAX + EPSILON;
    corners[1][1] = -FLT_MAX + EPSILON;
    corners[1][2] = -FLT_MAX + EPSILON;
    fixCorners();
}

__host__ __device__
BoundingBox& BoundingBox::operator=(const BoundingBox &other)
{
    if (this != &other) {
        corners[0][0] = other.corners[0][0];
        corners[0][1] = other.corners[0][1];
        corners[0][2] = other.corners[0][2];
        corners[1][0] = other.corners[1][0];
        corners[1][1] = other.corners[1][1];
        corners[1][2] = other.corners[1][2];
    }
    return *this;
}

__host__ __device__
BoundingBox::BoundingBox(const vec3 &min, const vec3 &max)
{
    this->corners[0][0] = min.x;
    this->corners[0][1] = min.y;
    this->corners[0][2] = min.z;
    this->corners[1][0] = max.x;
    this->corners[1][1] = max.y;
    this->corners[1][2] = max.z;

    fixCorners();
}

__host__ __device__ float BoundingBox::intersection_distance(const Ray &ray) {
    float tmin = 0.0f, tmax = FLT_MAX;
    for (int d = 0; d < 3; d++)
    {
        bool sign = ray.inv_direction.data[d] < 0.0f;
        float bmin = corners[sign][d];
        float bmax = corners[!sign][d];
        if (bmin == bmax)
        {
            bmin -= EPSILON;
            bmax += EPSILON;
        }
        
        float dmin = (bmin - ray.origin.data[d]) * (ray.inv_direction.data[d]);
        float dmax = (bmax - ray.origin.data[d]) * (ray.inv_direction.data[d]);

        tmin = fmaxf(dmin, tmin);
        tmax = fminf(dmax, tmax);
    }
    return (tmin < tmax) ? tmin : FLT_MAX;
}

__host__ __device__ void BoundingBox::grow(const Triangle& tri) {
    corners[0][0] = std::min({corners[0][0], tri.va.x, tri.vb.x, tri.vc.x});
    corners[0][1] = std::min({corners[0][1], tri.va.y, tri.vb.y, tri.vc.y});
    corners[0][2] = std::min({corners[0][2], tri.va.z, tri.vb.z, tri.vc.z});
    corners[1][0] = std::max({corners[1][0], tri.va.x, tri.vb.x, tri.vc.x});
    corners[1][1] = std::max({corners[1][1], tri.va.y, tri.vb.y, tri.vc.y});
    corners[1][2] = std::max({corners[1][2], tri.va.z, tri.vb.z, tri.vc.z});
    fixCorners();
}

__host__ __device__ void BoundingBox::grow(const BoundingBox& other) {
    corners[0][0] = std::min(corners[0][0], other.corners[0][0]);
    corners[0][1] = std::min(corners[0][1], other.corners[0][1]);
    corners[0][2] = std::min(corners[0][2], other.corners[0][2]);
    corners[1][0] = std::max(corners[1][0], other.corners[1][0]);
    corners[1][1] = std::max(corners[1][1], other.corners[1][1]);
    corners[1][2] = std::max(corners[1][2], other.corners[1][2]);
    fixCorners();
}

void printBoundingBox(const BoundingBox& b) {
    std::cout << "BoundingBox: (" << b.corners[0][0] << ", " << b.corners[0][1] << ", " << b.corners[0][2] << ") to (" << b.corners[1][0] << ", " << b.corners[1][1] << ", " << b.corners[1][2] << ")" << std::endl;
}

