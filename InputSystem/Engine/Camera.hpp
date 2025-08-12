// Camera.hpp
// Enhanced Camera system with better architecture and features
#pragma once

#include "Math.hpp"
#include "Transform.hpp"

namespace Engine {

    enum class CameraType {
        Perspective,
        Orthographic
    };

    enum class CameraMovementType {
        FPS,        // First-person style (move relative to camera orientation)
        Orbital,    // Orbit around a target point
        Free        // Free movement in world space
    };

    class Camera {
    public:
        // ========================================
        // Constructors
        // ========================================

        Camera(Math::Vec3 position = Math::Vector::make(0, 0, 5),
               Math::Vec3 target = Math::Vector::Constants::ZERO,
               Math::Vec3 up = Math::Vector::Constants::UP)
            : transform_()
            , worldUp_(up)
            , target_(target)
            , cameraType_(CameraType::Perspective)
            , movementType_(CameraMovementType::FPS)
            , fov_(60.0f)
            , nearPlane_(0.01f)
            , farPlane_(1000.0f)
            , orthoSize_(10.0f)
            , movementSpeed_(5.0f)
            , mouseSensitivity_(0.002f)
            , zoomSpeed_(2.0f)
            , yaw_(-90.0f)
            , pitch_(0.0f)
            , orbitDistance_(5.0f)
            , targetOffset_(Math::Vector::Constants::ZERO)
        {
            lookAt(position, target, up);
        }

        // ========================================
        // View Matrix Generation
        // ========================================

        Math::Mat4 getViewMatrix() const {
            if (movementType_ == CameraMovementType::Orbital) {
                return Math::Matrix::lookAt(getOrbitPosition(), target_ + targetOffset_, worldUp_);
            } else {
                Math::Vec3 pos = transform_.getPosition();
                Math::Vec3 forward = transform_.getForward();
                return Math::Matrix::lookAt(pos, pos + forward, worldUp_);
            }
        }

        // ========================================
        // Projection Matrix Generation
        // ========================================

        Math::Mat4 getProjectionMatrix(float aspect,
                                     float customFov = -1.0f,
                                     float customNear = -1.0f,
                                     float customFar = -1.0f) const {
            float fov = customFov > 0 ? customFov : fov_;
            float near = customNear > 0 ? customNear : nearPlane_;
            float far = customFar > 0 ? customFar : farPlane_;

            if (cameraType_ == CameraType::Perspective) {
                return Math::Matrix::perspectiveDegrees(fov, aspect, near, far);
            } else {
                float height = orthoSize_;
                float width = height * aspect;
                return Math::Matrix::orthographic(-width/2, width/2, -height/2, height/2, near, far);
            }
        }

        Math::Mat4 getViewProjectionMatrix(float aspect,
                                         float customFov = -1.0f,
                                         float customNear = -1.0f,
                                         float customFar = -1.0f) const {
            return Math::Matrix::multiply(
                getProjectionMatrix(aspect, customFov, customNear, customFar),
                getViewMatrix()
            );
        }

        // ========================================
        // Movement System
        // ========================================

        void processMovement(float deltaTime,
                           bool moveForward, bool moveBackward,
                           bool moveLeft, bool moveRight,
                           bool moveUp = false, bool moveDown = false) {

            float velocity = movementSpeed_ * deltaTime;

            switch (movementType_) {
                case CameraMovementType::FPS:
                    processFPSMovement(velocity, moveForward, moveBackward, moveLeft, moveRight, moveUp, moveDown);
                    break;

                case CameraMovementType::Orbital:
                    processOrbitalMovement(velocity, moveForward, moveBackward, moveLeft, moveRight, moveUp, moveDown);
                    break;

                case CameraMovementType::Free:
                    processFreeMovement(velocity, moveForward, moveBackward, moveLeft, moveRight, moveUp, moveDown);
                    break;
            }
        }

