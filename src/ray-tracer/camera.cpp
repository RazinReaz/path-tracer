#include "camera.h"
#include <iostream>

Camera::Camera(glm::vec3 pos, glm::vec3 up, float yaw, float pitch, float fov)
    : position(pos), up(up), yaw(yaw), pitch(pitch), fov(fov)
{
    worldUp = up;
    moveSpeed = 2.5f;
    sensitivity = 0.002f;
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::mat4
Camera::getViewMatrix()
{
    return glm::lookAt(position, position + front, up);
}

void Camera::handleMouseScroll(float yoffset)
{
    if (fov >= 1.0f && fov <= 45.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 45.0f)
        fov = 45.0f;
}

void Camera::handleKeyboardInput(Camera_Movement direction, float deltaTime)
{
    float cameraSpeed = moveSpeed * deltaTime;
    if (direction == FORWARD)
        position += cameraSpeed * front;
    if (direction == BACKWARD)
        position -= cameraSpeed * front;
    if (direction == LEFT)
        position -= cameraSpeed * right;
    if (direction == RIGHT)
        position += cameraSpeed * right;
}

void Camera::handleMouseMovement(float xoffset, float yoffset)
{
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraVectors();
}
