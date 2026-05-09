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
    float  trait;         //  4B
    uint   padding;       //  4B
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
    const device Particle *particles     [[buffer(0)]],
    device float          *features      [[buffer(1)]],
    constant float2       &cursorWorld   [[buffer(2)]],
    constant float        &deltaTime     [[buffer(3)]],
    constant uint         &particleCount [[buffer(4)]],
    constant float3       &audioDelta    [[buffer(5)]],
    constant uint         &enableCursor  [[buffer(6)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= particleCount) return;

    Particle p = particles[gid];

    if (p.alive == 0) {
        for (int i = 0; i < 7; i++) {
            features[gid * 7 + i] = 0.0f;
        }
        return;
    }

    float2 toCursor = float2(0.0f);
    if (enableCursor != 0) {
        toCursor = cursorWorld - float2(p.position.x, p.position.y);
    }
    
    float  noise    = fract(sin(float(gid) * 12.9898f + deltaTime * 100.0f) * 43758.5453f);
    
    features[gid * 7 + 0] = toCursor.x;
    features[gid * 7 + 1] = toCursor.y;
    features[gid * 7 + 2] = noise;
    features[gid * 7 + 3] = 1.0f;
    features[gid * 7 + 4] = audioDelta.x; // Bass Delta
    features[gid * 7 + 5] = audioDelta.y; // Mids Delta
    features[gid * 7 + 6] = audioDelta.z; // Highs Delta
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
    constant float3       &audioData     [[buffer(7)]],
    uint gid [[thread_position_in_grid]])
{
    if (gid >= particleCount) return;

    Particle p = particles[gid];
    if (p.alive == 0) return;

    float3 pos = p.position;
    float3 vel = p.velocity;

    // ---- 1. Read neural network acceleration output ----
    float2 aiAccelRaw = aiAccelIn[gid];
    
    // Scale neural intent by trait (Extroverts act more on it)
    float2 aiAccel = aiAccelRaw * mix(0.2f, 2.0f, p.trait);

    // ---- 2. Predator avoidance (rule-based override) ----
    for (uint i = 0; i < predatorCount; ++i) {
        float3 diff = pos - float3(predators[i].position);
        float  dist = length(diff);
        float  rad  = predators[i].radius;
        if (dist < rad && dist > 0.0001f) {
            aiAccel += float2(diff.x, diff.y) * (rad - dist) * 2.0f;
        }
    }

    float3 accel = float3(aiAccel.x, aiAccel.y, 0.0f);
    
    // ---- Beat Kick (Sudden velocity impulse & dash) ----
    float drag = 0.98f; // Viscous cooldown during quiet parts
    if (audioData.x > 0.6f) {
        drag = 0.90f;   // Less drag during loud parts for frantic movement
        vel *= 1.2f;    // Velocity multiplier (momentary surge)
        
        // Random directional dash
        float angle = fract(sin(float(gid) * 12.9898f + deltaTime * 100.0f) * 43758.5453f) * 6.2831853f;
        accel += float3(cos(angle), sin(angle), 0.0f) * 20.0f; 
    }

    // ---- 3. Euler integration ----
    vel = vel * drag + accel * deltaTime;

    // Clamp speed dynamically based on trait
    float adjustedMaxSpeed = maxSpeed * mix(0.5f, 2.0f, smoothstep(0.3f, 0.7f, p.trait));
    float speed = length(vel);
    if (speed > adjustedMaxSpeed)
        vel = normalize(vel) * adjustedMaxSpeed;

    pos += vel * deltaTime;

    // Hard Bounce boundary
    const float boundary = 20.0f;
    if (abs(pos.x) > boundary) {
        pos.x = boundary * sign(pos.x);
        vel.x = -abs(vel.x) * sign(pos.x);
    }
    if (abs(pos.y) > boundary) {
        pos.y = boundary * sign(pos.y);
        vel.y = -abs(vel.y) * sign(pos.y);
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
    float  pulseValue;
    float  trait;
};

vertex ParticleVertexOut vs_particle(
    const device Particle *particles             [[buffer(0)]],
    constant float4x4     &modelViewProjection   [[buffer(1)]],
    constant float4x4     &view                  [[buffer(2)]],
    constant float3       &audioData             [[buffer(3)]],
    uint vid [[vertex_id]])
{
    ParticleVertexOut out;
    Particle p = particles[vid];
    float3 pos = p.position;

    if (p.alive == 0) {
        out.position   = float4(0, 0, 0, 0);
        out.pointSize  = 0;
        out.color      = float4(0);
        out.pulseValue = 0.0f;
        out.trait      = 0.0f;
        return out;
    }

    out.position = modelViewProjection * float4(pos, 1.0);
    out.trait = p.trait;

    // Trait-Based Color Interpolation
    float3 introvertColor = mix(float3(0.0, 0.0, 0.8), float3(0.5, 0.0, 0.5), fract(float(vid) * 0.6180339887f));
    float3 extrovertColor = mix(float3(1.0, 0.0, 0.5), float3(1.0, 1.0, 0.0), fract(float(vid) * 0.6180339887f));
    float3 rgb = mix(introvertColor, extrovertColor, p.trait);
    out.color = float4(rgb, 1.0);

    // Staggered DNA Rhythm
    float pulse = 0.0f;
    uint modId = vid % 3;
    if (modId == 0) {
        pulse = audioData.x; // Bass
    } else if (modId == 1) {
        pulse = audioData.y; // Mids
    } else {
        pulse = audioData.z; // Highs
    }
    
    out.pulseValue = pulse;

    // Size shrinks with distance from camera, pulses heavily based on the particle's frequency band and trait
    float pulseScale = mix(0.2f, 5.0f, p.trait);
    float3 viewPos = (view * float4(pos, 1.0)).xyz;
    float  dist    = length(viewPos);
    out.pointSize  = clamp(2500.0f / (dist + 1.0f), 30.0f, 120.0f) * (1.0f + pulse * pulseScale);

    return out;
}

fragment float4 fs_particle(ParticleVertexOut in [[stage_in]],
                            float2 pointCoord [[point_coord]])
{
    // Soft glowing circle
    float2 uv   = pointCoord - 0.5;
    float  r    = length(uv);
    if (r > 0.5) discard_fragment();
    
    // Nucleus effect pulses heavily based on the particle's specific frequency and trait
    if (r < 0.15) {
        float nucleusIntensity = mix(1.0f, 6.0f, in.trait);
        return float4(min(in.color.rgb * (1.5 + in.pulseValue * nucleusIntensity), 1.0), 1.0);
    }

    float  glow = smoothstep(0.5, 0.0, r);
    return in.color * glow;
}
