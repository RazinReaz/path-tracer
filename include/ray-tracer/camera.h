#pragma once

#include "math/vec3.h"

#define radians(deg) ((deg) * 3.14159f / 180.0f)

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
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;
    
    float yaw;
    float pitch;
    float fov;

    float moveSpeed;
    float sensitivity;

    void updateCameraVectors();

public:
    Camera(vec3 position, vec3 up, float yaw = -90.0f, float pitch = 0.0f, float fov = 45.0f);
    void handleMouseMovement(float xoffset, float yoffset);
    void handleMouseScroll(float yoffset);
    void handleKeyboardInput(Camera_Movement direction, float deltaTime);
    float getFOV() const { return fov; }
    vec3 getPosition() const { return position; }
};



Camera::Camera(vec3 pos, vec3 up, float yaw, float pitch, float fov)
    : position(pos), up(up), yaw(yaw), pitch(pitch), fov(fov)
{
    worldUp = up;
    moveSpeed = 2.5f;
    sensitivity = 0.05f;
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    vec3 direction;
    direction.x = ::cosf(radians(yaw)) * ::cosf(radians(pitch));
    direction.y = ::sinf(radians(pitch));
    direction.z = ::sinf(radians(yaw)) * ::cosf(radians(pitch));
    front = direction.normalize();
    right = (front.cross(worldUp)).normalize();
    up = (right.cross(front)).normalize();
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


