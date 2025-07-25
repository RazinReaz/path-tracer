#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cuda_runtime.h>
#include <cuda_gl_interop.h>

#include <iostream>
#include <vector>

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
#include "utils/cuda_macro.h"

const int screenHeight = 512;
const int screenWidth = 512;
const int totalPixels = screenWidth * screenHeight;
const int bounces = 2;
const int SEED = 42;

float aspect = screenWidth / screenHeight;
float invWidth = 1.0f / screenWidth;
float invHeight = 1.0f / screenHeight;
Camera camera(vec3(0, 0, 5), vec3(0, 1, 0), -90.0f, 0.0f, 45.0f, 0.1f, 100.0f, aspect);
float widthMultiplier = invWidth * camera.fullwidth;
float heightMultiplier = invHeight * camera.fullheight;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;

bool screenshotTaken = false;


void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void processInput(GLFWwindow *window);
void takeScreenshot(GLFWwindow *window, const std::string &filename);


const char *vertexShaderPath = "assets/shaders/cuda/vert.vs";
const char *fragmentShaderPath = "assets/shaders/cuda/frag.fs";
// const char *modelPath = "assets/models/CornellBox/CornellBox-Original.obj";
const char *modelPath = "assets/models/cube/cube.obj";

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
    int bounces) 
{
	int pixelx = threadIdx.x + blockIdx.x * blockDim.x;
	int pixely = threadIdx.y + blockIdx.y * blockDim.y;
    if (pixelx >= w || pixely >= h) return;
	int offset = pixelx + pixely * w;

    float u = (float)pixelx * wMult;
    float v = (float)pixely * hMult;

    vec3 rayOrigin = camera.position;
    vec3 rayDest = camera.bottomleft + u * camera.right + v * camera.up;
    Ray ray(rayOrigin, rayDest - rayOrigin);

    vec3 attenuation(1.0f, 1.0f, 1.0f), color(0.0f, 0.0f, 0.0f);

    while(bounces--) {
        ray.reset_hit();
        d_scene->calculate_hit_by(ray);
        if (!ray.info.hit) {
            vec3 d_skycolor(0.63, 0.85, 0.92);  //! RAZIN this should be changed 
            color += d_skycolor * attenuation;
            // color.r = 0.7f;
            // color.g = 0.7f;
            // color.b = 1.0f;
            break;
        }
        // ray hit something
        Material mat = d_materials[ray.info.mat_idx];
        attenuation *= mat.albedo;
        color += mat.emission * attenuation;

        
        curandState_t state = states[offset];
        bounce(ray, mat, &state);
        states[offset] = state;
        
        // color.r = 0.5f * (ray.direction.x + 1.0f);
        // color.g = 0.5f * (ray.direction.y + 1.0f);
        // color.b = 0.5f * (ray.direction.z + 1.0f);
    }

	ptr[offset] = make_uchar4(color.r * 255, color.g * 255, color.b * 255, 255);
}



int main() {
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    GLFWwindow *window = glfwCreateWindow(screenWidth, screenHeight, "CUDA+OpenGL interop minimal", NULL, NULL);
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


    GLuint vao;
    GLuint pbo;
    GLuint texture;
    cudaGraphicsResource* cuda_resource;

    Shader shader(vertexShaderPath, fragmentShaderPath);

    Triangle *d_triangles;
    Scene *d_scene;
    std::cout << "loading model from " << modelPath << std::endl;
    std::vector<Triangle> h_triangles = loadTrianglesFromOBJ(modelPath);
    std::cout << "uploading scene to GPU"<< std::endl;
    uploadSceneToGPU(h_triangles, &d_triangles, &d_scene);

    //curand stuff
    curandState_t *d_states; //declare the states array
    CUDA_CHECK(cudaMalloc(&d_states, totalPixels * sizeof(curandState_t))); // allocate space in the GPU for the states array
    initialize_rng<<<(totalPixels + 255) / 256, 256>>>(d_states, SEED, totalPixels); // initialize the values of the states array in the GPU
    cudaDeviceSynchronize();
    std::cout << "rng states successfully initialized" << std::endl;
    


    // // sky
    // vec3 h_skycolor(0.63, 0.85, 0.92); // for example
    // CUDA_CHECK(cudaMemcpyToSymbol(d_skycolor, &h_skycolor, sizeof(vec3)));
    // std::cout << "Sky color successfully uploaded" << std::endl;
    
    
    
    // materials
    Material *d_materials;
    Material h_material;
    // load materials array from obj
    h_material.type = LAMBERTIAN;
    h_material.albedo = vec3(1.0f, 0.2f, 0.2f); 
    h_material.emission = vec3(0.1f, 0.1f, 0.1f); 
    CUDA_CHECK(cudaMalloc(&d_materials, 1 * sizeof(Material)));
    CUDA_CHECK(cudaMemcpy(d_materials, &h_material, 1 * sizeof(Material), cudaMemcpyHostToDevice));
    std::cout << "Materials data copied to device" << std::endl;


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

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        uchar4* device_pointer;
        size_t size;
        cudaGraphicsMapResources(1, &cuda_resource, NULL);        
        cudaGraphicsResourceGetMappedPointer((void**)&device_pointer, &size, cuda_resource);
        
        dim3 blocksPerGrid((screenWidth + 15)/16, (screenHeight + 15)/16);
        dim3 threadsPerBlock(16, 16);
        render<<<blocksPerGrid, threadsPerBlock>>>(device_pointer, 
            screenWidth, screenHeight, 
            widthMultiplier, heightMultiplier,
            camera, d_scene, d_materials, d_states,
            bounces);
            
        cudaDeviceSynchronize();
        cudaGraphicsUnmapResources(1, &cuda_resource, NULL);

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
    
    shader.del();
    glDeleteBuffers(1, &pbo);
    glDeleteTextures(1, &texture);
    cudaGraphicsUnregisterResource(cuda_resource);
    cudaFree(d_states);
    cudaFree(d_materials);
    freeSceneFromGPU(d_triangles, d_scene);
    glfwDestroyWindow(window); //! order of operation correct?
    glfwTerminate();
        
    return 0;
}


void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.handleKeyboardInput(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.handleKeyboardInput(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.handleKeyboardInput(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.handleKeyboardInput(RIGHT, deltaTime);
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




void takeScreenshot(GLFWwindow *window, const std::string &filename)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    std::vector<unsigned char> pixels(3 * width * height);

    glPixelStorei(GL_PACK_ALIGNMENT, 1); // Ensure tight packing
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Flip vertically (OpenGL's origin is bottom-left, most images are top-left)
    for (int j = 0; j < height / 2; ++j)
    {
        for (int i = 0; i < width * 3; ++i)
        {
            std::swap(pixels[j * width * 3 + i], pixels[(height - 1 - j) * width * 3 + i]);
        }
    }

    stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3);
    std::cout << "Screenshot saved to: " << filename << std::endl;
}