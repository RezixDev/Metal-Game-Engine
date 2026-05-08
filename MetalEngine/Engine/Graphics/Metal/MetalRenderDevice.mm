// Engine/Graphics/Metal/MetalRenderDevice.mm
#include "MetalRenderDevice.hpp"
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace Engine {
namespace Graphics {
namespace Metal {

// ---- MetalBuffer ----------------------------------------------------------

MetalBuffer::MetalBuffer(id<MTLDevice> device, BufferType /*type*/,
                         BufferUsage /*usage*/, size_t size, const void* data)
    : size_(size) {

    constexpr MTLResourceOptions options = MTLResourceStorageModeManaged;
    buffer_ = data ? [device newBufferWithBytes:data length:size options:options]
                   : [device newBufferWithLength:size options:options];

    if (!buffer_) throw std::runtime_error("Failed to create Metal buffer");
}

void MetalBuffer::update(const void* data, size_t size, size_t offset) {
    if (offset + size > size_) throw std::runtime_error("Buffer update out of bounds");
    std::memcpy(static_cast<char*>(buffer_.contents) + offset, data, size);
    [buffer_ didModifyRange:NSMakeRange(offset, size)];
}

// ---- MetalTexture ---------------------------------------------------------

MetalTexture::MetalTexture(id<MTLDevice> device, uint32_t width, uint32_t height,
                           TextureFormat format, TextureUsage usage,
                           const void* /*data*/)
    : width_(width), height_(height), format_(format) {

    MTLTextureDescriptor* descriptor =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:toMetalFormat(format)
                                                            width:width
                                                           height:height
                                                        mipmapped:NO];
    switch (usage) {
        case TextureUsage::ShaderRead:
            descriptor.usage = MTLTextureUsageShaderRead;
            break;
        case TextureUsage::RenderTarget:
            descriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
            break;
        case TextureUsage::DepthStencil:
            descriptor.usage = MTLTextureUsageRenderTarget;
            descriptor.storageMode = MTLStorageModePrivate;
            break;
    }

    texture_ = [device newTextureWithDescriptor:descriptor];
    if (!texture_) throw std::runtime_error("Failed to create Metal texture");
}

MTLPixelFormat MetalTexture::toMetalFormat(TextureFormat format) {
    switch (format) {
        case TextureFormat::RGBA8:           return MTLPixelFormatRGBA8Unorm;
        case TextureFormat::BGRA8:           return MTLPixelFormatBGRA8Unorm;
        case TextureFormat::RGB8:            return MTLPixelFormatRGBA8Unorm; // no native RGB8
        case TextureFormat::R8:              return MTLPixelFormatR8Unorm;
        case TextureFormat::RGBA16F:         return MTLPixelFormatRGBA16Float;
        case TextureFormat::RGBA32F:         return MTLPixelFormatRGBA32Float;
        case TextureFormat::Depth32Float:    return MTLPixelFormatDepth32Float;
        case TextureFormat::Depth24Stencil8: return MTLPixelFormatDepth24Unorm_Stencil8;
    }
    return MTLPixelFormatRGBA8Unorm;
}

// ---- MetalShader ----------------------------------------------------------

MetalShader::MetalShader(id<MTLDevice> device, ShaderStage stage,
                         const std::string& source, const std::string& name)
    : stage_(stage) {

    id<MTLLibrary> library = nil;
    NSError* error = nil;

    if (source.empty()) {
        library = [device newDefaultLibrary];
        if (!library) {
            throw std::runtime_error(
                "Failed to load default.metallib. "
                "Ensure Shaders.metal is compiled and copied into the bundle's Resources folder.");
        }
    } else {
        library = [device newLibraryWithSource:[NSString stringWithUTF8String:source.c_str()]
                                       options:nil
                                         error:&error];
        if (!library || error) throw std::runtime_error("Failed to compile shader: " + name);
    }

    NSString* functionName = name.empty()
        ? (stage == ShaderStage::Vertex ? @"vs_main" : @"fs_main")
        : [NSString stringWithUTF8String:name.c_str()];

    function_ = [library newFunctionWithName:functionName];
    if (!function_) {
        throw std::runtime_error("Failed to find shader function '" + name + "' in Metal library");
    }
}

// ---- MetalPipeline --------------------------------------------------------

MetalPipeline::MetalPipeline(id<MTLDevice> device, const PipelineDescriptor& descriptor)
    : renderState_(descriptor.renderState) {

    MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];

    auto vertexShader   = std::static_pointer_cast<MetalShader>(descriptor.vertexShader);
    auto fragmentShader = std::static_pointer_cast<MetalShader>(descriptor.fragmentShader);
    pipelineDescriptor.vertexFunction   = vertexShader->getMetalFunction();
    pipelineDescriptor.fragmentFunction = fragmentShader->getMetalFunction();
    pipelineDescriptor.vertexDescriptor = createMetalVertexDescriptor(descriptor.vertexLayout);

    for (size_t i = 0; i < descriptor.colorFormats.size(); ++i) {
        pipelineDescriptor.colorAttachments[i].pixelFormat =
            MetalTexture::toMetalFormat(descriptor.colorFormats[i]);
    }
    pipelineDescriptor.depthAttachmentPixelFormat =
        MetalTexture::toMetalFormat(descriptor.depthFormat);

    NSError* error = nil;
    renderPipelineState_ = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor
                                                                  error:&error];
    if (!renderPipelineState_ || error) {
        throw std::runtime_error("Failed to create render pipeline state");
    }

    MTLDepthStencilDescriptor* depthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
    depthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
    depthDescriptor.depthWriteEnabled    = descriptor.renderState.depthWriteEnabled;
    depthStencilState_ = [device newDepthStencilStateWithDescriptor:depthDescriptor];
}

