#pragma once

#include "math/vec3.h"
#include "ray-tracer/triangle.h"

class Scene {
    private:
        Triangle *triangles;
        size_t triangle_count;
    public:
        __host__ __device__ Scene(Triangle *triangles, size_t triangle_count)
            : triangles(triangles), triangle_count(triangle_count) {}
        __host__ __device__ void calculate_hit_by(Ray &ray);
};

__host__ __device__
void Scene::calculate_hit_by(Ray &ray) {
    for (size_t i = 0; i < triangle_count; ++i) {
        triangles[i].calculate_hit_by(ray);
    }
}

