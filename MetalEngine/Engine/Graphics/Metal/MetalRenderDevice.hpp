// Engine/Graphics/Metal/MetalRenderDevice.hpp
// Metal-backed implementation of the abstract RenderDevice interface.
#pragma once

#include "../RenderDevice.hpp"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

namespace Engine {
namespace Graphics {
namespace Metal {

class MetalBuffer : public Buffer {
public:
    MetalBuffer(id<MTLDevice> device, BufferType type, BufferUsage usage,
                size_t size, const void* data);

    void update(const void* data, size_t size, size_t offset = 0) override;
    size_t getSize() const override { return size_; }

    id<MTLBuffer> getMetalBuffer() const { return buffer_; }

private:
    id<MTLBuffer> buffer_;
    size_t size_;
};

class MetalTexture : public Texture {
public:
    MetalTexture(id<MTLDevice> device, uint32_t width, uint32_t height,
                 TextureFormat format, TextureUsage usage, const void* data);

    uint32_t getWidth()  const override { return width_; }
    uint32_t getHeight() const override { return height_; }
    TextureFormat getFormat() const override { return format_; }

    id<MTLTexture> getMetalTexture() const { return texture_; }

    static MTLPixelFormat toMetalFormat(TextureFormat format);

private:
    id<MTLTexture> texture_;
    uint32_t width_;
    uint32_t height_;
    TextureFormat format_;
};

class MetalShader : public Shader {
public:
    MetalShader(id<MTLDevice> device, ShaderStage stage,
                const std::string& source, const std::string& name);

    ShaderStage getStage() const override { return stage_; }
    bool isValid() const override          { return function_ != nil; }

    id<MTLFunction> getMetalFunction() const { return function_; }

private:
    id<MTLFunction> function_;
    ShaderStage stage_;
};

class MetalPipeline : public Pipeline {
public:
    MetalPipeline(id<MTLDevice> device, const PipelineDescriptor& descriptor);

    bool isValid() const override { return renderPipelineState_ != nil; }

    const RenderState& getRenderState() const { return renderState_; }
    id<MTLRenderPipelineState> getMetalPipelineState() const { return renderPipelineState_; }
    id<MTLDepthStencilState>   getDepthStencilState()   const { return depthStencilState_; }

    static MTLVertexDescriptor* createMetalVertexDescriptor(const VertexLayout& layout);

private:
    id<MTLRenderPipelineState> renderPipelineState_;
    id<MTLDepthStencilState>   depthStencilState_;
    RenderState                renderState_;
};

class MetalCommandBuffer : public CommandBuffer {
public:
    MetalCommandBuffer(id<MTLCommandBuffer> commandBuffer,
                       id<MTLRenderCommandEncoder> encoder);

    void setViewport(int x, int y, int width, int height) override;
    void setPipeline(PipelinePtr pipeline) override;

    void setVertexBuffer(BufferPtr buffer, uint32_t slot = 0) override;
    void setIndexBuffer(BufferPtr buffer) override;
    void setUniformBuffer(BufferPtr buffer, uint32_t slot) override;

    void drawIndexed(PrimitiveType primitive,
                     uint32_t indexCount,
                     uint32_t firstIndex   = 0,
                     int32_t  vertexOffset = 0) override;

    id<MTLRenderCommandEncoder> getMetalEncoder() const { return encoder_; }

    static MTLPrimitiveType toMetalPrimitiveType(PrimitiveType type);

private:
    id<MTLCommandBuffer>          commandBuffer_;
    id<MTLRenderCommandEncoder>   encoder_;
    std::shared_ptr<MetalPipeline> currentPipeline_;
    id<MTLBuffer>                 indexBuffer_;
};

class MetalRenderDevice : public RenderDevice {
public:
    MetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer);

    const std::string& getDeviceName() const override { return deviceName_; }
    bool isValid() const override                     { return device_ != nil; }

    BufferPtr   createBuffer(BufferType type, BufferUsage usage,
                             size_t size, const void* data = nullptr) override;
    TexturePtr  createTexture(uint32_t width, uint32_t height,
                              TextureFormat format, TextureUsage usage,
                              const void* data = nullptr) override;
    ShaderPtr   createShader(ShaderStage stage,
                             const std::string& source,
                             const std::string& name = "") override;
    PipelinePtr createPipeline(const PipelineDescriptor& descriptor) override;

    void beginFrame() override;
    void endFrame() override;

    CommandBufferPtr beginRenderPass(const RenderPassDescriptor& descriptor) override;
    void             endRenderPass(CommandBufferPtr commandBuffer) override;

    TexturePtr getDepthBuffer() override     { return depthBuffer_; }
    void       resizeBackBuffer(uint32_t width, uint32_t height) override;

private:
    void createDepthBuffer();

    id<MTLDevice>        device_;
    id<MTLCommandQueue>  commandQueue_;
    CAMetalLayer*        layer_;

    id<MTLCommandBuffer> currentCommandBuffer_;
    id<CAMetalDrawable>  currentDrawable_;

    TexturePtr  depthBuffer_;
    std::string deviceName_;

    uint32_t frameWidth_;
    uint32_t frameHeight_;
};

RenderDevicePtr createMetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer);

} // namespace Metal
} // namespace Graphics
} // namespace Engine
