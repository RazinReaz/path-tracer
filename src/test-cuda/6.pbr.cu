#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <iostream>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>


// for taking screenshots
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define DEBUG 1

#include "shader.h"
#include "math/vec3.h"
#include "ray-tracer/ray.h"
#include "ray-tracer/triangle.h"
#include "ray-tracer/camera.h"
#include "ray-tracer/scene.h"
#include "ray-tracer/materials.h"
#include "ray-tracer/bvh.h"
#include "ray-tracer/brdf.h"
#include "scene-loader/SceneLoader.h"
#include "scene-loader/OBJloader.h"
#include "scene-loader/upload.h"
#include "utils/cuda_utils.h"
#include "utils/renderStats.h"

const int screenHeight = 512;
const int screenWidth = 512;
const int totalPixels = screenWidth * screenHeight;
float aspect = screenWidth / screenHeight;
float invWidth = 1.0f / screenWidth;
float invHeight = 1.0f / screenHeight;

// Camera parameters
Camera camera(vec3(0, 1, 4.2), vec3(0, 1, 0), -90.0f, 0.0f, 45.0f, 0.1f, 100.0f, aspect);
float widthMultiplier = invWidth * camera.fullwidth;
float heightMultiplier = invHeight * camera.fullheight;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

bool screenshotTaken = false;
bool showTestCount = false; // Toggle for test count visualization
bool showStats = false; // Toggle for statistics display
int visualizationMode = 0; // 0: normal, 1: test count, 2: test count with opacity

const int bounces = 5;
const int spp = 1;
const int SEED = 42;
int frameCount = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void processInput(GLFWwindow *window);
void takeScreenshot(GLFWwindow *window, const std::string &filename);

// Function to calculate test count statistics
void calculateTestCountStats(bvhNode *d_bvh, Triangle *d_triangles, Camera camera, int maxTests) {
    std::vector<int32_t> testCounts;
    testCounts.reserve(screenWidth * screenHeight);
    
    // Sample test counts from a grid of rays
    for (int y = 0; y < screenHeight; y += 4) { // Sample every 4th pixel for performance
        for (int x = 0; x < screenWidth; x += 4) {
            float u = ((float)x + 0.5f) * widthMultiplier;
            float v = ((float)y + 0.5f) * heightMultiplier;
            
            vec3 rayOrigin = camera.position;
            vec3 rayDest = camera.bottomleft + u * camera.right + v * camera.up;
            Ray ray(rayOrigin, rayDest - rayOrigin);
            
            // Count triangle tests for this ray
            int32_t testCount = countTriangleTests(d_bvh, d_triangles, ray);
            testCounts.push_back(testCount);
        }
    }
    
    if (testCounts.empty()) return;
    
    // Calculate statistics
    std::sort(testCounts.begin(), testCounts.end());
    int32_t minTests = testCounts.front();
    maxTests = testCounts.back();
    int32_t medianTests = testCounts[testCounts.size() / 2];
    
    float avgTests = 0.0f;
    for (int count : testCounts) {
        avgTests += (float)count;
    }
    avgTests /= testCounts.size();
    
    // Calculate percentiles
    int32_t p90 = testCounts[(int)(testCounts.size() * 0.9f)];
    int32_t p95 = testCounts[(int)(testCounts.size() * 0.95f)];
    int32_t p99 = testCounts[(int)(testCounts.size() * 0.99f)];
    
    // Calculate variance and standard deviation
    float variance = 0.0f;
    for (int count : testCounts) {
        float diff = (float)count - avgTests;
        variance += diff * diff;
    }
    variance /= testCounts.size();
    float stdDev = sqrtf(variance);
    
    // Count rays in different test count ranges
    int lowTests = 0, mediumTests = 0, highTests = 0;
    for (int count : testCounts) {
        if (count <= 10) lowTests++;
        else if (count <= 100) mediumTests++;
        else highTests++;
    }
    
    std::cout << "\n=== Triangle Test Count Statistics ===" << std::endl;
    std::cout << "Min tests: " << minTests << std::endl;
    std::cout << "Max tests: " << maxTests << std::endl;
    std::cout << "Average tests: " << std::fixed << std::setprecision(1) << avgTests << std::endl;
    std::cout << "Median tests: " << medianTests << std::endl;
    std::cout << "Standard deviation: " << std::fixed << std::setprecision(1) << stdDev << std::endl;
    std::cout << "90th percentile: " << p90 << std::endl;
    std::cout << "95th percentile: " << p95 << std::endl;
    std::cout << "99th percentile: " << p99 << std::endl;
    std::cout << "\nDistribution:" << std::endl;
    std::cout << "  Low (≤10 tests): " << lowTests << " rays (" << std::fixed << std::setprecision(1) << (float)lowTests/testCounts.size()*100 << "%)" << std::endl;
    std::cout << "  Medium (11-100 tests): " << mediumTests << " rays (" << std::fixed << std::setprecision(1) << (float)mediumTests/testCounts.size()*100 << "%)" << std::endl;
    std::cout << "  High (>100 tests): " << highTests << " rays (" << std::fixed << std::setprecision(1) << (float)highTests/testCounts.size()*100 << "%)" << std::endl;
    std::cout << "\nTotal samples: " << testCounts.size() << std::endl;
    std::cout << "=====================================" << std::endl;
}

