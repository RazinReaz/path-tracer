#include <iostream>
#include <vector>
#include <string>

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include <tiny_obj_loader.h>
#include "ray-tracer/triangle.h"
#include "math/vec3.h"
#include "utils/cuda_utils.h"
#include "ray-tracer/bvh.h"

#include <unordered_map>

// Add output parameter for collecting materials
void loadTrianglesAndMaterialsFromOBJ(
    const std::string &filename,
    const std::string &mtlBasePath,
    std::vector<Triangle> &outTriangles,
    std::vector<Material> &outMaterials
) 
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(
        &attrib,
        &shapes,
        &materials,
        &warn,
        &err,
        filename.c_str(),
        mtlBasePath.c_str(),  // provide MTL base path
        true                  // load .mtl
    );

    if (!warn.empty())
        std::cerr << "WARN: " << warn << std::endl;
    if (!err.empty())
        std::cerr << "ERR: " << err << std::endl;
    if (!ret)
        throw std::runtime_error("Failed to load OBJ file");

    // Convert tinyobj materials to your own Material format
    outMaterials.clear();
    for (const auto &mat : materials) {
        Material m;
        m.type = MaterialType::LAMBERTIAN; // Default; you can customize using mat properties

        m.albedo = vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);

        // You can use mat.emission to detect EMISSIVE types, or mat.metallic if used
        if (mat.emission[0] > 0.01f || mat.emission[1] > 0.01f || mat.emission[2] > 0.01f) {
            m.type = MaterialType::EMISSIVE;
        }
        m.emission = vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
        outMaterials.push_back(m);
    }

    for (const auto &shape : shapes) {
        size_t index_offset = 0;

        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv < 3) {
                std::cerr << "Face with < 3 vertices skipped" << std::endl;
                index_offset += fv;
                continue;
            }

            std::vector<vec3> vertices(fv);
            std::vector<vec3> normals(fv, vec3(1, 0, 0));
            bool hasNormal = true;

            for (int i = 0; i < fv; ++i) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + i];
                float *vx = &attrib.vertices[3 * idx.vertex_index];
                vertices[i] = vec3(vx[0], vx[1], vx[2]);

                if (hasNormal && idx.normal_index >= 0) {
                    float *nx = &attrib.normals[3 * idx.normal_index];
                    normals[i] = vec3(nx[0], nx[1], nx[2]);
                } else {
                    hasNormal = false;
                }
            }

            if (!hasNormal) {
                vec3 e1 = vertices[1] - vertices[0];
                vec3 e2 = vertices[2] - vertices[0];
                vec3 faceNormal = e1.cross(e2);
                for (int i = 0; i < fv; ++i)
                    normals[i] = faceNormal;
            }

            // Get material ID
            int matID = -1;
            if (f < shape.mesh.material_ids.size()) {
                matID = shape.mesh.material_ids[f];
            }

            for (int i = 1; i < fv - 1; ++i) {
                outTriangles.emplace_back(
                    vertices[0], vertices[i], vertices[i + 1],
                    normals[0], normals[i], normals[i + 1],
                    matID
                );
            }

            index_offset += fv;
        }
    }
}


void uploadSceneToGPU(const std::vector<Triangle> &h_triangles, Triangle **d_triangles, Scene **d_scene) {
    size_t num_elements = h_triangles.size();
    if (num_elements == 0)
    {
        std::cerr << "Warning: empty triangle list, skipping GPU upload" << std::endl;
        *d_triangles = nullptr;
        *d_scene = nullptr;
        return;
    }

    Triangle *d_triangles_temp;
    Scene *d_scene_temp;

    // triangle array
    CUDA_CHECK(cudaMalloc((void **)&d_triangles_temp, num_elements * sizeof(Triangle)));
    CUDA_CHECK(cudaMemcpy(d_triangles_temp, h_triangles.data(), num_elements * sizeof(Triangle), cudaMemcpyHostToDevice));
    // scene
    CUDA_CHECK(cudaMalloc((void **)&d_scene_temp, sizeof(Scene)));
    Scene h_scene(d_triangles_temp, num_elements);
    CUDA_CHECK(cudaMemcpy(d_scene_temp, &h_scene, sizeof(Scene), cudaMemcpyHostToDevice));

    // set the pointers from the caller
    *d_triangles = d_triangles_temp;
    *d_scene = d_scene_temp;
}

void freeSceneFromGPU(Triangle *d_triangles, Scene *d_scene) {
    cudaFree(d_triangles);
    cudaFree(d_scene);
}


void uploadBVHToGPU(std::vector<bvhNode> &host_BVH, bvhNode **device_BVH)
{
    size_t num_elements = host_BVH.size();
    if (num_elements == 0)
    {
        std::cerr << "Warning: empty BVH list, skipping GPU upload" << std::endl;
        *device_BVH = nullptr;
        return;
    }
    CUDA_CHECK(cudaMalloc((void **)device_BVH, sizeof(bvhNode) * num_elements));
    CUDA_CHECK(cudaMemcpy(*device_BVH, host_BVH.data(), sizeof(bvhNode) * num_elements, cudaMemcpyHostToDevice));
}

void freeBVHFromGPU(bvhNode *d_BVH)
{
    CUDA_CHECK(cudaFree(d_BVH));
}

void uploadTrianglesToGPU(std::vector<Triangle> &host_triangles, Triangle **device_triangles)
{
    size_t num_elements = host_triangles.size();
    if (num_elements == 0)
    {
        std::cerr << "Warning: empty triangle list, skipping GPU upload" << std::endl;
        *device_triangles = nullptr;
        return;
    }
    CUDA_CHECK(cudaMalloc((void **)device_triangles, sizeof(Triangle) * num_elements));
    CUDA_CHECK(cudaMemcpy(*device_triangles, host_triangles.data(), sizeof(Triangle) * num_elements, cudaMemcpyHostToDevice));
}

void freeTrianglesFromGPU(Triangle *d_triangles)
{
    CUDA_CHECK(cudaFree(d_triangles));
}


