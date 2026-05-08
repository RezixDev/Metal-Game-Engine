// Transform.hpp
// Enhanced Transform system with better performance and features
#pragma once

#include "Math.hpp"
#include <vector>      // Added for std::vector
#include <algorithm>   // Added for std::remove

namespace Engine {

    class Transform {
    public:
        // ========================================
        // Constructors
        // ========================================

        Transform()
            : position_(Math::Vector::Constants::ZERO)
            , rotation_(Math::Quaternion::identity())
            , scale_(Math::Vector::Constants::ONE)
            , isDirty_(true)
            , parent_(nullptr)
        {}

        Transform(const Math::Vec3& position,
                 const Math::Quat& rotation = Math::Quaternion::identity(),
                 const Math::Vec3& scale = Math::Vector::Constants::ONE)
            : position_(position)
            , rotation_(rotation)
            , scale_(scale)
            , isDirty_(true)
            , parent_(nullptr)
        {}

        // ========================================
        // Position Operations
        // ========================================

        void setPosition(const Math::Vec3& position) {
            position_ = position;
            markDirty();
        }

        void setPosition(float x, float y, float z) {
            setPosition(Math::Vector::make(x, y, z));
        }

        void translate(const Math::Vec3& delta) {
            position_ += delta;
            markDirty();
        }

        void translateLocal(const Math::Vec3& localDelta) {
            translate(Math::Quaternion::rotateVector(rotation_, localDelta));
        }

        const Math::Vec3& getPosition() const { return position_; }

        Math::Vec3 getWorldPosition() const {
            if (parent_) {
                return Math::Matrix::getTranslation(getWorldMatrix());
            }
            return position_;
        }

        // ========================================
        // Rotation Operations
        // ========================================

        void setRotation(const Math::Quat& rotation) {
            rotation_ = Math::Quaternion::normalize(rotation);
            markDirty();
        }

        void setRotationEuler(float pitch, float yaw, float roll) {
            rotation_ = Math::Quaternion::fromEuler(pitch, yaw, roll);
            markDirty();
        }

        void setRotationEulerDegrees(float pitch, float yaw, float roll) {
            setRotationEuler(
                Math::degreesToRadians(pitch),
                Math::degreesToRadians(yaw),
                Math::degreesToRadians(roll)
            );
        }

        void rotate(const Math::Quat& deltaRotation) {
            rotation_ = Math::Quaternion::multiply(deltaRotation, rotation_);
            rotation_ = Math::Quaternion::normalize(rotation_);
            markDirty();
        }

        void rotateEuler(float pitch, float yaw, float roll) {
            rotate(Math::Quaternion::fromEuler(pitch, yaw, roll));
        }

        void rotateEulerDegrees(float pitch, float yaw, float roll) {
            rotateEuler(
                Math::degreesToRadians(pitch),
                Math::degreesToRadians(yaw),
                Math::degreesToRadians(roll)
            );
        }

        void rotateAroundAxis(const Math::Vec3& axis, float angle) {
            rotate(Math::Quaternion::fromAxisAngle(axis, angle));
        }

        const Math::Quat& getRotation() const { return rotation_; }

        Math::Vec3 getEulerAngles() const {
            return Math::Quaternion::toEuler(rotation_);
        }

        Math::Vec3 getEulerAnglesDegrees() const {
            return Math::Quaternion::toEulerDegrees(rotation_);
        }

        // ========================================
        // Scale Operations
        // ========================================

        void setScale(const Math::Vec3& scale) {
            scale_ = scale;
            markDirty();
        }

        void setScale(float uniform) {
            setScale(Math::Vector::make(uniform, uniform, uniform));
        }

        void setScale(float x, float y, float z) {
            setScale(Math::Vector::make(x, y, z));
        }

        void scale(const Math::Vec3& factor) {
            scale_ = Math::Vec3{scale_.x * factor.x, scale_.y * factor.y, scale_.z * factor.z};
            markDirty();
        }

        void scale(float uniform) {
            scale(Math::Vector::make(uniform, uniform, uniform));
        }

        const Math::Vec3& getScale() const { return scale_; }

        // ========================================
        // Direction Vectors
        // ========================================

        Math::Vec3 getForward() const {
            return Math::Quaternion::rotateVector(rotation_, Math::Vector::Constants::FORWARD);
        }

        Math::Vec3 getRight() const {
            return Math::Quaternion::rotateVector(rotation_, Math::Vector::Constants::RIGHT);
        }

        Math::Vec3 getUp() const {
            return Math::Quaternion::rotateVector(rotation_, Math::Vector::Constants::UP);
        }

