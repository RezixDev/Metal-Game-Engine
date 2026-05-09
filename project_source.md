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
)

set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
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

#include <vector>
#include <cstring>
#include <cstdlib>

namespace {

// macOS virtual key codes used by the camera controls.
constexpr unsigned short kVK_ANSI_A = 0x00;
constexpr unsigned short kVK_ANSI_S = 0x01;
constexpr unsigned short kVK_ANSI_D = 0x02;
constexpr unsigned short kVK_ANSI_Q = 0x0C;
constexpr unsigned short kVK_ANSI_W = 0x0D;
constexpr unsigned short kVK_ANSI_R = 0x0F;
constexpr unsigned short kVK_Space  = 0x31;

// 2D front-facing debug view — camera looks directly at the XY plane.
// To restore 3D perspective: position={0,2,5}, target={0,0,0}, kLockCamera=false.
constexpr Engine::Math::Vec3 kInitialPosition = {0.0f, 0.0f, 25.0f};
constexpr Engine::Math::Vec3 kInitialTarget   = {0.0f, 0.0f,  0.0f};
constexpr bool kLockCamera = true; // disable WASD + mouse-look for 2D debug

struct InputState {
    bool moveForward      = false;
    bool moveBackward     = false;
    bool moveLeft         = false;
    bool moveRight        = false;
    bool moveUp           = false;
    bool moveDown         = false;
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
    id<MTLBuffer>   vertexBuffer;
    id<MTLBuffer>   indexBuffer;
    NSUInteger      indexCount;
    matrix_float4x4 transform;
};

} // namespace

@implementation MetalRenderer {
    id<MTLDevice>              _device;
    id<MTLCommandQueue>        _commandQueue;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLDepthStencilState>   _depthStencilState;

    std::vector<SceneObject>   _scene;
    Engine::Camera             _camera;
    Engine::FrameClock         _clock;
    InputState                 _input;

    CGSize                     _drawableSize;

    // ---- Particle Sandbox ----
    id<MTLBuffer>               _particleBuffer;
    id<MTLBuffer>               _brainBuffer;
    id<MTLBuffer>               _predatorBuffer;
    id<MTLComputePipelineState> _computePipelineParticles;
    id<MTLComputePipelineState> _computePipelinePredators;
    id<MTLRenderPipelineState>  _particleRenderPipeline;

    NSUInteger                  _particleCount;
    simd_float2                 _cursorWorld;
    float                       _elapsedTime;
    CGSize                      _viewSize; // in points (not pixels)

    // FPS counter
    int                         _frameCount;
    float                       _fpsAccumulator;
    float                       _currentFPS;
}

// ---- Setup ----------------------------------------------------------------

- (instancetype)initWithMTKView:(MTKView *)view {
    self = [super init];
    if (!self) return nil;

    _device        = view.device;
    _commandQueue  = [_device newCommandQueue];
    _drawableSize  = view.drawableSize;

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

    _elapsedTime  = 0.0f;
    _cursorWorld  = simd_make_float2(0, 0);

    return self;
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
    vd.attributes[0].format = MTLVertexFormatFloat3; vd.attributes[0].offset = 0;  // position
    vd.attributes[1].format = MTLVertexFormatFloat3; vd.attributes[1].offset = 16; // color
    vd.attributes[2].format = MTLVertexFormatFloat3; vd.attributes[2].offset = 32; // normal
    vd.attributes[3].format = MTLVertexFormatFloat2; vd.attributes[3].offset = 48; // texCoord
    vd.layouts[0].stride       = sizeof(Engine::Vertex); // 64
    vd.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

    MTLRenderPipelineDescriptor *pd  = [[MTLRenderPipelineDescriptor alloc] init];
    pd.label                         = @"MainPipeline";
    pd.vertexFunction                = [library newFunctionWithName:@"vs_main"];
    pd.fragmentFunction              = [library newFunctionWithName:@"fs_main_lit"];
    pd.vertexDescriptor              = vd;
    pd.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    pd.depthAttachmentPixelFormat    = view.depthStencilPixelFormat;

    NSError *error = nil;
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pd error:&error];
    if (!_pipelineState) {
        [NSException raise:@"MetalRenderer"
                    format:@"Pipeline creation failed: %@", error];
    }

    MTLDepthStencilDescriptor *dsd = [[MTLDepthStencilDescriptor alloc] init];
    dsd.depthCompareFunction = MTLCompareFunctionLess;
    dsd.depthWriteEnabled    = YES;
    _depthStencilState = [_device newDepthStencilStateWithDescriptor:dsd];
}

