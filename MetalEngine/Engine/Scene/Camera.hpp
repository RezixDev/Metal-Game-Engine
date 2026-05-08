// Engine/Scene/Camera.hpp
#pragma once

#include "../Math/Math.hpp"

namespace Engine {

// Minimal FPS-style perspective camera. State is just position, yaw, and pitch.
class Camera {
public:
    Camera(Math::Vec3 position = {0.0f, 0.0f, 5.0f},
           Math::Vec3 target   = {0.0f, 0.0f, 0.0f},
           Math::Vec3 up       = Math::Vector::Constants::UP) {
        lookAt(position, target, up);
    }

    // ---- View / projection ----------------------------------------------------

    Math::Mat4 getViewMatrix() const {
        return Math::Matrix::lookAt(position_, position_ + getForward(), worldUp_);
    }

    Math::Mat4 getProjectionMatrix(float aspect) const {
        return Math::Matrix::perspectiveDegrees(fov_, aspect, nearPlane_, farPlane_);
    }

    // ---- Setup ----------------------------------------------------------------

    void lookAt(const Math::Vec3& position,
                const Math::Vec3& target,
                const Math::Vec3& up) {
        position_ = position;
        worldUp_  = Math::Vector::normalize(up);

        Math::Vec3 forward = Math::Vector::normalize(target - position);
        yaw_   = Math::radiansToDegrees(atan2f(forward.z, forward.x));
        pitch_ = Math::radiansToDegrees(asinf(forward.y));
    }

    void setFieldOfView(float fov)        { fov_ = Math::clamp(fov, 1.0f, 179.0f); }
    void setNearPlane(float n)            { nearPlane_ = n; }
    void setFarPlane(float f)             { farPlane_ = f; }
    void setMovementSpeed(float s)        { movementSpeed_ = s; }
    void setMouseSensitivity(float s)     { mouseSensitivity_ = s; }

    // ---- State queries --------------------------------------------------------

    Math::Vec3 getPosition() const { return position_; }

    Math::Vec3 getForward() const {
        const float yawR   = Math::degreesToRadians(yaw_);
        const float pitchR = Math::degreesToRadians(pitch_);
        return Math::Vector::normalize(Math::Vec3{
            cosf(yawR) * cosf(pitchR),
            sinf(pitchR),
            sinf(yawR) * cosf(pitchR),
        });
    }

    Math::Vec3 getRight() const {
        return Math::Vector::normalize(Math::Vector::cross(getForward(), worldUp_));
    }

    // ---- Input ----------------------------------------------------------------

    void processMovement(float deltaTime,
                         bool forward, bool backward,
                         bool left,    bool right,
                         bool up,      bool down) {
        const float v = movementSpeed_ * deltaTime;
        const Math::Vec3 fwd   = getForward();
        const Math::Vec3 rgt   = getRight();
        Math::Vec3 movement = Math::Vector::Constants::ZERO;
        if (forward)  movement += fwd * v;
        if (backward) movement -= fwd * v;
        if (right)    movement += rgt * v;
        if (left)     movement -= rgt * v;
        if (up)       movement += worldUp_ * v;
        if (down)     movement -= worldUp_ * v;
        position_ += movement;
    }

    void processMouseMovement(float deltaX, float deltaY) {
        yaw_   += deltaX * mouseSensitivity_ * Math::Constants::RAD_TO_DEG;
        pitch_ += deltaY * mouseSensitivity_ * Math::Constants::RAD_TO_DEG;
        pitch_  = Math::clamp(pitch_, -89.0f, 89.0f);
    }

private:
    Math::Vec3 position_{0.0f, 0.0f, 5.0f};
    Math::Vec3 worldUp_ = Math::Vector::Constants::UP;
    float yaw_   = -90.0f; // facing -Z by default
    float pitch_ = 0.0f;

    float fov_              = 60.0f;
    float nearPlane_        = 0.1f;
    float farPlane_         = 100.0f;
    float movementSpeed_    = 5.0f;
    float mouseSensitivity_ = 0.002f;
};

} // namespace Engine
