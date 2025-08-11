//
// Fixed Camera.cpp
//
#include "Camera.hpp"
#include <cmath>

Camera::Camera(vector_float3 position, vector_float3 target, vector_float3 up)
    : position_(position)
    , target_(target)
    , worldUp_(up)
    , movementSpeed_(2.5f)
    , yaw_(-90.0f)
    , pitch_(0.0f)
{
    // Calculate initial camera vectors from position and target
    vector_float3 direction = {
        target_.x - position_.x,
        target_.y - position_.y,
        target_.z - position_.z
    };
    
    // Normalize the direction to get forward vector
    forward_ = normalize(direction);
    
    // Calculate right and up vectors
    right_ = normalize(cross(forward_, worldUp_));
    up_ = normalize(cross(right_, forward_));
    
    // Calculate yaw and pitch from the forward vector for consistency
    // This ensures yaw/pitch match the initial look direction
    yaw_ = Math::radiansToDegrees(atan2f(forward_.z, forward_.x));
    pitch_ = Math::radiansToDegrees(asinf(forward_.y));
    

}

matrix_float4x4 Camera::getViewMatrix() const {
    vector_float3 f = normalize(forward_);
    vector_float3 r = normalize(right_);
    vector_float3 u = normalize(up_);
    
    matrix_float4x4 view;
    view.columns[0] = { r.x, u.x, -f.x, 0.0f };
    view.columns[1] = { r.y, u.y, -f.y, 0.0f };
    view.columns[2] = { r.z, u.z, -f.z, 0.0f };
    view.columns[3] = { -dot(r, position_), -dot(u, position_), dot(f, position_), 1.0f };
    
    return view;
}


matrix_float4x4 Camera::getProjectionMatrix(float aspect, float fovDegrees,
                                          float nearPlane, float farPlane) const {
    return Math::Matrix4::perspective(Math::degreesToRadians(fovDegrees),
                                    aspect, nearPlane, farPlane);
}

matrix_float4x4 Camera::getViewProjectionMatrix(float aspect, float fovDegrees,
                                              float nearPlane, float farPlane) const {
    matrix_float4x4 projection = getProjectionMatrix(aspect, fovDegrees, nearPlane, farPlane);
    matrix_float4x4 view = getViewMatrix();
    return Math::Matrix4::multiply(projection, view);
}

void Camera::processMovement(float deltaTime, bool moveForward, bool moveBackward,
                           bool moveLeft, bool moveRight, bool moveUp, bool moveDown) {
    float velocity = movementSpeed_ * deltaTime;
    
    // Move based on camera's current orientation
    if (moveForward) {
        position_.x += forward_.x * velocity;
        position_.y += forward_.y * velocity;
        position_.z += forward_.z * velocity;
    }
    if (moveBackward) {
        position_.x -= forward_.x * velocity;
        position_.y -= forward_.y * velocity;
        position_.z -= forward_.z * velocity;
    }
    if (moveLeft) {
        position_.x -= right_.x * velocity;
        position_.y -= right_.y * velocity;
        position_.z -= right_.z * velocity;
    }
    if (moveRight) {
        position_.x += right_.x * velocity;
        position_.y += right_.y * velocity;
        position_.z += right_.z * velocity;
    }
    if (moveUp) {
        position_.x += worldUp_.x * velocity;
        position_.y += worldUp_.y * velocity;
        position_.z += worldUp_.z * velocity;
    }
    if (moveDown) {
        position_.x -= worldUp_.x * velocity;
        position_.y -= worldUp_.y * velocity;
        position_.z -= worldUp_.z * velocity;
    }
}

void Camera::processMouseMovement(float deltaX, float deltaY, float sensitivity) {
    yaw_ += deltaX * sensitivity * 180.0f / M_PI;
    pitch_ -= deltaY * sensitivity * 180.0f / M_PI;
    
    // Constrain pitch to prevent screen flipping
    if (pitch_ > 89.0f) pitch_ = 89.0f;
    if (pitch_ < -89.0f) pitch_ = -89.0f;
    
    updateCameraVectors();
}

void Camera::setPosition(vector_float3 position) {
    position_ = position;
}

void Camera::setTarget(vector_float3 target) {
    target_ = target;
    
    // Recalculate forward vector based on new target
    vector_float3 direction = {
        target_.x - position_.x,
        target_.y - position_.y,
        target_.z - position_.z
    };
    forward_ = normalize(direction);
    
    // Recalculate right and up vectors
    right_ = normalize(cross(forward_, worldUp_));
    up_ = normalize(cross(right_, forward_));
    
    // Update yaw and pitch to match the new direction
    yaw_ = Math::radiansToDegrees(atan2f(forward_.z, forward_.x));
    pitch_ = Math::radiansToDegrees(asinf(forward_.y));
}

void Camera::setUpVector(vector_float3 up) {
    worldUp_ = up;
    // Recalculate right and up vectors with new world up
    right_ = normalize(cross(forward_, worldUp_));
    up_ = normalize(cross(right_, forward_));
}

void Camera::lookAt(vector_float3 position, vector_float3 target, vector_float3 up) {
    position_ = position;
    target_ = target;
    worldUp_ = up;
    
    // Calculate direction from position to target
    vector_float3 direction = {
        target_.x - position_.x,
        target_.y - position_.y,
        target_.z - position_.z
    };
    forward_ = normalize(direction);
    
    // Calculate right and up vectors
    right_ = normalize(cross(forward_, worldUp_));
    up_ = normalize(cross(right_, forward_));
    
    // Update yaw and pitch to match the new direction
    yaw_ = Math::radiansToDegrees(atan2f(forward_.z, forward_.x));
    pitch_ = Math::radiansToDegrees(asinf(forward_.y));
}

void Camera::updateCameraVectors() {
    // Calculate forward vector from yaw and pitch
    vector_float3 front;
    front.x = cosf(Math::degreesToRadians(yaw_)) * cosf(Math::degreesToRadians(pitch_));
    front.y = sinf(Math::degreesToRadians(pitch_));
    front.z = sinf(Math::degreesToRadians(yaw_)) * cosf(Math::degreesToRadians(pitch_));
    forward_ = normalize(front);
    
    // Calculate right and up vectors
    right_ = normalize(cross(forward_, worldUp_));
    up_ = normalize(cross(right_, forward_));
}

vector_float3 Camera::normalize(vector_float3 v) const {
    float length = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0f) {
        return { v.x / length, v.y / length, v.z / length };
    }
    return { 0.0f, 0.0f, 0.0f };
}

vector_float3 Camera::cross(vector_float3 a, vector_float3 b) const {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

float Camera::dot(vector_float3 a, vector_float3 b) const {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