- (void)buildComputePipelines {
    id<MTLLibrary> lib = [_device newDefaultLibrary];
    NSError *error = nil;

    id<MTLFunction> fnParticles = [lib newFunctionWithName:@"updateParticles"];
    _computePipelineParticles = [_device newComputePipelineStateWithFunction:fnParticles
                                                                      error:&error];
    if (!_computePipelineParticles) {
        [NSException raise:@"MetalRenderer"
                    format:@"Failed to create particle compute pipeline: %@", error];
    }

    id<MTLFunction> fnPredators = [lib newFunctionWithName:@"updatePredators"];
    _computePipelinePredators = [_device newComputePipelineStateWithFunction:fnPredators
                                                                      error:&error];
    if (!_computePipelinePredators) {
        [NSException raise:@"MetalRenderer"
                    format:@"Failed to create predator compute pipeline: %@", error];
    }
}

- (void)buildParticleRenderPipeline:(MTKView *)view {
    id<MTLLibrary> lib = [_device newDefaultLibrary];

    MTLRenderPipelineDescriptor *desc = [[MTLRenderPipelineDescriptor alloc] init];
    desc.label            = @"ParticlePipeline";
    desc.vertexFunction   = [lib newFunctionWithName:@"vs_particle"];
    desc.fragmentFunction = [lib newFunctionWithName:@"fs_particle"];
    desc.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    desc.depthAttachmentPixelFormat      = view.depthStencilPixelFormat;
    // No vertex descriptor — particle data comes from buffer(0) indexed by vertex_id.
    desc.inputPrimitiveTopology = MTLPrimitiveTopologyClassPoint;

    // Enable alpha blending for the glow effect
    desc.colorAttachments[0].blendingEnabled             = YES;
    desc.colorAttachments[0].sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
    desc.colorAttachments[0].destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[0].rgbBlendOperation           = MTLBlendOperationAdd;
    desc.colorAttachments[0].sourceAlphaBlendFactor      = MTLBlendFactorOne;
    desc.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    desc.colorAttachments[0].alphaBlendOperation         = MTLBlendOperationAdd;

    NSError *error = nil;
    _particleRenderPipeline = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
    if (!_particleRenderPipeline) {
        [NSException raise:@"MetalRenderer"
                    format:@"Failed to create particle render pipeline: %@", error];
    }
}

