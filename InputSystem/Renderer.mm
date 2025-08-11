//
// Simple Solid Renderer.mm - Multiple small solid cubes for better visibility
//
#import "Renderer.hpp"
#import "GeometryBuilder.hpp"
#import "MathUtilities.hpp"
#import <simd/simd.h>

// Uniform buffer structure
typedef struct {
    matrix_float4x4 modelViewProjection;
    matrix_float4x4 model;
    matrix_float4x4 view;
    matrix_float4x4 projection;
} Uniforms;

@implementation Renderer {
    id<MTLDevice> _device;
    CAMetalLayer* _layer;
    id<MTLCommandQueue> _commandQueue;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLDepthStencilState> _depthStencilState;
    id<MTLBuffer> _vertexBuffer;
    id<MTLBuffer> _indexBuffer;
    id<MTLBuffer> _uniformBuffer;
    id<MTLTexture> _depthTexture;
    
    NSUInteger _indexCount;
    BOOL _isCleanedUp;
    int _frameCount;
}

- (instancetype)initWithDevice:(id<MTLDevice>)device layer:(CAMetalLayer *)layer {
    if (!(self = [super init])) return nil;
    
    _device = device;
    _layer = layer;
    _commandQueue = [_device newCommandQueue];
    _frameCount = 0;
    
    // Setup pipeline
    if (![self setupPipeline]) {
        NSLog(@"Failed to setup pipeline");
        return nil;
    }
    
    // Setup geometry
    [self setupGeometry];
    
    // Setup uniform buffer
    _uniformBuffer = [_device newBufferWithLength:sizeof(Uniforms)
                                          options:MTLResourceStorageModeManaged];
    
    NSLog(@"🎯 Renderer initialized successfully");
    return self;
}

- (BOOL)setupPipeline {
    NSError* error = nil;
    
    // Load shaders
    id<MTLLibrary> library = [_device newDefaultLibrary];
    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vs_main"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fs_main"];
    
    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"❌ Failed to load shader functions");
        return NO;
    }
    
    NSLog(@"✅ Shaders loaded successfully");
    
    // Configure vertex descriptor
    MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
    
    // Position attribute
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[0].offset = offsetof(Vertex, position);
    vertexDescriptor.attributes[0].bufferIndex = 0;
    
    // Color attribute
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[1].offset = offsetof(Vertex, color);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    
    // Normal attribute
    vertexDescriptor.attributes[2].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[2].offset = offsetof(Vertex, normal);
    vertexDescriptor.attributes[2].bufferIndex = 0;
    
    // TexCoord attribute
    vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[3].offset = offsetof(Vertex, texCoord);
    vertexDescriptor.attributes[3].bufferIndex = 0;
    
    // Layout
    vertexDescriptor.layouts[0].stride = sizeof(Vertex);
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    // Create pipeline descriptor
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
    
    // Create pipeline state
    _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!_pipelineState) {
        NSLog(@"❌ Failed to create pipeline state: %@", error.localizedDescription);
        return NO;
    }
    
    // Create depth stencil state
    MTLDepthStencilDescriptor* depthDescriptor = [MTLDepthStencilDescriptor new];
    depthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
    depthDescriptor.depthWriteEnabled = YES;
    _depthStencilState = [_device newDepthStencilStateWithDescriptor:depthDescriptor];
    
    NSLog(@"✅ Pipeline created successfully");
    return YES;
}

- (void)setupGeometry {
    // Create a small cube that's easy to see
    MeshData cubeMesh = GeometryBuilder::createCube(1.0f); // Small 1x1x1 cube
    
    _vertexBuffer = [_device newBufferWithBytes:cubeMesh.vertexData.bytes
                                          length:cubeMesh.vertexData.length
                                         options:MTLResourceStorageModeManaged];
    
    _indexBuffer = [_device newBufferWithBytes:cubeMesh.indexData.bytes
                                         length:cubeMesh.indexData.length
                                        options:MTLResourceStorageModeManaged];
    
    _indexCount = cubeMesh.indexCount;
    
    NSLog(@"✅ Geometry created: %lu indices for small cube", (unsigned long)_indexCount);
}

- (void)ensureDepthTexture:(NSInteger)width height:(NSInteger)height {
    if (_depthTexture && _depthTexture.width == width && _depthTexture.height == height) {
        return;
    }
    
    MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                                                                           width:width
                                                                                          height:height
                                                                                       mipmapped:NO];
    descriptor.storageMode = MTLStorageModePrivate;
    descriptor.usage = MTLTextureUsageRenderTarget;
    _depthTexture = [_device newTextureWithDescriptor:descriptor];
}

