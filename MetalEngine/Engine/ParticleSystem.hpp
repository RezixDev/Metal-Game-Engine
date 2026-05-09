// Engine/ParticleSystem.hpp
// Shared data structures for the Sentient Particle Sandbox.
// These layouts must match the corresponding structs in Shaders.metal.
#pragma once

#include <simd/simd.h>
#include <cstdint>

namespace Engine {

// One particle's persistent state — position, velocity, and alive flag.
// 64 bytes, matching the GPU packed layout.
struct Particle {
    simd_float3 position;      // 16B (simd-aligned)
    simd_float3 velocity;      // 16B
    simd_float3 acceleration;  // 16B (reserved)
    float       mass;          //  4B
    uint32_t    alive;         //  4B
    uint32_t    padding[2];    //  8B
};
static_assert(sizeof(Particle) == 64, "Particle must be 64 bytes");

// A tiny neural network for each particle.
// Network: 4 inputs → 4 hidden (ReLU) → 2 outputs (acceleration x, z).
// We use float4x4 for both layers (output layer wastes 2 rows but keeps
// alignment simple and lets us use a single matrix multiply).
struct Brain {
    matrix_float4x4 w1;   // 64B — hidden layer weights
    simd_float4     b1;   // 16B — hidden layer bias
    matrix_float4x4 w2;   // 64B — output layer weights (only .xy used)
    simd_float4     b2;   // 16B — output layer bias   (only .xy used)
};
static_assert(sizeof(Brain) == 160, "Brain must be 160 bytes");

// Predator — orbits the origin and scares nearby particles.
struct Predator {
    simd_float3 position;  // 12B + 4B padding (simd alignment)
    float       radius;    //  4B
};
static_assert(sizeof(Predator) == 32, "Predator must be 32 bytes");

// Constants
constexpr uint32_t kParticleCount = 500000;
constexpr uint32_t kPredatorCount = 5;

} // namespace Engine
