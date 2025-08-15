// Math.hpp
// Unified Math System - Consolidates all math utilities
#pragma once

#include <simd/simd.h>
#include <cmath>

namespace Engine {
namespace Math {

    // ========================================
    // Type Definitions
    // ========================================

    using Vec2 = vector_float2;
    using Vec3 = vector_float3;
    using Vec4 = vector_float4;
    using Mat4 = matrix_float4x4;
    using Quat = simd_quatf;

    // ========================================
    // Constants
    // ========================================

    namespace Constants {
        constexpr float PI = M_PI;
        constexpr float TWO_PI = 2.0f * M_PI;
        constexpr float HALF_PI = M_PI * 0.5f;
        constexpr float DEG_TO_RAD = M_PI / 180.0f;
        constexpr float RAD_TO_DEG = 180.0f / M_PI;
        constexpr float EPSILON = 1e-6f;
    }

    // ========================================
    // Utility Functions
    // ========================================

    inline float degreesToRadians(float degrees) {
        return degrees * Constants::DEG_TO_RAD;
    }

    inline float radiansToDegrees(float radians) {
        return radians * Constants::RAD_TO_DEG;
    }

    template<typename T>
    inline T clamp(T value, T min, T max) {
        return value < min ? min : (value > max ? max : value);
    }

    template<typename T>
    inline T lerp(T a, T b, float t) {
        return a + (b - a) * t;
    }

    inline bool isEqual(float a, float b, float epsilon = Constants::EPSILON) {
        return std::abs(a - b) < epsilon;
    }

    // ========================================
    // Vector Operations
    // ========================================

    namespace Vector {

        // Vec2 operations
        inline Vec2 make(float x, float y) {
            return {x, y};
        }

        inline float dot(const Vec2& a, const Vec2& b) {
            return simd_dot(a, b);
        }

        inline float length(const Vec2& v) {
            return simd_length(v);
        }

        inline Vec2 normalize(const Vec2& v) {
            return simd_normalize(v);
        }

        // Vec3 operations
        inline Vec3 make(float x, float y, float z) {
            return {x, y, z};
        }

        inline Vec3 normalize(const Vec3& v) {
            return simd_normalize(v);
        }

        inline Vec3 cross(const Vec3& a, const Vec3& b) {
            return simd_cross(a, b);
        }

        inline float dot(const Vec3& a, const Vec3& b) {
            return simd_dot(a, b);
        }

        inline float length(const Vec3& v) {
            return simd_length(v);
        }

        inline float lengthSquared(const Vec3& v) {
            return simd_length_squared(v);
        }

        inline float distance(const Vec3& a, const Vec3& b) {
            return simd_distance(a, b);
        }

        inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
            return simd_mix(a, b, t);
        }

        inline Vec3 reflect(const Vec3& incident, const Vec3& normal) {
            return simd_reflect(incident, normal);
        }

        inline Vec3 refract(const Vec3& incident, const Vec3& normal, float eta) {
            return simd_refract(incident, normal, eta);
        }

        // Vec4 operations
        inline Vec4 make(float x, float y, float z, float w) {
            return {x, y, z, w};
        }

        inline Vec4 make(const Vec3& xyz, float w) {
            return {xyz.x, xyz.y, xyz.z, w};
        }

        inline float dot(const Vec4& a, const Vec4& b) {
            return simd_dot(a, b);
        }

        inline float length(const Vec4& v) {
            return simd_length(v);
        }

        inline Vec4 normalize(const Vec4& v) {
            return simd_normalize(v);
        }

        // Common vector constants
        namespace Constants {
            constexpr Vec3 ZERO = {0.0f, 0.0f, 0.0f};
            constexpr Vec3 ONE = {1.0f, 1.0f, 1.0f};
            constexpr Vec3 UP = {0.0f, 1.0f, 0.0f};
            constexpr Vec3 DOWN = {0.0f, -1.0f, 0.0f};
            constexpr Vec3 RIGHT = {1.0f, 0.0f, 0.0f};
            constexpr Vec3 LEFT = {-1.0f, 0.0f, 0.0f};
            constexpr Vec3 FORWARD = {0.0f, 0.0f, -1.0f};
            constexpr Vec3 BACK = {0.0f, 0.0f, 1.0f};
        }
    }

    // ========================================
    // Matrix Operations
    // ========================================

    namespace Matrix {

        inline Mat4 identity() {
            return matrix_identity_float4x4;
        }

        inline Mat4 multiply(const Mat4& a, const Mat4& b) {
            return simd_mul(a, b);
        }

        inline Vec4 multiply(const Mat4& m, const Vec4& v) {
            return simd_mul(m, v);
        }

        inline Mat4 transpose(const Mat4& m) {
            return simd_transpose(m);
        }