        void processMouseMovement(float deltaX, float deltaY, float customSensitivity = -1.0f) {
            float sensitivity = customSensitivity > 0 ? customSensitivity : mouseSensitivity_;

            switch (movementType_) {
                case CameraMovementType::FPS:
                case CameraMovementType::Free:
                    processFPSMouseMovement(deltaX, deltaY, sensitivity);
                    break;

                case CameraMovementType::Orbital:
                    processOrbitalMouseMovement(deltaX, deltaY, sensitivity);
                    break;
            }
        }

        void processZoom(float delta) {
            if (cameraType_ == CameraType::Perspective) {
                if (movementType_ == CameraMovementType::Orbital) {
                    orbitDistance_ = Math::clamp(orbitDistance_ - delta * zoomSpeed_, 0.5f, 100.0f);
                    updateOrbitPosition();
                } else {
                    fov_ = Math::clamp(fov_ - delta * zoomSpeed_, 1.0f, 120.0f);
                }
            } else {
                orthoSize_ = Math::clamp(orthoSize_ - delta * zoomSpeed_ * 0.1f, 0.1f, 100.0f);
            }
        }

        // ========================================
        // Position and Orientation
        // ========================================

        void setPosition(const Math::Vec3& position) {
            if (movementType_ == CameraMovementType::Orbital) {
                // For orbital camera, set the target and maintain distance
                setTarget(position);
            } else {
                transform_.setPosition(position);
            }
        }

        Math::Vec3 getPosition() const {
            if (movementType_ == CameraMovementType::Orbital) {
                return getOrbitPosition();
            } else {
                return transform_.getPosition();
            }
        }

        void setTarget(const Math::Vec3& target) {
            target_ = target;
            if (movementType_ == CameraMovementType::Orbital) {
                updateOrbitPosition();
            } else {
                lookAt(transform_.getPosition(), target, worldUp_);
            }
        }

        Math::Vec3 getTarget() const {
            if (movementType_ == CameraMovementType::Orbital) {
                return target_ + targetOffset_;
            } else {
                return transform_.getPosition() + transform_.getForward();
            }
        }

        void lookAt(const Math::Vec3& position, const Math::Vec3& target, const Math::Vec3& up) {
            transform_.setPosition(position);
            transform_.lookAt(target, up);
            worldUp_ = Math::Vector::normalize(up);
            target_ = target;

            // Update yaw and pitch from transform
            Math::Vec3 forward = transform_.getForward();
            yaw_ = Math::radiansToDegrees(atan2f(forward.z, forward.x));
            pitch_ = Math::radiansToDegrees(asinf(-forward.y));

            if (movementType_ == CameraMovementType::Orbital) {
                orbitDistance_ = Math::Vector::distance(position, target);
                updateOrbitPosition();
            }
        }

        // ========================================
        // Direction Vectors
        // ========================================

        Math::Vec3 getForward() const {
            if (movementType_ == CameraMovementType::Orbital) {
                return Math::Vector::normalize(target_ + targetOffset_ - getOrbitPosition());
            } else {
                return transform_.getForward();
            }
        }

        Math::Vec3 getRight() const {
            return transform_.getRight();
        }

        Math::Vec3 getUp() const {
            return transform_.getUp();
        }

        // ========================================
        // Camera Settings
        // ========================================

        void setCameraType(CameraType type) { cameraType_ = type; }
        CameraType getCameraType() const { return cameraType_; }

        void setMovementType(CameraMovementType type) {
            movementType_ = type;
            if (type == CameraMovementType::Orbital) {
                updateOrbitPosition();
            }
        }
        CameraMovementType getMovementType() const { return movementType_; }

        void setFieldOfView(float fov) { fov_ = Math::clamp(fov, 1.0f, 179.0f); }
        float getFieldOfView() const { return fov_; }

        void setNearPlane(float near) { nearPlane_ = Math::clamp(near, 0.001f, farPlane_ - 0.001f); }
        float getNearPlane() const { return nearPlane_; }

        void setFarPlane(float far) { farPlane_ = Math::clamp(far, nearPlane_ + 0.001f, 10000.0f); }
        float getFarPlane() const { return farPlane_; }

        void setOrthographicSize(float size) { orthoSize_ = Math::clamp(size, 0.1f, 1000.0f); }
        float getOrthographicSize() const { return orthoSize_; }

