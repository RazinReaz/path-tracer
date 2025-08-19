#pragma once



#include <vector>
#include <cmath>
#include <algorithm>
#include <stdint.h>
#include <utility> // for std::pair
#include <cfloat> // for FLT_MAX
#include <climits> // for UINT32_MAX
#include "math/vec3.h"
#include "ray-tracer/ray.h"
#include "ray-tracer/boundingBox.h"
#include "ray-tracer/triangle.h"


/*
This file will first sort the triangles using their Morton code.
We need the bounds of the scene to generate the Morton code, and also come up with an arbitrary number of bits
to divide the bounds into chunks. We will find which chunk each triangle falls into and then generate their Morton code.
We will then store the triangle and its Morton code into a pair and then sort the pairs according to the Morton code.
Then we will generate a vector of triangles by removing the Morton codes from the pair.
*/

typedef struct bvhNode {
    BoundingBox bbox;

    int32_t leftChildIndex;
    int32_t rightChildIndex;

    int32_t triangleIndex;
    int32_t triangleCount;

    __host__ __device__ bool isLeaf() const {
        return leftChildIndex == -1 && rightChildIndex == -1;
    }
} bvhNode;

const int BITS = 7;
const int MAX_DEPTH = 10;

inline std::ostream& operator<<(std::ostream& os, const bvhNode& node) {
    os << "bvhNode(";
    os << "bbox: ";
    os << "min(" << node.bbox.corners[0][0] << ", " << node.bbox.corners[0][1] << ", " << node.bbox.corners[0][2] << "), ";
    os << "max(" << node.bbox.corners[1][0] << ", " << node.bbox.corners[1][1] << ", " << node.bbox.corners[1][2] << "), ";
    os << "left: " << node.leftChildIndex << ", ";
    os << "right: " << node.rightChildIndex << ", ";
    os << "triIdx: " << node.triangleIndex << ", ";
    os << "triCount: " << node.triangleCount;
    os << ")";
    return os;
}




inline int mortonIndex(float x, float minx, float maxx, int bits)
{
    float norm = (x - minx) / (maxx - minx);
    int maxIndex = (1 << bits) - 1;
    return std::min(maxIndex, std::max(0, static_cast<int>(norm * maxIndex)));
}

inline int mortonCode(Triangle t, const float maxvec[3], const float minvec[3])
{
    vec3 centre = (t.va + t.vb + t.vc).scale(1.0f / 3.0f);
    int mortonX = mortonIndex(centre.x, minvec[0], maxvec[0], BITS);
    int mortonY = mortonIndex(centre.y, minvec[1], maxvec[1], BITS);
    int mortonZ = mortonIndex(centre.z, minvec[2], maxvec[2], BITS);

    int code = 0;
    //! interleave
    for (int i = 0; i < BITS; i++)
    {
        code |= ((mortonX >> i) & 1) << (3 * i);
        code |= ((mortonY >> i) & 1) << (3 * i + 1);
        code |= ((mortonZ >> i) & 1) << (3 * i + 2);
    }
    return code;
}

std::vector<std::pair<int, Triangle>> sortTriangleListByMortonCode(std::vector<Triangle> &triangles, float maxvec[3], float minvec[3])
{
    std::vector<std::pair<int, Triangle>> codedTriangles;
    codedTriangles.reserve(triangles.size());

    for (const auto &t : triangles)
    {
        int code = mortonCode(t, maxvec, minvec);
        codedTriangles.emplace_back(code, t);
    }

    // Sort by Morton code
    std::sort(codedTriangles.begin(), codedTriangles.end(),
              [](const auto &a, const auto &b)
              {
                  return a.first < b.first;
              });
    return codedTriangles;
}

// Count leading zeros (portable)
inline int countLeadingZeros(unsigned int x)
{
#if defined(_MSC_VER)
    unsigned long leading_zero = 0;
    if (_BitScanReverse(&leading_zero, x))
        return 31 - leading_zero;
    else
        return 32;
#else
    return x ? __builtin_clz(x) : 32;
#endif
}

