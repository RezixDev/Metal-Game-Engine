# Project Source Code

## File Tree
```
- CMakeLists.txt
- MetalEngine
  - AppDelegate.h
  - AppDelegate.m
  - Engine
    - Camera.hpp
    - Geometry.hpp
    - Math.hpp
    - ParticleSystem.hpp
    - Time.hpp
  - MetalRenderer.h
  - MetalRenderer.mm
  - MyMetalView.h
  - MyMetalView.mm
  - Shaders.metal
  - main.m
- run.sh
```

## `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)

project(MetalEngine VERSION 1.0.0 LANGUAGES CXX C OBJCXX OBJC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET "26.0")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT APPLE)
    message(FATAL_ERROR "MetalEngine requires macOS")
endif()

enable_language(OBJC)
enable_language(OBJCXX)

find_library(COCOA_FRAMEWORK      Cocoa      REQUIRED)
find_library(METAL_FRAMEWORK      Metal      REQUIRED)
find_library(METALKIT_FRAMEWORK   MetalKit   REQUIRED)
find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(MPS_FRAMEWORK        MetalPerformanceShaders      REQUIRED)
find_library(MPSGRAPH_FRAMEWORK   MetalPerformanceShadersGraph REQUIRED)
find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)
find_library(ACCELERATE_FRAMEWORK   Accelerate   REQUIRED)

add_executable(${PROJECT_NAME} MACOSX_BUNDLE
        MetalEngine/main.m
        MetalEngine/AppDelegate.m
        MetalEngine/MyMetalView.mm
        MetalEngine/MetalRenderer.mm
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${COCOA_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${METALKIT_FRAMEWORK}
        ${QUARTZCORE_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        ${MPS_FRAMEWORK}
        ${MPSGRAPH_FRAMEWORK}
        ${AVFOUNDATION_FRAMEWORK}
        ${ACCELERATE_FRAMEWORK}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
        OBJCXX_STANDARD 17
        CXX_STANDARD 17
        LINKER_LANGUAGE OBJCXX
)

target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall -Wextra -Wno-unused-parameter
        $<$<COMPILE_LANGUAGE:OBJC,OBJCXX>:-fobjc-arc>
)

# ===================================
# Metal Shader Compilation
# ===================================

# Compile Shaders.metal into default.metallib so [MTLDevice newDefaultLibrary]
# finds it inside the app bundle.
add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
        COMMAND xcrun -sdk macosx metal -std=metal4.0 -gline-tables-only -frecord-sources -c
                ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Shaders.metal
                -o ${CMAKE_CURRENT_BINARY_DIR}/Shaders.air
        COMMAND xcrun -sdk macosx metallib ${CMAKE_CURRENT_BINARY_DIR}/Shaders.air
                -o ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Shaders.metal
        COMMENT "Compiling Metal shaders into default.metallib"
)

add_custom_target(CompileShaders ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
)

add_dependencies(${PROJECT_NAME} CompileShaders)

# ===================================
# Copy resources to bundle
# ===================================

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_BINARY_DIR}/default.metallib
        $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/
)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Assets.xcassets)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/MetalEngine/Assets.xcassets
            $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/Assets.xcassets
    )
endif()

add_custom_target(run
        COMMAND $<TARGET_FILE:${PROJECT_NAME}>
        DEPENDS ${PROJECT_NAME}
        USES_TERMINAL
        COMMENT "Running ${PROJECT_NAME}"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0 -DDEBUG)
endif()

```

## `MetalEngine/AppDelegate.h`

```cpp
// AppDelegate.h
#import <AppKit/AppKit.h>

@class MyMetalView;

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow *window;
@property (strong) MyMetalView *metalView;
@end

```

## `MetalEngine/AppDelegate.m`

```cpp
// AppDelegate.m
#import "AppDelegate.h"
#import "MyMetalView.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect frame = NSMakeRect(0, 0, 960, 800);
    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled
                                | NSWindowStyleMaskClosable
                                | NSWindowStyleMaskResizable
                                | NSWindowStyleMaskMiniaturizable;

    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:styleMask
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    self.window.title = @"MetalEngine";
    self.window.minSize = NSMakeSize(400, 300);
    [self.window center];

    NSRect viewFrame = ((NSView *)self.window.contentView).bounds;
    self.metalView = [[MyMetalView alloc] initWithFrame:viewFrame];
    self.metalView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

    self.window.contentView = self.metalView;
    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.metalView];

    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

@end

```

## `MetalEngine/Engine/Camera.hpp`

```cpp
// Engine/Camera.hpp
#pragma once

#include "Math.hpp"

namespace Engine {

// Minimal FPS-style perspective camera. State is just position, yaw, and pitch.
class Camera {
public:
    Camera(Math::Vec3 position = {0.0f, 0.0f, 5.0f},
           Math::Vec3 target   = {0.0f, 0.0f, 0.0f},
           Math::Vec3 up       = Math::Vector::Constants::UP) {
        lookAt(position, target, up);
    }

    Math::Mat4 getViewMatrix() const {
        return Math::Matrix::lookAt(position_, position_ + getForward(), worldUp_);
    }

    Math::Mat4 getProjectionMatrix(float aspect) const {
        return Math::Matrix::perspectiveDegrees(fov_, aspect, nearPlane_, farPlane_);
    }

    void lookAt(const Math::Vec3& position,
                const Math::Vec3& target,
                const Math::Vec3& up) {
        position_ = position;
        worldUp_  = Math::Vector::normalize(up);

        Math::Vec3 forward = Math::Vector::normalize(target - position);
        yaw_   = Math::radiansToDegrees(atan2f(forward.z, forward.x));
        pitch_ = Math::radiansToDegrees(asinf(forward.y));
    }

    void setFieldOfView(float fov)        { fov_ = Math::clamp(fov, 1.0f, 179.0f); }
    void setNearPlane(float n)            { nearPlane_ = n; }
    void setFarPlane(float f)             { farPlane_ = f; }
    void setMovementSpeed(float s)        { movementSpeed_ = s; }
    void setMouseSensitivity(float s)     { mouseSensitivity_ = s; }

    Math::Vec3 getPosition() const { return position_; }

    Math::Vec3 getForward() const {
        const float yawR   = Math::degreesToRadians(yaw_);
        const float pitchR = Math::degreesToRadians(pitch_);
        return Math::Vector::normalize(Math::Vec3{
            cosf(yawR) * cosf(pitchR),
            sinf(pitchR),
            sinf(yawR) * cosf(pitchR),
        });
    }

    Math::Vec3 getRight() const {
        return Math::Vector::normalize(Math::Vector::cross(getForward(), worldUp_));
    }

    void processMovement(float deltaTime,
                         bool forward, bool backward,
                         bool left,    bool right,
                         bool up,      bool down) {
        const float v = movementSpeed_ * deltaTime;
        const Math::Vec3 fwd = getForward();
        const Math::Vec3 rgt = getRight();
        Math::Vec3 movement  = Math::Vector::Constants::ZERO;
        if (forward)  movement += fwd     * v;
        if (backward) movement -= fwd     * v;
        if (right)    movement += rgt     * v;
        if (left)     movement -= rgt     * v;
        if (up)       movement += worldUp_ * v;
        if (down)     movement -= worldUp_ * v;
        position_ += movement;
    }

    void processMouseMovement(float deltaX, float deltaY) {
        yaw_   += deltaX * mouseSensitivity_ * Math::Constants::RAD_TO_DEG;
        pitch_ += deltaY * mouseSensitivity_ * Math::Constants::RAD_TO_DEG;
        pitch_  = Math::clamp(pitch_, -89.0f, 89.0f);
    }

private:
    Math::Vec3 position_{0.0f, 0.0f, 5.0f};
    Math::Vec3 worldUp_ = Math::Vector::Constants::UP;
    float yaw_   = -90.0f;
    float pitch_ = 0.0f;

