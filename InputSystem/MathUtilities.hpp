// MathUtilities.hpp
// Updated to use Core::Math while maintaining backward compatibility
#pragma once
#include "Core/Math.hpp"
#include <simd/simd.h>
#include <cmath>

// Backward compatibility wrapper
namespace Math {
    using namespace Core;
    
    class Matrix4 {
    public:
        static matrix_float4x4 perspective(float fovYRadians, float aspect, float nearZ, float farZ) {
            return Core::Matrix::perspective(fovYRadians, aspect, nearZ, farZ);
        }
        
        static matrix_float4x4 translation(float x, float y, float z) {
            return Core::Matrix::translation(x, y, z);
        }
        
        static matrix_float4x4 rotationY(float radians) {
            return Core::Matrix::rotationY(radians);
        }
        
        static matrix_float4x4 rotationX(float radians) {
            return Core::Matrix::rotationX(radians);
        }
        
        static matrix_float4x4 scale(float x, float y, float z) {
            return Core::Matrix::scale(x, y, z);
        }
        
        static matrix_float4x4 multiply(const matrix_float4x4& a, const matrix_float4x4& b) {
            return Core::Matrix::multiply(a, b);
        }
        
        static matrix_float4x4 identity() {
            return Core::Matrix::identity();
        }
    };
    
    inline float degreesToRadians(float degrees) {
        return Core::degreesToRadians(degrees);
    }
    
    inline float radiansToDegrees(float radians) {
        return Core::radiansToDegrees(radians);
    }
}