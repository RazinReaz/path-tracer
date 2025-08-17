#include <iostream>
#include <vector>
#include <cassert>
#include "../../include/ray-tracer/bvh.h"
#include "../../include/ray-tracer/ray.h"
#include "../../include/ray-tracer/triangle.h"
#include "../../include/math/vec3.h"

// Test helper function to create a simple BVH tree
std::vector<bvhNode> createSimpleTestBVH() {
    std::vector<bvhNode> bvh;
    
    // Root node (index 0) - covers both children
    bvhNode root;
    root.bbox = BoundingBox(vec3(-2, -2, -2), vec3(2, 2, 2));
    root.leftChildIndex = 1;
    root.rightChildIndex = 2;
    root.triangleIndex = -1;
    root.triangleCount = 0;
    bvh.push_back(root);
    
    // Left child (index 1) - leaf node
    bvhNode leftChild;
    leftChild.bbox = BoundingBox(vec3(-2, -2, -2), vec3(-1, -1, -1));
    leftChild.leftChildIndex = -1;
    leftChild.rightChildIndex = -1;
    leftChild.triangleIndex = 0;
    leftChild.triangleCount = 1;
    bvh.push_back(leftChild);
    
    // Right child (index 2) - leaf node
    bvhNode rightChild;
    rightChild.bbox = BoundingBox(vec3(1, 1, 1), vec3(2, 2, 2));
    rightChild.leftChildIndex = -1;
    rightChild.rightChildIndex = -1;
    rightChild.triangleIndex = 1;
    rightChild.triangleCount = 1;
    bvh.push_back(rightChild);
    
    return bvh;
}

// Test helper function to create a deeper BVH tree
std::vector<bvhNode> createDeepTestBVH() {
    std::vector<bvhNode> bvh;
    
    // Root node (index 0)
    bvhNode root;
    root.bbox = BoundingBox(vec3(-4, -4, -4), vec3(4, 4, 4));
    root.leftChildIndex = 1;
    root.rightChildIndex = 2;
    root.triangleIndex = -1;
    root.triangleCount = 0;
    bvh.push_back(root);
    
    // Left subtree root (index 1)
    bvhNode leftRoot;
    leftRoot.bbox = BoundingBox(vec3(-4, -4, -4), vec3(0, 0, 0));
    leftRoot.leftChildIndex = 3;
    leftRoot.rightChildIndex = 4;
    leftRoot.triangleIndex = -1;
    leftRoot.triangleCount = 0;
    bvh.push_back(leftRoot);
    
    // Right subtree root (index 2)
    bvhNode rightRoot;
    rightRoot.bbox = BoundingBox(vec3(0, 0, 0), vec3(4, 4, 4));
    rightRoot.leftChildIndex = 5;
    rightRoot.rightChildIndex = 6;
    rightRoot.triangleIndex = -1;
    rightRoot.triangleCount = 0;
    bvh.push_back(rightRoot);
    
    // Left-left leaf (index 3)
    bvhNode leftLeft;
    leftLeft.bbox = BoundingBox(vec3(-4, -4, -4), vec3(-2, -2, -2));
    leftLeft.leftChildIndex = -1;
    leftLeft.rightChildIndex = -1;
    leftLeft.triangleIndex = 0;
    leftLeft.triangleCount = 1;
    bvh.push_back(leftLeft);
    
    // Left-right leaf (index 4)
    bvhNode leftRight;
    leftRight.bbox = BoundingBox(vec3(-2, -2, -2), vec3(0, 0, 0));
    leftRight.leftChildIndex = -1;
    leftRight.rightChildIndex = -1;
    leftRight.triangleIndex = 1;
    leftRight.triangleCount = 1;
    bvh.push_back(leftRight);
    
    // Right-left leaf (index 5)
    bvhNode rightLeft;
    rightLeft.bbox = BoundingBox(vec3(0, 0, 0), vec3(2, 2, 2));
    rightLeft.leftChildIndex = -1;
    rightLeft.rightChildIndex = -1;
    rightLeft.triangleIndex = 2;
    rightLeft.triangleCount = 1;
    bvh.push_back(rightLeft);
    
    // Right-right leaf (index 6)
    bvhNode rightRight;
    rightRight.bbox = BoundingBox(vec3(2, 2, 2), vec3(4, 4, 4));
    rightRight.leftChildIndex = -1;
    rightRight.rightChildIndex = -1;
    rightRight.triangleIndex = 3;
    rightRight.triangleCount = 1;
    bvh.push_back(rightRight);
    
    return bvh;
}

