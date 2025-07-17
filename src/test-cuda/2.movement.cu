#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cuda_runtime.h>
// #include <cudagl.h>
#include <cuda_gl_interop.h>

#include <iostream>

#define DEBUG 1

#include "shader.h"
#include "math/vec3.h"
#include "ray-tracer/ray.h"
#include "ray-tracer/triangle.h"
#include "ray-tracer/camera.h"


const int screenHeight = 512;
const int screenWidth = 512;
float aspect = screenWidth / screenHeight;
float invWidth = 1.0f / screenWidth;
float invHeight = 1.0f / screenHeight;
Camera camera(vec3(0, 0, 2), vec3(0, 1, 0), -90.0f, 0.0f, 45.0f, 0.1f, 100.0f, aspect);
float widthMultiplier = invWidth * camera.fullwidth;
float heightMultiplier = invHeight * camera.fullheight;

float deltaTime = 0.0f;
float lastFrame = 0.0f;
float lastX = screenWidth / 2.0f;
float lastY = screenHeight / 2.0f;
bool firstMouse = true;


void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);


const char *vertexShaderPath = "assets/shaders/cuda/vert.vs";
const char *fragmentShaderPath = "assets/shaders/cuda/frag.fs";

void __global__ render(uchar4 *ptr, const int w, const int h, const float wMult, const float hMult, Camera cam, Triangle tri) {
	int pixelx = threadIdx.x + blockIdx.x * blockDim.x;
	int pixely = threadIdx.y + blockIdx.y * blockDim.y;
    if (pixelx >= w || pixely >= h) return;
	int offset = pixelx + pixely * w;

    float u = (float)pixelx * wMult;
    float v = (float)pixely * hMult;

    vec3 rayOrigin = cam.position;
    vec3 rayDest = cam.bottomleft + u * cam.right + v * cam.up;
    Ray ray(rayOrigin, rayDest - rayOrigin);

    vec3 color;

    tri.calculate_hit_by(ray);
    if (ray.info.hit) {
        color.r = 1.0f;
    }

	ptr[offset].x = color.r * 255;
	ptr[offset].y = color.g * 255;
	ptr[offset].z = color.b * 255;
	ptr[offset].w = 255; 
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
    Triangle zaxis(
        vec3(-0.01f, 0.0f, 0.0f), vec3(0.01, 0.0f, 0.0f), vec3(0.0f, 0.02f, 0.5f),
        vec3(0, 1, 0), vec3(0, 1, 0), vec3(0, 1, 0) 
    );
    // X axis (red)
    Triangle xaxis(
        vec3(0.0f, 0.01f, 0.0f), vec3(0.0f, -0.01f, 0.0f), vec3(0.5f, 0.0f, 0.0f),
        vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1)
    );
    
    // Y axis (blue)
    Triangle yaxis(
        vec3(-0.01f, 0.0f, 0.0f), vec3(0.01f, 0.0f, 0.0f), vec3(0.0f, 0.5f, 0.0f),
        vec3(0, 0, 1), vec3(0, 0, 1), vec3(0, 0, 1)
    );

    

    // init VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Pixel buffer object for shared resource with CUDA
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, screenWidth * screenHeight * 4, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    cudaError_t err;
    cudaGraphicsGLRegisterBuffer(&cuda_resource, pbo, cudaGraphicsMapFlagsWriteDiscard); // using this flag because we only need to write, not read the previous stuff
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        std::cerr << "cudaGraphicsGLRegisterBuffer error: " << cudaGetErrorString(err) << std::endl;
    }
    // Texture for fullscreen quad
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, screenWidth, screenHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //! why
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //! why

    shader.use();
    shader.setInt("tex", 0);
    // glUniform1i(glGetUniformLocation(shader, "tex"), 0);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        uchar4* device_pointer;
        size_t size;
        cudaGraphicsMapResources(1, &cuda_resource, NULL);
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "cudaGraphicsMapResources error: " << cudaGetErrorString(err) << std::endl;
        }
        
        cudaGraphicsResourceGetMappedPointer((void**)&device_pointer, &size, cuda_resource);
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "cudaGraphicsResourceGetMappedPointer error: " << cudaGetErrorString(err) << std::endl;
        }
        
        dim3 blocksPerGrid((screenWidth + 15)/16, (screenHeight + 15)/16);
        dim3 threadsPerBlock(16, 16);
        render<<<blocksPerGrid, threadsPerBlock>>>(device_pointer, 
            screenWidth, screenHeight, 
            widthMultiplier, heightMultiplier,
            camera, yaxis);
        
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA kernel error: " << cudaGetErrorString(err) << std::endl;
        }
        cudaDeviceSynchronize();
        cudaGraphicsUnmapResources(1, &cuda_resource, NULL);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);     // Swap double buffer
        glfwPollEvents();            // Handle input events
    }

    shader.del();
    glDeleteBuffers(1, &pbo);
    glDeleteTextures(1, &texture);
    cudaGraphicsUnregisterResource(cuda_resource);
    glfwDestroyWindow(window);
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
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        camera.handleKeyboardInput(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        camera.handleKeyboardInput(DOWN, deltaTime);
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