        Math::Vec3 getBack() const {
            return Math::Quaternion::rotateVector(rotation_, Math::Vector::Constants::BACK);
        }

        Math::Vec3 getLeft() const {
            return Math::Quaternion::rotateVector(rotation_, Math::Vector::Constants::LEFT);
        }

        Math::Vec3 getDown() const {
            return Math::Quaternion::rotateVector(rotation_, Math::Vector::Constants::DOWN);
        }

        // ========================================
        // Matrix Operations
        // ========================================

        Math::Mat4 getLocalMatrix() const {
            if (isDirty_) {
                updateMatrix();
            }
            return cachedMatrix_;
        }

        Math::Mat4 getMatrix() const {
            return getLocalMatrix();
        }

        Math::Mat4 getModelMatrix() const {
            return getLocalMatrix();
        }

        Math::Mat4 getWorldMatrix() const {
            if (parent_) {
                return Math::Matrix::multiply(parent_->getWorldMatrix(), getLocalMatrix());
            }
            return getLocalMatrix();
        }

        Math::Mat4 getInverseMatrix() const {
            return Math::Matrix::inverse(getLocalMatrix());
        }

        Math::Mat4 getInverseWorldMatrix() const {
            return Math::Matrix::inverse(getWorldMatrix());
        }

        // ========================================
        // Look At Operations
        // ========================================

        void lookAt(const Math::Vec3& target, const Math::Vec3& up = Math::Vector::Constants::UP) {
            Math::Vec3 direction = Math::Vector::normalize(target - position_);
            lookInDirection(direction, up);
        }

        void lookInDirection(const Math::Vec3& direction, const Math::Vec3& up = Math::Vector::Constants::UP) {
            Math::Vec3 forward = Math::Vector::normalize(direction);
            Math::Vec3 right = Math::Vector::normalize(Math::Vector::cross(forward, up));
            Math::Vec3 actualUp = Math::Vector::cross(right, forward);

            // Create rotation matrix
            Math::Mat4 rotMatrix = Math::Matrix::identity();
            rotMatrix.columns[0] = Math::Vector::make(right, 0.0f);
            rotMatrix.columns[1] = Math::Vector::make(actualUp, 0.0f);
            rotMatrix.columns[2] = Math::Vector::make(-forward, 0.0f);

            // Convert to quaternion
            rotation_ = simd_quaternion(rotMatrix);
            rotation_ = Math::Quaternion::normalize(rotation_);
            markDirty();
        }

        // ========================================
        // Hierarchy Operations
        // ========================================

        void setParent(Transform* parent) {
            if (parent_ == parent) return;

            if (parent_) {
                // Remove from old parent's children
                auto& children = parent_->children_;
                children.erase(std::remove(children.begin(), children.end(), this), children.end());
            }

            parent_ = parent;

            if (parent_) {
                // Add to new parent's children
                parent_->children_.push_back(this);
            }

            markDirty();
        }

        Transform* getParent() const { return parent_; }

        const std::vector<Transform*>& getChildren() const { return children_; }

        void addChild(Transform* child) {
            if (child && child != this) {
                child->setParent(this);
            }
        }

        void removeChild(Transform* child) {
            if (child) {
                child->setParent(nullptr);
            }
        }

        void removeFromParent() {
            setParent(nullptr);
        }

        // ========================================
        // Utility Operations
        // ========================================

        void reset() {
            setPosition(Math::Vector::Constants::ZERO);
            setRotation(Math::Quaternion::identity());
            setScale(Math::Vector::Constants::ONE);
        }

        Transform interpolate(const Transform& other, float t) const {
            Transform result;
            result.setPosition(Math::Vector::lerp(position_, other.position_, t));
            result.setRotation(Math::Quaternion::slerp(rotation_, other.rotation_, t));
            result.setScale(Math::Vector::lerp(scale_, other.scale_, t));
            return result;
        }

        bool isEqual(const Transform& other, float epsilon = Math::Constants::EPSILON) const {
            return Math::Vector::distance(position_, other.position_) < epsilon &&
                   Math::Quaternion::dot(rotation_, other.rotation_) > (1.0f - epsilon) &&
                   Math::Vector::distance(scale_, other.scale_) < epsilon;
        }

        // ========================================
        // Coordinate Space Conversions
        // ========================================

        Math::Vec3 transformPoint(const Math::Vec3& point) const {
            Math::Mat4 matrix = getLocalMatrix();
            Math::Vec4 result = Math::Matrix::multiply(matrix, Math::Vector::make(point, 1.0f));
            return Math::Vec3{result.x, result.y, result.z};
        }