    float fov_              = 60.0f;
    float nearPlane_        = 0.1f;
    float farPlane_         = 100.0f;
    float movementSpeed_    = 5.0f;
    float mouseSensitivity_ = 0.002f;
};

} // namespace Engine

```

## `MetalEngine/Engine/Geometry.hpp`

```cpp
// Engine/Geometry.hpp
// Builders for primitive meshes. The Vertex layout matches the vertex
// descriptor configured in MetalRenderer (stride 64, simd_float3 alignment).
#pragma once

#include "Math.hpp"
#include <vector>
#include <cmath>
#include <cstdint>

namespace Engine {

struct Vertex {
    Math::Vec3 position;
    Math::Vec3 color;
    Math::Vec3 normal;
    Math::Vec2 texCoord;
};

struct MeshData {
    std::vector<Vertex>   vertices;
    std::vector<uint16_t> indices;
};

namespace Geometry {

inline MeshData createCube(float size = 1.0f) {
    MeshData m;
    const float h = size * 0.5f;

    const Math::Vec3 corners[8] = {
        {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},
        {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h},
    };
    const int faces[6][4] = {
        {0, 1, 2, 3}, {5, 4, 7, 6}, {4, 0, 3, 7},
        {1, 5, 6, 2}, {3, 2, 6, 7}, {4, 5, 1, 0},
    };
    const Math::Vec3 colors[6] = {
        {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
        {1, 1, 0}, {1, 0, 1}, {0, 1, 1},
    };
    const Math::Vec3 normals[6] = {
        { 0,  0, -1}, { 0,  0,  1}, {-1,  0,  0},
        { 1,  0,  0}, { 0,  1,  0}, { 0, -1,  0},
    };

    for (int f = 0; f < 6; ++f) {
        const uint16_t base = static_cast<uint16_t>(m.vertices.size());
        for (int v = 0; v < 4; ++v) {
            m.vertices.push_back({
                corners[faces[f][v]],
                colors[f],
                normals[f],
                {(v == 1 || v == 2) ? 1.0f : 0.0f,
                 (v == 2 || v == 3) ? 1.0f : 0.0f},
            });
        }
        m.indices.insert(m.indices.end(), {
            uint16_t(base + 0), uint16_t(base + 1), uint16_t(base + 2),
            uint16_t(base + 0), uint16_t(base + 2), uint16_t(base + 3),
        });
    }
    return m;
}

inline MeshData createSphere(float radius = 1.0f, int segments = 32, int rings = 16) {
    MeshData m;

    for (int ring = 0; ring <= rings; ++ring) {
        const float theta = Math::Constants::PI * ring / rings;
        const float st = sinf(theta), ct = cosf(theta);
        for (int seg = 0; seg <= segments; ++seg) {
            const float phi = 2.0f * Math::Constants::PI * seg / segments;
            const Math::Vec3 pos{
                radius * st * cosf(phi),
                radius * ct,
                radius * st * sinf(phi),
            };
            m.vertices.push_back({
                pos,
                {(pos.x + radius) / (2 * radius),
                 (pos.y + radius) / (2 * radius),
                 (pos.z + radius) / (2 * radius)},
                Math::Vector::normalize(pos),
                {static_cast<float>(seg) / segments,
                 static_cast<float>(ring) / rings},
            });
        }
    }
    for (int ring = 0; ring < rings; ++ring) {
        for (int seg = 0; seg < segments; ++seg) {
            const uint16_t a = ring * (segments + 1) + seg;
            const uint16_t b = a + segments + 1;
            m.indices.insert(m.indices.end(),
                {a, b, uint16_t(a + 1), b, uint16_t(b + 1), uint16_t(a + 1)});
        }
    }
    return m;
}

inline MeshData createPlane(float width = 10.0f, float depth = 10.0f, int subdivisions = 1) {
    MeshData m;
    const int verts  = subdivisions + 2;
    const float stepX = width / (verts - 1);
    const float stepZ = depth / (verts - 1);

    for (int z = 0; z < verts; ++z) {
        for (int x = 0; x < verts; ++x) {
            m.vertices.push_back({
                {-width * 0.5f + x * stepX, 0.0f, -depth * 0.5f + z * stepZ},
                {0.5f, 0.5f, 0.5f},
                {0, 1, 0},
                {static_cast<float>(x) / (verts - 1),
                 static_cast<float>(z) / (verts - 1)},
            });
        }
    }
    for (int z = 0; z < verts - 1; ++z) {
        for (int x = 0; x < verts - 1; ++x) {
            const uint16_t tl = z * verts + x;
            const uint16_t tr = tl + 1;
            const uint16_t bl = tl + verts;
            const uint16_t br = bl + 1;
            m.indices.insert(m.indices.end(), {tl, bl, tr, tr, bl, br});
        }
    }
    return m;
}

} // namespace Geometry
} // namespace Engine

```

## `MetalEngine/Engine/Math.hpp`

```cpp
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

```

## `MetalEngine/Engine/ParticleSystem.hpp`

```cpp
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
constexpr uint32_t kParticleCount = 20;
constexpr uint32_t kPredatorCount = 5;
constexpr uint32_t kInputSize     = 7;   // [cursor_dx, cursor_dy, noise, bias, Bass, Mids, Highs]
constexpr uint32_t kHiddenSize    = 16;  // wider hidden layer for shared brain
constexpr uint32_t kOutputSize    = 2;   // [accel_x, accel_y]

} // namespace Engine

```

## `MetalEngine/Engine/Time.hpp`

```cpp
// Engine/Time.hpp
#pragma once

#include <chrono>
#include <algorithm>

namespace Engine {

// Per-frame delta-time clock. Construct, call tick() once per frame, get dt.
class FrameClock {
public:
    float tick() {
        const auto now = Clock::now();
        const float dt = std::chrono::duration<float>(now - last_).count();
        last_ = now;
        // Cap dt so a long pause doesn't catapult the camera across the scene.
        return std::min(dt, 1.0f / 30.0f);
    }

private:
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point last_ = Clock::now();
};

} // namespace Engine

```

## `MetalEngine/MetalRenderer.h`

```cpp
// MetalRenderer.h
#pragma once

#import <MetalKit/MetalKit.h>

@interface MetalRenderer : NSObject <MTKViewDelegate>

- (instancetype)initWithMTKView:(MTKView *)view;

// Input — driven by the host view.
- (void)keyDown:(unsigned short)keyCode;
- (void)keyUp:(unsigned short)keyCode;
- (void)setMouseLookEnabled:(BOOL)enabled;
- (void)processMouseDeltaX:(float)dx deltaY:(float)dy;

// Particle sandbox — cursor tracking for particle flocking.
- (void)setCursorScreenPosition:(CGPoint)pt viewSize:(CGSize)viewSize;

// Brain evolution — perturb weights without recompiling the MPSGraph executable.
- (void)applyEvolutionStep:(float)mutationRate;

@end

```

## `MetalEngine/MetalRenderer.mm`

```cpp
// MetalRenderer.mm
#import "MetalRenderer.h"

#include "Engine/Camera.hpp"
#include "Engine/Geometry.hpp"
#include "Engine/ParticleSystem.hpp"
#include "Engine/Time.hpp"
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <MetalPerformanceShadersGraph/MetalPerformanceShadersGraph.h>
#import <AVFoundation/AVFoundation.h>
#import <Accelerate/Accelerate.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