int getSplitPosition(const std::vector<std::pair<int, Triangle>>& list, const int &begin, const int& end) {
    int firstcode = list[begin].first;
    int lastcode = list[end].first;
    int commonPrefixBits = countLeadingZeros(firstcode ^ lastcode);
    if (firstcode == lastcode) return (begin + end) / 2 ; //! means all codes are same and this can be a leaf node

    int split = begin;
    int step = end - begin;
    do
    {
        step = (step + 1) >> 1;
        int newSplit = split + step;
        if (newSplit < end)
        {
            int prefixBits = countLeadingZeros(firstcode ^ list[newSplit].first);
            if (prefixBits > commonPrefixBits)
                split = newSplit;
        }
    } while (step > 1);

    return split;
}

int32_t createTreeRecursive(
    const std::vector<std::pair<int, Triangle>> &codedTriangles, 
    std::vector<bvhNode> &BVH, 
    const int& begin, const int& end, 
    const int& depth)
{
    int firstcode = codedTriangles[begin].first;
    int lastcode = codedTriangles[end].first;


    if (firstcode == lastcode || begin == end || depth > MAX_DEPTH) { //! means if all codes are same and this can be a leaf node
        //leaf node logic
        int32_t leafIndex = static_cast<int32_t>(BVH.size());
        bvhNode leaf;
        for (int i = begin; i <= end; i++) {
            leaf.bbox.grow(codedTriangles[i].second);
        }
        leaf.triangleCount = end - begin + 1;
        leaf.triangleIndex = begin;
        leaf.leftChildIndex = leaf.rightChildIndex = -1;
        BVH.push_back(leaf);
        return leafIndex;
    }

    int m = getSplitPosition(codedTriangles, begin, end);
    int32_t parentIndex = static_cast<int32_t>(BVH.size());
    BVH.push_back(bvhNode());
    
    // Recursively build children
    int32_t L = createTreeRecursive(codedTriangles, BVH, begin, m, depth + 1);
    int32_t R = createTreeRecursive(codedTriangles, BVH, m + 1, end, depth + 1);
    
    // Update parent node
    bvhNode &parent = BVH[parentIndex];
    parent.leftChildIndex = L;
    parent.rightChildIndex = R;
    parent.triangleCount = 0;  // Internal nodes don't contain triangles directly
    parent.triangleIndex = -1;
    
    // Calculate bounding box for this node
    parent.bbox = BoundingBox();
    if (L >= 0)  parent.bbox.grow(BVH[L].bbox);
    if (R >= 0)  parent.bbox.grow(BVH[R].bbox);
    return parentIndex;
}

std::vector<bvhNode> createBVHandSortTriangles(std::vector<Triangle> &triangles)
{
    std::vector<bvhNode> BVH;
    
    if (triangles.empty()) {
        return BVH; // Return empty BVH
    }
    
    // Create root node
    bvhNode root;
    for (const auto &t : triangles) {
        root.bbox.grow(t);
    }
    
    std::vector<std::pair<int, Triangle>> codedTriangles = sortTriangleListByMortonCode(triangles, root.bbox.corners[1], root.bbox.corners[0]);
    /// modify the original triangles array according to the codedTriangles
    for (int i = 0; i < codedTriangles.size(); i++) {
        triangles[i] = codedTriangles[i].second;
    }    
    createTreeRecursive(codedTriangles, BVH, 0, static_cast<int>(codedTriangles.size()) - 1, 0);
    return BVH;
}