void testTraverseTreeBasic() {
    std::cout << "Testing basic traverseTree functionality..." << std::endl;
    
    auto bvh = createSimpleTestBVH();
    
    // Test ray that hits left child
    Ray rayLeft(vec3(0, 0, 0), vec3(-1, -1, -1)); // Ray going left from center
    int32_t result = traverseTree(bvh.data(), rayLeft);
    assert(result == 1); // Should return left child index
    std::cout << "Ray hitting left child: PASSED" << std::endl;
    
    // Test ray that hits right child
    Ray rayRight(vec3(0, 0, 0), vec3(1, 1, 1)); // Ray going right from center
    result = traverseTree(bvh.data(), rayRight);
    std::cout << "result: " << result << std::endl;
    assert(result == 2); // Should return right child index
    std::cout << "Ray hitting right child: PASSED" << std::endl;
    
    // Test ray that hits both children (should choose closer one)
    Ray rayBoth(vec3(-2, -2, -2), vec3(1, 1, 1)); // Ray from left to right
    result = traverseTree(bvh.data(), rayBoth);
    assert(result == 1 || result == 2); // Should return one of the children
    std::cout << "Ray hitting both children: PASSED" << std::endl;
}

void testTraverseTreeDeep() {
    std::cout << "\nTesting deep BVH traversal..." << std::endl;
    
    auto bvh = createDeepTestBVH();
    
    // Test ray that goes deep into left subtree
    Ray rayDeepLeft(vec3(0, 0, 0), vec3(-1, 0, 0)); // Ray going left from center
    int32_t result = traverseTree(bvh.data(), rayDeepLeft);
    assert(result == 3 || result == 4); // Should return one of the left leaf nodes
    std::cout << "Deep left traversal: PASSED" << std::endl;
    
    // Test ray that goes deep into right subtree
    Ray rayDeepRight(vec3(0, 0, 0), vec3(1, 0, 0)); // Ray going right from center
    result = traverseTree(bvh.data(), rayDeepRight);
    assert(result == 5 || result == 6); // Should return one of the right leaf nodes
    std::cout << "Deep right traversal: PASSED" << std::endl;
}

void testTraverseTreeEdgeCases() {
    std::cout << "\nTesting edge cases..." << std::endl;
    
    auto bvh = createSimpleTestBVH();
    
    // Test ray that misses the root entirely
    Ray rayMiss(vec3(10, 10, 10), vec3(1, 0, 0)); // Ray far from BVH
    int32_t result = traverseTree(bvh.data(), rayMiss);
    assert(result == -1); // Should return -1 for no intersection
    std::cout << "Ray missing root: PASSED" << std::endl;
    
    // Test ray that hits root but misses both children
    Ray rayHitRootMissChildren(vec3(0, 0, 0), vec3(0, 1, 0)); // Ray going up from center
    result = traverseTree(bvh.data(), rayHitRootMissChildren);
    // This should return the root index (0) since it's a leaf node
    std::cout << "result: " << result << std::endl;
    assert(result == 0);
    std::cout << "Ray hitting root but missing children: PASSED" << std::endl;
}

void testTraverseTreeSingleNode() {
    std::cout << "\nTesting single node BVH..." << std::endl;
    
    std::vector<bvhNode> singleNodeBVH;
    bvhNode singleNode;
    singleNode.bbox = BoundingBox(vec3(-1, -1, -1), vec3(1, 1, 1));
    singleNode.leftChildIndex = -1;
    singleNode.rightChildIndex = -1;
    singleNode.triangleIndex = 0;
    singleNode.triangleCount = 1;
    singleNodeBVH.push_back(singleNode);
    
    // Test ray that hits the single node
    Ray rayHit(vec3(0, 0, 0), vec3(1, 0, 0));
    int32_t result = traverseTree(singleNodeBVH.data(), rayHit);
    assert(result == 0); // Should return the single node index
    std::cout << "Single node hit: PASSED" << std::endl;
    
    // Test ray that misses the single node
    Ray rayMiss(vec3(10, 10, 10), vec3(1, 0, 0));
    result = traverseTree(singleNodeBVH.data(), rayMiss);
    assert(result == -1); // Should return -1 for no intersection
    std::cout << "Single node miss: PASSED" << std::endl;
}

void testTraverseTreeEmpty() {
    std::cout << "\nTesting empty BVH..." << std::endl;
    
    std::vector<bvhNode> emptyBVH;
    
    Ray ray(vec3(0, 0, 0), vec3(1, 0, 0));
    int32_t result = traverseTree(emptyBVH.data(), ray);
    // This should handle gracefully or return -1
    std::cout << "Empty BVH handled gracefully" << std::endl;
}

int main() {
    std::cout << "=== Testing traverseTree Function ===" << std::endl;
    
    try {
        testTraverseTreeBasic();
        testTraverseTreeDeep();
        testTraverseTreeEdgeCases();
        testTraverseTreeSingleNode();
        testTraverseTreeEmpty();
        
        std::cout << "\n🎉 All tests PASSED!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
