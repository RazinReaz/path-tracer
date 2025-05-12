#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "Shader.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
const char *vertexShaderPath = "assets/shaders/vert.vs";
const char *fragmentShaderPath = "assets/shaders/frag.fs";

int main()
{
    /* Initialize the library */
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello World", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Load OpenGL functions via GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    
    Shader shaderProgram(vertexShaderPath, fragmentShaderPath);

    // rectangle vertices
    float vertices[] = {
        0.4f, 0.5f, 0.0f,  
        0.5f, -0.5f, 0.0f, 
        -0.5f, -0.5f, 0.0f, 
        -0.5f, 0.5f, 0.0f   
    };
    unsigned int indices[] = {
        // note that we start from 0!
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    VAO vao;
    vao.bind(); 

    VBO vbo(vertices, sizeof(vertices));
    EBO ebo(indices, sizeof(indices));

    vao.linkVBO(vbo, 0); // link VBO to VAO, 0 is the layout location in the shader

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    vbo.unbind(); // Unbind VBO, VAO is still bound

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    vao.unbind(); // Unbind VAO

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {

        // Process input
        processInput(window);
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram.use();
        vao.bind(); // Bind the VAO
        // No need to bind EBO as it's already bound to VAO
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    vao.del(); // Delete the VAO
    vbo.del(); // Delete the VBO
    ebo.del(); // Delete the EBO
    shaderProgram.del(); // Delete the shader program
    glfwTerminate();
    return 0;
}

// Callback to resize viewport when window size changes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}