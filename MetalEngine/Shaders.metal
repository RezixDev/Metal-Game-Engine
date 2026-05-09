// Shaders.metal
#include <metal_stdlib>
using namespace metal;

// ============================================================================
// Existing Mesh Rendering
// ============================================================================

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
    float3 normal   [[attribute(2)]];
    float2 texCoord [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 color;
    float3 normal;
    float2 texCoord;
    float3 worldPosition;
};

struct Uniforms {
    float4x4 modelViewProjection;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

vertex VertexOut vs_main(VertexIn in [[stage_in]],
                         constant Uniforms& uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position      = uniforms.modelViewProjection * float4(in.position, 1.0);
    float4 worldPos   = uniforms.model               * float4(in.position, 1.0);
    out.worldPosition = worldPos.xyz;
    out.normal        = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);
    out.color         = in.color;
    out.texCoord      = in.texCoord;
    return out;
}

fragment float4 fs_main_lit(VertexOut in [[stage_in]]) {
    constexpr float3 lightDirection = float3(0.5, -0.7, -0.5);
    constexpr float3 lightColor     = float3(1.0,  1.0,  1.0);
    constexpr float3 ambient        = float3(0.2,  0.2,  0.2);

    float3 normal      = normalize(in.normal);
    float  diffuse     = max(dot(normal, -normalize(lightDirection)), 0.0);
    float3 finalColor  = in.color * (ambient + diffuse * lightColor);
    return float4(finalColor, 1.0);
}

// ============================================================================
// Sentient Particle Sandbox — GPU-side data structures
// ============================================================================

struct Particle {
    float3 position;      // 16B (matches simd_float3)
    float3 velocity;      // 16B
    float3 acceleration;  // 16B
    float  mass;          //  4B
    uint   alive;         //  4B
    uint   padding[2];    //  8B = 64B total
};

struct Predator {
    float3 position;  // 16 bytes (matches simd_float3 alignment)
    float  radius;    //  4 bytes
    float  pad[3];    // 12 bytes padding to match CPU sizeof(Predator) = 32
};

// ============================================================================
// Compute Kernel: Update Predators
// ============================================================================

kernel void updatePredators(
    device Predator *predators     [[buffer(0)]],
    constant uint   &predatorCount [[buffer(1)]],
    constant float  &time          [[buffer(2)]],
    uint idx [[thread_position_in_grid]])
{
    if (idx >= predatorCount) return;

    // Each predator orbits the origin at a different radius and phase.
    float angle  = time * 0.5f + float(idx) * 1.2566f; // golden-ish spacing
    float radius = 8.0f + float(idx) * 2.0f;
    predators[idx].position = float3(cos(angle) * radius,
                                      sin(angle) * radius,
                                      0.0f);
    predators[idx].radius = 2.5f;
}

// ============================================================================
// Compute Kernel: Prepare Features for MPSGraph Neural Network
// ============================================================================
//
// Writes one float4 per particle: [cursor_dx, cursor_dy, noise, bias]
// These become the input tensor for the shared brain (MPSGraph inference).