MTLVertexDescriptor* MetalPipeline::createMetalVertexDescriptor(const VertexLayout& layout) {
    MTLVertexDescriptor* descriptor = [[MTLVertexDescriptor alloc] init];

    for (size_t i = 0; i < layout.attributes.size(); ++i) {
        const auto& attr = layout.attributes[i];

        MTLVertexFormat format = MTLVertexFormatFloat3;
        switch (attr.format) {
            case VertexFormat::Float:  format = MTLVertexFormatFloat;  break;
            case VertexFormat::Float2: format = MTLVertexFormatFloat2; break;
            case VertexFormat::Float3: format = MTLVertexFormatFloat3; break;
            case VertexFormat::Float4: format = MTLVertexFormatFloat4; break;
            case VertexFormat::UInt:   format = MTLVertexFormatUInt;   break;
            case VertexFormat::UInt2:  format = MTLVertexFormatUInt2;  break;
            case VertexFormat::UInt3:  format = MTLVertexFormatUInt3;  break;
            case VertexFormat::UInt4:  format = MTLVertexFormatUInt4;  break;
        }

        descriptor.attributes[i].format      = format;
        descriptor.attributes[i].offset      = attr.offset;
        descriptor.attributes[i].bufferIndex = attr.bufferIndex;
    }

    descriptor.layouts[0].stride       = layout.stride;
    descriptor.layouts[0].stepRate     = 1;
    descriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    return descriptor;
}

// ---- MetalCommandBuffer ---------------------------------------------------

MetalCommandBuffer::MetalCommandBuffer(id<MTLCommandBuffer> commandBuffer,
                                       id<MTLRenderCommandEncoder> encoder)
    : commandBuffer_(commandBuffer), encoder_(encoder), indexBuffer_(nil) {}

void MetalCommandBuffer::setViewport(int x, int y, int width, int height) {
    MTLViewport viewport = {
        .originX = static_cast<double>(x),
        .originY = static_cast<double>(y),
        .width   = static_cast<double>(width),
        .height  = static_cast<double>(height),
        .znear   = 0.0,
        .zfar    = 1.0,
    };
    [encoder_ setViewport:viewport];
}

void MetalCommandBuffer::setPipeline(PipelinePtr pipeline) {
    currentPipeline_ = std::static_pointer_cast<MetalPipeline>(pipeline);
    [encoder_ setRenderPipelineState:currentPipeline_->getMetalPipelineState()];
    [encoder_ setDepthStencilState:currentPipeline_->getDepthStencilState()];

    const auto& state = currentPipeline_->getRenderState();
    switch (state.cullMode) {
        case CullMode::None:  [encoder_ setCullMode:MTLCullModeNone];  break;
        case CullMode::Front: [encoder_ setCullMode:MTLCullModeFront]; break;
        case CullMode::Back:  [encoder_ setCullMode:MTLCullModeBack];  break;
    }
    [encoder_ setFrontFacingWinding:
        state.windingOrder == WindingOrder::Clockwise
            ? MTLWindingClockwise
            : MTLWindingCounterClockwise];
}

