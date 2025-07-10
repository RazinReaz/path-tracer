#include <stdio.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cuda_runtime.h>
// #include <cudagl.h>
#include <cuda_gl_interop.h>

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define DIM 512

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

const char* vertexShaderSrc = R"(
    #version 330 core
    out vec2 texCoord;
    void main() {
        const vec2 pos[3] = vec2[3](
            vec2(-1.0, -1.0),
            vec2( 3.0, -1.0),
            vec2(-1.0,  3.0)
        );
        gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);
        texCoord = (gl_Position.xy + 1.0) * 0.5;
    }
)";
    
const char* fragmentShaderSrc = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 texCoord;
    uniform sampler2D tex;
    void main() {
        FragColor = texture(tex, texCoord);
    }
)";

void __global__ kernel(uchar4 *ptr) {
	int x = threadIdx.x + blockIdx.x * blockDim.x;
	int y = threadIdx.y + blockIdx.y * blockDim.y;
    if (x >= DIM || y >= DIM) return;
	int offset = x + y * DIM;

	float fx = x/(float)DIM - 0.5f;
	float fy = y/(float)DIM - 0.5f;
	unsigned char green = 128 + 127 * sin(abs(fx * 100) - abs(fy * 100));

	ptr[offset].x = 0;
	ptr[offset].y = green;
	ptr[offset].z = 0;
	ptr[offset].w = 255; 
}

GLuint createShaderProgram() {
    auto compile = [](GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

int main() {
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }
    GLFWwindow *window = glfwCreateWindow(DIM, DIM, "CUDA+OpenGL interop minimal", NULL, NULL);
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
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    
    GLuint vao, vbo;
    GLuint pbo;
    GLuint texture;
    cudaGraphicsResource* cuda_resource;
    
    // shader and program
    GLuint shader = createShaderProgram();

    // init VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Pixel buffer object for shared resource with CUDA
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, DIM * DIM * 4, nullptr, GL_DYNAMIC_DRAW);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, DIM, DIM, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); //! why
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); //! why

    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "tex"), 0);


    while (!glfwWindowShouldClose(window)) {
        processInput(window);

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
        
        dim3 blocksPerGrid((DIM + 15)/16, (DIM + 15)/16);
        dim3 threadsPerBlock(16, 16);
        kernel<<<blocksPerGrid, threadsPerBlock>>>(device_pointer);
        
        err = cudaGetLastError();
        if (err != cudaSuccess) {
            std::cerr << "CUDA kernel error: " << cudaGetErrorString(err) << std::endl;
        }
        cudaDeviceSynchronize();

        cudaGraphicsUnmapResources(1, &cuda_resource, NULL);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, DIM, DIM, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glClear(GL_COLOR_BUFFER_BIT);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);     // Swap double buffer
        glfwPollEvents();            // Handle input events
    }

    glDeleteProgram(shader);;
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
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}