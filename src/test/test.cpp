#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);

const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;
// Vertex shader source code
const char *vertex_shader_source = "#version 420 core\n"
                                   "layout (location = 0) in vec3 aPos;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                   "}\0";
// Fragment shader source code
const char *blue_fragment_shader = "#version 420 core\n"
                                   "out vec4 FragColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);\n"
                                   "}\0";

const char *changing_fragment_shader = "#version 420 core\n"
                                     "out vec4 FragColor;\n"
                                     "uniform vec4 changingColor;\n"
                                     "void main()\n"
                                     "{\n"
                                     "   FragColor = changingColor;\n"
                                     "}\0";
int main(void)
{
    /* Initialize the library */
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "2 triangles 2 colors", NULL, NULL);
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

    // configuring the vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertexShader);

    // configuring the blue fragment shader
    unsigned int blueFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(blueFragmentShader, 1, &blue_fragment_shader, NULL);
    glCompileShader(blueFragmentShader);


    unsigned int changingFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(changingFragmentShader, 1, &changing_fragment_shader, NULL);
    glCompileShader(changingFragmentShader);

    // creating the shader program
    unsigned int blueShaderProgram = glCreateProgram();
    glAttachShader(blueShaderProgram, vertexShader);
    glAttachShader(blueShaderProgram, blueFragmentShader);
    glLinkProgram(blueShaderProgram);

    unsigned int changingShaderProgram = glCreateProgram();
    glAttachShader(changingShaderProgram, vertexShader);
    glAttachShader(changingShaderProgram, changingFragmentShader);
    glLinkProgram(changingShaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(blueFragmentShader);
    glDeleteShader(changingFragmentShader);



    // rectangle vertices
    float triangle_1[] = {
        -0.5f, -0.5f, 0.0f,
        -0.2f, -0.5f, 0.0f,
        -0.2f,  0.5f, 0.0f 
    };
    float triangle_2[] = {
        0.5f, -0.5f, 0.0f, 
        0.2f, -0.5f, 0.0f,
        0.5f,  0.5f, 0.0f,
    };


    unsigned int VAOs[2], VBOs[2];
    glGenVertexArrays(2, VAOs);
    glGenBuffers(2, VBOs);

    glBindVertexArray(VAOs[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_1), triangle_1, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0); 
    glEnableVertexAttribArray(0);
    
    glBindVertexArray(VAOs[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle_2), triangle_2, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0); 
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(blueShaderProgram);
        glBindVertexArray(VAOs[0]);
        glDrawArrays(GL_TRIANGLES, 0, 3); // Draw the first triangle


        float time = glfwGetTime();
        float redValue = (sin(time) / 2.0f) + 0.5f; // Calculate red value based on time
        float greenValue = (sin(time + 1.0f) / 2.0f) + 0.5f; // Calculate green value based on time
        float blueValue = (sin(time + 2.0f) / 2.0f) + 0.5f; // Calculate blue value based on time

        // Set the uniform color in the shader program
        int colorLocation = glGetUniformLocation(changingShaderProgram, "changingColor");

        glUseProgram(changingShaderProgram);
        glUniform4f(colorLocation, redValue, greenValue, blueValue, 1.0f); // Set the color uniform
        glBindVertexArray(VAOs[1]);
        glDrawArrays(GL_TRIANGLES, 0, 3); // Draw the second triangle


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(2, VAOs);
    glDeleteBuffers(2, VBOs);
    glDeleteProgram(blueShaderProgram);
    glDeleteProgram(changingShaderProgram);
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