// Marsaglia polar-method Gaussian sampler — zero mean, unit variance.
static float gaussianRandom() {
  static bool haveSpare = false;
  static float spare;
  if (haveSpare) {
    haveSpare = false;
    return spare;
  }
  haveSpare = true;
  float u, v, s;
  do {
    u = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    v = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    s = u * u + v * v;
  } while (s >= 1.0f || s == 0.0f);
  float mul = sqrtf(-2.0f * logf(s) / s);
  spare = v * mul;
  return u * mul;
}

// macOS virtual key codes used by the camera controls.
constexpr unsigned short kVK_ANSI_A = 0x00;
constexpr unsigned short kVK_ANSI_S = 0x01;
constexpr unsigned short kVK_ANSI_D = 0x02;
constexpr unsigned short kVK_ANSI_Q = 0x0C;
constexpr unsigned short kVK_ANSI_W = 0x0D;
constexpr unsigned short kVK_ANSI_R = 0x0F;
constexpr unsigned short kVK_ANSI_1 = 0x12;
constexpr unsigned short kVK_ANSI_2 = 0x13;
constexpr unsigned short kVK_Space = 0x31;
constexpr unsigned short kVK_ANSI_C = 0x08;

// 2D front-facing debug view — camera looks directly at the XY plane.
// To restore 3D perspective: position={0,2,5}, target={0,0,0},
// kLockCamera=false.
constexpr Engine::Math::Vec3 kInitialPosition = {0.0f, 0.0f, 25.0f};
constexpr Engine::Math::Vec3 kInitialTarget = {0.0f, 0.0f, 0.0f};
constexpr bool kLockCamera = true; // disable WASD + mouse-look for 2D debug

struct InputState {
  bool moveForward = false;
  bool moveBackward = false;
  bool moveLeft = false;
  bool moveRight = false;
  bool moveUp = false;
  bool moveDown = false;
  bool mouseLookEnabled = false;
};

// Layout matches the Uniforms struct in Shaders.metal.
struct Uniforms {
  matrix_float4x4 modelViewProjection;
  matrix_float4x4 model;
  matrix_float4x4 view;
  matrix_float4x4 projection;
};

// One renderable scene object — GPU buffers + a per-object world transform.
struct SceneObject {
  id<MTLBuffer> vertexBuffer;
  id<MTLBuffer> indexBuffer;
  NSUInteger indexCount;
  matrix_float4x4 transform;
};

} // namespace

@implementation MetalRenderer {
  id<MTLDevice> _device;
  id<MTLCommandQueue> _commandQueue;
  id<MTLRenderPipelineState> _pipelineState;
  id<MTLDepthStencilState> _depthStencilState;

  std::vector<SceneObject> _scene;
  Engine::Camera _camera;
  Engine::FrameClock _clock;
  InputState _input;

  CGSize _drawableSize;

  // ---- Particle Sandbox ----
  id<MTLBuffer> _particleBuffer;
  id<MTLBuffer> _predatorBuffer;
  id<MTLComputePipelineState> _computePipelinePrepare;
  id<MTLComputePipelineState> _computePipelinePhysics;
  id<MTLComputePipelineState> _computePipelinePredators;
  id<MTLRenderPipelineState> _particleRenderPipeline;

  // ---- MPSGraph ----
  MPSGraph *_brainGraph;
  MPSGraphExecutable *_brainExecutable;
  id<MTLBuffer> _featureBuffer;
  id<MTLBuffer> _accelBuffer;
  // Persistent weight buffers — Shared mode so CPU can mutate them without
  // recompiling the graph. GPU reads them each frame via MPSGraphTensorData.
  id<MTLBuffer> _weightBufferL1; // [kInputSize  × kHiddenSize]
  id<MTLBuffer> _weightBufferL2; // [kHiddenSize × kOutputSize]
  MPSGraphTensor *_inputTensor;
  MPSGraphTensor *_w1Tensor;
  MPSGraphTensor *_w2Tensor;
  MPSGraphTensor *_outputTensor;

  NSUInteger _particleCount;
  simd_float2 _cursorWorld;
  float _elapsedTime;
  CGSize _viewSize; // in points (not pixels)

  // FPS counter
  int _frameCount;
  float _fpsAccumulator;
  float _currentFPS;

  // ---- Audio Reactivity ----
  AVAudioEngine *_audioEngine;
  simd_float3 _audioBands; // Bass, Mids, Highs
  
  FFTSetup _fftSetup;
  DSPSplitComplex _fftComplex;
  float *_fftWindow;
  float *_fftMag;
  uint32_t _log2n;
  uint32_t _n;
  uint32_t _nOver2;
  
  BOOL _enableCursorAttraction;
}

// ---- Setup ----------------------------------------------------------------

- (instancetype)initWithMTKView:(MTKView *)view {
  self = [super init];
  if (!self)
    return nil;

  _device = view.device;
  _commandQueue = [_device newCommandQueue];
  _drawableSize = view.drawableSize;

  _camera = Engine::Camera(kInitialPosition, kInitialTarget,
                           Engine::Math::Vector::Constants::UP);
  _camera.setFieldOfView(60.0f);
  _camera.setNearPlane(0.1f);
  _camera.setFarPlane(100.0f);
  _camera.setMovementSpeed(5.0f);
  _camera.setMouseSensitivity(0.002f);

  [self buildPipelineForView:view];
  [self buildComputePipelines];
  [self buildParticleRenderPipeline:view];
  [self buildScene];
  [self initParticleData];
  [self setupAudioEngine];

  _elapsedTime = 0.0f;
  _cursorWorld = simd_make_float2(0, 0);
  _enableCursorAttraction = NO;

  return self;
}

- (void)setupAudioEngine {
    _audioEngine = [[AVAudioEngine alloc] init];
    AVAudioInputNode *inputNode = _audioEngine.inputNode;
    AVAudioFormat *format = [inputNode inputFormatForBus:0];
    
    // Request permission explicitly (helps with initial launch)
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeAudio completionHandler:^(BOOL granted) {
        if (!granted) {
            NSLog(@"Microphone access denied.");
        }
    }];
    
    __weak MetalRenderer *weakSelf = self;
    [inputNode installTapOnBus:0 bufferSize:4096 format:format block:^(AVAudioPCMBuffer *buffer, AVAudioTime *when) {
        [weakSelf processAudioBuffer:buffer];
    }];
    
    NSError *error;
    [_audioEngine startAndReturnError:&error];
    if (error) {
        NSLog(@"Audio engine failed to start: %@", error);
    }
}

