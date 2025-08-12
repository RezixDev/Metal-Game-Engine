// Core/Transform.hpp
// Transform component for game objects
#pragma once
#include "Math.hpp"

namespace Core {

    class Transform {
    public:
        Transform()
            : position_{0, 0, 0}
            , rotation_(Quaternion::identity())
            , scale_{1, 1, 1}
            , isDirty_(true)
        {}

        // Position
        void setPosition(const Vec3& position) {
            position_ = position;
            isDirty_ = true;
        }

        void setPosition(float x, float y, float z) {
            setPosition(Vec3{x, y, z});
        }

        void translate(const Vec3& delta) {
            position_ += delta;
            isDirty_ = true;
        }

        const Vec3& getPosition() const { return position_; }

        // Rotation
        void setRotation(const Quat& rotation) {
            rotation_ = rotation;
            isDirty_ = true;
        }

        void setRotationEuler(float pitch, float yaw, float roll) {
            rotation_ = Quaternion::fromEuler(pitch, yaw, roll);
            isDirty_ = true;
        }

        void setRotationEulerDegrees(float pitch, float yaw, float roll) {
            setRotationEuler(
                degreesToRadians(pitch),
                degreesToRadians(yaw),
                degreesToRadians(roll)
            );
        }

        void rotate(const Quat& delta) {
            rotation_ = Quaternion::multiply(delta, rotation_);
            isDirty_ = true;
        }

        void rotateEuler(float pitch, float yaw, float roll) {
            rotate(Quaternion::fromEuler(pitch, yaw, roll));
        }

        const Quat& getRotation() const { return rotation_; }

        // Scale
        void setScale(const Vec3& scale) {
            scale_ = scale;
            isDirty_ = true;
        }

        void setScale(float uniform) {
            setScale(Vec3{uniform, uniform, uniform});
        }

        void setScale(float x, float y, float z) {
            setScale(Vec3{x, y, z});
        }

        const Vec3& getScale() const { return scale_; }

        // Direction vectors (based on rotation)
        Vec3 getForward() const {
            Mat4 rot = Quaternion::toMatrix(rotation_);
            return Vector::normalize(Vec3{-rot.columns[2].x, -rot.columns[2].y, -rot.columns[2].z});
        }

        Vec3 getRight() const {
            Mat4 rot = Quaternion::toMatrix(rotation_);
            return Vector::normalize(Vec3{rot.columns[0].x, rot.columns[0].y, rot.columns[0].z});
        }

        Vec3 getUp() const {
            Mat4 rot = Quaternion::toMatrix(rotation_);
            return Vector::normalize(Vec3{rot.columns[1].x, rot.columns[1].y, rot.columns[1].z});
        }

        // Matrix generation
        Mat4 getMatrix() const {
            if (isDirty_) {
                updateMatrix();
            }
            return cachedMatrix_;
        }

        Mat4 getModelMatrix() const {
            return getMatrix();
        }

        // Look at target
        void lookAt(const Vec3& target, const Vec3& up = Vec3{0, 1, 0}) {
            Vec3 forward = Vector::normalize(target - position_);
            Vec3 right = Vector::normalize(Vector::cross(forward, up));
            Vec3 actualUp = Vector::cross(right, forward);

            // Create rotation matrix and convert to quaternion
            Mat4 rotMatrix = Matrix::identity();
            rotMatrix.columns[0] = Vec4{right.x, right.y, right.z, 0};
            rotMatrix.columns[1] = Vec4{actualUp.x, actualUp.y, actualUp.z, 0};
            rotMatrix.columns[2] = Vec4{-forward.x, -forward.y, -forward.z, 0};

            // Extract quaternion from matrix (simplified for this case)
            rotation_ = simd_quaternion(rotMatrix);
            isDirty_ = true;
        }

        // Hierarchy support (for future scene graph)
        void setParent(Transform* parent) {
            parent_ = parent;
            isDirty_ = true;
        }

        Transform* getParent() const { return parent_; }

        Mat4 getWorldMatrix() const {
            if (parent_) {
                return Matrix::multiply(parent_->getWorldMatrix(), getMatrix());
            }
            return getMatrix();
        }

        Vec3 getWorldPosition() const {
            if (parent_) {
                Mat4 worldMat = getWorldMatrix();
                return Matrix::getTranslation(worldMat);
            }
            return position_;
        }

    private:
        void updateMatrix() const {
            Mat4 t = Matrix::translation(position_);
            Mat4 r = Quaternion::toMatrix(rotation_);
            Mat4 s = Matrix::scale(scale_);

            cachedMatrix_ = Matrix::multiply(Matrix::multiply(t, r), s);
            isDirty_ = false;
        }

        Vec3 position_;
        Quat rotation_;
        Vec3 scale_;

        mutable Mat4 cachedMatrix_;
        mutable bool isDirty_;

        Transform* parent_ = nullptr;
    };

}