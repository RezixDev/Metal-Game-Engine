// Engine/Graphics/Metal/MetalRenderDevice.mm
// NEW FILE - Metal implementation file
#include "MetalRenderDevice.hpp"
#include <stdexcept>
#include <cstring>

namespace Engine {
namespace Graphics {
namespace Metal {

    // ========================================
    // MetalBuffer Implementation
    // ========================================

    MetalBuffer::MetalBuffer(id<MTLDevice> device, BufferType type, BufferUsage usage, size_t size, const void* data)
        : type_(type), usage_(usage), size_(size), isMapped_(false) {

        MTLResourceOptions options = MTLResourceStorageModeManaged;

        if (data) {
            buffer_ = [device newBufferWithBytes:data length:size options:options];
        } else {
            buffer_ = [device newBufferWithLength:size options:options];
        }

        if (!buffer_) {
            throw std::runtime_error("Failed to create Metal buffer");
        }
    }

    void* MetalBuffer::map() {
        if (isMapped_) {
            throw std::runtime_error("Buffer is already mapped");
        }
        isMapped_ = true;
        return buffer_.contents;
    }

    void MetalBuffer::unmap() {
        if (!isMapped_) {
            throw std::runtime_error("Buffer is not mapped");
        }
        [buffer_ didModifyRange:NSMakeRange(0, size_)];
        isMapped_ = false;
    }

    void MetalBuffer::update(const void* data, size_t size, size_t offset) {
        if (offset + size > size_) {
            throw std::runtime_error("Buffer update out of bounds");
        }

        std::memcpy(static_cast<char*>(buffer_.contents) + offset, data, size);
        [buffer_ didModifyRange:NSMakeRange(offset, size)];
    }

    // ========================================
    // MetalTexture Implementation
    // ========================================

    MetalTexture::MetalTexture(id<MTLDevice> device, uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, const void* data)
        : width_(width), height_(height), format_(format), usage_(usage) {

        MTLTextureDescriptor* descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:toMetalFormat(format)
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

        if (!texture_) {
            throw std::runtime_error("Failed to create Metal texture");
        }

        if (data && usage != TextureUsage::DepthStencil) {
            update(data, width * height * 4); // Assuming 4 bytes per pixel for now
        }
    }

    void MetalTexture::update(const void* data, size_t size) {
        if (!data || usage_ == TextureUsage::DepthStencil) {
            return;
        }

        MTLRegion region = MTLRegionMake2D(0, 0, width_, height_);
        NSUInteger bytesPerRow = width_ * 4; // Assuming 4 bytes per pixel

        [texture_ replaceRegion:region
                    mipmapLevel:0
                      withBytes:data
                    bytesPerRow:bytesPerRow];
    }

    MTLPixelFormat MetalTexture::toMetalFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGBA8: return MTLPixelFormatRGBA8Unorm;
            case TextureFormat::BGRA8: return MTLPixelFormatBGRA8Unorm;
            case TextureFormat::RGB8: return MTLPixelFormatRGBA8Unorm; // Metal doesn't support RGB8 directly
            case TextureFormat::R8: return MTLPixelFormatR8Unorm;
            case TextureFormat::RGBA16F: return MTLPixelFormatRGBA16Float;
            case TextureFormat::RGBA32F: return MTLPixelFormatRGBA32Float;
            case TextureFormat::Depth32Float: return MTLPixelFormatDepth32Float;
            case TextureFormat::Depth24Stencil8: return MTLPixelFormatDepth24Unorm_Stencil8;
            default: return MTLPixelFormatRGBA8Unorm;
        }
    }

    // ========================================
    // MetalShader Implementation
    // ========================================

    MetalShader::MetalShader(id<MTLDevice> device, ShaderStage stage,
                             const std::string& source, const std::string& name)
        : stage_(stage), name_(name) {

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
            if (!library || error) {
                throw std::runtime_error("Failed to compile shader: " + name);
            }
        }

        NSString* functionName = name.empty()
            ? (stage == ShaderStage::Vertex ? @"vs_main" : @"fs_main")
            : [NSString stringWithUTF8String:name.c_str()];