const char *vertexShaderPath = "assets/shaders/cuda/vert.vs";
const char *fragmentShaderPath = "assets/shaders/cuda/frag.fs";
const char *mtlBasePath = "assets/models/obj/bunny-pbr-small/";
const char *modelObjPath = "assets/models/obj/bunny-pbr-small/bunny-pbr-small.obj";

// __global__ vec3 skyColor(0.63, 0.85, 0.92);

__global__ void initialize_rng(curandStatePhilox4_32_10_t* states, unsigned long seed, int total) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx >= total) return;
    // Same seed for reproducibility, unique sequence per pixel
    curand_init(seed, idx, 0, &states[idx]);
}


__global__
void render(
    uchar4 *ptr, 
    const int w, const int h, 
    const float wMult, const float hMult, 
    Camera camera, 
    Triangle *d_triangles, bvhNode *d_bvh,
     Material *d_materials, curandStatePhilox4_32_10_t *states,
    int bounces, int spp,
    float *d_framebuffer, float weight
) 
{
	int pixelx = threadIdx.x + blockIdx.x * blockDim.x;
	int pixely = threadIdx.y + blockIdx.y * blockDim.y;
    if (pixelx >= w || pixely >= h) return;
	int offset = pixelx + pixely * w;

    curandStatePhilox4_32_10_t state = states[offset];

    vec3 light(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < spp; i++) {
        // Generate 4 random values at once
        float4 rand4 = curand_uniform4(&state);
        
        float u = ((float)pixelx + rand4.x) * wMult;
        float v = ((float)pixely + rand4.y) * hMult;
        
        vec3 rayOrigin = camera.position;
        vec3 rayDest = camera.bottomleft + u * camera.right + v * camera.up;
        Ray ray(rayOrigin, rayDest - rayOrigin);
        
        vec3  attenuation(1.0f, 1.0f, 1.0f);
        int nBounces = bounces;
        vec3 newdir, weight;
        while(nBounces--) {
            ray.reset_hit();
            traverseTree(d_bvh, d_triangles, ray);
            if (!ray.info.hit) {
                vec3 d_skycolor(0.63f, 0.85f, 0.92f);  //! this should be changed 
                light += d_skycolor * attenuation;
                break;
            }

            Material mat = d_materials[ray.info.mat_idx];
            bool refracted = evalBRDF(ray.info.norm, -ray.direction, mat, &state, newdir, weight, ray.info);
            attenuation *= weight;
            float eps = refracted ? -0.001f : 0.001f;
            ray.set_origin_and_direction(ray.origin + ray.direction * ray.info.t + ray.info.norm * eps, newdir);
            light += mat.emission * attenuation; 
        }
    }
    light.scale(1.0f / spp);
    states[offset] = state;

    // write to framebuffer
    int base = offset * 3;
    d_framebuffer[base + 0] = d_framebuffer[base + 0] * (1.0f - weight) + light.r * weight;
    d_framebuffer[base + 1] = d_framebuffer[base + 1] * (1.0f - weight) + light.g * weight;
    d_framebuffer[base + 2] = d_framebuffer[base + 2] * (1.0f - weight) + light.b * weight;

    ptr[offset] = make_uchar4(
        fminf(255.0f, d_framebuffer[base + 0] * 255.0f), 
        fminf(255.0f, d_framebuffer[base + 1] * 255.0f),
        fminf(255.0f, d_framebuffer[base + 2] * 255.0f),
        255
    );
}

