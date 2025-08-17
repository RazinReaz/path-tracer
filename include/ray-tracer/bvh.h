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

void printBVHNode(const bvhNode& node) {
    std::cout << "BoundingBox: (" << node.bbox.corners[0][0] << ", " << node.bbox.corners[0][1] << ", " << node.bbox.corners[0][2] << ") to (" << node.bbox.corners[1][0] << ", " << node.bbox.corners[1][1] << ", " << node.bbox.corners[1][2] << ")" << std::endl;
    std::cout << "leftChildIndex: " << node.leftChildIndex << " rightChildIndex: " << node.rightChildIndex << std::endl;
    std::cout << "triangleIndex: " << node.triangleIndex << " triangleCount: " << node.triangleCount << std::endl;
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
    bvhNode &parent,
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
    BVH.push_back(parent);
    
    // Recursively build children
    int32_t leftChildIndex = createTreeRecursive(codedTriangles, BVH, parent, begin, m, depth + 1);
    int32_t rightChildIndex = createTreeRecursive(codedTriangles, BVH, parent, m + 1, end, depth + 1);
    
    // Update parent node
    BVH[parentIndex].leftChildIndex = leftChildIndex;
    BVH[parentIndex].rightChildIndex = rightChildIndex;
    BVH[parentIndex].triangleCount = 0;  // Internal nodes don't contain triangles directly
    BVH[parentIndex].triangleIndex = -1;
    
    // Calculate bounding box for this node
    parent.bbox = BoundingBox();
    if (leftChildIndex >= 0) {
        parent.bbox.grow(BVH[leftChildIndex].bbox);
    }
    if (rightChildIndex >= 0) {
        parent.bbox.grow(BVH[rightChildIndex].bbox);
    }
    
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
    createTreeRecursive(codedTriangles, BVH, root, 0, static_cast<int>(codedTriangles.size()) - 1, 0);
    return BVH;
}


// return the lead node of the bvh that is intersected by the ray
__host__ __device__
int32_t traverseTree(bvhNode *BVH, Ray &ray){
    if (!BVH) return -1;
    
    int32_t nodeIndex = 0;
    float tleft = 100000.0f;
    if (!BVH[nodeIndex].bbox.intersect(ray, tleft)) {
        // no intersections with the root node
        return -1;
    }
    float tright = 100000.0f;
    while(!BVH[nodeIndex].isLeaf()){
        int32_t leftIndex = BVH[nodeIndex].leftChildIndex;
        int32_t rightIndex = BVH[nodeIndex].rightChildIndex;

        bool hitsleft = BVH[leftIndex].bbox.intersect(ray, tleft);
        bool hitsright = BVH[rightIndex].bbox.intersect(ray, tright);
        if (!hitsleft && !hitsright) break;

        bool one = hitsleft ^ hitsright;
        bool both = hitsleft && hitsright;
        
        if (both) {
            nodeIndex = (tleft < tright) ? leftIndex : rightIndex;
        } else if (one) {
            nodeIndex = (hitsleft) ? leftIndex : rightIndex;
        }
        
    }
    return nodeIndex;
}

__host__ __device__
int32_t countIntersections(bvhNode *BVH, Ray &ray){
    float tleft = 100000.0f;
    int32_t nodeIndex = 0;
    if (!BVH[nodeIndex].bbox.intersect(ray, tleft)) {
        // no intersections with the root node
        return 0;
    }
    float tright = 100000.0f;
    int32_t count = 0;
    while(!BVH[nodeIndex].isLeaf()){
        count++;
        int32_t leftIndex = BVH[nodeIndex].leftChildIndex;
        int32_t rightIndex = BVH[nodeIndex].rightChildIndex;

        bool hitsleft = BVH[leftIndex].bbox.intersect(ray, tleft);
        bool hitsright = BVH[rightIndex].bbox.intersect(ray, tright);
        if (!hitsleft && !hitsright) break;
        
        if (tleft < tright) {
            nodeIndex = leftIndex;
        } else {
            nodeIndex = rightIndex;
        }
    }
    return count;
}





