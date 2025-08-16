#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include "../../include/ray-tracer/bvh.h"
#include "../../include/ray-tracer/triangle.h"
#include "../../include/math/vec3.h"

// Performance benchmark for BVH creation
class BVHBenchmark {
private:
    std::vector<Triangle> triangles;
    
    // Generate random triangles for testing
    void generateRandomTriangles(int count) {
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
    
public:
    // Run benchmark for a specific triangle count
    void runBenchmark(int triangleCount, int iterations = 5) {
        std::cout << "=== BVH Performance Benchmark ===" << std::endl;
        std::cout << "Triangles: " << triangleCount << std::endl;
        std::cout << "Iterations: " << iterations << std::endl;
        std::cout << std::endl;
        
        // Generate test data
        std::cout << "Generating test data..." << std::endl;
        generateRandomTriangles(triangleCount);
        
        // Warm up run
        std::cout << "Warming up..." << std::endl;
        auto warmup = createTree(triangles);
        
        // Benchmark runs
        std::vector<long long> times;
        times.reserve(iterations);
        
        std::cout << "Running benchmarks..." << std::endl;
        for (int i = 0; i < iterations; i++) {
            auto start = std::chrono::high_resolution_clock::now();
            
            auto bvh = createTree(triangles);
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            times.push_back(duration.count());
            
            std::cout << "  Run " << (i + 1) << ": " << duration.count() << " μs, " 
                      << bvh.size() << " nodes" << std::endl;
        }
        
        // Calculate statistics
        if (times.size() > 1) {
            // Remove fastest and slowest times for more stable results
            std::sort(times.begin(), times.end());
            times.erase(times.begin()); // Remove fastest
            times.pop_back(); // Remove slowest
            
            long long totalTime = 0;
            for (auto time : times) {
                totalTime += time;
            }
            
            double avgTime = static_cast<double>(totalTime) / times.size();
            double avgTimePerTriangle = avgTime / triangleCount;
            
            std::cout << std::endl;
            std::cout << "=== Results ===" << std::endl;
            std::cout << "Average time: " << std::fixed << std::setprecision(2) 
                      << avgTime << " μs" << std::endl;
            std::cout << "Time per triangle: " << std::fixed << std::setprecision(4) 
                      << avgTimePerTriangle << " μs" << std::endl;
            std::cout << "Triangles per second: " << std::fixed << std::setprecision(0) 
                      << (1000000.0 / avgTimePerTriangle) << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    // Run comprehensive benchmark suite
    void runComprehensiveBenchmark() {
        std::cout << "BVH Comprehensive Performance Benchmark" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << std::endl;
        
        std::vector<int> testSizes = {10, 50, 100, 500, 1000, 5000, 10000};
        
        for (int size : testSizes) {
            runBenchmark(size, 3); // Fewer iterations for larger scenes
            std::cout << std::string(50, '-') << std::endl;
        }
        
        std::cout << "Benchmark completed!" << std::endl;
    }
};

int main() {
    BVHBenchmark benchmark;
    
    // Run comprehensive benchmark
    benchmark.runComprehensiveBenchmark();
    
    return 0;
}
