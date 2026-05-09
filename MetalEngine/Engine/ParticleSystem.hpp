// Engine/ParticleSystem.hpp
// Shared data structures for the Sentient Particle Sandbox.
// These layouts must match the corresponding structs in Shaders.metal.
#pragma once

#include <simd/simd.h>
#include <cstdint>

namespace Engine {

// One particle's persistent state — position, velocity, and alive flag.
// 64 bytes, matching the GPU float3 layout (simd_float3 = 16 bytes on Apple).
struct Particle {
    simd_float3 position;      // 16B (simd-aligned)
    simd_float3 velocity;      // 16B
    simd_float3 acceleration;  // 16B (reserved)
    float       mass;          //  4B
    uint32_t    alive;         //  4B
    uint32_t    padding[2];    //  8B
};
static_assert(sizeof(Particle) == 64, "Particle must be 64 bytes");

// Predator — orbits the origin and scares nearby particles.
struct Predator {
    simd_float3 position;  // 12B + 4B padding (simd alignment)
    float       radius;    //  4B
};
static_assert(sizeof(Predator) == 32, "Predator must be 32 bytes");

// NOTE: Brain struct removed — neural network weights now live in MPSGraph
// as shared tensors (all particles use the same brain, mutated in real-time).

// Constants
constexpr uint32_t kParticleCount = 500000;
constexpr uint32_t kPredatorCount = 5;
constexpr uint32_t kInputSize     = 4;   // [cursor_dx, cursor_dy, noise, bias]
constexpr uint32_t kHiddenSize    = 16;  // wider hidden layer for shared brain
constexpr uint32_t kOutputSize    = 2;   // [accel_x, accel_y]

} // namespace Engine
