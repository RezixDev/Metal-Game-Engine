// Engine/Math.hpp
// simd-backed vector and matrix utilities. Sharing simd types with shaders
// gives us matching layouts in uniform/vertex buffers for free.
#pragma once

#include <simd/simd.h>
#include <cmath>

namespace Engine {
namespace Math {

using Vec2 = vector_float2;
using Vec3 = vector_float3;
using Vec4 = vector_float4;
using Mat4 = matrix_float4x4;

namespace Constants {
    constexpr float PI         = M_PI;
    constexpr float DEG_TO_RAD = M_PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / M_PI;
    constexpr float EPSILON    = 1e-6f;
}

inline float degreesToRadians(float d) { return d * Constants::DEG_TO_RAD; }
inline float radiansToDegrees(float r) { return r * Constants::RAD_TO_DEG; }

template<typename T>
inline T clamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

namespace Vector {

inline Vec3 make(float x, float y, float z) { return {x, y, z}; }
inline float dot(const Vec3& a, const Vec3& b) { return simd_dot(a, b); }
inline float length(const Vec3& v)            { return simd_length(v); }
inline Vec3  cross(const Vec3& a, const Vec3& b) { return simd_cross(a, b); }

inline Vec3 normalize(const Vec3& v) {
    const float len = simd_length(v);
    return len < Constants::EPSILON ? Vec3{0, 0, 0} : v / len;
}

namespace Constants {
    constexpr Vec3 ZERO    {0,  0,  0};
    constexpr Vec3 UP      {0,  1,  0};
    constexpr Vec3 FORWARD {0,  0, -1};
    constexpr Vec3 RIGHT   {1,  0,  0};
}

} // namespace Vector

namespace Matrix {

inline Mat4 identity()                              { return matrix_identity_float4x4; }
inline Mat4 multiply(const Mat4& a, const Mat4& b)  { return simd_mul(a, b); }

inline Mat4 translation(float x, float y, float z) {
    return Mat4{{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {x, y, z, 1},
    }};
}

inline Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    const Vec3 f = Vector::normalize(center - eye);
    const Vec3 r = Vector::normalize(Vector::cross(f, up));
    const Vec3 u = Vector::cross(r, f);
    return Mat4{{
        { r.x, u.x, -f.x, 0},
        { r.y, u.y, -f.y, 0},
        { r.z, u.z, -f.z, 0},
        {-Vector::dot(r, eye), -Vector::dot(u, eye), Vector::dot(f, eye), 1},
    }};
}

// Metal-style perspective: depth maps to [0, 1] with -Z forward in view space.
inline Mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
    const float f  = 1.0f / tanf(fovYRadians * 0.5f);
    const float nf = nearZ - farZ;
    return Mat4{{
        {f/aspect, 0, 0,                 0},
        {0,        f, 0,                 0},
        {0,        0, farZ/nf,          -1},
        {0,        0, (farZ*nearZ)/nf,   0},
    }};
}

inline Mat4 perspectiveDegrees(float fovDegrees, float aspect, float nearZ, float farZ) {
    return perspective(degreesToRadians(fovDegrees), aspect, nearZ, farZ);
}

} // namespace Matrix

} // namespace Math
} // namespace Engine
