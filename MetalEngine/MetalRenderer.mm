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
