#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <utility> // for std::pair
#include "math/vec3.h"
#include "ray-tracer/triangle.h"

/*
    This file will first sort the triangles using their Morton code.
    We need the bounds of the scene to generate the Morton code, and also come up with an arbitrary number of bits
    to divide the bounds into chunks. We will find which chunk each triangle falls into and then generate their Morton code.
    We will then store the triangle and its Morton code into a pair and then sort the pairs according to the Morton code.
    Then we will generate a vector of triangles by removing the Morton codes from the pair.
*/

const int BITS = 3;

// Find the bounding box of all triangles
void calculateAndAssignBounds(std::vector<Triangle> &triangles, vec3 &maxvec, vec3 &minvec)
{
    for (const auto &triangle : triangles)
    {
        maxvec.x = std::max({maxvec.x, triangle.va.x, triangle.vb.x, triangle.vc.x});
        maxvec.y = std::max({maxvec.y, triangle.va.y, triangle.vb.y, triangle.vc.y});
        maxvec.z = std::max({maxvec.z, triangle.va.z, triangle.vb.z, triangle.vc.z});

        minvec.x = std::min({minvec.x, triangle.va.x, triangle.vb.x, triangle.vc.x});
        minvec.y = std::min({minvec.y, triangle.va.y, triangle.vb.y, triangle.vc.y});
        minvec.z = std::min({minvec.z, triangle.va.z, triangle.vb.z, triangle.vc.z});
    }
}
inline void mortonIndex(float x, float minx, float maxx, float bits) {
    float norm = (x - minx) / (maxx - minx);
    int maxIndex = (1<<bits) - 1;
    return std::clamp(static_cast<int>(normalized * maxIndex), 0, maxIndex);
}

inline int mortonCode(Triangle t, vec3& maxvec, vec3 & minvec) {
    vec3 centre = (t.va + t.vb + t.vc ).scale(1.0f / 3.0f);
    int mortonX = mortonIndex(centre.x, minvec.x, maxvec.x, BITS);
    int mortonY = mortonIndex(centre.y, minvec.y, maxvec.y, BITS);
    int mortonZ = mortonIndex(centre.z, minvec.z, maxvec.z, BITS);

    int code = 0;
    //! interleave
    for (int i = 0; i < BITS; i++) {
        code |= ((mortonX << i) & 1) << (3 * i);
        code |= ((mortonY << i) & 1) << (3 * i + 1);
        code |= ((mortonZ << i) & 1) << (3 * i + 2);
    }
    return code;
}

std::vector sortTriangleList(std::vector &triangles) {
    vec3 minvec(INFINITY, INFINITY, INFINITY), maxvec(-INFINITY, -INFINITY, -INFINITY);
    calculateAndAssignBounds(triangles, maxvec, minvec);

    std::vector<std::pair<int, Triangle>> codedTriangles;
    codedTriangles.reserve(triangles.size());

    for (const auto &t: triangles) {
        int code = mortonCode(t, maxvec, minvec);
        codedTriangles.emplace_back(code, t);
    }

    // Sort by Morton code
    std::sort(codedTriangles.begin(), codedTriangles.end(),
              [](const auto &a, const auto &b)
              {
                  return a.first < b.first;
              });

    // Extract sorted triangles
    std::vector<Triangle> sortedTriangles;
    sortedTriangles.reserve(triangles.size());
    for (const auto &pair : codedTriangles)
    {
        sortedTriangles.push_back(pair.second);
    }

    return sortedTriangles;
}