        inline float determinant(const Mat4& m) {
            return simd_determinant(m);
        }

        inline Mat4 inverse(const Mat4& m) {
            return simd_inverse(m);
        }

        // Transformation matrices
        inline Mat4 translation(float x, float y, float z) {
            return Mat4{{
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {x, y, z, 1}
            }};
        }

        inline Mat4 translation(const Vec3& t) {
            return translation(t.x, t.y, t.z);
        }

        inline Mat4 scale(float x, float y, float z) {
            return Mat4{{
                {x, 0, 0, 0},
                {0, y, 0, 0},
                {0, 0, z, 0},
                {0, 0, 0, 1}
            }};
        }

        inline Mat4 scale(const Vec3& s) {
            return scale(s.x, s.y, s.z);
        }

        inline Mat4 scale(float uniform) {
            return scale(uniform, uniform, uniform);
        }

        inline Mat4 rotationX(float radians) {
            float c = cosf(radians);
            float s = sinf(radians);
            return Mat4{{
                {1, 0,  0, 0},
                {0, c, -s, 0},
                {0, s,  c, 0},
                {0, 0,  0, 1}
            }};
        }

        inline Mat4 rotationY(float radians) {
            float c = cosf(radians);
            float s = sinf(radians);
            return Mat4{{
                { c, 0, -s, 0},
                { 0, 1,  0, 0},
                { s, 0,  c, 0},
                { 0, 0,  0, 1}
            }};
        }

        inline Mat4 rotationZ(float radians) {
            float c = cosf(radians);
            float s = sinf(radians);
            return Mat4{{
                { c, -s, 0, 0},
                { s,  c, 0, 0},
                { 0,  0, 1, 0},
                { 0,  0, 0, 1}
            }};
        }

        inline Mat4 rotation(float x, float y, float z) {
            return multiply(rotationZ(z), multiply(rotationY(y), rotationX(x)));
        }

        inline Mat4 rotation(const Vec3& euler) {
            return rotation(euler.x, euler.y, euler.z);
        }

        // View and projection matrices
        inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
            Vec3 f = Vector::normalize(center - eye);
            Vec3 r = Vector::normalize(Vector::cross(f, up));
            Vec3 u = Vector::cross(r, f);

            return Mat4{{
                { r.x, u.x, -f.x, 0},
                { r.y, u.y, -f.y, 0},
                { r.z, u.z, -f.z, 0},
                {-Vector::dot(r, eye), -Vector::dot(u, eye), Vector::dot(f, eye), 1}
            }};
        }

