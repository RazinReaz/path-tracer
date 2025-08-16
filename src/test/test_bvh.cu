#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include "../../include/ray-tracer/bvh.h"
#include "../../include/ray-tracer/triangle.h"
#include "../../include/math/vec3.h"

// Test class for BVH functionality
class BVHTester {
private:
    std::vector<Triangle> triangles;
    std::vector<bvhNode> bvh;
    
    // Create a simple cube made of triangles
    void createCubeTriangles() {
        triangles.clear();
        
        vec3 v1(0, 0, 0);
        vec3 v2(1, 0, 0);
        vec3 v3(1, 1, 0);
        vec3 v4(0, 1, 0);
        vec3 v5(0, 0, 1);
        vec3 v6(1, 0, 1);
        vec3 v7(1, 1, 1);
        vec3 v8(0, 1, 1);
        
        vec3 n(0, 0, 1); // Simple normal
        
        // Front face (2 triangles)
        triangles.emplace_back(v1, v2, v3, n, n, n, 0);
        triangles.emplace_back(v1, v3, v4, n, n, n, 0);
        
        // Back face (2 triangles)
        triangles.emplace_back(v5, v7, v6, n, n, n, 0);
        triangles.emplace_back(v5, v8, v7, n, n, n, 0);
        
        // Left face (2 triangles)
        triangles.emplace_back(v1, v4, v8, n, n, n, 0);
        triangles.emplace_back(v1, v8, v5, n, n, n, 0);
        
        // Right face (2 triangles)
        triangles.emplace_back(v2, v6, v7, n, n, n, 0);
        triangles.emplace_back(v2, v7, v3, n, n, n, 0);
        
        // Top face (2 triangles)
        triangles.emplace_back(v4, v3, v7, n, n, n, 0);
        triangles.emplace_back(v4, v7, v8, n, n, n, 0);
        
        // Bottom face (2 triangles)
        triangles.emplace_back(v1, v5, v6, n, n, n, 0);
        triangles.emplace_back(v1, v6, v2, n, n, n, 0);
    }
    
