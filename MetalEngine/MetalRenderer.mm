// MetalRenderer.mm
#import "MetalRenderer.h"

#include "Engine/Camera.hpp"
#include "Engine/Geometry.hpp"
#include "Engine/Time.hpp"

#include <vector>
#include <cstring>

namespace {

// macOS virtual key codes used by the camera controls.
constexpr unsigned short kVK_ANSI_A = 0x00;
constexpr unsigned short kVK_ANSI_S = 0x01;
constexpr unsigned short kVK_ANSI_D = 0x02;
constexpr unsigned short kVK_ANSI_Q = 0x0C;
constexpr unsigned short kVK_ANSI_W = 0x0D;
constexpr unsigned short kVK_ANSI_R = 0x0F;
constexpr unsigned short kVK_Space  = 0x31;

constexpr Engine::Math::Vec3 kInitialPosition = {0.0f, 2.0f, 5.0f};
constexpr Engine::Math::Vec3 kInitialTarget   = {0.0f, 0.0f, 0.0f};

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
    [self buildScene];

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
    using namespace Engine;
    [self addMesh:Geometry::createCube(1.0f)
      atTransform:Math::Matrix::identity()];
    [self addMesh:Geometry::createCube(0.8f)
      atTransform:Math::Matrix::translation( 5.0f, 0.0f, 0.0f)];
    [self addMesh:Geometry::createCube(0.8f)
      atTransform:Math::Matrix::translation(-3.0f, 0.0f, 0.0f)];
    [self addMesh:Geometry::createCube(0.8f)
      atTransform:Math::Matrix::translation( 0.0f, 3.0f, 0.0f)];
    [self addMesh:Geometry::createCube(0.8f)
      atTransform:Math::Matrix::translation( 0.0f,-3.0f, 0.0f)];
    [self addMesh:Geometry::createSphere(0.7f, 16, 12)
      atTransform:Math::Matrix::translation( 0.0f, 0.0f,-5.0f)];
    [self addMesh:Geometry::createPlane(20.0f, 20.0f, 1)
      atTransform:Math::Matrix::translation( 0.0f,-5.0f, 0.0f)];
}

// ---- MTKViewDelegate ------------------------------------------------------

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    _drawableSize = size;
}

- (void)drawInMTKView:(MTKView *)view {
    @autoreleasepool {
        const float dt = _clock.tick();
        _camera.processMovement(dt,
            _input.moveForward, _input.moveBackward,
            _input.moveLeft,    _input.moveRight,
            _input.moveUp,      _input.moveDown);

        MTLRenderPassDescriptor *passDesc = view.currentRenderPassDescriptor;
        if (!passDesc) return;

        const float aspect = _drawableSize.height > 0
            ? static_cast<float>(_drawableSize.width / _drawableSize.height)
            : 16.0f / 9.0f;
        const matrix_float4x4 viewMat = _camera.getViewMatrix();
        const matrix_float4x4 projMat = _camera.getProjectionMatrix(aspect);

        id<MTLCommandBuffer>        cmd = [_commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> enc = [cmd renderCommandEncoderWithDescriptor:passDesc];

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
    if (_input.mouseLookEnabled) {
        _camera.processMouseMovement(dx, dy);
    }
}

@end