        inline Mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
            float f = 1.0f / tanf(fovYRadians * 0.5f);
            float nf = nearZ - farZ;
            return Mat4{{
                {f/aspect, 0,            0,               0},
                {0,        f,            0,               0},
                {0,        0,   farZ/nf,                -1},
                {0,        0, (farZ*nearZ)/nf,           0}
            }};
        }

        inline Mat4 perspectiveDegrees(float fovYDegrees, float aspect, float nearZ, float farZ) {
            return perspective(degreesToRadians(fovYDegrees), aspect, nearZ, farZ);
        }

        inline Mat4 orthographic(float left, float right, float bottom, float top,
                                float nearZ, float farZ) {
            float rl = right - left;
            float tb = top - bottom;
            float fn = farZ - nearZ;

            return Mat4{{
                {2.0f/rl,        0,          0,       0},
                {0,         2.0f/tb,          0,       0},
                {0,              0,    -2.0f/fn,       0},
                {-(right+left)/rl, -(top+bottom)/tb, -(farZ+nearZ)/fn, 1}
            }};
        }

        // Component extraction
        inline Vec3 getTranslation(const Mat4& m) {
            return Vec3{m.columns[3].x, m.columns[3].y, m.columns[3].z};
        }

        inline Vec3 getScale(const Mat4& m) {
            return Vec3{
                Vector::length(Vec3{m.columns[0].x, m.columns[0].y, m.columns[0].z}),
                Vector::length(Vec3{m.columns[1].x, m.columns[1].y, m.columns[1].z}),
                Vector::length(Vec3{m.columns[2].x, m.columns[2].y, m.columns[2].z})
            };
        }

        inline Vec3 getRight(const Mat4& m) {
            return Vector::normalize(Vec3{m.columns[0].x, m.columns[0].y, m.columns[0].z});
        }

        inline Vec3 getUp(const Mat4& m) {
            return Vector::normalize(Vec3{m.columns[1].x, m.columns[1].y, m.columns[1].z});
        }

        inline Vec3 getForward(const Mat4& m) {
            return Vector::normalize(Vec3{-m.columns[2].x, -m.columns[2].y, -m.columns[2].z});
        }
    }

    // ========================================
    // Quaternion Operations
    // ========================================

    namespace Quaternion {

        inline Quat identity() {
            return simd_quaternion(0.0f, 0.0f, 0.0f, 1.0f);
        }

        inline Quat make(float x, float y, float z, float w) {
            return simd_quaternion(x, y, z, w);
        }

        inline Quat fromAxisAngle(const Vec3& axis, float angle) {
            float halfAngle = angle * 0.5f;
            float s = sinf(halfAngle);
            Vec3 normalizedAxis = Vector::normalize(axis);
            return simd_quaternion(
                normalizedAxis.x * s,
                normalizedAxis.y * s,
                normalizedAxis.z * s,
                cosf(halfAngle)
            );
        }

        inline Quat fromEuler(float pitch, float yaw, float roll) {
            float cy = cosf(yaw * 0.5f);
            float sy = sinf(yaw * 0.5f);
            float cp = cosf(pitch * 0.5f);
            float sp = sinf(pitch * 0.5f);
            float cr = cosf(roll * 0.5f);
            float sr = sinf(roll * 0.5f);

            return simd_quaternion(
                sr * cp * cy - cr * sp * sy,
                cr * sp * cy + sr * cp * sy,
                cr * cp * sy - sr * sp * cy,
                cr * cp * cy + sr * sp * sy
            );
        }

        inline Quat fromEulerDegrees(float pitch, float yaw, float roll) {
            return fromEuler(
                degreesToRadians(pitch),
                degreesToRadians(yaw),
                degreesToRadians(roll)
            );
        }

        inline Mat4 toMatrix(const Quat& q) {
            return simd_matrix4x4(q);
        }

        inline Quat multiply(const Quat& a, const Quat& b) {
            return simd_mul(a, b);
        }

        inline Quat conjugate(const Quat& q) {
            return simd_conjugate(q);
        }

        inline Quat inverse(const Quat& q) {
            return simd_inverse(q);
        }

        inline float length(const Quat& q) {
            return simd_length(q);
        }

        inline Quat normalize(const Quat& q) {
            return simd_normalize(q);
        }

        inline Quat slerp(const Quat& a, const Quat& b, float t) {
            return simd_slerp(a, b, t);
        }

        inline float dot(const Quat& a, const Quat& b) {
            return simd_dot(a, b);
        }

        inline Vec3 rotateVector(const Quat& q, const Vec3& v) {
            return simd_act(q, v);
        }

        // Convert quaternion to Euler angles (in radians)
        inline Vec3 toEuler(const Quat& q) {
            Vec3 angles;

            // Roll (x-axis rotation)
            float sinr_cosp = 2 * (q.vector.w * q.vector.x + q.vector.y * q.vector.z);
            float cosr_cosp = 1 - 2 * (q.vector.x * q.vector.x + q.vector.y * q.vector.y);
            angles.x = atan2f(sinr_cosp, cosr_cosp);

            // Pitch (y-axis rotation)
            float sinp = 2 * (q.vector.w * q.vector.y - q.vector.z * q.vector.x);
            if (std::abs(sinp) >= 1)
                angles.y = copysignf(Constants::HALF_PI, sinp); // Use 90 degrees if out of range
            else
                angles.y = asinf(sinp);

            // Yaw (z-axis rotation)
            float siny_cosp = 2 * (q.vector.w * q.vector.z + q.vector.x * q.vector.y);
            float cosy_cosp = 1 - 2 * (q.vector.y * q.vector.y + q.vector.z * q.vector.z);
            angles.z = atan2f(siny_cosp, cosy_cosp);

            return angles;
        }

        inline Vec3 toEulerDegrees(const Quat& q) {
            Vec3 radians = toEuler(q);
            return Vec3{
                radiansToDegrees(radians.x),
                radiansToDegrees(radians.y),
                radiansToDegrees(radians.z)
            };
        }
    }

    // ========================================
    // Backward Compatibility Wrappers
    // ========================================

    // For existing code that uses the old Matrix4 class
    class Matrix4 {
    public:
        static Mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
            return Matrix::perspective(fovYRadians, aspect, nearZ, farZ);
        }

        static Mat4 translation(float x, float y, float z) {
            return Matrix::translation(x, y, z);
        }

        static Mat4 rotationY(float radians) {
            return Matrix::rotationY(radians);
        }

        static Mat4 rotationX(float radians) {
            return Matrix::rotationX(radians);
        }

        static Mat4 scale(float x, float y, float z) {
            return Matrix::scale(x, y, z);
        }

        static Mat4 multiply(const Mat4& a, const Mat4& b) {
            return Matrix::multiply(a, b);
        }

        static Mat4 identity() {
            return Matrix::identity();
        }
    };

} // namespace Math
} // namespace Engine

// ========================================
// Global namespace aliases for convenience
// ========================================

// Alias the Engine::Math namespace to global Math for backward compatibility
namespace Math = Engine::Math;