- (void)draw:(double)deltaTime withCamera:(Camera&)camera {
    if (_isCleanedUp) {
        NSLog(@"Warning: Attempting to draw on cleaned up renderer");
        return;
    }
    
    _frameCount++;
    
    // Get drawable
    id<CAMetalDrawable> drawable = [_layer nextDrawable];
    if (!drawable) {
        NSLog(@"❌ No drawable available");
        return;
    }
    
    id<MTLTexture> framebufferTexture = drawable.texture;
    [self ensureDepthTexture:framebufferTexture.width height:framebufferTexture.height];
    
    // Calculate aspect ratio
    float aspect = (float)framebufferTexture.width / (float)framebufferTexture.height;
    
    // Get matrices from camera
    matrix_float4x4 projection = camera.getProjectionMatrix(aspect);
    matrix_float4x4 view = camera.getViewMatrix();
    
    // Create render pass
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    renderPass.colorAttachments[0].texture = framebufferTexture;
    renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
    renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
    renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.1, 0.3, 1.0); // Dark blue background
    renderPass.depthAttachment.texture = _depthTexture;
    renderPass.depthAttachment.loadAction = MTLLoadActionClear;
    renderPass.depthAttachment.storeAction = MTLStoreActionDontCare;
    renderPass.depthAttachment.clearDepth = 1.0;
    
    // Create command buffer and encoder
    id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    
    // Configure render encoder
    [encoder setRenderPipelineState:_pipelineState];
    [encoder setDepthStencilState:_depthStencilState];
    [encoder setCullMode:MTLCullModeBack]; // Enable back-face culling
    [encoder setFrontFacingWinding:MTLWindingCounterClockwise];
    
    // Set vertex buffer
    [encoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
    
    // Define cube positions - create a 3D grid pattern
    vector_float3 cubePositions[] = {
        // Central cross pattern
        {0.0f,  0.0f,  0.0f},   // Center cube (RED from cube colors)
        {3.0f,  0.0f,  0.0f},   // Right
        {-3.0f, 0.0f,  0.0f},   // Left
        {0.0f,  3.0f,  0.0f},   // Up
        {0.0f, -3.0f,  0.0f},   // Down
        {0.0f,  0.0f,  3.0f},   // Forward
        {0.0f,  0.0f, -3.0f},   // Back
        
        // Corner cubes for depth perception
        {2.0f,  2.0f,  2.0f},
        {-2.0f, -2.0f, -2.0f},
        {2.0f, -2.0f,  2.0f},
        {-2.0f,  2.0f, -2.0f},
        
        // Additional reference points
        {5.0f,  0.0f,  0.0f},   // Far right
        {-5.0f, 0.0f,  0.0f},   // Far left
        {0.0f,  0.0f,  5.0f},   // Far forward
        {0.0f,  0.0f, -5.0f},   // Far back
    };
    
    int numCubes = sizeof(cubePositions) / sizeof(cubePositions[0]);
    
    // Draw each cube at different positions
    for (int i = 0; i < numCubes; i++) {
        // Create model matrix for this cube
        matrix_float4x4 translation = Math::Matrix4::translation(
            cubePositions[i].x,
            cubePositions[i].y,
            cubePositions[i].z
        );
        
        // No rotation - keep cubes stationary for easy reference
        matrix_float4x4 model = translation;
        
        // Combine matrices
        matrix_float4x4 viewModel = Math::Matrix4::multiply(view, model);
        matrix_float4x4 mvp = Math::Matrix4::multiply(projection, viewModel);
        
        // Update uniform buffer for this cube
        Uniforms uniforms = {
            .modelViewProjection = mvp,
            .model = model,
            .view = view,
            .projection = projection
        };
        
        memcpy(_uniformBuffer.contents, &uniforms, sizeof(uniforms));
        [_uniformBuffer didModifyRange:NSMakeRange(0, sizeof(uniforms))];
        
        // Set uniform buffer
        [encoder setVertexBuffer:_uniformBuffer offset:0 atIndex:1];
        
        // Draw this cube
        [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                            indexCount:_indexCount
                             indexType:MTLIndexTypeUInt16
                           indexBuffer:_indexBuffer
                       indexBufferOffset:0];
    }
    
    [encoder endEncoding];
    
    // Present drawable
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
    
    // Debug output every 60 frames
    if (_frameCount % 60 == 0) {
        vector_float3 pos = camera.getPosition();
        vector_float3 forward = camera.getForward();
        
        NSLog(@"🎯 Frame %d - Camera pos: (%.2f, %.2f, %.2f)", _frameCount, pos.x, pos.y, pos.z);
        NSLog(@"👁️ Looking direction: (%.3f, %.3f, %.3f)", forward.x, forward.y, forward.z);
        NSLog(@"📦 %d SOLID cubes at various positions", numCubes);
    }
}

- (void)cleanup {
    if (_isCleanedUp) return;
    
    NSLog(@"🧹 Cleaning up Metal resources...");
    
    _vertexBuffer = nil;
    _indexBuffer = nil;
    _uniformBuffer = nil;
    _depthTexture = nil;
    _pipelineState = nil;
    _depthStencilState = nil;
    _commandQueue = nil;
    
    _device = nil;
    _layer = nil;
    
    _isCleanedUp = YES;
}

- (void)dealloc {
    [self cleanup];
    NSLog(@"Renderer deallocated");
}

@end