void MetalCommandBuffer::setVertexBuffer(BufferPtr buffer, uint32_t slot) {
    auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
    [encoder_ setVertexBuffer:metalBuffer->getMetalBuffer() offset:0 atIndex:slot];
}

void MetalCommandBuffer::setIndexBuffer(BufferPtr buffer) {
    indexBuffer_ = std::static_pointer_cast<MetalBuffer>(buffer)->getMetalBuffer();
}

void MetalCommandBuffer::setUniformBuffer(BufferPtr buffer, uint32_t slot) {
    auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
    // Bind to both stages so vertex and fragment shaders can read from
    // the same uniform slot.
    [encoder_ setVertexBuffer:metalBuffer->getMetalBuffer()   offset:0 atIndex:slot];
    [encoder_ setFragmentBuffer:metalBuffer->getMetalBuffer() offset:0 atIndex:slot];
}

void MetalCommandBuffer::drawIndexed(PrimitiveType primitive,
                                     uint32_t indexCount,
                                     uint32_t firstIndex,
                                     int32_t /*vertexOffset*/) {
    if (!indexBuffer_) throw std::runtime_error("No index buffer set for indexed draw call");
    [encoder_ drawIndexedPrimitives:toMetalPrimitiveType(primitive)
                         indexCount:indexCount
                          indexType:MTLIndexTypeUInt16
                        indexBuffer:indexBuffer_
                  indexBufferOffset:firstIndex * sizeof(uint16_t)];
}

MTLPrimitiveType MetalCommandBuffer::toMetalPrimitiveType(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Points:        return MTLPrimitiveTypePoint;
        case PrimitiveType::Lines:         return MTLPrimitiveTypeLine;
        case PrimitiveType::LineStrip:     return MTLPrimitiveTypeLineStrip;
        case PrimitiveType::Triangles:     return MTLPrimitiveTypeTriangle;
        case PrimitiveType::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
    }
    return MTLPrimitiveTypeTriangle;
}

// ---- MetalRenderDevice ----------------------------------------------------

MetalRenderDevice::MetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer)
    : device_(device),
      layer_(layer),
      currentCommandBuffer_(nil),
      currentDrawable_(nil),
      frameWidth_(0),
      frameHeight_(0) {

    deviceName_   = std::string([[device name] UTF8String]);
    commandQueue_ = [device newCommandQueue];
    if (!commandQueue_) throw std::runtime_error("Failed to create Metal command queue");

    layer_.device          = device_;
    layer_.pixelFormat     = MTLPixelFormatBGRA8Unorm;
    layer_.framebufferOnly = YES;

    CGSize ds = layer_.drawableSize;
    if (ds.width < 1 || ds.height < 1) {
        const CGFloat scale = layer_.contentsScale > 0 ? layer_.contentsScale : 1.0;
        const CGSize  pts   = layer_.bounds.size;
        layer_.drawableSize = CGSizeMake(std::max<CGFloat>(1, pts.width  * scale),
                                         std::max<CGFloat>(1, pts.height * scale));
        ds = layer_.drawableSize;
    }
    frameWidth_  = static_cast<uint32_t>(ds.width);
    frameHeight_ = static_cast<uint32_t>(ds.height);

    createDepthBuffer();
}

BufferPtr MetalRenderDevice::createBuffer(BufferType type, BufferUsage usage,
                                          size_t size, const void* data) {
    return std::make_shared<MetalBuffer>(device_, type, usage, size, data);
}

TexturePtr MetalRenderDevice::createTexture(uint32_t width, uint32_t height,
                                            TextureFormat format, TextureUsage usage,
                                            const void* data) {
    return std::make_shared<MetalTexture>(device_, width, height, format, usage, data);
}

ShaderPtr MetalRenderDevice::createShader(ShaderStage stage,
                                          const std::string& source,
                                          const std::string& name) {
    return std::make_shared<MetalShader>(device_, stage, source, name);
}

