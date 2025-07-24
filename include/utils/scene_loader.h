#include <iostream>
#include <vector>
#include <string>

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif

#include <tiny_obj_loader.h>
#include "ray-tracer/triangle.h"
#include "math/vec3.h"
#include "utils/cuda_macro.h"


std::vector<Triangle> loadTrianglesFromOBJ(const std::string &filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool ret = tinyobj::LoadObj(
        &attrib,
        &shapes,
        nullptr, // don't load materials
        &warn,
        &err,
        filename.c_str(),
        nullptr, // no .mtl base path
        false    // don't load .mtl even if it exists
    );

    if (!warn.empty())
        std::cerr << "WARN: " << warn << std::endl;
    if (!err.empty())
        std::cerr << "ERR: " << err << std::endl;
    if (!ret)
        throw std::runtime_error("Failed to load OBJ file");

    std::vector<Triangle> triangles;

    for (const auto &shape : shapes)
    {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
        {
            int fv = shape.mesh.num_face_vertices[f];
            if (fv < 3)
            {
                std::cerr << "Face with < 3 vertices skipped" << std::endl;
                index_offset += fv;
                continue;
            }

            // Store all vertices of this face
            std::vector<vec3> vertices(fv);
            std::vector<vec3> normals(fv, vec3(1, 0, 0)); // Default normal
            bool hasNormal = true;

            for (int i = 0; i < fv; ++i)
            {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + i];
                float *vx = &attrib.vertices[3 * idx.vertex_index];
                vertices[i] = vec3(vx[0], vx[1], vx[2]);

                if (hasNormal && idx.normal_index >= 0)
                {
                    float *nx = &attrib.normals[3 * idx.normal_index];
                    normals[i] = vec3(nx[0], nx[1], nx[2]);
                } else {
                    hasNormal = false;
                }
            }


            if (!hasNormal) {
                vec3 e1 = vertices[1] - vertices[0];
                vec3 e2 = vertices[3] - vertices[0];
                vec3 faceNormal = e1.cross(e2);
                normals[0] = normals[1] = normals[2] = normals[3] = faceNormal;
            }

            // Fan triangulation: (v0,v1,v2), (v0,v2,v3), ...
            for (int i = 1; i < fv - 1; ++i)
            {
                triangles.emplace_back(
                    vertices[0], vertices[i], vertices[i + 1],
                    normals[0], normals[i], normals[i + 1]
                );
            }

            index_offset += fv;
        }
    }


    return triangles;
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