        void setMovementSpeed(float speed) { movementSpeed_ = Math::clamp(speed, 0.1f, 100.0f); }
        float getMovementSpeed() const { return movementSpeed_; }

        void setMouseSensitivity(float sensitivity) { mouseSensitivity_ = Math::clamp(sensitivity, 0.0001f, 0.1f); }
        float getMouseSensitivity() const { return mouseSensitivity_; }

        void setZoomSpeed(float speed) { zoomSpeed_ = Math::clamp(speed, 0.1f, 10.0f); }
        float getZoomSpeed() const { return zoomSpeed_; }

        // ========================================
        // Orbital Camera Settings
        // ========================================

        void setOrbitDistance(float distance) {
            orbitDistance_ = Math::clamp(distance, 0.1f, 1000.0f);
            if (movementType_ == CameraMovementType::Orbital) {
                updateOrbitPosition();
            }
        }
        float getOrbitDistance() const { return orbitDistance_; }

        void setTargetOffset(const Math::Vec3& offset) {
            targetOffset_ = offset;
            if (movementType_ == CameraMovementType::Orbital) {
                updateOrbitPosition();
            }
        }
        const Math::Vec3& getTargetOffset() const { return targetOffset_; }

        // ========================================
        // Ray Casting
        // ========================================

        struct Ray {
            Math::Vec3 origin;
            Math::Vec3 direction;
        };

        Ray screenPointToRay(const Math::Vec2& screenPoint, const Math::Vec2& screenSize) const {
            // Convert screen coordinates to normalized device coordinates
            float x = (2.0f * screenPoint.x) / screenSize.x - 1.0f;
            float y = 1.0f - (2.0f * screenPoint.y) / screenSize.y;

            // Create ray in clip space
            Math::Vec4 rayClip = Math::Vector::make(x, y, -1.0f, 1.0f);

            // Transform to eye space
            float aspect = screenSize.x / screenSize.y;
            Math::Mat4 projInverse = Math::Matrix::inverse(getProjectionMatrix(aspect));
            Math::Vec4 rayEye = Math::Matrix::multiply(projInverse, rayClip);
            rayEye.z = -1.0f;
            rayEye.w = 0.0f;

            // Transform to world space
            Math::Mat4 viewInverse = Math::Matrix::inverse(getViewMatrix());
            Math::Vec4 rayWorld = Math::Matrix::multiply(viewInverse, rayEye);

            Ray ray;
            ray.origin = getPosition();
            ray.direction = Math::Vector::normalize(Math::Vec3{rayWorld.x, rayWorld.y, rayWorld.z});

            return ray;
        }

        // ========================================
        // Frustum Information
        // ========================================

        struct FrustumCorners {
            Math::Vec3 nearTopLeft, nearTopRight, nearBottomLeft, nearBottomRight;
            Math::Vec3 farTopLeft, farTopRight, farBottomLeft, farBottomRight;
        };

        FrustumCorners getFrustumCorners(float aspect) const {
            FrustumCorners corners;

            Math::Mat4 viewProj = getViewProjectionMatrix(aspect);
            Math::Mat4 invViewProj = Math::Matrix::inverse(viewProj);

            // NDC coordinates for frustum corners
            Math::Vec4 ndcCorners[8] = {
                {-1, -1, -1, 1}, {1, -1, -1, 1}, {1, 1, -1, 1}, {-1, 1, -1, 1},  // Near
                {-1, -1,  1, 1}, {1, -1,  1, 1}, {1, 1,  1, 1}, {-1, 1,  1, 1}   // Far
            };

            // Transform to world space
            Math::Vec3 worldCorners[8];
            for (int i = 0; i < 8; i++) {
                Math::Vec4 worldPos = Math::Matrix::multiply(invViewProj, ndcCorners[i]);
                worldPos /= worldPos.w;  // Perspective divide
                worldCorners[i] = Math::Vec3{worldPos.x, worldPos.y, worldPos.z};
            }

            corners.nearBottomLeft = worldCorners[0];
            corners.nearBottomRight = worldCorners[1];
            corners.nearTopRight = worldCorners[2];
            corners.nearTopLeft = worldCorners[3];
            corners.farBottomLeft = worldCorners[4];
            corners.farBottomRight = worldCorners[5];
            corners.farTopRight = worldCorners[6];
            corners.farTopLeft = worldCorners[7];

            return corners;
        }

