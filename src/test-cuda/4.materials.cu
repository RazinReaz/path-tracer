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
#include "utils/scene_loader.h"
#include "utils/cuda_utils.h"
#include "utils/renderStats.h"

const int screenHeight = 512;
const int screenWidth = 512;
const int totalPixels = screenWidth * screenHeight;
float aspect = screenWidth / screenHeight;
float invWidth = 1.0f / screenWidth;
float invHeight = 1.0f / screenHeight;

// Camera parameters
Camera camera(vec3(0, 1, 3), vec3(0, 1, 0), -90.0f, 0.0f, 45.0f, 0.1f, 100.0f, aspect);
float widthMultiplier = invWidth * camera.fullwidth;
float heightMultiplier = invHeight * camera.fullheight;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

bool screenshotTaken = false;

const int bounces = 5;
const int spp = 1;
const int SEED = 42;
int frameCount = 0;

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void processInput(GLFWwindow *window);
void takeScreenshot(GLFWwindow *window, const std::string &filename);


const char *vertexShaderPath = "assets/shaders/cuda/vert.vs";
const char *fragmentShaderPath = "assets/shaders/cuda/frag.fs";
const char *mtlBasePath = "assets/models/CornellBox/";
const char *modelObjPath = "assets/models/CornellBox/CornellBox-Original.obj";
// const char *modelObjPath = "assets/models/cube/cube.obj";
// const char *mtlBasePath = "assets/models/cube/";

// __global__ vec3 skyColor(0.63, 0.85, 0.92);

__global__ void initialize_rng(curandState_t *states, unsigned long seed, int total) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx > total) return;
    curand_init(seed, idx, 0, &states[idx]);
}

__global__
void render(
    uchar4 *ptr, 
    const int w, const int h, 
    const float wMult, const float hMult, 
    Camera camera, 
    Scene *d_scene, Material *d_materials, curandState_t *states,
    int bounces, int spp,
    float *d_framebuffer, float weight
) 
{
	int pixelx = threadIdx.x + blockIdx.x * blockDim.x;
	int pixely = threadIdx.y + blockIdx.y * blockDim.y;
    if (pixelx >= w || pixely >= h) return;
	int offset = pixelx + pixely * w;

    curandState_t state = states[offset];

    vec3 light(0.0f, 0.0f, 0.0f);
    for (int i = 0; i < spp; i++) {
        float u = ((float)pixelx + curand_uniform(&state)) * wMult;
        float v = ((float)pixely + curand_uniform(&state)) * hMult;
        
        vec3 rayOrigin = camera.position;
        vec3 rayDest = camera.bottomleft + u * camera.right + v * camera.up;
        Ray ray(rayOrigin, rayDest - rayOrigin);
        
        vec3  attenuation(1.0f, 1.0f, 1.0f);
        int nBounces = bounces;
        while(nBounces--) {
            ray.reset_hit();
            d_scene->calculate_hit_by(ray);
            if (!ray.info.hit) {
                // vec3 d_skycolor(0.1f, 0.1f, 0.1f);  //! this should be changed 
                vec3 d_skycolor(0.63f, 0.85f, 0.92f);  //! this should be changed 
                light += d_skycolor * attenuation;
                break;
            }
            light += vec3((ray.info.norm.x + 1) * 0.5f, (ray.info.norm.y + 1) * 0.5f, (ray.info.norm.z + 1) * 0.5f);
            break;
            // Material mat = d_materials[ray.info.mat_idx];
            // // light += mat.albedo;
            // attenuation *= mat.albedo;
            // light += mat.emission * attenuation;  //! this should be changed
            // state = states[offset];
            // bounce(ray, mat, &state);
            // states[offset] = state;
        }
    }
    light.scale(1.0f / spp);

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

void resetFrameCount() {
    frameCount = 1;
}

int main() {
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "Materials", NULL, NULL);
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


    

    Shader shader(vertexShaderPath, fragmentShaderPath);

    Triangle *d_triangles;
    Scene *d_scene;
    std::vector<Triangle> h_triangles;
    std::vector<Material> h_materials;
    std::cout << "loading model from " << modelObjPath << std::endl;

    loadTrianglesAndMaterialsFromOBJ(modelObjPath, mtlBasePath, h_triangles, h_materials);
    uploadSceneToGPU(h_triangles, &d_triangles, &d_scene);
    std::cout << "Scene successfully upload to GPU"<< std::endl;

    // materials
    // load materials array from obj
    Material *d_materials;
    size_t materialCount = h_materials.size();
    if (materialCount == 0) {
        std::cerr << "No materials found in the model. Exiting." << std::endl;
        //TODO: if no materials then define default ones?
        return -1;
    }
    CUDA_CHECK(cudaMalloc(&d_materials, materialCount * sizeof(Material)));
    CUDA_CHECK(cudaMemcpy(d_materials, h_materials.data(), materialCount * sizeof(Material), cudaMemcpyHostToDevice));
    std::cout << "Materials successfully uploaded to GPU" << std::endl;

    //curand stuff
    curandState_t *d_states; //declare the states array
    CUDA_CHECK(cudaMalloc(&d_states, totalPixels * sizeof(curandState_t))); // allocate space in the GPU for the states array
    initialize_rng<<<(totalPixels + 255) / 256, 256>>>(d_states, SEED, totalPixels); // initialize the values of the states array in the GPU
    cudaDeviceSynchronize();
    std::cout << "rng states successfully initialized" << std::endl;

    // allocate memory for frame buffer
    float *d_framebuffer;
    CUDA_CHECK(cudaMalloc(&d_framebuffer, totalPixels * 3 * sizeof(float)));
    CUDA_CHECK(cudaMemset(d_framebuffer, 0, totalPixels * 3 * sizeof(float)));
    std::cout << "Framebuffer successfully allocated" << std::endl;


    // // sky
    // vec3 h_skycolor(0.63, 0.85, 0.92); // for example
    // CUDA_CHECK(cudaMemcpyToSymbol(d_skycolor, &h_skycolor, sizeof(vec3)));
    // std::cout << "Sky color successfully uploaded" << std::endl;
    

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
        
        //#######################################################
        //################## stats start ########################
        //#######################################################
        stats.frameStart();
        frameCount++;
        float frameWeight = 1.0f / (float)frameCount;
        dim3 blocksPerGrid((screenWidth + 15)/16, (screenHeight + 15)/16);
        dim3 threadsPerBlock(16, 16);
        render<<<blocksPerGrid, threadsPerBlock>>>(device_pointer, 
            screenWidth, screenHeight, 
            widthMultiplier, heightMultiplier,
            camera, d_scene, d_materials, d_states,
            bounces, spp,
            d_framebuffer, frameWeight
        );
        cudaDeviceSynchronize();
        cudaGraphicsUnmapResources(1, &cuda_resource, NULL);
        stats.frameEnd();
        //#####################################################
        //################## stats end ########################
        //#####################################################

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
    }
    stats.saveLog("./logs");
    takeScreenshot(window, "./logs");
    shader.del();
    glDeleteBuffers(1, &pbo);
    glDeleteTextures(1, &texture);
    cudaGraphicsUnregisterResource(cuda_resource);
    CUDA_CHECK(cudaFree(d_states));
    CUDA_CHECK(cudaFree(d_materials));
    CUDA_CHECK(cudaFree(d_framebuffer));
    freeSceneFromGPU(d_triangles, d_scene);
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
        takeScreenshot(window, "./assets/screenshots/screenshot.png");
        screenshotTaken = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        screenshotTaken = false;
    }
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