__host__ __device__
void traverseTree(bvhNode *d_BVH, Triangle *d_triangles, Ray &ray) {
    int32_t nodeIndexStack[MAX_DEPTH];
    int32_t stackPointer = 0;
    nodeIndexStack[stackPointer++] = 0; // pushing the root into the stack
    while (stackPointer > 0) {
        int32_t nodeIndex = nodeIndexStack[--stackPointer];
        if (d_BVH[nodeIndex].isLeaf()) {            
            for (int b = d_BVH[nodeIndex].triangleIndex, i = 0; i < d_BVH[nodeIndex].triangleCount; i++) {
                d_triangles[b + i].calculate_hit_by(ray);
            }
        } else {
            int32_t L = d_BVH[nodeIndex].leftChildIndex;
            int32_t R = d_BVH[nodeIndex].rightChildIndex;
            float tleft = d_BVH[L].bbox.intersection_distance(ray);
            float tright = d_BVH[R].bbox.intersection_distance(ray);
            if (tleft > tright) {
                if(tleft < ray.info.t) nodeIndexStack[stackPointer++] = L;
                if(tright < ray.info.t) nodeIndexStack[stackPointer++] = R;
            } else {
                if(tright < ray.info.t) nodeIndexStack[stackPointer++] = R;
                if(tleft < ray.info.t) nodeIndexStack[stackPointer++] = L;
            }
        }
    }
}


// Given a vector of bvhNodes, create thin triangles along the wireframe of their bounding boxes.
// Returns a vector of Triangle objects representing the wireframes.
// Each triangle will have per-vertex normals (all the same, perpendicular to the quad face).
inline std::vector<Triangle> createBVHWireframeTriangles(const std::vector<bvhNode>& bvhNodes, float thickness = 0.05f) {
    std::vector<Triangle> triangles;

    for (const auto& node : bvhNodes) {
        int material_index = node.isLeaf() ? 0: 1;
        // 8 corners of the bounding box
        vec3 corners[8] = {
            vec3(node.bbox.corners[0][0], node.bbox.corners[0][1], node.bbox.corners[0][2]),
            vec3(node.bbox.corners[1][0], node.bbox.corners[0][1], node.bbox.corners[0][2]),
            vec3(node.bbox.corners[1][0], node.bbox.corners[1][1], node.bbox.corners[0][2]),
            vec3(node.bbox.corners[0][0], node.bbox.corners[1][1], node.bbox.corners[0][2]),
            vec3(node.bbox.corners[0][0], node.bbox.corners[0][1], node.bbox.corners[1][2]),
            vec3(node.bbox.corners[1][0], node.bbox.corners[0][1], node.bbox.corners[1][2]),
            vec3(node.bbox.corners[1][0], node.bbox.corners[1][1], node.bbox.corners[1][2]),
            vec3(node.bbox.corners[0][0], node.bbox.corners[1][1], node.bbox.corners[1][2])
        };

        // 12 edges of the bounding box, each as a pair of indices into corners[]
        int edges[12][2] = {
            {0,1}, {1,2}, {2,3}, {3,0}, // bottom face
            {4,5}, {5,6}, {6,7}, {7,4}, // top face
            {0,4}, {1,5}, {2,6}, {3,7}  // vertical edges
        };

        // For each edge, create a thin rectangle (2 triangles) along the edge
        for (int e = 0; e < 12; ++e) {
            vec3 a = corners[edges[e][0]];
            vec3 b = corners[edges[e][1]];
            vec3 dir = (b - a);
            dir.normalize_self();

            // Find a vector perpendicular to dir for thickness
            vec3 up(0, 1, 0);
            if (fabs(dir.dot(up)) > 0.99f) up = vec3(1, 0, 0);
            vec3 perp = dir.cross(up);
            perp.normalize_self();
            perp = perp * (thickness * 0.5f);

            // Make a quad (rectangle) along the edge, perpendicular to dir
            vec3 v0 = a + perp;
            vec3 v1 = a - perp;
            vec3 v2 = b + perp;
            vec3 v3 = b - perp;

            // The normal for the quad face (perpendicular to both dir and perp)
            vec3 normal = dir.cross(perp);
            normal.normalize_self();

            // Two triangles for the quad, with per-vertex normals
            triangles.emplace_back(
                v0, v1, v2,
                normal, normal, normal,
                material_index
            );
            triangles.emplace_back(
                v2, v1, v3,
                normal, normal, normal,
                material_index
            );
        }
    }
    return triangles;
}