    // Create random triangles for stress testing
    void createRandomTriangles(int count) {
        triangles.clear();
        triangles.reserve(count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posDist(-10.0f, 10.0f);
        std::uniform_real_distribution<float> sizeDist(0.1f, 2.0f);
        
        for (int i = 0; i < count; i++) {
            vec3 center(posDist(gen), posDist(gen), posDist(gen));
            float size = sizeDist(gen);
            
            vec3 v1 = center + vec3(-size, -size, 0);
            vec3 v2 = center + vec3(size, -size, 0);
            vec3 v3 = center + vec3(0, size, 0);
            
            vec3 normal(0, 0, 1);
            triangles.emplace_back(v1, v2, v3, normal, normal, normal, i % 5);
        }
    }

    bool isLeaf(const bvhNode& node) {
        return node.triangleCount > 0;
    }

    bool isInternal(const bvhNode& node) {
        return node.triangleCount == 0;
    }
    
    // Print BVH tree structure recursively
    void printBVHTree(const std::vector<bvhNode>& nodes, int nodeIndex, int depth = 0) {
        if (nodeIndex < 0 || nodeIndex >= nodes.size()) return;
        
        const bvhNode& node = nodes[nodeIndex];
        std::string indent(depth * 2, ' ');
        
        if (isLeaf(node)) {
            std::cout << indent << "Leaf[" << nodeIndex << "]: "
                      << "triangles=" << node.triangleCount
                      << ", triIndex=" << node.triangleIndex
                      << ", bbox=(" << node.bbox.corners[0][0] << "," << node.bbox.corners[0][1] << "," << node.bbox.corners[0][2] << ")"
                      << " to (" << node.bbox.corners[1][0] << "," << node.bbox.corners[1][1] << "," << node.bbox.corners[1][2] << ")"
                      << std::endl;
        } else {
            std::cout << indent << "Internal[" << nodeIndex << "]: "
                      << "left=" << node.leftChildIndex << ", right=" << node.rightChildIndex
                      << ", bbox=(" << node.bbox.corners[0][0] << "," << node.bbox.corners[0][1] << "," << node.bbox.corners[0][2] << ")"
                      << " to (" << node.bbox.corners[1][0] << "," << node.bbox.corners[1][1] << "," << node.bbox.corners[1][2] << ")" 
                      << " triangle count: " << node.triangleCount
                      << std::endl;
            
            if (node.leftChildIndex >= 0) {
                printBVHTree(nodes, node.leftChildIndex, depth + 1);
            }
            if (node.rightChildIndex >= 0) {
                printBVHTree(nodes, node.rightChildIndex, depth + 1);
            }
        }
    }
    
    // Validate BVH structure
    bool validateBVH(const std::vector<bvhNode>& nodes, int nodeIndex) {
        if (nodeIndex < 0 || nodeIndex >= nodes.size()) return false;
        
        const bvhNode& node = nodes[nodeIndex];
        
        if (isLeaf(node)) {
            // Leaf node validation
            if (node.triangleCount <= 0) {
                std::cout << "ERROR: Leaf node " << nodeIndex << " has invalid triangle count: " << node.triangleCount << std::endl;
                return false;
            }
            if (node.leftChildIndex != -1 || node.rightChildIndex != -1) {
                std::cout << "ERROR: Leaf node " << nodeIndex << " has children" << std::endl;
                return false;
            }
        } else {
            // Internal node validation
            if (node.triangleCount != 0) {
                std::cout << "ERROR: Internal node " << nodeIndex << " has triangle count: " << node.triangleCount << std::endl;
                return false;
            }
            if (node.leftChildIndex < 0 && node.rightChildIndex < 0) {
                std::cout << "ERROR: Internal node " << nodeIndex << " has no children" << std::endl;
                return false;
            }
            
            // Recursively validate children
            if (node.leftChildIndex >= 0) {
                if (!validateBVH(nodes, node.leftChildIndex)) return false;
            }
            if (node.rightChildIndex >= 0) {
                if (!validateBVH(nodes, node.rightChildIndex)) return false;
            }
        }
        
        return true;
    }
    
public:
    // Test basic BVH creation with cube
    bool testBasicBVH() {
        std::cout << "\n=== Testing Basic BVH Creation (Cube) ===" << std::endl;
        
        createCubeTriangles();
        std::cout << "Created " << triangles.size() << " triangles for cube" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        bvh = createTree(triangles);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "BVH created in " << duration.count() << " microseconds" << std::endl;
        std::cout << "Number of nodes: " << bvh.size() << std::endl;
        
        if (bvh.empty()) {
            std::cout << "ERROR: BVH is empty!" << std::endl;
            return false;
        }
        
        // Print tree structure
        std::cout << "\nBVH Tree Structure:" << std::endl;
        printBVHTree(bvh, 0);
        
        // Validate structure
        std::cout << "\nValidating BVH structure..." << std::endl;
        if (validateBVH(bvh, 0)) {
            std::cout << "BVH structure is valid!" << std::endl;
            return true;
        } else {
            std::cout << "BVH structure validation failed!" << std::endl;
            return false;
        }
    }
    
    // Test BVH creation with random triangles
    bool testRandomBVH(int triangleCount) {
        std::cout << "\n=== Testing Random BVH Creation (" << triangleCount << " triangles) ===" << std::endl;
        
        createRandomTriangles(triangleCount);
        std::cout << "Created " << triangles.size() << " random triangles" << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();
        bvh = createTree(triangles);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "BVH created in " << duration.count() << " microseconds" << std::endl;
        std::cout << "Number of nodes: " << bvh.size() << std::endl;
        
        if (bvh.empty()) {
            std::cout << "ERROR: BVH is empty!" << std::endl;
            return false;
        }
        
        // Validate structure
        std::cout << "Validating BVH structure..." << std::endl;
        if (validateBVH(bvh, 0)) {
            std::cout << "Random BVH structure is valid!" << std::endl;
            return true;
        } else {
            std::cout << "Random BVH structure validation failed!" << std::endl;
            return false;
        }
    }
    
    // Test edge cases
    bool testEdgeCases() {
        std::cout << "\n=== Testing Edge Cases ===" << std::endl;
        
        // Test empty triangle list
        std::cout << "Testing empty triangle list..." << std::endl;
        triangles.clear();
        bvh = createTree(triangles);
        if (bvh.empty()) {
            std::cout << "PASS: Empty triangle list handled correctly" << std::endl;
        } else {
            std::cout << "FAIL: Empty triangle list should return empty BVH" << std::endl;
            return false;
        }
        
        // Test single triangle
        std::cout << "Testing single triangle..." << std::endl;
        triangles.clear();
        vec3 v1(0, 0, 0), v2(1, 0, 0), v3(0, 1, 0), n(0, 0, 1);
        triangles.emplace_back(v1, v2, v3, n, n, n, 0);
        bvh = createTree(triangles);
        if (bvh.size() == 1 && isLeaf(bvh[0])) {
            std::cout << "PASS: Single triangle handled correctly" << std::endl;
        } else {
            std::cout << "FAIL: Single triangle should create single leaf node" << std::endl;
            return false;
        }
        
        return true;
    }
    
    // Run all tests
    void runAllTests() {
        std::cout << "Starting BVH Tests..." << std::endl;
        
        bool allPassed = true;
        
        // Test edge cases first
        if (!testEdgeCases()) {
            allPassed = false;
        }
        
        // Test basic BVH
        if (!testBasicBVH()) {
            allPassed = false;
        }
        
        // Test random BVH with different sizes
        std::vector<int> testSizes = {10, 100, 1000};
        for (int size : testSizes) {
            if (!testRandomBVH(size)) {
                allPassed = false;
            }
        }
        
        std::cout << "\n=== Test Results ===" << std::endl;
        if (allPassed) {
            std::cout << "ALL TESTS PASSED! BVH implementation is working correctly." << std::endl;
        } else {
            std::cout << "SOME TESTS FAILED! BVH implementation has issues." << std::endl;
        }
    }
};

int main() {
    std::cout << "BVH Test Suite" << std::endl;
    std::cout << "==============" << std::endl;
    
    BVHTester tester;
    tester.runAllTests();
    
    return 0;
}