__global__
void renderTestCount(
    uchar4 *ptr, 
    const int w, const int h, 
    const float wMult, const float hMult, 
    Camera camera, 
    Triangle *d_triangles, bvhNode *d_bvh,
    int maxTests
) 
{
	int pixelx = threadIdx.x + blockIdx.x * blockDim.x;
	int pixely = threadIdx.y + blockIdx.y * blockDim.y;
    if (pixelx >= w || pixely >= h) return;
	int offset = pixelx + pixely * w;

    float u = ((float)pixelx + 0.5f) * wMult;
    float v = ((float)pixely + 0.5f) * hMult;
    
    vec3 rayOrigin = camera.position;
    vec3 rayDest = camera.bottomleft + u * camera.right + v * camera.up;
    Ray ray(rayOrigin, rayDest - rayOrigin);
    
    // Count triangle tests for this ray
    int32_t testCount = countTriangleTests(d_bvh, d_triangles, ray);
    
    // Normalize test count to 0-1 range (using log scale for better distribution)
    float normalizedCount = testCount > 0 ? logf((float)testCount + 1.0f) / logf((float)maxTests + 1.0f) : 0.0f;
    
    // Create a color based on test count using a heat map
    vec3 color;
    if (normalizedCount < 0.25f) {
        // Blue to cyan (very few tests)
        float t = normalizedCount / 0.25f;
        color = vec3(0.0f, t, 1.0f);
    } else if (normalizedCount < 0.5f) {
        // Cyan to green (few tests)
        float t = (normalizedCount - 0.25f) / 0.25f;
        color = vec3(0.0f, 1.0f, 1.0f - t);
    } else if (normalizedCount < 0.75f) {
        // Green to yellow (medium tests)
        float t = (normalizedCount - 0.5f) / 0.25f;
        color = vec3(t, 1.0f, 0.0f);
    } else {
        // Yellow to red (many tests)
        float t = (normalizedCount - 0.75f) / 0.25f;
        color = vec3(1.0f, 1.0f - t, 0.0f);
    }
    
    // Add some brightness variation for better visualization
    float brightness = 0.4f + normalizedCount * 0.6f;
    color.scale(brightness);

    ptr[offset] = make_uchar4(
        fminf(255.0f, color.r * 255.0f), 
        fminf(255.0f, color.g * 255.0f),
        fminf(255.0f, color.b * 255.0f),
        255
    );
}

__global__
void renderTestCountOpacity(
    uchar4 *ptr, 
    const int w, const int h, 
    const float wMult, const float hMult, 
    Camera camera, 
    Triangle *d_triangles, bvhNode *d_bvh,
    int maxTests
) 
{
	int pixelx = threadIdx.x + blockIdx.x * blockDim.x;
	int pixely = threadIdx.y + blockIdx.y * blockDim.y;
    if (pixelx >= w || pixely >= h) return;
	int offset = pixelx + pixely * w;

    float u = ((float)pixelx + 0.5f) * wMult;
    float v = ((float)pixely + 0.5f) * hMult;
    
    vec3 rayOrigin = camera.position;
    vec3 rayDest = camera.bottomleft + u * camera.right + v * camera.up;
    Ray ray(rayOrigin, rayDest - rayOrigin);
    
    // Count triangle tests for this ray
    int32_t testCount = countTriangleTests(d_bvh, d_triangles, ray);
    
    // Normalize test count to 0-1 range (using log scale for better distribution)
    float normalizedCount = testCount > 0 ? logf((float)testCount + 1.0f) / logf((float)maxTests + 1.0f) : 0.0f;
    
    // Create a grayscale color based on test count
    float intensity = normalizedCount;
    vec3 color(intensity, intensity, intensity);
    
    // Calculate opacity based on test count (more tests = more opaque)
    int alpha = (int)(128 + normalizedCount * 127); // 128-255 range
    
    ptr[offset] = make_uchar4(
        fminf(255.0f, color.r * 255.0f), 
        fminf(255.0f, color.g * 255.0f),
        fminf(255.0f, color.b * 255.0f),
        alpha
    );
}

void resetFrameCount() {
    frameCount = 1;
}

void updateWindowTitle(GLFWwindow *window) {
    std::string title = "BVH Ray Tracer - ";
    if (visualizationMode == 0) {
        title += "Normal Rendering";
    } else if (visualizationMode == 1) {
        title += "Test Count Visualization";
    } else if (visualizationMode == 2) {
        title += "Test Count with Opacity";
    }
    glfwSetWindowTitle(window, title.c_str());
}