        // ========================================
        // Animation and Interpolation
        // ========================================

        void animateToPosition(const Math::Vec3& targetPosition, float duration) {
            // This would typically be handled by an animation system
            // For now, just set the position directly
            setPosition(targetPosition);
        }

        void animateToTarget(const Math::Vec3& newTarget, float duration) {
            setTarget(newTarget);
        }

        // ========================================
        // Debug and Utilities
        // ========================================

        void debugPrint() const {
            printf("Camera Debug Info:\n");
            printf("  Position: (%.3f, %.3f, %.3f)\n", getPosition().x, getPosition().y, getPosition().z);
            printf("  Target: (%.3f, %.3f, %.3f)\n", getTarget().x, getTarget().y, getTarget().z);
            printf("  Forward: (%.3f, %.3f, %.3f)\n", getForward().x, getForward().y, getForward().z);
            printf("  Type: %s\n", (cameraType_ == CameraType::Perspective) ? "Perspective" : "Orthographic");
            printf("  Movement: %s\n", getMovementTypeString());
            printf("  FOV: %.1f°\n", fov_);
            printf("  Near/Far: %.3f / %.3f\n", nearPlane_, farPlane_);
            if (movementType_ == CameraMovementType::Orbital) {
                printf("  Orbit Distance: %.3f\n", orbitDistance_);
            }
        }

    private:
        // ========================================
        // Internal State
        // ========================================

        Engine::Transform transform_;
        Math::Vec3 worldUp_;
        Math::Vec3 target_;
        Math::Vec3 targetOffset_;

        CameraType cameraType_;
        CameraMovementType movementType_;

        // Projection parameters
        float fov_;
        float nearPlane_;
        float farPlane_;
        float orthoSize_;

        // Movement parameters
        float movementSpeed_;
        float mouseSensitivity_;
        float zoomSpeed_;

        // Orientation (for FPS-style movement)
        float yaw_;
        float pitch_;

        // Orbital camera parameters
        float orbitDistance_;

        // ========================================
        // Internal Methods
        // ========================================

        void processFPSMovement(float velocity, bool forward, bool backward,
                              bool left, bool right, bool up, bool down) {
            Math::Vec3 movement = Math::Vector::Constants::ZERO;

            if (forward) movement += transform_.getForward() * velocity;
            if (backward) movement -= transform_.getForward() * velocity;
            if (left) movement -= transform_.getRight() * velocity;
            if (right) movement += transform_.getRight() * velocity;
            if (up) movement += worldUp_ * velocity;
            if (down) movement -= worldUp_ * velocity;

            transform_.translate(movement);
        }

        void processOrbitalMovement(float velocity, bool forward, bool backward,
                                  bool left, bool right, bool up, bool down) {
            if (forward) orbitDistance_ = Math::clamp(orbitDistance_ - velocity, 0.1f, 1000.0f);
            if (backward) orbitDistance_ = Math::clamp(orbitDistance_ + velocity, 0.1f, 1000.0f);

            Math::Vec3 movement = Math::Vector::Constants::ZERO;
            if (left) movement -= transform_.getRight() * velocity;
            if (right) movement += transform_.getRight() * velocity;
            if (up) movement += worldUp_ * velocity;
            if (down) movement -= worldUp_ * velocity;

            target_ += movement;
            updateOrbitPosition();
        }

        void processFreeMovement(float velocity, bool forward, bool backward,
                               bool left, bool right, bool up, bool down) {
            Math::Vec3 movement = Math::Vector::Constants::ZERO;

            if (forward) movement += Math::Vector::Constants::FORWARD * velocity;
            if (backward) movement += Math::Vector::Constants::BACK * velocity;
            if (left) movement += Math::Vector::Constants::LEFT * velocity;
            if (right) movement += Math::Vector::Constants::RIGHT * velocity;
            if (up) movement += Math::Vector::Constants::UP * velocity;
            if (down) movement += Math::Vector::Constants::DOWN * velocity;

            transform_.translate(movement);
        }