- (void)processAudioBuffer:(AVAudioPCMBuffer *)buffer {
    uint32_t len = buffer.frameLength;
    if (len == 0) return;
    
    uint32_t log2n = log2f((float)len);
    uint32_t n = 1 << log2n; // Use next lower power of 2
    if (n > 8192) n = 8192;  // Cap
    
    uint32_t nOver2 = n / 2;
    
    if (_n != n) {
        if (_fftSetup) vDSP_destroy_fftsetup(_fftSetup);
        if (_fftComplex.realp) free(_fftComplex.realp);
        if (_fftComplex.imagp) free(_fftComplex.imagp);
        if (_fftWindow) free(_fftWindow);
        if (_fftMag) free(_fftMag);
        
        _n = n;
        _log2n = log2n;
        _nOver2 = nOver2;
        
        _fftSetup = vDSP_create_fftsetup(_log2n, kFFTRadix2);
        _fftComplex.realp = (float *)malloc(_nOver2 * sizeof(float));
        _fftComplex.imagp = (float *)malloc(_nOver2 * sizeof(float));
        _fftWindow = (float *)malloc(_n * sizeof(float));
        _fftMag = (float *)malloc(_nOver2 * sizeof(float));
        
        vDSP_hann_window(_fftWindow, _n, vDSP_HALF_WINDOW);
    }
    
    float *channelData = buffer.floatChannelData[0];
    std::vector<float> windowedData(_n);
    vDSP_vmul(channelData, 1, _fftWindow, 1, windowedData.data(), 1, _n);
    
    vDSP_ctoz((DSPComplex *)windowedData.data(), 2, &_fftComplex, 1, _nOver2);
    vDSP_fft_zrip(_fftSetup, &_fftComplex, 1, _log2n, FFT_FORWARD);
    vDSP_zvmags(&_fftComplex, 1, _fftMag, 1, _nOver2);
    
    // Frequency bands (assuming ~44.1kHz rate => Nyquist 22kHz)
    int bassEnd = (int)(0.011f * _nOver2);
    int midsEnd = (int)(0.18f * _nOver2);
    int highsEnd = (int)(0.90f * _nOver2);
    
    if (bassEnd < 1) bassEnd = 1;
    if (midsEnd <= bassEnd) midsEnd = bassEnd + 1;
    if (highsEnd <= midsEnd) highsEnd = midsEnd + 1;
    
    float bass = 0.0f;
    for(int i = 1; i < bassEnd; i++) bass += _fftMag[i];
    bass /= (float)(bassEnd - 1 + 1e-5f);
    
    float mids = 0.0f;
    for(int i = bassEnd; i < midsEnd; i++) mids += _fftMag[i];
    mids /= (float)(midsEnd - bassEnd + 1e-5f);
    
    float highs = 0.0f;
    for(int i = midsEnd; i < highsEnd; i++) highs += _fftMag[i];
    highs /= (float)(highsEnd - midsEnd + 1e-5f);
    
    // Scale and normalize (adjust multiplier if needed based on mic sensitivity)
    bass = fminf(sqrtf(bass) * 0.20f, 1.0f);
    mids = fminf(sqrtf(mids) * 0.20f, 1.0f);
    highs = fminf(sqrtf(highs) * 0.20f, 1.0f);
    
    // Thread safety: basic atomic-like read/write of simd_float3
    _audioBands = simd_make_float3(
        _audioBands.x * 0.7f + bass * 0.3f,
        _audioBands.y * 0.7f + mids * 0.3f,
        _audioBands.z * 0.7f + highs * 0.3f
    );
}

- (void)buildPipelineForView:(MTKView *)view {
  id<MTLLibrary> library = [_device newDefaultLibrary];
  if (!library) {
    [NSException raise:@"MetalRenderer"
                format:@"Failed to load default.metallib from app bundle"];
  }

  MTLVertexDescriptor *vd = [[MTLVertexDescriptor alloc] init];
  // The Vertex struct uses simd_float3 (16-byte aligned) for position/color/
  // normal and simd_float2 for texCoord, packed in declaration order.
  vd.attributes[0].format = MTLVertexFormatFloat3;
  vd.attributes[0].offset = 0; // position
  vd.attributes[1].format = MTLVertexFormatFloat3;
  vd.attributes[1].offset = 16; // color
  vd.attributes[2].format = MTLVertexFormatFloat3;
  vd.attributes[2].offset = 32; // normal
  vd.attributes[3].format = MTLVertexFormatFloat2;
  vd.attributes[3].offset = 48;                  // texCoord
  vd.layouts[0].stride = sizeof(Engine::Vertex); // 64
  vd.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

  MTLRenderPipelineDescriptor *pd = [[MTLRenderPipelineDescriptor alloc] init];
  pd.label = @"MainPipeline";
  pd.vertexFunction = [library newFunctionWithName:@"vs_main"];
  pd.fragmentFunction = [library newFunctionWithName:@"fs_main_lit"];
  pd.vertexDescriptor = vd;
  pd.colorAttachments[0].pixelFormat = view.colorPixelFormat;
  pd.depthAttachmentPixelFormat = view.depthStencilPixelFormat;

  NSError *error = nil;
  _pipelineState = [_device newRenderPipelineStateWithDescriptor:pd
                                                           error:&error];
  if (!_pipelineState) {
    [NSException raise:@"MetalRenderer"
                format:@"Pipeline creation failed: %@", error];
  }

  MTLDepthStencilDescriptor *dsd = [[MTLDepthStencilDescriptor alloc] init];
  dsd.depthCompareFunction = MTLCompareFunctionLess;
  dsd.depthWriteEnabled = YES;
  _depthStencilState = [_device newDepthStencilStateWithDescriptor:dsd];
}

- (void)buildComputePipelines {
  id<MTLLibrary> lib = [_device newDefaultLibrary];
  NSError *error = nil;

  id<MTLFunction> fnPrepare = [lib newFunctionWithName:@"prepareFeatures"];
  _computePipelinePrepare =
      [_device newComputePipelineStateWithFunction:fnPrepare error:&error];
  if (!_computePipelinePrepare)
    [NSException raise:@"MetalRenderer"
                format:@"Failed to create prepareFeatures pipeline: %@", error];

  id<MTLFunction> fnPhysics = [lib newFunctionWithName:@"applyPhysics"];
  _computePipelinePhysics =
      [_device newComputePipelineStateWithFunction:fnPhysics error:&error];
  if (!_computePipelinePhysics)
    [NSException raise:@"MetalRenderer"
                format:@"Failed to create applyPhysics pipeline: %@", error];

  id<MTLFunction> fnPredators = [lib newFunctionWithName:@"updatePredators"];
  _computePipelinePredators =
      [_device newComputePipelineStateWithFunction:fnPredators error:&error];
  if (!_computePipelinePredators) {
    [NSException
         raise:@"MetalRenderer"
        format:@"Failed to create predator compute pipeline: %@", error];
  }
}

- (void)buildParticleRenderPipeline:(MTKView *)view {
  id<MTLLibrary> lib = [_device newDefaultLibrary];

  MTLRenderPipelineDescriptor *desc =
      [[MTLRenderPipelineDescriptor alloc] init];
  desc.label = @"ParticlePipeline";
  desc.vertexFunction = [lib newFunctionWithName:@"vs_particle"];
  desc.fragmentFunction = [lib newFunctionWithName:@"fs_particle"];
  desc.colorAttachments[0].pixelFormat = view.colorPixelFormat;
  desc.depthAttachmentPixelFormat = view.depthStencilPixelFormat;
  // No vertex descriptor — particle data comes from buffer(0) indexed by
  // vertex_id.
  desc.inputPrimitiveTopology = MTLPrimitiveTopologyClassPoint;

  // Enable alpha blending for the glow effect
  desc.colorAttachments[0].blendingEnabled = YES;
  desc.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
  desc.colorAttachments[0].destinationRGBBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  desc.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
  desc.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
  desc.colorAttachments[0].destinationAlphaBlendFactor =
      MTLBlendFactorOneMinusSourceAlpha;
  desc.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;

  NSError *error = nil;
  _particleRenderPipeline =
      [_device newRenderPipelineStateWithDescriptor:desc error:&error];
  if (!_particleRenderPipeline) {
    [NSException raise:@"MetalRenderer"
                format:@"Failed to create particle render pipeline: %@", error];
  }
}