kernel void prepareFeatures(
    const device Particle *particles    [[buffer(0)]],
    device float4         *features     [[buffer(1)]],
    constant float2       &cursorWorld  [[buffer(2)]],
    constant float        &deltaTime   [[buffer(3)]],
    constant uint         &particleCount [[buffer(4)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= particleCount) return;

    Particle p = particles[gid];

    if (p.alive == 0) {
        features[gid] = float4(0.0f);
        return;
    }

    float2 toCursor = cursorWorld - float2(p.position.x, p.position.y);
    float  noise    = fract(sin(float(gid) * 12.9898f + deltaTime * 100.0f) * 43758.5453f);
    features[gid]   = float4(toCursor.x, toCursor.y, noise, 1.0f);
}

// ============================================================================
// Compute Kernel: Apply Physics (reads MPSGraph output + predator avoidance)
// ============================================================================
//
// Reads the [N, 2] acceleration output from the neural network and applies
// predator avoidance, drag, speed clamping, boundary forces, and integration.

kernel void applyPhysics(
    device Particle       *particles     [[buffer(0)]],
    const device float2   *aiAccelIn     [[buffer(1)]],
    constant Predator     *predators     [[buffer(2)]],
    constant uint         &predatorCount [[buffer(3)]],
    constant float        &deltaTime     [[buffer(4)]],
    constant float        &maxSpeed      [[buffer(5)]],
    constant uint         &particleCount [[buffer(6)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= particleCount) return;

    Particle p = particles[gid];
    if (p.alive == 0) return;

    float3 pos = p.position;
    float3 vel = p.velocity;

    // ---- 1. Read neural network acceleration output ----
    float2 aiAccel = aiAccelIn[gid];

    // ---- 2. Predator avoidance (rule-based override) ----
    for (uint i = 0; i < predatorCount; ++i) {
        float3 diff = pos - float3(predators[i].position);
        float  dist = length(diff);
        float  rad  = predators[i].radius;
        if (dist < rad && dist > 0.0001f) {
            aiAccel += float2(diff.x, diff.y) * (rad - dist) * 2.0f;
        }
    }

    // ---- 3. Euler integration ----
    float3 accel = float3(aiAccel.x, aiAccel.y, 0.0f);
    const float drag = 0.98f;
    vel = vel * drag + accel * deltaTime;

    // Clamp speed
    float speed = length(vel);
    if (speed > maxSpeed)
        vel = normalize(vel) * maxSpeed;

    pos += vel * deltaTime;

    // Soft boundary — nudge particles back toward center if they wander too far
    const float boundary = 15.0f;
    if (length(float2(pos.x, pos.y)) > boundary) {
        float2 toCenter = -normalize(float2(pos.x, pos.y));
        vel.x += toCenter.x * deltaTime * 5.0f;
        vel.y += toCenter.y * deltaTime * 5.0f;
    }

    // Write back
    p.position = pos;
    p.velocity = vel;
    particles[gid] = p;
}

// ============================================================================
// Particle Rendering — Point Sprites
// ============================================================================

struct ParticleVertexOut {
    float4 position  [[position]];
    float4 color;
    float  pointSize [[point_size]];
};

vertex ParticleVertexOut vs_particle(
    const device Particle *particles             [[buffer(0)]],
    constant float4x4     &modelViewProjection   [[buffer(1)]],
    constant float4x4     &view                  [[buffer(2)]],
    uint vid [[vertex_id]])
{
    ParticleVertexOut out;
    Particle p = particles[vid];
    float3 pos = p.position;

    if (p.alive == 0) {
        out.position  = float4(0, 0, 0, 0);
        out.pointSize = 0;
        out.color     = float4(0);
        return out;
    }

    out.position = modelViewProjection * float4(pos, 1.0);

    // Color based on speed: blue (slow) → orange (fast)
    float3 vel   = p.velocity;
    float  speed = length(vel);
    float  t     = saturate(speed / 3.0f);
    float4 slowColor = float4(0.2, 0.55, 1.0, 1.0);
    float4 fastColor = float4(1.0, 0.4,  0.1, 1.0);
    out.color = mix(slowColor, fastColor, t);

    // Size shrinks with distance from camera
    float3 viewPos = (view * float4(pos, 1.0)).xyz;
    float  dist    = length(viewPos);
    out.pointSize  = clamp(300.0f / (dist + 1.0f), 1.0f, 8.0f);

    return out;
}

fragment float4 fs_particle(ParticleVertexOut in [[stage_in]],
                            float2 pointCoord [[point_coord]])
{
    // Soft glowing circle
    float2 uv   = pointCoord - 0.5;
    float  r    = length(uv);
    if (r > 0.5) discard_fragment();
    float  glow = smoothstep(0.5, 0.0, r);
    return in.color * glow;
}
