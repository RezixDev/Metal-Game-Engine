// Core/Math.hpp
// Consolidated math utilities with clear namespace organization
#pragma once
#include <simd/simd.h>
#include <cmath>

namespace Core {
    
    // Type aliases for clarity
    using Vec2 = vector_float2;
    using Vec3 = vector_float3;
    using Vec4 = vector_float4;
    using Mat4 = matrix_float4x4;
    using Quat = simd_quatf;
    
    // Math constants
    namespace Constants {
        constexpr float PI = M_PI;
        constexpr float TWO_PI = 2.0f * M_PI;
        constexpr float HALF_PI = M_PI * 0.5f;
        constexpr float DEG_TO_RAD = M_PI / 180.0f;
        constexpr float RAD_TO_DEG = 180.0f / M_PI;
    }
    
    // Conversion utilities
    inline float degreesToRadians(float degrees) {
        return degrees * Constants::DEG_TO_RAD;
    }
    
    inline float radiansToDegrees(float radians) {
        return radians * Constants::RAD_TO_DEG;
    }
    
    // Vector operations
    namespace Vector {
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
        
        inline float distance(const Vec3& a, const Vec3& b) {
            return simd_distance(a, b);
        }
        
        inline Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
            return simd_mix(a, b, t);
        }
    }
    
    // Matrix operations
    namespace Matrix {
        inline Mat4 identity() {
            return matrix_identity_float4x4;
        }
        
        inline Mat4 multiply(const Mat4& a, const Mat4& b) {
            return simd_mul(a, b);
        }
        
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
        
        // Extract components
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
    }
    
    // Quaternion operations
    namespace Quaternion {
        inline Quat identity() {
            return simd_quaternion(0.0f, 0.0f, 0.0f, 1.0f);
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
            // Convert Euler angles to quaternion
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
        
        inline Mat4 toMatrix(const Quat& q) {
            return simd_matrix4x4(q);
        }
        
        inline Quat slerp(const Quat& a, const Quat& b, float t) {
            return simd_slerp(a, b, t);
        }

        inline Quat multiply(const Quat& a, const Quat& b) {
            return simd_mul(a, b);
        }
    }
    
    // Common math functions
    namespace Math {
        template<typename T>
        inline T clamp(T value, T min, T max) {
            return value < min ? min : (value > max ? max : value);
        }
        
        template<typename T>
        inline T lerp(T a, T b, float t) {
            return a + (b - a) * t;
        }
        
        template<typename T>
        inline T smoothstep(T edge0, T edge1, T x) {
            T t = clamp((x - edge0) / (edge1 - edge0), T(0), T(1));
            return t * t * (T(3) - T(2) * t);
        }
    }
}