- (void)initParticleData {
  _particleCount = Engine::kParticleCount;

  // ---- Particles: random positions on the ground plane ----
  {
    _particleBuffer =
        [_device newBufferWithLength:_particleCount * sizeof(Engine::Particle)
                             options:MTLResourceStorageModeShared];
    _particleBuffer.label = @"ParticleBuffer";
  }

  // ---- MPSGraph Buffers ----
  NSUInteger featureBytes = _particleCount * Engine::kInputSize * sizeof(float);
  _featureBuffer = [_device newBufferWithLength:featureBytes
                                        options:MTLResourceStorageModePrivate];
  _featureBuffer.label = @"FeatureBuffer";

  NSUInteger accelBytes = _particleCount * Engine::kOutputSize * sizeof(float);
  _accelBuffer = [_device newBufferWithLength:accelBytes
                                      options:MTLResourceStorageModePrivate];
  _accelBuffer.label = @"AccelBuffer";

  // ---- Weight Buffers (Shared — CPU-mutable, GPU-readable) ----
  const NSUInteger l1Count = Engine::kInputSize * Engine::kHiddenSize;
  const NSUInteger l2Count = Engine::kHiddenSize * Engine::kOutputSize;

  _weightBufferL1 = [_device newBufferWithLength:l1Count * sizeof(float)
                                         options:MTLResourceStorageModeShared];
  _weightBufferL1.label = @"WeightBufferL1";

  _weightBufferL2 = [_device newBufferWithLength:l2Count * sizeof(float)
                                         options:MTLResourceStorageModeShared];
  _weightBufferL2.label = @"WeightBufferL2";

  // Build and compile the graph once — weights injected per-frame via feeds.
  [self buildBrainGraph];

  // ---- Predators: initial positions in orbit ----
  {
    _predatorBuffer = [_device
        newBufferWithLength:Engine::kPredatorCount * sizeof(Engine::Predator)
                    options:MTLResourceStorageModeShared];
    _predatorBuffer.label = @"PredatorBuffer";
  }

  [self resetSimulation];

  NSLog(@"🧠 Particle Sandbox: %lu particles, %u predators initialised",
        (unsigned long)_particleCount, Engine::kPredatorCount);
}

- (void)resetSimulation {
  _elapsedTime = 0.0f;
  _camera.lookAt(kInitialPosition, kInitialTarget,
                 Engine::Math::Vector::Constants::UP);

  Engine::Particle *particles = (Engine::Particle *)_particleBuffer.contents;
  srand(42);
  for (NSUInteger i = 0; i < _particleCount; ++i) {
    particles[i].position = {((rand() / (float)RAND_MAX) * 30.0f) - 15.0f,
                             ((rand() / (float)RAND_MAX) * 30.0f) - 15.0f, 0.0f};
    particles[i].velocity = {0, 0, 0};
    particles[i].acceleration = {0, 0, 0};
    particles[i].mass = 1.0f;
    particles[i].alive = 1;
    particles[i].padding[0] = 0;
    particles[i].padding[1] = 0;
  }

  const NSUInteger l1Count = Engine::kInputSize * Engine::kHiddenSize;
  const NSUInteger l2Count = Engine::kHiddenSize * Engine::kOutputSize;

  float *w1 = (float *)_weightBufferL1.contents;
  for (NSUInteger i = 0; i < l1Count; ++i)
    w1[i] = gaussianRandom() * 0.1f;
  // Hard-wire cursor→hidden sensitivity so particles respond from frame one.
  w1[0] = 1.0f;                           // input[0] (cursor x) → hidden[0]
  w1[1 * Engine::kHiddenSize + 1] = 1.0f; // input[1] (cursor y) → hidden[1]
  w1[4 * Engine::kHiddenSize + 2] = 2.0f; // input[4] (bass) -> hidden[2]
  w1[5 * Engine::kHiddenSize + 3] = 2.0f; // input[5] (mids) -> hidden[3]
  w1[6 * Engine::kHiddenSize + 4] = 2.0f; // input[6] (highs) -> hidden[4]

  float *w2 = (float *)_weightBufferL2.contents;
  for (NSUInteger i = 0; i < l2Count; ++i)
    w2[i] = gaussianRandom() * 0.1f;
  // Hard-wire hidden→output so accel_x and accel_y are live from the start.
  w2[0] = 1.0f;                           // hidden[0] → accel_x
  w2[1 * Engine::kOutputSize + 1] = 1.0f; // hidden[1] → accel_y
  w2[2 * Engine::kOutputSize + 0] = 1.0f; // hidden[2] -> accel_x
  w2[3 * Engine::kOutputSize + 1] = 1.0f; // hidden[3] -> accel_y
  w2[4 * Engine::kOutputSize + 0] = -1.0f; // hidden[4] -> accel_x

  Engine::Predator *predators = (Engine::Predator *)_predatorBuffer.contents;
  for (uint32_t i = 0; i < Engine::kPredatorCount; ++i) {
    float angle = (float)i * 1.2566f;
    predators[i].position = {cosf(angle) * 8.0f, sinf(angle) * 8.0f, 0.0f};
    predators[i].radius = 2.5f;
  }
}

// Builds the MLP graph and compiles it once into _brainExecutable.
// Weights (w1, w2) are placeholders backed by _weightBufferL1 /
// _weightBufferL2, so the graph never needs to be recompiled when weights
// change.
//
// Placeholder creation order determines the inputsArray order used at
// inference:
//   [0] _inputTensor  — feature buffer  [N, kInputSize]
//   [1] _w1Tensor     — L1 weights      [kInputSize,  kHiddenSize]
//   [2] _w2Tensor     — L2 weights      [kHiddenSize, kOutputSize]
- (void)buildBrainGraph {
  _brainGraph = [[MPSGraph alloc] init];

  // ---- Placeholders (creation order = inputsArray order) ----
  _inputTensor = [_brainGraph
      placeholderWithShape:@[ @(Engine::kParticleCount), @(Engine::kInputSize) ]
                  dataType:MPSDataTypeFloat32
                      name:@"features"];

  _w1Tensor = [_brainGraph
      placeholderWithShape:@[ @(Engine::kInputSize), @(Engine::kHiddenSize) ]
                  dataType:MPSDataTypeFloat32
                      name:@"w1"];

  _w2Tensor = [_brainGraph
      placeholderWithShape:@[ @(Engine::kHiddenSize), @(Engine::kOutputSize) ]
                  dataType:MPSDataTypeFloat32
                      name:@"w2"];

  // ---- Biases remain constants (zero-initialised, rarely tuned) ----
  float b1Vals[Engine::kHiddenSize] = {};
  float b2Vals[Engine::kOutputSize] = {};
  MPSGraphTensor *b1T = [_brainGraph
      constantWithData:[NSData dataWithBytes:b1Vals length:sizeof(b1Vals)]
                 shape:@[ @1, @(Engine::kHiddenSize) ]
              dataType:MPSDataTypeFloat32];
  MPSGraphTensor *b2T = [_brainGraph
      constantWithData:[NSData dataWithBytes:b2Vals length:sizeof(b2Vals)]
                 shape:@[ @1, @(Engine::kOutputSize) ]
              dataType:MPSDataTypeFloat32];

  // ---- MLP forward pass: h = ReLU(input @ w1 + b1) ----
  MPSGraphTensor *h =
      [_brainGraph matrixMultiplicationWithPrimaryTensor:_inputTensor
                                         secondaryTensor:_w1Tensor
                                                    name:nil];
  h = [_brainGraph additionWithPrimaryTensor:h secondaryTensor:b1T name:nil];
  h = [_brainGraph reLUWithTensor:h name:@"hidden"];

  // ---- Output: out = h @ w2 + b2 ----
  MPSGraphTensor *out =
      [_brainGraph matrixMultiplicationWithPrimaryTensor:h
                                         secondaryTensor:_w2Tensor
                                                    name:nil];
  _outputTensor = [_brainGraph additionWithPrimaryTensor:out
                                         secondaryTensor:b2T
                                                    name:@"acceleration"];

  // ---- Compile once — shapes for all three placeholders must be declared ----
  MPSGraphDevice *graphDevice = [MPSGraphDevice deviceWithMTLDevice:_device];
  _brainExecutable = [_brainGraph
          compileWithDevice:graphDevice
                      feeds:@{
                        _inputTensor : [[MPSGraphShapedType alloc]
                            initWithShape:@[
                              @(Engine::kParticleCount), @(Engine::kInputSize)
                            ]
                                 dataType:MPSDataTypeFloat32],
                        _w1Tensor : [[MPSGraphShapedType alloc]
                            initWithShape:@[
                              @(Engine::kInputSize), @(Engine::kHiddenSize)
                            ]
                                 dataType:MPSDataTypeFloat32],
                        _w2Tensor : [[MPSGraphShapedType alloc]
                            initWithShape:@[
                              @(Engine::kHiddenSize), @(Engine::kOutputSize)
                            ]
                                 dataType:MPSDataTypeFloat32],
                      }
              targetTensors:@[ _outputTensor ]
           targetOperations:nil
      compilationDescriptor:nil];
}

