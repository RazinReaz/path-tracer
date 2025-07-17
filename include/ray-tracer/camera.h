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
    __host__ __device__ void updateCameraVectors();
public:
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;

    
    float yaw;
    float pitch;
    float fov;
    float aspect;

    float near;
    float far;
    vec3 bottomleft;
    float halfheight, fullheight;
    float halfwidth, fullwidth;

    float moveSpeed;
    float sensitivity;
    __host__ __device__ Camera(vec3 position, vec3 up, float yaw = -90.0f, float pitch = 0.0f, float fov = 45.0f, float near = 0.1f, float far = 100.0f, float aspect = 1.0f);
    __host__ __device__ void handleMouseMovement(float xoffset, float yoffset);
    // __host__ __device__ void handleMouseScroll(float yoffset);
    __host__ __device__ void handleKeyboardInput(Camera_Movement direction, float deltaTime);
    __host__ __device__ float getFOV() const { return fov; }
};


__host__ __device__ 
Camera::Camera(vec3 pos, vec3 up, float yaw, float pitch, float fov, float near, float far, float aspect)
    : position(pos), up(up), yaw(yaw), pitch(pitch), fov(fov), near(near), far(far), aspect(aspect)
{
    worldUp = up;
    moveSpeed = 0.5f;
    sensitivity = 0.05f;
    halfheight = tanf(radians(fov) / 2.0f) * near;
    halfwidth = halfheight * aspect;
    fullheight = halfheight * 2.0f;
    fullwidth = halfwidth * 2.0f;
    
    updateCameraVectors();
}

__host__ __device__
void Camera::updateCameraVectors()
{
    vec3 direction;
    direction.x = ::cosf(radians(yaw)) * ::cosf(radians(pitch));
    direction.y = ::sinf(radians(pitch));
    direction.z = ::sinf(radians(yaw)) * ::cosf(radians(pitch));
    front = direction.normalize();
    right = (front.cross(worldUp)).normalize();
    up = (right.cross(front)).normalize();
    bottomleft = position + front * near - up * halfheight - right * halfwidth;
}


// void Camera::handleMouseScroll(float yoffset)
// {
//     if (fov >= 1.0f && fov <= 45.0f)
//         fov -= yoffset;
//     if (fov <= 1.0f)
//         fov = 1.0f;
//     if (fov >= 45.0f)
//         fov = 45.0f;
// }

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
    if (direction == UP)
        position += cameraSpeed * up;
    if (direction == DOWN)
        position -= cameraSpeed * up;
    updateCameraVectors();
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