- (void)initParticleData {
    _particleCount = Engine::kParticleCount;

    // ---- Particles: random positions on the ground plane ----
    {
        std::vector<Engine::Particle> particles(_particleCount);
        srand(42);
        for (NSUInteger i = 0; i < _particleCount; ++i) {
            particles[i].position = {
                (rand() / (float)RAND_MAX - 0.5f) * 20.0f,
                (rand() / (float)RAND_MAX - 0.5f) * 20.0f,
                0.0f
            };
            particles[i].velocity     = {0, 0, 0};
            particles[i].acceleration = {0, 0, 0};
            particles[i].mass         = 1.0f;
            particles[i].alive        = 1;
            particles[i].padding[0]   = 0;
            particles[i].padding[1]   = 0;
        }
        _particleBuffer = [_device newBufferWithBytes:particles.data()
                                               length:particles.size() * sizeof(Engine::Particle)
                                              options:MTLResourceStorageModeShared];
        _particleBuffer.label = @"ParticleBuffer";
    }

    // ---- Brains: identity-like weights that gently steer toward cursor ----
    {
        std::vector<Engine::Brain> brains(_particleCount);
        for (NSUInteger i = 0; i < _particleCount; ++i) {
            // Hidden layer: pass through with slight randomisation
            float r1 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
            float r2 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
            float r3 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;
            float r4 = (rand() / (float)RAND_MAX - 0.5f) * 0.1f;

            brains[i].w1 = (matrix_float4x4){{
                {1.0f + r1, 0, 0, 0},
                {0, 1.0f + r2, 0, 0},
                {0, 0, 1.0f + r3, 0},
                {0, 0, 0, 1.0f + r4},
            }};
            brains[i].b1 = simd_make_float4(0, 0, 0, 0);

            // Output layer: map hidden[0,1] (cursor dx, dz) → acceleration
            // Strong gain so particles visibly accelerate toward cursor
            float gain = 0.6f + (rand() / (float)RAND_MAX) * 0.4f;
            float cross = (rand() / (float)RAND_MAX - 0.5f) * 0.05f;
            brains[i].w2 = (matrix_float4x4){{
                {gain,  cross, 0, 0},
                {cross, gain,  0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0},
            }};
            brains[i].b2 = simd_make_float4(0, 0, 0, 0);
        }
        _brainBuffer = [_device newBufferWithBytes:brains.data()
                                            length:brains.size() * sizeof(Engine::Brain)
                                           options:MTLResourceStorageModeShared];
        _brainBuffer.label = @"BrainBuffer";
    }

    // ---- Predators: initial positions in orbit ----
    {
        std::vector<Engine::Predator> predators(Engine::kPredatorCount);
        for (uint32_t i = 0; i < Engine::kPredatorCount; ++i) {
            float angle = (float)i * 1.2566f;
            predators[i].position = {cosf(angle) * 8.0f, sinf(angle) * 8.0f, 0.0f};
            predators[i].radius   = 2.5f;
        }
        _predatorBuffer = [_device newBufferWithBytes:predators.data()
                                               length:predators.size() * sizeof(Engine::Predator)
                                              options:MTLResourceStorageModeShared];
        _predatorBuffer.label = @"PredatorBuffer";
    }

    NSLog(@"🧠 Particle Sandbox: %lu particles, %u predators initialised",
          (unsigned long)_particleCount, Engine::kPredatorCount);
}