// Perturb a random subset of weights by Gaussian noise scaled by mutationRate.
// Called on keypress — no recompilation needed because weights are
// placeholders.
- (void)applyEvolutionStep:(float)mutationRate {
  const NSUInteger l1Count = Engine::kInputSize * Engine::kHiddenSize;
  const NSUInteger l2Count = Engine::kHiddenSize * Engine::kOutputSize;

  float *w1 = (float *)_weightBufferL1.contents;
  for (NSUInteger i = 0; i < l1Count; ++i) {
    if ((rand() / (float)RAND_MAX) < 0.3f)
      w1[i] += gaussianRandom() * mutationRate;
  }

  float *w2 = (float *)_weightBufferL2.contents;
  for (NSUInteger i = 0; i < l2Count; ++i) {
    if ((rand() / (float)RAND_MAX) < 0.3f)
      w2[i] += gaussianRandom() * mutationRate;
  }
}

- (void)addMesh:(const Engine::MeshData &)data
    atTransform:(matrix_float4x4)transform {

  SceneObject object;
  object.vertexBuffer =
      [_device newBufferWithBytes:data.vertices.data()
                           length:data.vertices.size() * sizeof(Engine::Vertex)
                          options:MTLResourceStorageModeManaged];
  object.indexBuffer =
      [_device newBufferWithBytes:data.indices.data()
                           length:data.indices.size() * sizeof(uint16_t)
                          options:MTLResourceStorageModeManaged];
  object.indexCount = data.indices.size();
  object.transform = transform;
  _scene.push_back(object);
}

- (void)buildScene {
  // Scene meshes disabled — particle sandbox is the sole visual.
  // using namespace Engine;
  // [self addMesh:Geometry::createCube(1.0f)
  //   atTransform:Math::Matrix::identity()];
  // [self addMesh:Geometry::createCube(0.8f)
  //   atTransform:Math::Matrix::translation( 5.0f, 0.0f, 0.0f)];
  // [self addMesh:Geometry::createCube(0.8f)
  //   atTransform:Math::Matrix::translation(-3.0f, 0.0f, 0.0f)];
  // [self addMesh:Geometry::createCube(0.8f)
  //   atTransform:Math::Matrix::translation( 0.0f, 3.0f, 0.0f)];
  // [self addMesh:Geometry::createCube(0.8f)
  //   atTransform:Math::Matrix::translation( 0.0f,-3.0f, 0.0f)];
  // [self addMesh:Geometry::createSphere(0.7f, 16, 12)
  //   atTransform:Math::Matrix::translation( 0.0f, 0.0f,-5.0f)];
  // [self addMesh:Geometry::createPlane(20.0f, 20.0f, 1)
  //   atTransform:Math::Matrix::translation( 0.0f,-5.0f, 0.0f)];
}

// ---- Cursor → World -------------------------------------------------------

- (simd_float2)cursorWorldPositionForScreenPoint:(CGPoint)screenPt {
  // Use view size in points (not drawable size in pixels) for correct
  // NDC mapping, especially on Retina displays.
  float viewW = _viewSize.width > 0 ? (float)_viewSize.width : 960.0f;
  float viewH = _viewSize.height > 0 ? (float)_viewSize.height : 800.0f;
  float aspect = viewH > 0 ? viewW / viewH : 16.0f / 9.0f;

  matrix_float4x4 proj = _camera.getProjectionMatrix(aspect);
  matrix_float4x4 viewM = _camera.getViewMatrix();
  matrix_float4x4 vp = simd_mul(proj, viewM);
  matrix_float4x4 invVP = simd_inverse(vp);

  // Normalised device coords — screenPt is in points with origin at bottom-left
  float x = (2.0f * (float)screenPt.x) / viewW - 1.0f;
  float y = (2.0f * (float)screenPt.y) / viewH - 1.0f;

  simd_float4 rayStartNDC = {x, y, 0.0f, 1.0f};
  simd_float4 rayEndNDC = {x, y, 1.0f, 1.0f};

  simd_float4 rayStart = simd_mul(invVP, rayStartNDC);
  rayStart /= rayStart.w;
  simd_float4 rayEnd = simd_mul(invVP, rayEndNDC);
  rayEnd /= rayEnd.w;

  // Intersect with the z=0 plane
  simd_float3 dir = simd_normalize(rayEnd.xyz - rayStart.xyz);
  if (fabs(dir.z) < 1e-4f)
    return simd_make_float2(0, 0);
  float t = -rayStart.z / dir.z;
  simd_float3 hit = rayStart.xyz + dir * t;
  return simd_make_float2(hit.x, hit.y);
}

- (void)setCursorScreenPosition:(CGPoint)pt viewSize:(CGSize)viewSize {
  _viewSize = viewSize;
  _cursorWorld = [self cursorWorldPositionForScreenPoint:pt];
}

// ---- MTKViewDelegate ------------------------------------------------------

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
  _drawableSize = size;
}

