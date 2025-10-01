#pragma once

#include <iostream>
#include <vector>
#include <string>

#include "utils/cuda_utils.h"
#include "ray-tracer/triangle.h"
#include "ray-tracer/bvh.h"
#include "ray-tracer/materials.h"

void uploadBVHToGPU(std::vector<bvhNode> &h_BVH, bvhNode **d_BVH) {
    size_t num_elements = h_BVH.size();
    if (num_elements == 0) {
        std::cerr << "Warning: empty BVH list, skipping GPU upload" << std::endl;
        *d_BVH = nullptr;
        return;
    }
    CUDA_CHECK(cudaMalloc((void **)d_BVH, sizeof(bvhNode) * num_elements));
    CUDA_CHECK(cudaMemcpy(*d_BVH, h_BVH.data(), sizeof(bvhNode) * num_elements, cudaMemcpyHostToDevice));
}

void freeBVHFromGPU(bvhNode *d_BVH) {
    CUDA_CHECK(cudaFree(d_BVH));
}

void uploadTrianglesToGPU(std::vector<Triangle> &host_triangles, Triangle **device_triangles) {
    size_t num_elements = host_triangles.size();
    if (num_elements == 0) {
        std::cerr << "Warning: empty triangle list, skipping GPU upload" << std::endl;
        *device_triangles = nullptr;
        return;
    }
    CUDA_CHECK(cudaMalloc((void **)device_triangles, sizeof(Triangle) * num_elements));
    CUDA_CHECK(cudaMemcpy(*device_triangles, host_triangles.data(), sizeof(Triangle) * num_elements, cudaMemcpyHostToDevice));
}

void freeTrianglesFromGPU(Triangle *d_triangles) {
    CUDA_CHECK(cudaFree(d_triangles));
}

void uploadMaterialsToGPU(std::vector<Material> &host_materials, Material **device_materials) {
    size_t num_elements = host_materials.size();
    if (num_elements == 0) {
        std::cerr << "Warning: empty material list, skipping GPU upload" << std::endl;
        *device_materials = nullptr;
        return;
    }
    CUDA_CHECK(cudaMalloc((void **)device_materials, sizeof(Material) * num_elements));
    CUDA_CHECK(cudaMemcpy(*device_materials, host_materials.data(), sizeof(Material) * num_elements, cudaMemcpyHostToDevice));
}

void freeMaterialsFromGPU(Material *d_materials) {
    CUDA_CHECK(cudaFree(d_materials));
}

void allocateFrameBuffer(int totalPixels, float **d_frameBuffer) {
    CUDA_CHECK(cudaMalloc(d_frameBuffer, totalPixels * 3 * sizeof(float)));
    CUDA_CHECK(cudaMemset(*d_frameBuffer, 0.0f, totalPixels * 3 * sizeof(float)));
}

void freeFrameBuffer(float *d_frameBuffer) {
    CUDA_CHECK(cudaFree(d_frameBuffer));
}