- (void)addMesh:(const Engine::MeshData&)data
   atTransform:(matrix_float4x4)transform {

    SceneObject object;
    object.vertexBuffer = [_device newBufferWithBytes:data.vertices.data()
                                               length:data.vertices.size() * sizeof(Engine::Vertex)
                                              options:MTLResourceStorageModeManaged];
    object.indexBuffer  = [_device newBufferWithBytes:data.indices.data()
                                               length:data.indices.size() * sizeof(uint16_t)
                                              options:MTLResourceStorageModeManaged];
    object.indexCount   = data.indices.size();
    object.transform    = transform;
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
    float viewW = _viewSize.width  > 0 ? (float)_viewSize.width  : 960.0f;
    float viewH = _viewSize.height > 0 ? (float)_viewSize.height : 800.0f;
    float aspect = viewH > 0 ? viewW / viewH : 16.0f / 9.0f;

    matrix_float4x4 proj  = _camera.getProjectionMatrix(aspect);
    matrix_float4x4 viewM = _camera.getViewMatrix();
    matrix_float4x4 vp    = simd_mul(proj, viewM);
    matrix_float4x4 invVP = simd_inverse(vp);

    // Normalised device coords — screenPt is in points with origin at bottom-left
    float x = (2.0f * (float)screenPt.x) / viewW - 1.0f;
    float y = (2.0f * (float)screenPt.y) / viewH - 1.0f;

    simd_float4 rayStartNDC = {x, y, 0.0f, 1.0f};
    simd_float4 rayEndNDC   = {x, y, 1.0f, 1.0f};

    simd_float4 rayStart = simd_mul(invVP, rayStartNDC);
    rayStart /= rayStart.w;
    simd_float4 rayEnd = simd_mul(invVP, rayEndNDC);
    rayEnd /= rayEnd.w;

    // Intersect with the z=0 plane
    simd_float3 dir = simd_normalize(rayEnd.xyz - rayStart.xyz);
    if (fabs(dir.z) < 1e-4f) return simd_make_float2(0, 0);
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
                view.window.title = [NSString stringWithFormat:@"MetalEngine — %.0f FPS | %lu particles",
                                     _currentFPS, (unsigned long)_particleCount];
            });
        }

        if (!kLockCamera) {
            _camera.processMovement(dt,
                _input.moveForward, _input.moveBackward,
                _input.moveLeft,    _input.moveRight,
                _input.moveUp,      _input.moveDown);
        }

        MTLRenderPassDescriptor *passDesc = view.currentRenderPassDescriptor;
        if (!passDesc) return;

        const float aspect = _drawableSize.height > 0
            ? static_cast<float>(_drawableSize.width / _drawableSize.height)
            : 16.0f / 9.0f;
        const matrix_float4x4 viewMat = _camera.getViewMatrix();
        const matrix_float4x4 projMat = _camera.getProjectionMatrix(aspect);

        id<MTLCommandBuffer> cmd = [_commandQueue commandBuffer];

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

        // ==== Compute: Update Particles (Neural Network + Physics) ====
        {
            id<MTLComputeCommandEncoder> compEnc = [cmd computeCommandEncoder];
            [compEnc setComputePipelineState:_computePipelineParticles];
            [compEnc setBuffer:_particleBuffer  offset:0 atIndex:0];
            [compEnc setBuffer:_brainBuffer     offset:0 atIndex:1];
            [compEnc setBuffer:_predatorBuffer  offset:0 atIndex:2];
            uint32_t predCount = Engine::kPredatorCount;
            [compEnc setBytes:&predCount      length:sizeof(uint32_t)    atIndex:3];
            [compEnc setBytes:&_cursorWorld    length:sizeof(simd_float2) atIndex:4];
            float dtCopy = dt;
            [compEnc setBytes:&dtCopy         length:sizeof(float)       atIndex:5];
            float maxSpeed = 10.0f;
            [compEnc setBytes:&maxSpeed       length:sizeof(float)       atIndex:6];

            NSUInteger threadgroupSize = _computePipelineParticles.maxTotalThreadsPerThreadgroup;
            if (threadgroupSize > 256) threadgroupSize = 256;
            MTLSize threads = MTLSizeMake(_particleCount, 1, 1);
            MTLSize groups  = MTLSizeMake(threadgroupSize, 1, 1);
            [compEnc dispatchThreads:threads threadsPerThreadgroup:groups];
            [compEnc endEncoding];
        }

        // ==== Render: Scene Meshes + Particles ====
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:passDesc];

        // --- Draw existing meshes ---
        [enc setRenderPipelineState:_pipelineState];
        [enc setDepthStencilState:_depthStencilState];
        [enc setCullMode:MTLCullModeBack];
        [enc setFrontFacingWinding:MTLWindingCounterClockwise];

        for (const auto& object : _scene) {
            const matrix_float4x4 mvp =
                simd_mul(projMat, simd_mul(viewMat, object.transform));

            Uniforms uniforms;
            uniforms.modelViewProjection = mvp;
            uniforms.model               = object.transform;
            uniforms.view                = viewMat;
            uniforms.projection          = projMat;

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
        MTLDepthStencilDescriptor *particleDepthDesc = [[MTLDepthStencilDescriptor alloc] init];
        particleDepthDesc.depthCompareFunction = MTLCompareFunctionLess;
        particleDepthDesc.depthWriteEnabled    = NO;
        id<MTLDepthStencilState> particleDepthState =
            [_device newDepthStencilStateWithDescriptor:particleDepthDesc];
        [enc setDepthStencilState:particleDepthState];

        matrix_float4x4 mvp = simd_mul(projMat, viewMat);
        [enc setVertexBuffer:_particleBuffer offset:0 atIndex:0];
        [enc setVertexBytes:&mvp     length:sizeof(mvp)     atIndex:1];
        [enc setVertexBytes:&viewMat length:sizeof(viewMat) atIndex:2];
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
        case kVK_ANSI_W: _input.moveForward  = true; break;
        case kVK_ANSI_S: _input.moveBackward = true; break;
        case kVK_ANSI_A: _input.moveLeft     = true; break;
        case kVK_ANSI_D: _input.moveRight    = true; break;
        case kVK_Space:  _input.moveUp       = true; break;
        case kVK_ANSI_Q: _input.moveDown     = true; break;
        case kVK_ANSI_R:
            _camera.lookAt(kInitialPosition, kInitialTarget,
                           Engine::Math::Vector::Constants::UP);
            break;
    }
}

