#pragma once

#include "SceneLoader.h"
#include "tiny_gltf.h"
#include "math/vec3.h"
#include "math/mat4.h"
#include "ray-tracer/triangle.h"
#include "ray-tracer/materials.h"

class GLTFLoader : public SceneLoader
{
    public:
    GLTFLoader(const std::string &filename, const std::string &mtlBasePath);
    ~GLTFLoader();
    void loadTrianglesAndMaterials(std::vector<Triangle> &outTriangles, std::vector<Material> &outMaterials) override;
};

GLTFLoader::GLTFLoader(const std::string &filename, const std::string &mtlBasePath)
    : SceneLoader(filename, mtlBasePath)
{
}

GLTFLoader::~GLTFLoader() { 
}

// Helper: build mat4 from glTF node transform
mat4 getNodeTransform(const tinygltf::Node &node) {
    mat4 T(1.0f);

    if (!node.matrix.empty()) {
        float m[16];
        for (int i = 0; i < 16; i++) m[i] = (float)node.matrix[i];
        return mat4(m);
    }

    // Translation
    if (!node.translation.empty()) {
        mat4 M(1.0f);
        M.m[12] = (float)node.translation[0];
        M.m[13] = (float)node.translation[1];
        M.m[14] = (float)node.translation[2];
        T = mat4_mul(T, M);
    }

    // Rotation (quaternion)
    if (!node.rotation.empty()) {
        double x = node.rotation[0], y = node.rotation[1], z = node.rotation[2], w = node.rotation[3];
        mat4 R(1.0f);
        R.m[0] = 1 - 2*y*y - 2*z*z; R.m[4] = 2*x*y - 2*z*w;     R.m[8]  = 2*x*z + 2*y*w;
        R.m[1] = 2*x*y + 2*z*w;     R.m[5] = 1 - 2*x*x - 2*z*z; R.m[9]  = 2*y*z - 2*x*w;
        R.m[2] = 2*x*z - 2*y*w;     R.m[6] = 2*y*z + 2*x*w;     R.m[10] = 1 - 2*x*x - 2*y*y;
        T = mat4_mul(T, R);
    }

    // Scale
    if (!node.scale.empty()) {
        mat4 S(1.0f);
        S.m[0] = (float)node.scale[0];
        S.m[5] = (float)node.scale[1];
        S.m[10] = (float)node.scale[2];
        T = mat4_mul(T, S);
    }

    return T;
}

// Recursive traversal
void traverseNode(const tinygltf::Model &model, int nodeIndex, 
                  const mat4 &parentTransform,
                  std::vector<Triangle> &outTriangles,
                  std::vector<Material> &outMaterials) {
    const auto &node = model.nodes[nodeIndex];
    mat4 local = getNodeTransform(node);
    mat4 world = mat4_mul(parentTransform, local);

    // If node has a mesh, load it with this world transform
    if (node.mesh >= 0) {
        const auto &mesh = model.meshes[node.mesh];
        for (auto &prim : mesh.primitives) {
            const float *bufPositions = nullptr;
            const float *bufNormals = nullptr;

            // Positions
            const auto &posAccessor = model.accessors[prim.attributes.find("POSITION")->second];
            const auto &posView = model.bufferViews[posAccessor.bufferView];
            bufPositions = reinterpret_cast<const float*>(
                &model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]);

            // Normals
            auto nit = prim.attributes.find("NORMAL");
            if (nit != prim.attributes.end()) {
                const auto &normAccessor = model.accessors[nit->second];
                const auto &normView = model.bufferViews[normAccessor.bufferView];
                bufNormals = reinterpret_cast<const float*>(
                    &model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]);
            }

            // Indices
            const auto &idxAccessor = model.accessors[prim.indices];
            const auto &idxView = model.bufferViews[idxAccessor.bufferView];
            const unsigned char *indices = &model.buffers[idxView.buffer].data[idxAccessor.byteOffset + idxView.byteOffset];

            int materialIndex = prim.material < 0 ? 0 : prim.material;

            for (size_t i = 0; i < idxAccessor.count; i += 3) {
                uint32_t idx[3];
                if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                    auto ptr = (uint16_t*)indices;
                    idx[0] = ptr[i+0]; idx[1] = ptr[i+1]; idx[2] = ptr[i+2];
                } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                    auto ptr = (uint32_t*)indices;
                    idx[0] = ptr[i+0]; idx[1] = ptr[i+1]; idx[2] = ptr[i+2];
                } else {
                    auto ptr = (uint8_t*)indices;
                    idx[0] = ptr[i+0]; idx[1] = ptr[i+1]; idx[2] = ptr[i+2];
                }

                Triangle tri;
                // Apply world transform
                tri.v0 = mat4_mul_point(world, vec3(bufPositions[3*idx[0]], bufPositions[3*idx[0]+1], bufPositions[3*idx[0]+2]));
                tri.v1 = mat4_mul_point(world, vec3(bufPositions[3*idx[1]], bufPositions[3*idx[1]+1], bufPositions[3*idx[1]+2]));
                tri.v2 = mat4_mul_point(world, vec3(bufPositions[3*idx[2]], bufPositions[3*idx[2]+1], bufPositions[3*idx[2]+2]));

                if (bufNormals) {
                    tri.n0 = mat4_mul_vector(world, vec3(bufNormals[3*idx[0]], bufNormals[3*idx[0]+1], bufNormals[3*idx[0]+2])).normalize();
                    tri.n1 = mat4_mul_vector(world, vec3(bufNormals[3*idx[1]], bufNormals[3*idx[1]+1], bufNormals[3*idx[1]+2])).normalize();
                    tri.n2 = mat4_mul_vector(world, vec3(bufNormals[3*idx[2]], bufNormals[3*idx[2]+1], bufNormals[3*idx[2]+2])).normalize();
                } else {
                    vec3 e1 = tri.v1 - tri.v0;
                    vec3 e2 = tri.v2 - tri.v0;
                    vec3 n = e1.cross(e2).normalize();
                    tri.n0 = tri.n1 = tri.n2 = n;
                }

                tri.materialIndex = materialIndex;
                outTriangles.push_back(tri);
            }
        }
    }

    // Recurse children
    for (int child : node.children) {
        traverseNode(model, child, world, outTriangles, outMaterials);
    }
}

// Main function
void GLTFLoader::loadTrianglesAndMaterials(std::vector<Triangle> &outTriangles,
                                           std::vector<Material> &outMaterials) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, m_filename);
    if (!ret) throw std::runtime_error("Failed to load glTF: " + m_filename);

    outMaterials.clear();
    for (auto &mat : model.materials) {
        Material m{};
        if (mat.values.find("baseColorFactor") != mat.values.end()) {
            auto c = mat.values["baseColorFactor"].ColorFactor();
            m.baseColor = vec3(c[0], c[1], c[2]);
        } else {
            m.baseColor = vec3(1,1,1);
        }
        if (mat.values.find("roughnessFactor") != mat.values.end()) {
            m.roughness = (float)mat.values["roughnessFactor"].Factor();
        } else {
            m.roughness = 0.5f;
        }
        if (mat.values.find("metallicFactor") != mat.values.end()) {
            m.metallic = (float)mat.values["metallicFactor"].Factor();
        } else {
            m.metallic = 0.0f;
        }
        m.ior = 1.5f; // default IOR
        outMaterials.push_back(m);
    }

    // Traverse scene
    outTriangles.clear();
    for (int nodeIdx : model.scenes[model.defaultScene].nodes) {
        traverseNode(model, nodeIdx, mat4(1.0f), outTriangles, outMaterials);
    }
}