        void processFPSMouseMovement(float deltaX, float deltaY, float sensitivity) {
            yaw_ += deltaX * sensitivity * Math::Constants::RAD_TO_DEG;
            pitch_ -= deltaY * sensitivity * Math::Constants::RAD_TO_DEG;

            pitch_ = Math::clamp(pitch_, -89.0f, 89.0f);

            updateCameraVectors();
        }

        void processOrbitalMouseMovement(float deltaX, float deltaY, float sensitivity) {
            yaw_ += deltaX * sensitivity * Math::Constants::RAD_TO_DEG;
            pitch_ -= deltaY * sensitivity * Math::Constants::RAD_TO_DEG;

            pitch_ = Math::clamp(pitch_, -89.0f, 89.0f);

            updateOrbitPosition();
        }

        void updateCameraVectors() {
            // Calculate forward direction from yaw and pitch
            Math::Vec3 front;
            front.x = cosf(Math::degreesToRadians(yaw_)) * cosf(Math::degreesToRadians(pitch_));
            front.y = sinf(Math::degreesToRadians(pitch_));
            front.z = sinf(Math::degreesToRadians(yaw_)) * cosf(Math::degreesToRadians(pitch_));

            // Update transform orientation
            transform_.lookInDirection(Math::Vector::normalize(front), worldUp_);
        }

        void updateOrbitPosition() {
            // Calculate position based on spherical coordinates
            float yawRad = Math::degreesToRadians(yaw_);
            float pitchRad = Math::degreesToRadians(pitch_);

            Math::Vec3 offset;
            offset.x = orbitDistance_ * cosf(pitchRad) * cosf(yawRad);
            offset.y = orbitDistance_ * sinf(pitchRad);
            offset.z = orbitDistance_ * cosf(pitchRad) * sinf(yawRad);

            Math::Vec3 newPosition = target_ + targetOffset_ + offset;
            transform_.setPosition(newPosition);
            transform_.lookAt(target_ + targetOffset_, worldUp_);
        }

        Math::Vec3 getOrbitPosition() const {
            float yawRad = Math::degreesToRadians(yaw_);
            float pitchRad = Math::degreesToRadians(pitch_);

            Math::Vec3 offset;
            offset.x = orbitDistance_ * cosf(pitchRad) * cosf(yawRad);
            offset.y = orbitDistance_ * sinf(pitchRad);
            offset.z = orbitDistance_ * cosf(pitchRad) * sinf(yawRad);

            return target_ + targetOffset_ + offset;
        }

        const char* getMovementTypeString() const {
            switch (movementType_) {
                case CameraMovementType::FPS: return "FPS";
                case CameraMovementType::Orbital: return "Orbital";
                case CameraMovementType::Free: return "Free";
                default: return "Unknown";
            }
        }
    };

    // ========================================
    // Camera Utilities
    // ========================================

    namespace CameraUtils {

        // Create a camera looking at a target from a distance
        inline Camera createLookAt(const Math::Vec3& position, const Math::Vec3& target,
                                 const Math::Vec3& up = Math::Vector::Constants::UP) {
            Camera camera(position, target, up);
            return camera;
        }

        // Create an orbital camera around a target
        inline Camera createOrbital(const Math::Vec3& target, float distance,
                                   float yaw = -90.0f, float pitch = 0.0f) {
            Camera camera;
            camera.setMovementType(CameraMovementType::Orbital);
            camera.setTarget(target);
            camera.setOrbitDistance(distance);

            // Set initial angles
            Math::Vec3 position;
            position.x = target.x + distance * cosf(Math::degreesToRadians(pitch)) * cosf(Math::degreesToRadians(yaw));
            position.y = target.y + distance * sinf(Math::degreesToRadians(pitch));
            position.z = target.z + distance * cosf(Math::degreesToRadians(pitch)) * sinf(Math::degreesToRadians(yaw));

            camera.lookAt(position, target, Math::Vector::Constants::UP);
            return camera;
        }

