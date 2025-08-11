//
// Camera.hpp
//
#pragma once
#include <simd/simd.h>
#include "MathUtilities.hpp"

class Camera {
public:
    // Constructor
    Camera(vector_float3 position = {0.0f, 0.0f, 5.0f},
           vector_float3 target = {0.0f, 0.0f, 0.0f},
           vector_float3 up = {0.0f, 1.0f, 0.0f});

    // Matrix getters
    matrix_float4x4 getViewMatrix() const;
    matrix_float4x4 getProjectionMatrix(float aspect, float fovDegrees = 60.0f,
                                      float nearPlane = 0.01f, float farPlane = 100.0f) const;
    matrix_float4x4 getViewProjectionMatrix(float aspect, float fovDegrees = 60.0f,
                                          float nearPlane = 0.01f, float farPlane = 100.0f) const;

    // Movement methods
    void processMovement(float deltaTime, bool moveForward, bool moveBackward,
                        bool moveLeft, bool moveRight, bool moveUp = false, bool moveDown = false);
    void processMouseMovement(float deltaX, float deltaY, float sensitivity = 0.001f);

    // Position and orientation control
    void setPosition(vector_float3 position);
    void setTarget(vector_float3 target);
    void setUpVector(vector_float3 up);
    void lookAt(vector_float3 position, vector_float3 target, vector_float3 up);

    // Getters
    vector_float3 getPosition() const { return position_; }
    vector_float3 getTarget() const { return target_; }
    vector_float3 getForward() const { return forward_; }
    vector_float3 getRight() const { return right_; }
    vector_float3 getUp() const { return up_; }

    // Configuration
    void setMovementSpeed(float speed) { movementSpeed_ = speed; }
    float getMovementSpeed() const { return movementSpeed_; }

private:
    // Camera vectors
    vector_float3 position_;
    vector_float3 target_;
    vector_float3 forward_;
    vector_float3 right_;
    vector_float3 up_;
    vector_float3 worldUp_;

    // Camera settings
    float movementSpeed_;
    float yaw_;   // Y-axis rotation
    float pitch_; // X-axis rotation

    // Helper methods
    void updateCameraVectors();
    vector_float3 normalize(vector_float3 v) const;
    vector_float3 cross(vector_float3 a, vector_float3 b) const;
    float dot(vector_float3 a, vector_float3 b) const;
};