- (void)keyUp:(unsigned short)keyCode {
    switch (keyCode) {
        case kVK_ANSI_W: _input.moveForward  = false; break;
        case kVK_ANSI_S: _input.moveBackward = false; break;
        case kVK_ANSI_A: _input.moveLeft     = false; break;
        case kVK_ANSI_D: _input.moveRight    = false; break;
        case kVK_Space:  _input.moveUp       = false; break;
        case kVK_ANSI_Q: _input.moveDown     = false; break;
    }
}

- (void)setMouseLookEnabled:(BOOL)enabled {
    _input.mouseLookEnabled = enabled;
}

- (void)processMouseDeltaX:(float)dx deltaY:(float)dy {
    if (kLockCamera) return;
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

// Tiny neural network per particle.
// Network: 4 inputs → 4 hidden (ReLU) → 2 outputs (accel x,z).
// Both layers use float4x4 for alignment; output layer uses only .xy.
struct Brain {
    float4x4 w1;   // hidden layer weights
    float4   b1;   // hidden layer bias
    float4x4 w2;   // output layer weights
    float4   b2;   // output layer bias
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
// Compute Kernel: Update Particles (Neural Network + Physics)
// ============================================================================

kernel void updateParticles(
    device Particle     *particles    [[buffer(0)]],
    device Brain        *brains       [[buffer(1)]],
    constant Predator   *predators    [[buffer(2)]],
    constant uint       &predatorCount[[buffer(3)]],
    constant float2     &cursorWorld  [[buffer(4)]],
    constant float      &deltaTime   [[buffer(5)]],
    constant float      &maxSpeed    [[buffer(6)]],
    uint gid [[thread_position_in_grid]])
{
    Particle p = particles[gid];
    if (p.alive == 0) return;

    float3 pos = p.position;
    float3 vel = p.velocity;

    // ---- 1. Build neural network input ----
    // [ distance-to-cursor-x, distance-to-cursor-y, noise, 1.0 (bias term) ]
    float2 toCursor = cursorWorld - float2(pos.x, pos.y);
    float  noise    = fract(sin(float(gid) * 12.9898f + deltaTime * 100.0f) * 43758.5453f);
    float4 input    = float4(toCursor.x, toCursor.y, noise, 1.0f);

    // ---- 2. Forward pass through tiny neural network ----
    Brain b = brains[gid];

    // Hidden layer: h = ReLU(w1 * input + b1)
    float4 hidden = b.w1 * input + b.b1;
    hidden = max(hidden, float4(0.0));

    // Output layer: out = (w2 * hidden + b2).xy → 2D acceleration
    float4 raw_out = b.w2 * hidden + b.b2;
    float2 aiAccel = raw_out.xy;

    // ---- 3. Predator avoidance (rule-based override) ----
    for (uint i = 0; i < predatorCount; ++i) {
        float3 diff = pos - float3(predators[i].position);
        float  dist = length(diff);
        float  rad  = predators[i].radius;
        if (dist < rad && dist > 0.0001f) {
            // Strong repulsion that scales with proximity
            aiAccel += float2(diff.x, diff.y) * (rad - dist) * 2.0f;
        }
    }

    // ---- 4. Euler integration ----
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

