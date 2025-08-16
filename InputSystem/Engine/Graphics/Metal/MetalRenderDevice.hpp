// Engine/Graphics/Metal/MetalRenderDevice.hpp
// NEW FILE - Metal-specific implementation of RenderDevice
#pragma once

#include "../RenderDevice.hpp"
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

namespace Engine {
namespace Graphics {
namespace Metal {

    // Forward declarations for Metal-specific classes
    class MetalBuffer;
    class MetalTexture;
    class MetalShader;
    class MetalPipeline;
    class MetalCommandBuffer;

    // ========================================
    // Metal Buffer Implementation
    // ========================================

    class MetalBuffer : public Buffer {
    public:
        MetalBuffer(id<MTLDevice> device, BufferType type, BufferUsage usage, size_t size, const void* data);
        ~MetalBuffer() override = default;

        void* map() override;
        void unmap() override;
        void update(const void* data, size_t size, size_t offset = 0) override;

        size_t getSize() const override { return size_; }
        BufferType getType() const override { return type_; }
        BufferUsage getUsage() const override { return usage_; }

        id<MTLBuffer> getMetalBuffer() const { return buffer_; }

    private:
        id<MTLBuffer> buffer_;
        BufferType type_;
        BufferUsage usage_;
        size_t size_;
        bool isMapped_;
    };

    // ========================================
    // Metal Texture Implementation
    // ========================================

    class MetalTexture : public Texture {
    public:
        MetalTexture(id<MTLDevice> device, uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, const void* data);
        ~MetalTexture() override = default;

        uint32_t getWidth() const override { return width_; }
        uint32_t getHeight() const override { return height_; }
        uint32_t getDepth() const override { return 1; }
        TextureFormat getFormat() const override { return format_; }
        TextureUsage getUsage() const override { return usage_; }

        void update(const void* data, size_t size) override;

        id<MTLTexture> getMetalTexture() const { return texture_; }

        static MTLPixelFormat toMetalFormat(TextureFormat format);

    private:
        id<MTLTexture> texture_;
        uint32_t width_;
        uint32_t height_;
        TextureFormat format_;
        TextureUsage usage_;
    };

    // ========================================
    // Metal Shader Implementation
    // ========================================

    class MetalShader : public Shader {
    public:
        MetalShader(id<MTLDevice> device, ShaderStage stage, const std::string& source, const std::string& name);
        ~MetalShader() override = default;

        ShaderStage getStage() const override { return stage_; }
        const std::string& getName() const override { return name_; }
        bool isValid() const override { return function_ != nil; }

        id<MTLFunction> getMetalFunction() const { return function_; }

    private:
        id<MTLFunction> function_;
        ShaderStage stage_;
        std::string name_;
    };

    // ========================================
    // Metal Pipeline Implementation
    // ========================================

    class MetalPipeline : public Pipeline {
    public:
        MetalPipeline(id<MTLDevice> device, const PipelineDescriptor& descriptor);
        ~MetalPipeline() override = default;

        const PipelineDescriptor& getDescriptor() const override { return descriptor_; }
        bool isValid() const override { return renderPipelineState_ != nil; }

        id<MTLRenderPipelineState> getMetalPipelineState() const { return renderPipelineState_; }
        id<MTLDepthStencilState> getDepthStencilState() const { return depthStencilState_; }

        static MTLVertexDescriptor* createMetalVertexDescriptor(const VertexLayout& layout);

    private:
        id<MTLRenderPipelineState> renderPipelineState_;
        id<MTLDepthStencilState> depthStencilState_;
        PipelineDescriptor descriptor_;
    };

    // ========================================
    // Metal Command Buffer Implementation
    // ========================================

    class MetalCommandBuffer : public CommandBuffer {
    public:
        MetalCommandBuffer(id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> encoder);
        ~MetalCommandBuffer() override = default;

        // Render state
        void setViewport(int x, int y, int width, int height) override;
        void setScissor(int x, int y, int width, int height) override;
        void setPipeline(PipelinePtr pipeline) override;

