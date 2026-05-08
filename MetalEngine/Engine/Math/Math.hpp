// Engine/Math/Math.hpp
// Vector / matrix utilities used by the engine. Backed by Apple's simd types so
// that buffers shared with Metal shaders have the right layout for free.
#pragma once

#include <simd/simd.h>
#include <cmath>

namespace Engine {
namespace Math {

using Vec2 = vector_float2;
using Vec3 = vector_float3;
using Vec4 = vector_float4;
using Mat4 = matrix_float4x4;

// ---- Scalar constants / helpers -------------------------------------------

namespace Constants {
    constexpr float PI         = M_PI;
    constexpr float DEG_TO_RAD = M_PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / M_PI;
    constexpr float EPSILON    = 1e-6f;
}

inline float degreesToRadians(float degrees) { return degrees * Constants::DEG_TO_RAD; }
inline float radiansToDegrees(float radians) { return radians * Constants::RAD_TO_DEG; }

template<typename T>
inline T clamp(T value, T lo, T hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

// ---- Vector operations ----------------------------------------------------

namespace Vector {

inline Vec2 make(float x, float y)                   { return {x, y}; }
inline Vec3 make(float x, float y, float z)          { return {x, y, z}; }
inline Vec4 make(float x, float y, float z, float w) { return {x, y, z, w}; }
inline Vec4 make(const Vec3& xyz, float w)           { return {xyz.x, xyz.y, xyz.z, w}; }

inline float dot(const Vec3& a, const Vec3& b)       { return simd_dot(a, b); }
inline float length(const Vec3& v)                   { return simd_length(v); }
inline float distance(const Vec3& a, const Vec3& b)  { return simd_distance(a, b); }
inline Vec3  cross(const Vec3& a, const Vec3& b)     { return simd_cross(a, b); }
inline Vec3  lerp(const Vec3& a, const Vec3& b, float t) { return simd_mix(a, b, t); }

inline Vec3 normalize(const Vec3& v) {
    const float len = simd_length(v);
    return len < Constants::EPSILON ? Vec3{0, 0, 0} : v / len;
}

namespace Constants {
    constexpr Vec3 ZERO    {0.0f,  0.0f,  0.0f};
    constexpr Vec3 ONE     {1.0f,  1.0f,  1.0f};
    constexpr Vec3 UP      {0.0f,  1.0f,  0.0f};
    constexpr Vec3 DOWN    {0.0f, -1.0f,  0.0f};
    constexpr Vec3 RIGHT   {1.0f,  0.0f,  0.0f};
    constexpr Vec3 LEFT    {-1.0f, 0.0f,  0.0f};
    constexpr Vec3 FORWARD {0.0f,  0.0f, -1.0f};
    constexpr Vec3 BACK    {0.0f,  0.0f,  1.0f};
}

} // namespace Vector

// ---- Matrix operations ----------------------------------------------------

namespace Matrix {

inline Mat4 identity()                               { return matrix_identity_float4x4; }
inline Mat4 multiply(const Mat4& a, const Mat4& b)   { return simd_mul(a, b); }
inline Vec4 multiply(const Mat4& m, const Vec4& v)   { return simd_mul(m, v); }
inline Mat4 inverse(const Mat4& m)                   { return simd_inverse(m); }

inline Mat4 translation(float x, float y, float z) {
    return Mat4{{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {x, y, z, 1},
    }};
}
inline Mat4 translation(const Vec3& t) { return translation(t.x, t.y, t.z); }

inline Mat4 scale(float x, float y, float z) {
    return Mat4{{
        {x, 0, 0, 0},
        {0, y, 0, 0},
        {0, 0, z, 0},
        {0, 0, 0, 1},
    }};
}

// Right-handed look-at, the convention Metal expects for the standard
// "forward = -Z in view space" perspective.
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

// Reverse-no, this is the standard depth-[0,1] perspective matrix Metal/D3D
// use; far/near are positive distances.
inline Mat4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
    const float f  = 1.0f / tanf(fovYRadians * 0.5f);
    const float nf = nearZ - farZ;
    return Mat4{{
        {f/aspect, 0,       0,                 0},
        {0,        f,       0,                 0},
        {0,        0,       farZ/nf,          -1},
        {0,        0,       (farZ*nearZ)/nf,   0},
    }};
}

inline Mat4 perspectiveDegrees(float fovYDegrees, float aspect, float nearZ, float farZ) {
    return perspective(degreesToRadians(fovYDegrees), aspect, nearZ, farZ);
}

inline Vec3 getTranslation(const Mat4& m) {
    return Vec3{m.columns[3].x, m.columns[3].y, m.columns[3].z};
}

} // namespace Matrix

} // namespace Math
} // namespace Engine
