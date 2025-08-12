// Engine/Camera.hpp
// Improved camera system using Core utilities
#pragma once
#include "../Core/Math.hpp"
#include "../Core/Transform.hpp"

namespace Engine {

    using namespace Core;

    enum class CameraType {
        Perspective,
        Orthographic
    };

    class Camera {
    public:
        // Constructor maintains backward compatibility
        Camera(Vec3 position = Vec3{0, 0, 5},
               Vec3 target = Vec3{0, 0, 0},
               Vec3 up = Vec3{0, 1, 0})
            : transform_()
            , worldUp_(up)
            , fov_(60.0f)
            , nearPlane_(0.01f)
            , farPlane_(100.0f)
            , cameraType_(CameraType::Perspective)
            , movementSpeed_(2.5f)
            , mouseSensitivity_(0.001f)
            , yaw_(-90.0f)
            , pitch_(0.0f)
        {
            lookAt(position, target, up);
        }

        // Matrix getters (backward compatible)
        Mat4 getViewMatrix() const {
            Vec3 pos = transform_.getPosition();
            Vec3 forward = getForward();
            Vec3 right = getRight();
            Vec3 up = getUp();

            Mat4 view;
            view.columns[0] = Vec4{right.x, up.x, -forward.x, 0};
            view.columns[1] = Vec4{right.y, up.y, -forward.y, 0};
            view.columns[2] = Vec4{right.z, up.z, -forward.z, 0};
            view.columns[3] = Vec4{
                -Vector::dot(right, pos),
                -Vector::dot(up, pos),
                Vector::dot(forward, pos),
                1
            };

            return view;
        }

        Mat4 getProjectionMatrix(float aspect, float fovDegrees = -1.0f,
                                float nearPlane = -1.0f, float farPlane = -1.0f) const {
            float fov = fovDegrees > 0 ? fovDegrees : fov_;
            float near = nearPlane > 0 ? nearPlane : nearPlane_;
            float far = farPlane > 0 ? farPlane : farPlane_;

            if (cameraType_ == CameraType::Perspective) {
                return Matrix::perspective(degreesToRadians(fov), aspect, near, far);
            } else {
                float height = orthoSize_;
                float width = height * aspect;
                return Matrix::orthographic(-width/2, width/2, -height/2, height/2, near, far);
            }
        }

        Mat4 getViewProjectionMatrix(float aspect, float fovDegrees = -1.0f,
                                    float nearPlane = -1.0f, float farPlane = -1.0f) const {
            return Matrix::multiply(
                getProjectionMatrix(aspect, fovDegrees, nearPlane, farPlane),
                getViewMatrix()
            );
        }

        // Movement (backward compatible)
        void processMovement(float deltaTime, bool moveForward, bool moveBackward,
                           bool moveLeft, bool moveRight, bool moveUp = false, bool moveDown = false) {
            float velocity = movementSpeed_ * deltaTime;

            Vec3 forward = getForward();
            Vec3 right = getRight();
            Vec3 movement = Vec3{0, 0, 0};

            if (moveForward) movement += forward * velocity;
            if (moveBackward) movement -= forward * velocity;
            if (moveLeft) movement -= right * velocity;
            if (moveRight) movement += right * velocity;
            if (moveUp) movement += worldUp_ * velocity;
            if (moveDown) movement -= worldUp_ * velocity;

            transform_.translate(movement);
        }

        void processMouseMovement(float deltaX, float deltaY, float sensitivity = -1.0f) {
            float sens = sensitivity > 0 ? sensitivity : mouseSensitivity_;

            yaw_ += deltaX * sens * Constants::RAD_TO_DEG;
            pitch_ -= deltaY * sens * Constants::RAD_TO_DEG;

            // Constrain pitch
            pitch_ = Math::clamp(pitch_, -89.0f, 89.0f);

            updateCameraVectors();
        }

        // Position and orientation (backward compatible)
        void setPosition(Vec3 position) {
            transform_.setPosition(position);
        }

        void setTarget(Vec3 target) {
            lookAt(transform_.getPosition(), target, worldUp_);
        }

        void lookAt(Vec3 position, Vec3 target, Vec3 up) {
            transform_.setPosition(position);
            worldUp_ = Vector::normalize(up);

            Vec3 direction = target - position;
            forward_ = Vector::normalize(direction);
            right_ = Vector::normalize(Vector::cross(forward_, worldUp_));
            up_ = Vector::normalize(Vector::cross(right_, forward_));

            // Update yaw and pitch from forward vector
            yaw_ = radiansToDegrees(atan2f(forward_.z, forward_.x));
            pitch_ = radiansToDegrees(asinf(forward_.y));
        }

        // Getters (backward compatible)
        Vec3 getPosition() const { return transform_.getPosition(); }
        Vec3 getTarget() const { return transform_.getPosition() + forward_; }
        Vec3 getForward() const { return forward_; }
        Vec3 getRight() const { return right_; }
        Vec3 getUp() const { return up_; }

        // Settings
        void setMovementSpeed(float speed) { movementSpeed_ = speed; }
        float getMovementSpeed() const { return movementSpeed_; }

        void setMouseSensitivity(float sensitivity) { mouseSensitivity_ = sensitivity; }
        float getMouseSensitivity() const { return mouseSensitivity_; }

        void setFieldOfView(float fov) { fov_ = fov; }
        float getFieldOfView() const { return fov_; }

        void setNearPlane(float near) { nearPlane_ = near; }
        float getNearPlane() const { return nearPlane_; }

        void setFarPlane(float far) { farPlane_ = far; }
        float getFarPlane() const { return farPlane_; }

        void setCameraType(CameraType type) { cameraType_ = type; }
        CameraType getCameraType() const { return cameraType_; }

        void setOrthographicSize(float size) { orthoSize_ = size; }
        float getOrthographicSize() const { return orthoSize_; }

        // New features
        void setRotation(float pitch, float yaw, float roll = 0.0f) {
            pitch_ = pitch;
            yaw_ = yaw;
            updateCameraVectors();
        }

        void zoom(float delta) {
            if (cameraType_ == CameraType::Perspective) {
                fov_ = Math::clamp(fov_ - delta, 1.0f, 120.0f);
            } else {
                orthoSize_ = Math::clamp(orthoSize_ - delta * 0.1f, 0.1f, 100.0f);
            }
        }

    private:
        void updateCameraVectors() {
            // Calculate forward from yaw and pitch
            Vec3 front;
            front.x = cosf(degreesToRadians(yaw_)) * cosf(degreesToRadians(pitch_));
            front.y = sinf(degreesToRadians(pitch_));
            front.z = sinf(degreesToRadians(yaw_)) * cosf(degreesToRadians(pitch_));

            forward_ = Vector::normalize(front);
            right_ = Vector::normalize(Vector::cross(forward_, worldUp_));
            up_ = Vector::normalize(Vector::cross(right_, forward_));
        }

        Transform transform_;

        // Camera vectors
        Vec3 forward_;
        Vec3 right_;
        Vec3 up_;
        Vec3 worldUp_;

        // Camera settings
        float fov_;
        float nearPlane_;
        float farPlane_;
        float orthoSize_ = 10.0f;
        CameraType cameraType_;

        // Movement settings
        float movementSpeed_;
        float mouseSensitivity_;

        // Orientation
        float yaw_;
        float pitch_;
    };

}