        // Resource binding
        void setVertexBuffer(BufferPtr buffer, uint32_t slot = 0) override;
        void setIndexBuffer(BufferPtr buffer) override;
        void setUniformBuffer(BufferPtr buffer, uint32_t slot) override;
        void setTexture(TexturePtr texture, uint32_t slot) override;
        void setSamplerState(const SamplerState& sampler, uint32_t slot) override;

        // Drawing commands
        void draw(PrimitiveType primitive, uint32_t vertexCount, uint32_t firstVertex = 0) override;
        void drawIndexed(PrimitiveType primitive, uint32_t indexCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0) override;
        void drawInstanced(PrimitiveType primitive, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) override;

        // Uniform updates
        void setUniform(const std::string& name, float value) override;
        void setUniform(const std::string& name, const Math::Vec2& value) override;
        void setUniform(const std::string& name, const Math::Vec3& value) override;
        void setUniform(const std::string& name, const Math::Vec4& value) override;
        void setUniform(const std::string& name, const Math::Mat4& value) override;

        id<MTLCommandBuffer> getMetalCommandBuffer() const { return commandBuffer_; }
        id<MTLRenderCommandEncoder> getMetalEncoder() const { return encoder_; }

        static MTLPrimitiveType toMetalPrimitiveType(PrimitiveType type);

    private:
        id<MTLCommandBuffer> commandBuffer_;
        id<MTLRenderCommandEncoder> encoder_;
        std::shared_ptr<MetalPipeline> currentPipeline_;
        id<MTLBuffer> indexBuffer_;
    };

    // ========================================
    // Metal RenderDevice Implementation
    // ========================================

    class MetalRenderDevice : public RenderDevice {
    public:
        MetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer);
        ~MetalRenderDevice() override = default;

        // Device information
        const std::string& getDeviceName() const override { return deviceName_; }
        bool isValid() const override { return device_ != nil; }

        // Resource creation
        BufferPtr createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data = nullptr) override;
        TexturePtr createTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, const void* data = nullptr) override;
        ShaderPtr createShader(ShaderStage stage, const std::string& source, const std::string& name = "") override;
        PipelinePtr createPipeline(const PipelineDescriptor& descriptor) override;

        // Frame management
        void beginFrame() override;
        void endFrame() override;
        void present() override;

        // Render pass management
        CommandBufferPtr beginRenderPass(const RenderPassDescriptor& descriptor) override;
        void endRenderPass(CommandBufferPtr commandBuffer) override;

        // Default framebuffer access
        TexturePtr getBackBuffer() override;
        TexturePtr getDepthBuffer() override;
        void resizeBackBuffer(uint32_t width, uint32_t height) override;

        // Device capabilities
        uint32_t getMaxTextureSize() const override;
        uint32_t getMaxVertexAttributes() const override;
        bool supportsComputeShaders() const override { return true; }
        bool supportsGeometryShaders() const override { return false; }

        // Debug and profiling
        void setDebugName(const std::string& name) override;
        void beginDebugGroup(const std::string& name) override;
        void endDebugGroup() override;

        // Metal-specific access
        id<MTLDevice> getMetalDevice() const { return device_; }
        CAMetalLayer* getMetalLayer() const { return layer_; }

    private:
        void createDepthBuffer();

        id<MTLDevice> device_;
        id<MTLCommandQueue> commandQueue_;
        CAMetalLayer* layer_;

        id<MTLCommandBuffer> currentCommandBuffer_;
        id<CAMetalDrawable> currentDrawable_;

        TexturePtr depthBuffer_;
        std::string deviceName_;

        uint32_t frameWidth_;
        uint32_t frameHeight_;
    };

    // ========================================
    // Metal RenderDevice Factory
    // ========================================

    RenderDevicePtr createMetalRenderDevice(id<MTLDevice> device, CAMetalLayer* layer);

} // namespace Metal
} // namespace Graphics
} // namespace Engine