        function_ = [library newFunctionWithName:functionName];
        if (!function_) {
            throw std::runtime_error("Failed to find shader function '" + name + "' in Metal library");
        }
    }

    // ========================================
    // MetalPipeline Implementation
    // ========================================

    MetalPipeline::MetalPipeline(id<MTLDevice> device, const PipelineDescriptor& descriptor)
        : descriptor_(descriptor) {

        MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];

        // Set shaders
        auto vertexShader = std::static_pointer_cast<MetalShader>(descriptor.vertexShader);
        auto fragmentShader = std::static_pointer_cast<MetalShader>(descriptor.fragmentShader);

        pipelineDescriptor.vertexFunction = vertexShader->getMetalFunction();
        pipelineDescriptor.fragmentFunction = fragmentShader->getMetalFunction();

        // Set vertex descriptor
        pipelineDescriptor.vertexDescriptor = createMetalVertexDescriptor(descriptor.vertexLayout);

        // Set color attachments
        for (size_t i = 0; i < descriptor.colorFormats.size(); ++i) {
            pipelineDescriptor.colorAttachments[i].pixelFormat = MetalTexture::toMetalFormat(descriptor.colorFormats[i]);
        }

        // Set depth format
        pipelineDescriptor.depthAttachmentPixelFormat = MetalTexture::toMetalFormat(descriptor.depthFormat);

        // Create pipeline state
        NSError* error = nil;
        renderPipelineState_ = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

        if (!renderPipelineState_ || error) {
            throw std::runtime_error("Failed to create render pipeline state");
        }

        // Create depth stencil state
        MTLDepthStencilDescriptor* depthDescriptor = [[MTLDepthStencilDescriptor alloc] init];
        depthDescriptor.depthCompareFunction = MTLCompareFunctionLess;
        depthDescriptor.depthWriteEnabled = descriptor.renderState.depthWriteEnabled;

        depthStencilState_ = [device newDepthStencilStateWithDescriptor:depthDescriptor];
    }

    MTLVertexDescriptor* MetalPipeline::createMetalVertexDescriptor(const VertexLayout& layout) {
        MTLVertexDescriptor* vertexDescriptor = [[MTLVertexDescriptor alloc] init];

        for (size_t i = 0; i < layout.attributes.size(); ++i) {
            const auto& attr = layout.attributes[i];

            MTLVertexFormat format;
            switch (attr.format) {
                case VertexFormat::Float: format = MTLVertexFormatFloat; break;
                case VertexFormat::Float2: format = MTLVertexFormatFloat2; break;
                case VertexFormat::Float3: format = MTLVertexFormatFloat3; break;
                case VertexFormat::Float4: format = MTLVertexFormatFloat4; break;
                case VertexFormat::UInt: format = MTLVertexFormatUInt; break;
                case VertexFormat::UInt2: format = MTLVertexFormatUInt2; break;
                case VertexFormat::UInt3: format = MTLVertexFormatUInt3; break;
                case VertexFormat::UInt4: format = MTLVertexFormatUInt4; break;
                default: format = MTLVertexFormatFloat3; break;
            }

            vertexDescriptor.attributes[i].format = format;
            vertexDescriptor.attributes[i].offset = attr.offset;
            vertexDescriptor.attributes[i].bufferIndex = attr.bufferIndex;
        }

        vertexDescriptor.layouts[0].stride = layout.stride;
        vertexDescriptor.layouts[0].stepRate = 1;
        vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

        return vertexDescriptor;
    }

    // ========================================
    // MetalCommandBuffer Implementation
    // ========================================

    MetalCommandBuffer::MetalCommandBuffer(id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> encoder)
        : commandBuffer_(commandBuffer), encoder_(encoder), indexBuffer_(nil) {
    }

    void MetalCommandBuffer::setViewport(int x, int y, int width, int height) {
        MTLViewport viewport = {
            .originX = static_cast<double>(x),
            .originY = static_cast<double>(y),
            .width = static_cast<double>(width),
            .height = static_cast<double>(height),
            .znear = 0.0,
            .zfar = 1.0
        };
        [encoder_ setViewport:viewport];
    }

    void MetalCommandBuffer::setScissor(int x, int y, int width, int height) {
        MTLScissorRect scissor = {
            .x = static_cast<NSUInteger>(x),
            .y = static_cast<NSUInteger>(y),
            .width = static_cast<NSUInteger>(width),
            .height = static_cast<NSUInteger>(height)
        };
        [encoder_ setScissorRect:scissor];
    }

    void MetalCommandBuffer::setPipeline(PipelinePtr pipeline) {
        currentPipeline_ = std::static_pointer_cast<MetalPipeline>(pipeline);
        [encoder_ setRenderPipelineState:currentPipeline_->getMetalPipelineState()];
        [encoder_ setDepthStencilState:currentPipeline_->getDepthStencilState()];

        // Set cull mode based on render state
        const auto& renderState = currentPipeline_->getDescriptor().renderState;
        switch (renderState.cullMode) {
            case CullMode::None:
                [encoder_ setCullMode:MTLCullModeNone];
                break;
            case CullMode::Front:
                [encoder_ setCullMode:MTLCullModeFront];
                break;
            case CullMode::Back:
                [encoder_ setCullMode:MTLCullModeBack];
                break;
        }

        // Set winding order
        MTLWinding winding = (renderState.windingOrder == WindingOrder::Clockwise) ?
            MTLWindingClockwise : MTLWindingCounterClockwise;
        [encoder_ setFrontFacingWinding:winding];
    }

    void MetalCommandBuffer::setVertexBuffer(BufferPtr buffer, uint32_t slot) {
        auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
        [encoder_ setVertexBuffer:metalBuffer->getMetalBuffer() offset:0 atIndex:slot];
    }

    void MetalCommandBuffer::setIndexBuffer(BufferPtr buffer) {
        auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
        indexBuffer_ = metalBuffer->getMetalBuffer();
    }

    void MetalCommandBuffer::setUniformBuffer(BufferPtr buffer, uint32_t slot) {
        auto metalBuffer = std::static_pointer_cast<MetalBuffer>(buffer);
        // Bind to both the vertex and fragment stages so either stage can
        // read the same uniforms at the same buffer slot.
        [encoder_ setVertexBuffer:metalBuffer->getMetalBuffer()   offset:0 atIndex:slot];
        [encoder_ setFragmentBuffer:metalBuffer->getMetalBuffer() offset:0 atIndex:slot];
    }

    void MetalCommandBuffer::setTexture(TexturePtr texture, uint32_t slot) {
        auto metalTexture = std::static_pointer_cast<MetalTexture>(texture);
        [encoder_ setFragmentTexture:metalTexture->getMetalTexture() atIndex:slot];
    }

    void MetalCommandBuffer::setSamplerState(const SamplerState& sampler, uint32_t slot) {
        // For now, use default sampler. In a full implementation, we'd create MTLSamplerState
        // based on the SamplerState parameters
    }

    void MetalCommandBuffer::draw(PrimitiveType primitive, uint32_t vertexCount, uint32_t firstVertex) {
        MTLPrimitiveType metalPrimitive = toMetalPrimitiveType(primitive);
        [encoder_ drawPrimitives:metalPrimitive vertexStart:firstVertex vertexCount:vertexCount];
    }

    void MetalCommandBuffer::drawIndexed(PrimitiveType primitive, uint32_t indexCount, uint32_t firstIndex, int32_t vertexOffset) {
        if (!indexBuffer_) {
            throw std::runtime_error("No index buffer set for indexed draw call");
        }

        MTLPrimitiveType metalPrimitive = toMetalPrimitiveType(primitive);
        [encoder_ drawIndexedPrimitives:metalPrimitive
                             indexCount:indexCount
                              indexType:MTLIndexTypeUInt16
                            indexBuffer:indexBuffer_
                      indexBufferOffset:firstIndex * sizeof(uint16_t)];
    }

    void MetalCommandBuffer::drawInstanced(PrimitiveType primitive, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
        MTLPrimitiveType metalPrimitive = toMetalPrimitiveType(primitive);
        [encoder_ drawPrimitives:metalPrimitive
                     vertexStart:firstVertex
                     vertexCount:vertexCount
                   instanceCount:instanceCount
                    baseInstance:firstInstance];
    }

    void MetalCommandBuffer::setUniform(const std::string& name, float value) {
        // Simplified uniform setting - in a full implementation, we'd track uniform locations
        // For now, this is a placeholder
    }

    void MetalCommandBuffer::setUniform(const std::string& name, const Math::Vec2& value) {
        // Placeholder - implement uniform tracking system
    }

    void MetalCommandBuffer::setUniform(const std::string& name, const Math::Vec3& value) {
        // Placeholder - implement uniform tracking system
    }

    void MetalCommandBuffer::setUniform(const std::string& name, const Math::Vec4& value) {
        // Placeholder - implement uniform tracking system
    }

    void MetalCommandBuffer::setUniform(const std::string& name, const Math::Mat4& value) {
        // Placeholder - implement uniform tracking system
    }

    MTLPrimitiveType MetalCommandBuffer::toMetalPrimitiveType(PrimitiveType type) {
        switch (type) {
            case PrimitiveType::Points: return MTLPrimitiveTypePoint;
            case PrimitiveType::Lines: return MTLPrimitiveTypeLine;
            case PrimitiveType::LineStrip: return MTLPrimitiveTypeLineStrip;
            case PrimitiveType::Triangles: return MTLPrimitiveTypeTriangle;
            case PrimitiveType::TriangleStrip: return MTLPrimitiveTypeTriangleStrip;
            default: return MTLPrimitiveTypeTriangle;
        }
    }

    // ========================================
    // MetalRenderDevice Implementation
    // ========================================

    MetalRenderDevice::MetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer)
        : device_(device),
          layer_(layer),
          currentCommandBuffer_(nil),
          currentDrawable_(nil),
          frameWidth_(0),
          frameHeight_(0) {

        deviceName_ = std::string([[device name] UTF8String]);
        commandQueue_ = [device newCommandQueue];
        if (!commandQueue_) {
            throw std::runtime_error("Failed to create Metal command queue");
        }

        layer_.device          = device_;
        layer_.pixelFormat     = MTLPixelFormatBGRA8Unorm;
        layer_.framebufferOnly = YES;

        CGSize ds = layer_.drawableSize;
        if (ds.width < 1 || ds.height < 1) {
            CGFloat scale = layer_.contentsScale > 0 ? layer_.contentsScale : 1.0;
            CGSize pts = layer_.bounds.size;
            layer_.drawableSize = CGSizeMake(std::max<CGFloat>(1, pts.width  * scale),
                                             std::max<CGFloat>(1, pts.height * scale));
            ds = layer_.drawableSize;
        }
        frameWidth_  = static_cast<uint32_t>(ds.width);
        frameHeight_ = static_cast<uint32_t>(ds.height);

        createDepthBuffer();
    }

    BufferPtr MetalRenderDevice::createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data) {
        return std::make_shared<MetalBuffer>(device_, type, usage, size, data);
    }

    TexturePtr MetalRenderDevice::createTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, const void* data) {
        return std::make_shared<MetalTexture>(device_, width, height, format, usage, data);
    }

    ShaderPtr MetalRenderDevice::createShader(ShaderStage stage, const std::string& source, const std::string& name) {
        return std::make_shared<MetalShader>(device_, stage, source, name);
    }

    PipelinePtr MetalRenderDevice::createPipeline(const PipelineDescriptor& descriptor) {
        return std::make_shared<MetalPipeline>(device_, descriptor);
    }

    void MetalRenderDevice::beginFrame() {
        currentCommandBuffer_ = [commandQueue_ commandBuffer];
        currentDrawable_ = [layer_ nextDrawable];

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

    void MetalRenderDevice::present() {
        if (currentCommandBuffer_ && currentDrawable_) {
            [currentCommandBuffer_ presentDrawable:currentDrawable_];
        }
    }

    CommandBufferPtr MetalRenderDevice::beginRenderPass(const RenderPassDescriptor& descriptor) {
        if (!currentCommandBuffer_) {
            throw std::runtime_error("No active command buffer for render pass");
        }

        MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];

        // Color attachment
        if (descriptor.colorTarget) {
            auto metalTexture = std::static_pointer_cast<MetalTexture>(descriptor.colorTarget);
            renderPassDescriptor.colorAttachments[0].texture = metalTexture->getMetalTexture();
        } else {
            // Use backbuffer
            renderPassDescriptor.colorAttachments[0].texture = currentDrawable_.texture;
        }

        renderPassDescriptor.colorAttachments[0].loadAction =
            (descriptor.colorLoadAction == LoadAction::Clear) ? MTLLoadActionClear : MTLLoadActionLoad;
        renderPassDescriptor.colorAttachments[0].storeAction =
            (descriptor.colorStoreAction == StoreAction::Store) ? MTLStoreActionStore : MTLStoreActionDontCare;
        renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(
            descriptor.clearValue.color.x,
            descriptor.clearValue.color.y,
            descriptor.clearValue.color.z,
            descriptor.clearValue.color.w
        );

        // Depth attachment
        if (descriptor.depthTarget) {
            auto metalTexture = std::static_pointer_cast<MetalTexture>(descriptor.depthTarget);
            renderPassDescriptor.depthAttachment.texture = metalTexture->getMetalTexture();
        } else if (depthBuffer_) {
            auto metalTexture = std::static_pointer_cast<MetalTexture>(depthBuffer_);
            renderPassDescriptor.depthAttachment.texture = metalTexture->getMetalTexture();
        }

        if (renderPassDescriptor.depthAttachment.texture) {
            renderPassDescriptor.depthAttachment.loadAction =
                (descriptor.depthLoadAction == LoadAction::Clear) ? MTLLoadActionClear : MTLLoadActionLoad;
            renderPassDescriptor.depthAttachment.storeAction =
                (descriptor.depthStoreAction == StoreAction::Store) ? MTLStoreActionStore : MTLStoreActionDontCare;
            renderPassDescriptor.depthAttachment.clearDepth = descriptor.clearValue.depth;
        }

        id<MTLRenderCommandEncoder> encoder = [currentCommandBuffer_ renderCommandEncoderWithDescriptor:renderPassDescriptor];

        if (!encoder) {
            throw std::runtime_error("Failed to create render command encoder");
        }

        return std::make_shared<MetalCommandBuffer>(currentCommandBuffer_, encoder);
    }

    void MetalRenderDevice::endRenderPass(CommandBufferPtr commandBuffer) {
        auto metalCommandBuffer = std::static_pointer_cast<MetalCommandBuffer>(commandBuffer);
        [metalCommandBuffer->getMetalEncoder() endEncoding];
    }

    TexturePtr MetalRenderDevice::getBackBuffer() {
        // The current drawable is owned by the layer and consumed each frame.
        // Render passes reach it via currentDrawable_.texture inside beginRenderPass,
        // so there is no stable Texture handle to expose here.
        return nullptr;
    }

    TexturePtr MetalRenderDevice::getDepthBuffer() {
        return depthBuffer_;
    }

    void MetalRenderDevice::resizeBackBuffer(uint32_t width, uint32_t height) {
        frameWidth_ = width;
        frameHeight_ = height;
        layer_.drawableSize = CGSizeMake(width, height);
        createDepthBuffer();
    }

    uint32_t MetalRenderDevice::getMaxTextureSize() const {
        return 16384; // Metal's typical max texture size
    }

    uint32_t MetalRenderDevice::getMaxVertexAttributes() const {
        return 31; // Metal's max vertex attributes
    }

    void MetalRenderDevice::setDebugName(const std::string& name) {
        // Metal debug naming would go here
    }

    void MetalRenderDevice::beginDebugGroup(const std::string& name) {
        if (currentCommandBuffer_) {
            [currentCommandBuffer_ pushDebugGroup:[NSString stringWithUTF8String:name.c_str()]];
        }
    }

    void MetalRenderDevice::endDebugGroup() {
        if (currentCommandBuffer_) {
            [currentCommandBuffer_ popDebugGroup];
        }
    }

    void MetalRenderDevice::createDepthBuffer() {
        if (frameWidth_ > 0 && frameHeight_ > 0) {
            depthBuffer_ = createTexture(frameWidth_, frameHeight_, TextureFormat::Depth32Float, TextureUsage::DepthStencil);
        }
    }

    // ========================================
    // Factory Function
    // ========================================

    RenderDevicePtr createMetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer) {
        return std::make_shared<MetalRenderDevice>(device, layer);
    }

} // namespace Metal
} // namespace Graphics
} // namespace Engine