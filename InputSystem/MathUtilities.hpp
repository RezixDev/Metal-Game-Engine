#pragma once
#include <simd/simd.h>
#include <cmath>

namespace Math {
    class Matrix4 {
    public:
        // Corrected perspective matrix (row-major initialization for column-major storage)
        static matrix_float4x4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
            float f = 1.0f / tanf(fovYRadians * 0.5f);
            float nf = nearZ - farZ;
            return matrix_float4x4{{
                { f / aspect, 0,            0,               0 },
                { 0,          f,            0,               0 },
                { 0,          0,   farZ / nf,              -1 },
                { 0,          0, (farZ*nearZ) / nf,        0 }
            }};
        }
        
        // Corrected translation matrix
        static matrix_float4x4 translation(float x, float y, float z) {
            return matrix_float4x4{{
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {x, y, z, 1}
            }};
        }
        
        // Matrix4::rotationY — right-handed, +Z forward, column vectors
        static matrix_float4x4 rotationY(float radians) {
            float c = cosf(radians);
            float s = sinf(radians);
            return matrix_float4x4{{
                {  c, 0, -s, 0 },
                {  0, 1,  0, 0 },
                {  s, 0,  c, 0 },
                {  0, 0,  0, 1 }
            }};
        }

        
        static matrix_float4x4 rotationX(float radians) {
            float c = cosf(radians);
            float s = sinf(radians);
            return matrix_float4x4{{
                {1, 0,  0, 0},
                {0, c, -s, 0},
                {0, s,  c, 0},
                {0, 0,  0, 1}
            }};
        }
        
        static matrix_float4x4 scale(float x, float y, float z) {
            return matrix_float4x4{{
                {x, 0, 0, 0},
                {0, y, 0, 0},
                {0, 0, z, 0},
                {0, 0, 0, 1}
            }};
        }
        
        static matrix_float4x4 multiply(const matrix_float4x4& a, const matrix_float4x4& b) {
            return simd_mul(a, b);
        }
        
        static matrix_float4x4 identity() {
            return matrix_identity_float4x4;
        }
    };
    
    inline float degreesToRadians(float degrees) {
        return degrees * (M_PI / 180.0f);
    }
    
    inline float radiansToDegrees(float radians) {
        return radians * (180.0f / M_PI);
    }
}