PipelinePtr MetalRenderDevice::createPipeline(const PipelineDescriptor& descriptor) {
    return std::make_shared<MetalPipeline>(device_, descriptor);
}

void MetalRenderDevice::beginFrame() {
    currentCommandBuffer_ = [commandQueue_ commandBuffer];
    currentDrawable_      = [layer_ nextDrawable];
    if (!currentCommandBuffer_ || !currentDrawable_) {
        throw std::runtime_error("Failed to begin Metal frame");
    }
}

void MetalRenderDevice::endFrame() {
    if (currentCommandBuffer_) {
        if (currentDrawable_) {
            [currentCommandBuffer_ presentDrawable:currentDrawable_];
        }
        [currentCommandBuffer_ commit];
        currentCommandBuffer_ = nil;
        currentDrawable_      = nil;
    }
}

CommandBufferPtr MetalRenderDevice::beginRenderPass(const RenderPassDescriptor& descriptor) {
    if (!currentCommandBuffer_) {
        throw std::runtime_error("No active command buffer for render pass");
    }

    MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];

    if (descriptor.colorTarget) {
        renderPassDescriptor.colorAttachments[0].texture =
            std::static_pointer_cast<MetalTexture>(descriptor.colorTarget)->getMetalTexture();
    } else {
        renderPassDescriptor.colorAttachments[0].texture = currentDrawable_.texture;
    }
    renderPassDescriptor.colorAttachments[0].loadAction =
        descriptor.colorLoadAction == LoadAction::Clear ? MTLLoadActionClear : MTLLoadActionLoad;
    renderPassDescriptor.colorAttachments[0].storeAction =
        descriptor.colorStoreAction == StoreAction::Store ? MTLStoreActionStore : MTLStoreActionDontCare;
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(
        descriptor.clearValue.color.x, descriptor.clearValue.color.y,
        descriptor.clearValue.color.z, descriptor.clearValue.color.w);

    if (descriptor.depthTarget) {
        renderPassDescriptor.depthAttachment.texture =
            std::static_pointer_cast<MetalTexture>(descriptor.depthTarget)->getMetalTexture();
    } else if (depthBuffer_) {
        renderPassDescriptor.depthAttachment.texture =
            std::static_pointer_cast<MetalTexture>(depthBuffer_)->getMetalTexture();
    }

    if (renderPassDescriptor.depthAttachment.texture) {
        renderPassDescriptor.depthAttachment.loadAction =
            descriptor.depthLoadAction == LoadAction::Clear ? MTLLoadActionClear : MTLLoadActionLoad;
        renderPassDescriptor.depthAttachment.storeAction =
            descriptor.depthStoreAction == StoreAction::Store ? MTLStoreActionStore : MTLStoreActionDontCare;
        renderPassDescriptor.depthAttachment.clearDepth = descriptor.clearValue.depth;
    }

    id<MTLRenderCommandEncoder> encoder =
        [currentCommandBuffer_ renderCommandEncoderWithDescriptor:renderPassDescriptor];
    if (!encoder) throw std::runtime_error("Failed to create render command encoder");

    return std::make_shared<MetalCommandBuffer>(currentCommandBuffer_, encoder);
}

void MetalRenderDevice::endRenderPass(CommandBufferPtr commandBuffer) {
    [std::static_pointer_cast<MetalCommandBuffer>(commandBuffer)->getMetalEncoder() endEncoding];
}

void MetalRenderDevice::resizeBackBuffer(uint32_t width, uint32_t height) {
    frameWidth_  = width;
    frameHeight_ = height;
    layer_.drawableSize = CGSizeMake(width, height);
    createDepthBuffer();
}

void MetalRenderDevice::createDepthBuffer() {
    if (frameWidth_ > 0 && frameHeight_ > 0) {
        depthBuffer_ = createTexture(frameWidth_, frameHeight_,
                                     TextureFormat::Depth32Float,
                                     TextureUsage::DepthStencil);
    }
}

RenderDevicePtr createMetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer) {
    return std::make_shared<MetalRenderDevice>(device, layer);
}

} // namespace Metal
} // namespace Graphics
} // namespace Engine
