#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera
{
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    float yaw;
    float pitch;
    float fov;

    float moveSpeed;
    float sensitivity;

    void updateCameraVectors();

public:
    Camera(glm::vec3 position, glm::vec3 up, float yaw = -90.0f, float pitch = 0.0f, float fov = 45.0f);
    glm::mat4 getViewMatrix();
    void handleMouseMovement(float xoffset, float yoffset);
    void handleMouseScroll(float yoffset);
    void handleKeyboardInput(Camera_Movement direction, float deltaTime);
    float getFOV() const { return fov; }
};