int main() {
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "BVH", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Use core OpenGL 3+
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    // Set initial window title
    updateWindowTitle(window);

    Shader shader(vertexShaderPath, fragmentShaderPath);
    SceneLoader *loader = new OBJLoader(modelObjPath, mtlBasePath);

    Triangle *d_triangles;
    bvhNode *d_bvh;
    Material *d_materials;
    float *d_framebuffer;

    std::vector<Triangle> h_triangles;
    std::vector<Material> h_materials;
    std::vector<bvhNode> h_bvh;

    std::cout << "loading model from " << modelObjPath << std::endl;
    loader->loadTrianglesAndMaterials(h_triangles, h_materials);
    std::cout << "number of triangles: " << h_triangles.size() << std::endl;
    std::cout << "triangles and PBR materials loaded" << std::endl;
    h_bvh = createBVHandSortTriangles(h_triangles);
    std::cout << "BVH created and triangles sorted" << std::endl;


    uploadTrianglesToGPU(h_triangles, &d_triangles);
    uploadBVHToGPU(h_bvh, &d_bvh);
    std::cout << "BVH and triangles successfully uploaded to GPU" << std::endl;
    std::cout << "Size of bvh node: " << sizeof(bvhNode) << std::endl;
    // materials
    uploadMaterialsToGPU(h_materials, &d_materials);
    std::cout << "Materials successfully uploaded to GPU" << std::endl;

    //curand stuff
    curandStatePhilox4_32_10_t *d_states; //declare the states array
    CUDA_CHECK(cudaMalloc(&d_states, totalPixels * sizeof(curandStatePhilox4_32_10_t))); // allocate space in the GPU for the states array
    initialize_rng<<<(totalPixels + 255) / 256, 256>>>(d_states, SEED, totalPixels); // initialize the values of the states array in the GPU
    std::cout << "rng states successfully initialized" << std::endl;

    // allocate memory for frame buffer
    allocateFrameBuffer(totalPixels, &d_framebuffer);
    std::cout << "Framebuffer successfully allocated" << std::endl;

    std::cout << "size of vec3: " << sizeof(vec3) << std::endl;
    std::cout << "size of ray: " << sizeof(Ray) << std::endl;
    std::cout << "size of triangle: " << sizeof(Triangle) << std::endl;
    std::cout << "size of material: " << sizeof(Material) << std::endl;
    std::cout << "size of info: " << sizeof(Info) << std::endl;

    GLuint vao;
    GLuint pbo;
    GLuint texture;
    cudaGraphicsResource* cuda_resource;

    // init VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Pixel buffer object for shared resource with CUDA
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, screenWidth * screenHeight * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    cudaGraphicsGLRegisterBuffer(&cuda_resource, pbo, cudaGraphicsMapFlagsWriteDiscard); // using this flag because we only need to write, not read the previous stuff

    // Texture for fullscreen quad
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 

    shader.use();
    shader.setInt("tex", 0);
    // glUniform1i(glGetUniformLocation(shader, "tex"), 0);


    //For logging the results
    RenderStats stats(h_triangles.size(), screenWidth, screenHeight, bounces, spp);
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        uchar4* device_pointer;
        size_t size;
        cudaGraphicsMapResources(1, &cuda_resource, NULL);        
        cudaGraphicsResourceGetMappedPointer((void**)&device_pointer, &size, cuda_resource);
        
        stats.frameStart();
        frameCount++;
        float frameWeight = 1.0f / (float)frameCount;
        dim3 blocksPerGrid((screenWidth + 15)/16, (screenHeight + 15)/16);
        dim3 threadsPerBlock(16, 16);
        if (visualizationMode == 0) {
            render<<<blocksPerGrid, threadsPerBlock>>>(device_pointer, 
                screenWidth, screenHeight, 
                widthMultiplier, heightMultiplier,
                camera, d_triangles, d_bvh, d_materials, d_states,
                bounces, spp,
                d_framebuffer, frameWeight
            );
        } else if (visualizationMode == 1) {
            renderTestCount<<<blocksPerGrid, threadsPerBlock>>>(device_pointer, 
                screenWidth, screenHeight, 
                widthMultiplier, heightMultiplier,
                camera, d_triangles, d_bvh, h_triangles.size() * h_triangles.size() // Pass maxTests
            );
        } else if (visualizationMode == 2) {
            renderTestCountOpacity<<<blocksPerGrid, threadsPerBlock>>>(device_pointer, 
                screenWidth, screenHeight, 
                widthMultiplier, heightMultiplier,
                camera, d_triangles, d_bvh, h_triangles.size() * h_triangles.size() // Pass maxTests
            );
        }
        cudaDeviceSynchronize();
        cudaGraphicsUnmapResources(1, &cuda_resource, NULL);
        stats.frameEnd();


        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        
        processInput(window);
        glfwSwapBuffers(window);     // Swap double buffer
        glfwPollEvents();            // Handle input events
        
        // Update window title every frame to show current mode
        updateWindowTitle(window);
    }
    stats.saveLog("./logs");
    takeScreenshot(window, "./logs");

    shader.del();
    glDeleteBuffers(1, &pbo);
    glDeleteTextures(1, &texture);
    cudaGraphicsUnregisterResource(cuda_resource);
    CUDA_CHECK(cudaFree(d_states));

    freeMaterialsFromGPU(d_materials);
    freeTrianglesFromGPU(d_triangles);
    freeFrameBuffer(d_framebuffer);
    freeBVHFromGPU(d_bvh);

    glfwDestroyWindow(window); //! order of operation correct?
    glfwTerminate();
        
    return 0;
}