        Math::Vec3 transformDirection(const Math::Vec3& direction) const {
            return Math::Quaternion::rotateVector(rotation_, direction);
        }

        Math::Vec3 inverseTransformPoint(const Math::Vec3& point) const {
            Math::Mat4 invMatrix = getInverseMatrix();
            Math::Vec4 result = Math::Matrix::multiply(invMatrix, Math::Vector::make(point, 1.0f));
            return Math::Vec3{result.x, result.y, result.z};
        }

        Math::Vec3 inverseTransformDirection(const Math::Vec3& direction) const {
            return Math::Quaternion::rotateVector(Math::Quaternion::conjugate(rotation_), direction);
        }

        // ========================================
        // Debug and Serialization
        // ========================================

        void debugPrint() const {
            printf("Transform:\n");
            printf("  Position: (%.3f, %.3f, %.3f)\n", position_.x, position_.y, position_.z);
            printf("  Rotation: (%.3f, %.3f, %.3f, %.3f)\n",
                   rotation_.vector.x, rotation_.vector.y, rotation_.vector.z, rotation_.vector.w);
            printf("  Scale: (%.3f, %.3f, %.3f)\n", scale_.x, scale_.y, scale_.z);

            Math::Vec3 euler = getEulerAnglesDegrees();
            printf("  Euler (degrees): (%.3f, %.3f, %.3f)\n", euler.x, euler.y, euler.z);
        }

        // For serialization (you can extend this based on your needs)
        struct SerializedData {
            Math::Vec3 position;
            Math::Quat rotation;
            Math::Vec3 scale;
        };

        SerializedData serialize() const {
            return {position_, rotation_, scale_};
        }

        void deserialize(const SerializedData& data) {
            position_ = data.position;
            rotation_ = data.rotation;
            scale_ = data.scale;
            markDirty();
        }

    private:
        // ========================================
        // Internal State
        // ========================================

        Math::Vec3 position_;
        Math::Quat rotation_;
        Math::Vec3 scale_;

        mutable Math::Mat4 cachedMatrix_;
        mutable bool isDirty_;

        Transform* parent_;
        std::vector<Transform*> children_;

        // ========================================
        // Internal Methods
        // ========================================

        void markDirty() {
            isDirty_ = true;
            // Mark all children as dirty too
            for (Transform* child : children_) {
                child->markDirty();
            }
        }

        void updateMatrix() const {
            Math::Mat4 t = Math::Matrix::translation(position_);
            Math::Mat4 r = Math::Quaternion::toMatrix(rotation_);
            Math::Mat4 s = Math::Matrix::scale(scale_);

            cachedMatrix_ = Math::Matrix::multiply(Math::Matrix::multiply(t, r), s);
            isDirty_ = false;
        }
    };

    // ========================================
    // Transform Utilities
    // ========================================

    namespace TransformUtils {

        // Create a transform that looks from one point to another
        inline Transform createLookAt(const Math::Vec3& from, const Math::Vec3& to,
                                    const Math::Vec3& up = Math::Vector::Constants::UP) {
            Transform transform;
            transform.setPosition(from);
            transform.lookAt(to, up);
            return transform;
        }

        // Create a transform from TRS components
        inline Transform createTRS(const Math::Vec3& translation,
                                 const Math::Quat& rotation,
                                 const Math::Vec3& scale) {
            return Transform(translation, rotation, scale);
        }

        // Create a transform from Euler angles
        inline Transform createFromEuler(const Math::Vec3& position,
                                       const Math::Vec3& eulerAngles,
                                       const Math::Vec3& scale = Math::Vector::Constants::ONE) {
            Transform transform;
            transform.setPosition(position);
            transform.setRotationEuler(eulerAngles.x, eulerAngles.y, eulerAngles.z);
            transform.setScale(scale);
            return transform;
        }

        // Calculate relative transform
        inline Transform getRelativeTransform(const Transform& from, const Transform& to) {
            Math::Mat4 fromInverse = from.getInverseWorldMatrix();
            Math::Mat4 toWorld = to.getWorldMatrix();
            Math::Mat4 relative = Math::Matrix::multiply(fromInverse, toWorld);

            Transform result;
            result.setPosition(Math::Matrix::getTranslation(relative));
            // Note: Extracting rotation and scale from matrix is more complex
            // This is a simplified version - you might want to implement proper decomposition
            return result;
        }
    }

} // namespace Engine

// ========================================
// Backward Compatibility
// ========================================

// For existing code that expects Core::Transform
namespace Core {
    using Transform = Engine::Transform;
}