        // Create a perspective camera
        inline Camera createPerspective(float fov = 60.0f, float near = 0.01f, float far = 1000.0f) {
            Camera camera;
            camera.setCameraType(CameraType::Perspective);
            camera.setFieldOfView(fov);
            camera.setNearPlane(near);
            camera.setFarPlane(far);
            return camera;
        }

        // Create an orthographic camera
        inline Camera createOrthographic(float size = 10.0f, float near = 0.01f, float far = 1000.0f) {
            Camera camera;
            camera.setCameraType(CameraType::Orthographic);
            camera.setOrthographicSize(size);
            camera.setNearPlane(near);
            camera.setFarPlane(far);
            return camera;
        }

        // Smooth camera transitions
        inline Camera interpolate(const Camera& from, const Camera& to, float t) {
            Camera result = from;

            // Interpolate position
            Math::Vec3 fromPos = from.getPosition();
            Math::Vec3 toPos = to.getPosition();
            result.setPosition(Math::Vector::lerp(fromPos, toPos, t));

            // Interpolate target (for orbital cameras)
            if (from.getMovementType() == CameraMovementType::Orbital ||
                to.getMovementType() == CameraMovementType::Orbital) {
                Math::Vec3 fromTarget = from.getTarget();
                Math::Vec3 toTarget = to.getTarget();
                result.setTarget(Math::Vector::lerp(fromTarget, toTarget, t));
            }

            // Interpolate FOV
            float fromFov = from.getFieldOfView();
            float toFov = to.getFieldOfView();
            result.setFieldOfView(Math::lerp(fromFov, toFov, t));

            return result;
        }

        // Calculate screen space bounds of a 3D bounding box
        struct ScreenBounds {
            Math::Vec2 min, max;
            bool visible;
        };

        inline ScreenBounds getScreenBounds(const Camera& camera,
                                          const Math::Vec3& boundsMin,
                                          const Math::Vec3& boundsMax,
                                          const Math::Vec2& screenSize) {
            ScreenBounds result = {{FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX}, false};

            float aspect = screenSize.x / screenSize.y;
            Math::Mat4 viewProj = camera.getViewProjectionMatrix(aspect);

            // Test all 8 corners of the bounding box
            Math::Vec3 corners[8] = {
                {boundsMin.x, boundsMin.y, boundsMin.z},
                {boundsMax.x, boundsMin.y, boundsMin.z},
                {boundsMin.x, boundsMax.y, boundsMin.z},
                {boundsMax.x, boundsMax.y, boundsMin.z},
                {boundsMin.x, boundsMin.y, boundsMax.z},
                {boundsMax.x, boundsMin.y, boundsMax.z},
                {boundsMin.x, boundsMax.y, boundsMax.z},
                {boundsMax.x, boundsMax.y, boundsMax.z}
            };

            bool anyVisible = false;

            for (int i = 0; i < 8; i++) {
                Math::Vec4 clipPos = Math::Matrix::multiply(viewProj, Math::Vector::make(corners[i], 1.0f));

                if (clipPos.w > 0) {  // In front of camera
                    anyVisible = true;

                    // Convert to NDC
                    Math::Vec3 ndc = Math::Vec3{clipPos.x / clipPos.w, clipPos.y / clipPos.w, clipPos.z / clipPos.w};

                    // Convert to screen space
                    Math::Vec2 screen;
                    screen.x = (ndc.x + 1.0f) * 0.5f * screenSize.x;
                    screen.y = (1.0f - ndc.y) * 0.5f * screenSize.y;

                    result.min.x = fminf(result.min.x, screen.x);
                    result.min.y = fminf(result.min.y, screen.y);
                    result.max.x = fmaxf(result.max.x, screen.x);
                    result.max.y = fmaxf(result.max.y, screen.y);
                }
            }

            result.visible = anyVisible;
            return result;
        }
    }

} // namespace Engine

// ========================================
// Backward Compatibility
// ========================================

// For existing code that expects the old Camera interface
using Camera = Engine::Camera;