void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        resetFrameCount();
        camera.handleKeyboardInput(FORWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        resetFrameCount();
        camera.handleKeyboardInput(BACKWARD, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        resetFrameCount();
        camera.handleKeyboardInput(LEFT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        resetFrameCount();
        camera.handleKeyboardInput(RIGHT, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !screenshotTaken) {
        takeScreenshot(window, "./assets/screenshots");
        screenshotTaken = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        screenshotTaken = false;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !screenshotTaken) {
        visualizationMode = (visualizationMode + 1) % 3;
        resetFrameCount(); // Reset frame count when switching modes
        std::cout << "Visualization mode toggled: " << visualizationMode << std::endl;
        updateWindowTitle(window);
        screenshotTaken = true; // Prevent multiple toggles
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        screenshotTaken = false;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS && !screenshotTaken) {
        std::cout << "\n=== Controls ===" << std::endl;
        std::cout << "WASD: Move camera" << std::endl;
        std::cout << "Mouse: Look around" << std::endl;
        std::cout << "T: Toggle visualization mode (0: Normal, 1: Test Count, 2: Test Count with Opacity)" << std::endl;
        // std::cout << "S: Show test count statistics" << std::endl;
        std::cout << "P: Take screenshot" << std::endl;
        std::cout << "H: Show this help" << std::endl;
        std::cout << "M: Show current mode" << std::endl;
        std::cout << "ESC: Exit" << std::endl;
        std::cout << "================" << std::endl;
        screenshotTaken = true;
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE) {
        screenshotTaken = false;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !screenshotTaken) {
        std::cout << "Current visualization mode: " << visualizationMode;
        if (visualizationMode == 0) {
            std::cout << " (Normal Rendering)";
        } else if (visualizationMode == 1) {
            std::cout << " (Test Count Visualization)";
        } else if (visualizationMode == 2) {
            std::cout << " (Test Count with Opacity)";
        }
        std::cout << std::endl;
        screenshotTaken = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        screenshotTaken = false;
    }
    // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !screenshotTaken) {
    //     if (visualizationMode == 1 || visualizationMode == 2) { // Only show stats if test count is active
    //         std::cout << "Calculating test count statistics..." << std::endl;
    //         calculateTestCountStats(d_bvh, d_triangles, camera, h_triangles.size());
    //     } else {
    //         std::cout << "Test count visualization must be enabled (press T) to show statistics" << std::endl;
    //     }
    //     screenshotTaken = true;
    // }
    // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
    //     screenshotTaken = false;
    // }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// Process mouse movement
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    resetFrameCount(); // reset frame count on mouse movement
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    camera.handleMouseMovement(xoffset, yoffset);
}

void takeScreenshot(GLFWwindow *window, const std::string &folderPath)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    std::vector<unsigned char> pixels(3 * width * height);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically
    for (int j = 0; j < height / 2; ++j)
    {
        for (int i = 0; i < width * 3; ++i)
        {
            std::swap(pixels[j * width * 3 + i], pixels[(height - 1 - j) * width * 3 + i]);
        }
    }
    // Generate timestamp for filename
    std::time_t now = std::time(nullptr);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &localTime);
    std::string filePath = folderPath + "/screenshot_" + buffer + ".png";
    // Save PNG
    stbi_write_png(filePath.c_str(), width, height, 3, pixels.data(), width * 3);
    std::cout << "Screenshot saved to: " << filePath << std::endl;
}