- (void)drawInMTKView:(MTKView *)view {
  @autoreleasepool {
    const float dt = _clock.tick();
    _elapsedTime += dt;

    // ---- FPS Counter ----
    _frameCount++;
    _fpsAccumulator += dt;
    if (_fpsAccumulator >= 0.5f) {
      _currentFPS = _frameCount / _fpsAccumulator;
      _frameCount = 0;
      _fpsAccumulator = 0.0f;
      dispatch_async(dispatch_get_main_queue(), ^{
        view.window.title = [NSString
            stringWithFormat:@"MetalEngine — %.0f FPS | %lu particles",
                             _currentFPS, (unsigned long)_particleCount];
      });
    }

    if (!kLockCamera) {
      _camera.processMovement(dt, _input.moveForward, _input.moveBackward,
                              _input.moveLeft, _input.moveRight, _input.moveUp,
                              _input.moveDown);
    }

    MTLRenderPassDescriptor *passDesc = view.currentRenderPassDescriptor;
    if (!passDesc)
      return;

    const float aspect =
        _drawableSize.height > 0
            ? static_cast<float>(_drawableSize.width / _drawableSize.height)
            : 16.0f / 9.0f;
    const matrix_float4x4 viewMat = _camera.getViewMatrix();
    const matrix_float4x4 projMat = _camera.getProjectionMatrix(aspect);

    id<MTLCommandBuffer> cmd =
        [MPSCommandBuffer commandBufferFromCommandQueue:_commandQueue];

    // ==== Compute: Update Predators ====
    {
      id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
      [compEnc setComputePipelineState:_computePipelinePredators];
      [compEnc setBuffer:_predatorBuffer offset:0 atIndex:0];
      uint32_t predCount = Engine::kPredatorCount;
      [compEnc setBytes:&predCount length:sizeof(predCount) atIndex:1];
      [compEnc setBytes:&_elapsedTime length:sizeof(float) atIndex:2];
      [compEnc dispatchThreads:MTLSizeMake(Engine::kPredatorCount, 1, 1)
          threadsPerThreadgroup:MTLSizeMake(Engine::kPredatorCount, 1, 1)];
      [compEnc endEncoding];
    }

    // ==== Compute: Prepare Features ====
    {
      id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
      [compEnc setComputePipelineState:_computePipelinePrepare];
      [compEnc setBuffer:_particleBuffer offset:0 atIndex:0];
      [compEnc setBuffer:_featureBuffer offset:0 atIndex:1];
      [compEnc setBytes:&_cursorWorld length:sizeof(simd_float2) atIndex:2];
      float dtCopy = dt;
      [compEnc setBytes:&dtCopy length:sizeof(float) atIndex:3];
      uint32_t pCount = _particleCount;
      [compEnc setBytes:&pCount length:sizeof(uint32_t) atIndex:4];
      [compEnc setBytes:&_audioBands length:sizeof(simd_float3) atIndex:5];
      uint32_t enableCursor = _enableCursorAttraction ? 1 : 0;
      [compEnc setBytes:&enableCursor length:sizeof(uint32_t) atIndex:6];

      NSUInteger threadgroupSize =
          _computePipelinePrepare.maxTotalThreadsPerThreadgroup;
      if (threadgroupSize > 256)
        threadgroupSize = 256;
      MTLSize threads = MTLSizeMake(_particleCount, 1, 1);
      MTLSize groups = MTLSizeMake(threadgroupSize, 1, 1);
      [compEnc dispatchThreads:threads threadsPerThreadgroup:groups];
      [compEnc endEncoding];
    }

    // ==== MPSGraph: Inference ====
    // inputsArray order must match placeholder creation order in
    // buildBrainGraph:
    //   [0] features, [1] w1, [2] w2
    MPSGraphTensorData *featureData = [[MPSGraphTensorData alloc]
        initWithMTLBuffer:_featureBuffer
                    shape:@[ @(_particleCount), @(Engine::kInputSize) ]
                 dataType:MPSDataTypeFloat32];
    MPSGraphTensorData *w1Data = [[MPSGraphTensorData alloc]
        initWithMTLBuffer:_weightBufferL1
                    shape:@[ @(Engine::kInputSize), @(Engine::kHiddenSize) ]
                 dataType:MPSDataTypeFloat32];
    MPSGraphTensorData *w2Data = [[MPSGraphTensorData alloc]
        initWithMTLBuffer:_weightBufferL2
                    shape:@[ @(Engine::kHiddenSize), @(Engine::kOutputSize) ]
                 dataType:MPSDataTypeFloat32];
    MPSGraphTensorData *outputData = [[MPSGraphTensorData alloc]
        initWithMTLBuffer:_accelBuffer
                    shape:@[ @(_particleCount), @(Engine::kOutputSize) ]
                 dataType:MPSDataTypeFloat32];

    [_brainExecutable encodeToCommandBuffer:cmd
                                inputsArray:@[ featureData, w1Data, w2Data ]
                               resultsArray:@[ outputData ]
                        executionDescriptor:nil];

    // ==== Compute: Apply Physics ====
    {
      id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
      [compEnc setComputePipelineState:_computePipelinePhysics];
      [compEnc setBuffer:_particleBuffer offset:0 atIndex:0];
      [compEnc setBuffer:_accelBuffer offset:0 atIndex:1];
      [compEnc setBuffer:_predatorBuffer offset:0 atIndex:2];
      uint32_t predCount = Engine::kPredatorCount;
      [compEnc setBytes:&predCount length:sizeof(uint32_t) atIndex:3];
      float dtCopy = dt;
      [compEnc setBytes:&dtCopy length:sizeof(float) atIndex:4];
      float maxSpeed = 5.0f;
      [compEnc setBytes:&maxSpeed length:sizeof(float) atIndex:5];
      uint32_t pCount = _particleCount;
      [compEnc setBytes:&pCount length:sizeof(uint32_t) atIndex:6];

      NSUInteger threadgroupSize =
          _computePipelinePhysics.maxTotalThreadsPerThreadgroup;
      if (threadgroupSize > 256)
        threadgroupSize = 256;
      MTLSize threads = MTLSizeMake(_particleCount, 1, 1);
      MTLSize groups = MTLSizeMake(threadgroupSize, 1, 1);
      [compEnc dispatchThreads:threads threadsPerThreadgroup:groups];
      [compEnc endEncoding];
    }

    // ==== Render: Scene Meshes + Particles ====
    id<MTLRenderCommandEncoder> enc =
        [cmd renderCommandEncoderWithDescriptor:passDesc];

    // --- Draw existing meshes ---
    [enc setRenderPipelineState:_pipelineState];
    [enc setDepthStencilState:_depthStencilState];
    [enc setCullMode:MTLCullModeBack];
    [enc setFrontFacingWinding:MTLWindingCounterClockwise];

    for (const auto &object : _scene) {
      const matrix_float4x4 mvp =
          simd_mul(projMat, simd_mul(viewMat, object.transform));

      Uniforms uniforms;
      uniforms.modelViewProjection = mvp;
      uniforms.model = object.transform;
      uniforms.view = viewMat;
      uniforms.projection = projMat;

      // setVertexBytes uses Metal's transient inline buffer (≤4 KB),
      // so each draw gets its own copy without us juggling buffers.
      [enc setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:1];
      [enc setVertexBuffer:object.vertexBuffer offset:0 atIndex:0];
      [enc drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                      indexCount:object.indexCount
                       indexType:MTLIndexTypeUInt16
                     indexBuffer:object.indexBuffer
               indexBufferOffset:0];
    }

    // --- Draw particles (point sprites) ---
    [enc setRenderPipelineState:_particleRenderPipeline];
    // Keep depth test but disable depth write so particles don't z-fight
    MTLDepthStencilDescriptor *particleDepthDesc =
        [[MTLDepthStencilDescriptor alloc] init];
    particleDepthDesc.depthCompareFunction = MTLCompareFunctionLess;
    particleDepthDesc.depthWriteEnabled = NO;
    id<MTLDepthStencilState> particleDepthState =
        [_device newDepthStencilStateWithDescriptor:particleDepthDesc];
    [enc setDepthStencilState:particleDepthState];

    matrix_float4x4 mvp = simd_mul(projMat, viewMat);
    [enc setVertexBuffer:_particleBuffer offset:0 atIndex:0];
    [enc setVertexBytes:&mvp length:sizeof(mvp) atIndex:1];
    [enc setVertexBytes:&viewMat length:sizeof(viewMat) atIndex:2];
    [enc setVertexBytes:&_audioBands length:sizeof(simd_float3) atIndex:3];
    [enc drawPrimitives:MTLPrimitiveTypePoint
            vertexStart:0
            vertexCount:_particleCount];

    [enc endEncoding];
    [cmd presentDrawable:view.currentDrawable];
    [cmd commit];
  }
}

// ---- Input ----------------------------------------------------------------

