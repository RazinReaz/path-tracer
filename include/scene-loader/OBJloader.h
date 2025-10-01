#pragma once

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#endif
#include "tiny_obj_loader.h"
#include "SceneLoader.h"
#include <iostream>
#include "utils/vec3_utils.h"

class OBJLoader : public SceneLoader
{
    public:
    OBJLoader(const std::string &filename, const std::string &mtlBasePath);
    ~OBJLoader();
    void loadTrianglesAndMaterials(std::vector<Triangle> &outTriangles, std::vector<Material> &outMaterials) override;
};

OBJLoader::OBJLoader(const std::string &filename, const std::string &mtlBasePath)
    : SceneLoader(filename, mtlBasePath)
{
}

void OBJLoader::loadTrianglesAndMaterials(std::vector<Triangle> &outTriangles, std::vector<Material> &outMaterials)
{
    // tinyobj::attrib_t attrib;
    // std::vector<tinyobj::shape_t> shapes;
    // std::vector<tinyobj::material_t> materials;
    // std::string warn, err;

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = m_mtlBasePath;
    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(m_filename, reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error() << std::endl;
        }
        exit(1);
    }
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning() << std::endl;
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    // bool ret = tinyobj::LoadObj(
    //     &attrib,
    //     &shapes,
    //     &materials,
    //     &warn,
    //     &err,
    //     m_filename.c_str(),
    //     m_mtlBasePath.c_str(),  // provide MTL base path
    //     true                  // load .mtl
    // );

    outMaterials.clear();
    for (const auto &mat : materials) {
        Material m;
        // m.type = MaterialType::DIFFUSE;
        // if (!is_zero_vector(mat.specular[0], mat.specular[1], mat.specular[2]) && mat.shininess > 10.0f) {
        //     m.type = MaterialType::SPECULAR;
        //     m.albedo = vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
        // } else {
        //     m.type = MaterialType::DIFFUSE;
        //     m.albedo = vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        // }
        if (mat.roughness > 0.01f) {
            m.roughness = mat.roughness;
        } else {
            m.roughness = sqrt(2 / (mat.shininess + 2));
        }
        m.ior = mat.ior;
        m.metalness = mat.metallic;
        if (m.metalness > 0.5f) {
            m.type = MaterialType::SPECULAR;
            m.albedo = vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
            if (is_zero_vector(m.albedo.r, m.albedo.g, m.albedo.b)) {
                m.albedo = vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
            }
        } else {
            m.type = MaterialType::DIFFUSE;
            m.albedo = vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        }
        m.emission = vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
        outMaterials.push_back(m);
        std::cout << m << std::endl;
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

            // loop over vertices in a face
            for (int v = 0; v < fv; ++v) {
                tinyobj::index_t idx = shape.mesh.indices[index_offset + v];
                float vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                float vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                float vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                vertices[v] = vec3(vx, vy, vz);

                if (hasNormal && idx.normal_index >= 0) {
                    float nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    float ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    float nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    normals[v] = vec3(nx, ny, nz);
                } else {
                    hasNormal = false;
                }
            }

            if (!hasNormal) {
                std::cout << "!";
                vec3 e1 = vertices[1] - vertices[0];
                vec3 e2 = vertices[2] - vertices[0];
                vec3 faceNormal = e1.cross(e2);
                for (int v = 0; v < fv; ++v)
                    normals[v] = faceNormal;
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