- (void)keyDown:(unsigned short)keyCode {
  switch (keyCode) {
  case kVK_ANSI_W:
    _input.moveForward = true;
    break;
  case kVK_ANSI_S:
    _input.moveBackward = true;
    break;
  case kVK_ANSI_A:
    _input.moveLeft = true;
    break;
  case kVK_ANSI_D:
    _input.moveRight = true;
    break;
  case kVK_Space:
    _input.moveUp = true;
    break;
  case kVK_ANSI_Q:
    _input.moveDown = true;
    break;
  case kVK_ANSI_R:
    [self resetSimulation];
    break;
  case kVK_ANSI_1:
    [self applyEvolutionStep:0.5f]; // moderate mutation burst
    break;
  case kVK_ANSI_2:
    [self applyEvolutionStep:1.5f]; // subtle fine-tuning nudge
    break;
  case kVK_ANSI_C:
    _enableCursorAttraction = !_enableCursorAttraction;
    break;
  }
}

- (void)keyUp:(unsigned short)keyCode {
  switch (keyCode) {
  case kVK_ANSI_W:
    _input.moveForward = false;
    break;
  case kVK_ANSI_S:
    _input.moveBackward = false;
    break;
  case kVK_ANSI_A:
    _input.moveLeft = false;
    break;
  case kVK_ANSI_D:
    _input.moveRight = false;
    break;
  case kVK_Space:
    _input.moveUp = false;
    break;
  case kVK_ANSI_Q:
    _input.moveDown = false;
    break;
  }
}

- (void)setMouseLookEnabled:(BOOL)enabled {
  _input.mouseLookEnabled = enabled;
}

- (void)processMouseDeltaX:(float)dx deltaY:(float)dy {
  if (kLockCamera)
    return;
  if (_input.mouseLookEnabled) {
    _camera.processMouseMovement(dx, dy);
  }
}

@end

```

## `MetalEngine/MyMetalView.h`

```cpp
// MyMetalView.h
#pragma once

#import <MetalKit/MetalKit.h>

@interface MyMetalView : MTKView
@end

```

## `MetalEngine/MyMetalView.mm`

```cpp
// MyMetalView.mm
#import "MyMetalView.h"
#import "MetalRenderer.h"

@implementation MyMetalView {
    MetalRenderer *_renderer;
    NSPoint        _lastMouseLocation;
    BOOL           _firstMouseMove;
    NSTrackingArea *_trackingArea;
}

- (instancetype)initWithFrame:(NSRect)frame {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device) {
        NSLog(@"❌ Failed to create Metal device");
        return nil;
    }

    self = [super initWithFrame:frame device:device];
    if (!self) return nil;

    self.colorPixelFormat        = MTLPixelFormatBGRA8Unorm;
    self.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
    self.preferredFramesPerSecond = 60;

    _renderer    = [[MetalRenderer alloc] initWithMTKView:self];
    self.delegate = _renderer;

    NSLog(@"🎮 Controls: WASD=move, Space=up, Q=down, R=reset, Right-click+drag=look");
    NSLog(@"🧠 Move cursor to attract particles — predators orbit and repel them");
    return self;
}

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)canBecomeKeyView      { return YES; }

- (void)viewDidMoveToWindow {
    [super viewDidMoveToWindow];
    if (self.window) {
        [self.window makeFirstResponder:self];
        // Accept mouse-moved events without needing a button press
        [self.window setAcceptsMouseMovedEvents:YES];
    }
}

// ---- Mouse Tracking -------------------------------------------------------

- (void)updateTrackingAreas {
    [super updateTrackingAreas];
    if (_trackingArea) {
        [self removeTrackingArea:_trackingArea];
    }
    _trackingArea = [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:(NSTrackingMouseMoved |
                      NSTrackingActiveInKeyWindow |
                      NSTrackingInVisibleRect)
               owner:self
            userInfo:nil];
    [self addTrackingArea:_trackingArea];
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint loc = [self convertPoint:event.locationInWindow fromView:nil];
    [_renderer setCursorScreenPosition:loc viewSize:self.bounds.size];
}

// ---- Keyboard -------------------------------------------------------------

- (void)keyDown:(NSEvent *)event { [_renderer keyDown:event.keyCode]; }
- (void)keyUp:(NSEvent *)event   { [_renderer keyUp:event.keyCode]; }

// ---- Right-mouse drag drives look ----------------------------------------

- (void)rightMouseDown:(NSEvent *)event {
    [self.window makeFirstResponder:self];
    [_renderer setMouseLookEnabled:YES];
    _firstMouseMove    = YES;
    _lastMouseLocation = [self convertPoint:event.locationInWindow fromView:nil];
}

- (void)rightMouseUp:(NSEvent *)event {
    [_renderer setMouseLookEnabled:NO];
}

- (void)rightMouseDragged:(NSEvent *)event {
    NSPoint current = [self convertPoint:event.locationInWindow fromView:nil];
    if (!_firstMouseMove) {
        const float dx = current.x - _lastMouseLocation.x;
        const float dy = current.y - _lastMouseLocation.y;
        [_renderer processMouseDeltaX:dx deltaY:dy];
    } else {
        _firstMouseMove = NO;
    }
    _lastMouseLocation = current;
}

@end

```

## `MetalEngine/Shaders.metal`

```cpp
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
    const device Particle *particles     [[buffer(0)]],
    device float          *features      [[buffer(1)]],
    constant float2       &cursorWorld   [[buffer(2)]],
    constant float        &deltaTime     [[buffer(3)]],
    constant uint         &particleCount [[buffer(4)]],
    constant float3       &audioData     [[buffer(5)]],
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
    features[gid * 7 + 4] = audioData.x; // Bass
    features[gid * 7 + 5] = audioData.y; // Mids
    features[gid * 7 + 6] = audioData.z; // Highs
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
    const float drag = 0.95f;
    vel = vel * drag + accel * deltaTime;

    // Clamp speed
    float speed = length(vel);
    if (speed > maxSpeed)
        vel = normalize(vel) * maxSpeed;

    pos += vel * deltaTime;

    // Soft boundary — nudge particles back toward center if they wander too far
    const float boundary = 20.0f;
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
    float  bass;
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
        out.position  = float4(0, 0, 0, 0);
        out.pointSize = 0;
        out.color     = float4(0);
        out.bass      = 0.0f;
        return out;
    }

    out.position = modelViewProjection * float4(pos, 1.0);

    // DNA Color based on vertex ID
    float hue = fract(float(vid) * 0.618033988749895);
    // Convert hue to RGB using smooth function
    float3 rgb = clamp(abs(fmod(hue * 6.0 + float3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    out.color = float4(rgb, 1.0);

    out.bass = audioData.x;

    // Size shrinks with distance from camera, pulses with bass heavily
    float3 viewPos = (view * float4(pos, 1.0)).xyz;
    float  dist    = length(viewPos);
    out.pointSize  = clamp(2500.0f / (dist + 1.0f), 30.0f, 120.0f) * (1.0f + audioData.x * 4.0f);

    return out;
}

fragment float4 fs_particle(ParticleVertexOut in [[stage_in]],
                            float2 pointCoord [[point_coord]])
{
    // Soft glowing circle
    float2 uv   = pointCoord - 0.5;
    float  r    = length(uv);
    if (r > 0.5) discard_fragment();
    
    // Nucleus effect pulses with bass heavily
    if (r < 0.15) {
        return float4(min(in.color.rgb * (1.5 + in.bass * 6.0), 1.0), 1.0);
    }

    float  glow = smoothstep(0.5, 0.0, r);
    return in.color * glow;
}

```

## `MetalEngine/main.m`

```cpp
//main.m
#import <AppKit/AppKit.h>
#import "AppDelegate.h"

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];

        [app run];
    }
    return 0;
}

```

## `run.sh`

```bash
#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -e

echo "🧹 Cleaning previous build..."
rm -rf build

echo "⚙️ Configuring CMake..."
cmake -B build

echo "🔨 Compiling project..."
cmake --build build

echo "🚀 Running MetalEngine..."
cmake --build build --target run

```

