// Engine/Graphics/RenderDevice.hpp
#pragma once

#include "../Core/Math.hpp"
#include "../Core/Base/Types.hpp"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace Engine {
namespace Graphics {

    // Forward declarations
    class Buffer;
    class Texture;
    class Shader;
    class Pipeline;
    class CommandBuffer;

    // Smart pointer aliases
    using BufferPtr = std::shared_ptr<Buffer>;
    using TexturePtr = std::shared_ptr<Texture>;
    using ShaderPtr = std::shared_ptr<Shader>;
    using PipelinePtr = std::shared_ptr<Pipeline>;
    using CommandBufferPtr = std::shared_ptr<CommandBuffer>;

    // ========================================
    // Vertex Format Definitions
    // ========================================

    enum class VertexFormat {
        Float,
        Float2,
        Float3,
        Float4,
        UInt,
        UInt2,
        UInt3,
        UInt4
    };

    struct VertexAttribute {
        std::string name;
        VertexFormat format;
        uint32_t offset;
        uint32_t bufferIndex = 0;
    };

    struct VertexLayout {
        std::vector<VertexAttribute> attributes;
        uint32_t stride;

        // Common vertex layouts
        static VertexLayout Position3D();
        static VertexLayout PositionColor();
        static VertexLayout PositionNormalTexCoord();
        static VertexLayout PositionColorNormalTexCoord();
    };

    // ========================================
    // Render State Definitions
    // ========================================

    enum class CullMode {
        None,
        Front,
        Back
    };

    enum class WindingOrder {
        Clockwise,
        CounterClockwise
    };

    enum class CompareFunction {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    enum class BlendMode {
        Opaque,
        AlphaBlend,
        Additive,
        Multiply
    };

    struct RenderState {
        CullMode cullMode = CullMode::Back;
        WindingOrder windingOrder = WindingOrder::CounterClockwise;
        CompareFunction depthFunction = CompareFunction::Less;
        bool depthWriteEnabled = true;
        bool depthTestEnabled = true;
        BlendMode blendMode = BlendMode::Opaque;
    };

    // ========================================
    // Buffer Interface
    // ========================================

    enum class BufferUsage {
        Static,    // Written once, read many times
        Dynamic,   // Updated frequently
        Stream     // Updated every frame
    };

    enum class BufferType {
        Vertex,
        Index,
        Uniform
    };

    class Buffer {
    public:
        virtual ~Buffer() = default;

        virtual void* map() = 0;
        virtual void unmap() = 0;
        virtual void update(const void* data, size_t size, size_t offset = 0) = 0;

        virtual size_t getSize() const = 0;
        virtual BufferType getType() const = 0;
        virtual BufferUsage getUsage() const = 0;
    };

    // ========================================
    // Texture Interface
    // ========================================

    enum class TextureFormat {
        RGBA8,
        BGRA8,
        RGB8,
        R8,
        RGBA16F,
        RGBA32F,
        Depth32Float,
        Depth24Stencil8
    };

    enum class TextureUsage {
        ShaderRead,
        RenderTarget,
        DepthStencil
    };

    enum class TextureFilter {
        Nearest,
        Linear
    };

    enum class TextureWrap {
        Repeat,
        ClampToEdge,
        MirrorRepeat
    };

    struct SamplerState {
        TextureFilter minFilter = TextureFilter::Linear;
        TextureFilter magFilter = TextureFilter::Linear;
        TextureWrap wrapU = TextureWrap::Repeat;
        TextureWrap wrapV = TextureWrap::Repeat;
        TextureWrap wrapW = TextureWrap::Repeat;
    };

    class Texture {
    public:
        virtual ~Texture() = default;

        virtual uint32_t getWidth() const = 0;
        virtual uint32_t getHeight() const = 0;
        virtual uint32_t getDepth() const = 0;
        virtual TextureFormat getFormat() const = 0;
        virtual TextureUsage getUsage() const = 0;

        virtual void update(const void* data, size_t size) = 0;
    };

    // ========================================
    // Shader Interface
    // ========================================

    enum class ShaderStage {
        Vertex,
        Fragment,
        Compute,
        Geometry
    };

    class Shader {
    public:
        virtual ~Shader() = default;

        virtual ShaderStage getStage() const = 0;
        virtual const std::string& getName() const = 0;
        virtual bool isValid() const = 0;
    };

    // ========================================
    // Pipeline Interface
    // ========================================

    struct PipelineDescriptor {
        ShaderPtr vertexShader;
        ShaderPtr fragmentShader;
        VertexLayout vertexLayout;
        RenderState renderState;
        std::vector<TextureFormat> colorFormats;
        TextureFormat depthFormat = TextureFormat::Depth32Float;
        std::string name = "Pipeline";
    };

    class Pipeline {
    public:
        virtual ~Pipeline() = default;

        virtual const PipelineDescriptor& getDescriptor() const = 0;
        virtual bool isValid() const = 0;
    };

    // ========================================
    // Command Buffer Interface
    // ========================================

    enum class PrimitiveType {
        Points,
        Lines,
        LineStrip,
        Triangles,
        TriangleStrip
    };

    class CommandBuffer {
    public:
        virtual ~CommandBuffer() = default;

        // Render state
        virtual void setViewport(int x, int y, int width, int height) = 0;
        virtual void setScissor(int x, int y, int width, int height) = 0;
        virtual void setPipeline(PipelinePtr pipeline) = 0;

        // Resource binding
        virtual void setVertexBuffer(BufferPtr buffer, uint32_t slot = 0) = 0;
        virtual void setIndexBuffer(BufferPtr buffer) = 0;
        virtual void setUniformBuffer(BufferPtr buffer, uint32_t slot) = 0;
        virtual void setTexture(TexturePtr texture, uint32_t slot) = 0;
        virtual void setSamplerState(const SamplerState& sampler, uint32_t slot) = 0;

        // Drawing commands
        virtual void draw(PrimitiveType primitive, uint32_t vertexCount, uint32_t firstVertex = 0) = 0;
        virtual void drawIndexed(PrimitiveType primitive, uint32_t indexCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0) = 0;
        virtual void drawInstanced(PrimitiveType primitive, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;

        // Uniform updates (for simple uniforms)
        virtual void setUniform(const std::string& name, float value) = 0;
        virtual void setUniform(const std::string& name, const Math::Vec2& value) = 0;
        virtual void setUniform(const std::string& name, const Math::Vec3& value) = 0;
        virtual void setUniform(const std::string& name, const Math::Vec4& value) = 0;
        virtual void setUniform(const std::string& name, const Math::Mat4& value) = 0;
    };

    // ========================================
    // Render Pass Interface
    // ========================================

    struct ClearValue {
        Math::Vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
        float depth = 1.0f;
        uint32_t stencil = 0;
    };

    enum class LoadAction {
        DontCare,
        Load,
        Clear
    };

    enum class StoreAction {
        DontCare,
        Store
    };

    struct RenderPassDescriptor {
        TexturePtr colorTarget;
        TexturePtr depthTarget;
        LoadAction colorLoadAction = LoadAction::Clear;
        StoreAction colorStoreAction = StoreAction::Store;
        LoadAction depthLoadAction = LoadAction::Clear;
        StoreAction depthStoreAction = StoreAction::DontCare;
        ClearValue clearValue;
    };

    // ========================================
    // Main RenderDevice Interface
    // ========================================

    class RenderDevice {
    public:
        virtual ~RenderDevice() = default;

        // Device information
        virtual const std::string& getDeviceName() const = 0;
        virtual bool isValid() const = 0;

        // Resource creation
        virtual BufferPtr createBuffer(BufferType type, BufferUsage usage, size_t size, const void* data = nullptr) = 0;
        virtual TexturePtr createTexture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, const void* data = nullptr) = 0;
        virtual ShaderPtr createShader(ShaderStage stage, const std::string& source, const std::string& name = "") = 0;
        virtual PipelinePtr createPipeline(const PipelineDescriptor& descriptor) = 0;

        // Frame management
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void present() = 0;

        // Render pass management
        virtual CommandBufferPtr beginRenderPass(const RenderPassDescriptor& descriptor) = 0;
        virtual void endRenderPass(CommandBufferPtr commandBuffer) = 0;

        // Default framebuffer access
        virtual TexturePtr getBackBuffer() = 0;
        virtual TexturePtr getDepthBuffer() = 0;
        virtual void resizeBackBuffer(uint32_t width, uint32_t height) = 0;

        // Device capabilities
        virtual uint32_t getMaxTextureSize() const = 0;
        virtual uint32_t getMaxVertexAttributes() const = 0;
        virtual bool supportsComputeShaders() const = 0;
        virtual bool supportsGeometryShaders() const = 0;

        // Debug and profiling
        virtual void setDebugName(const std::string& name) = 0;
        virtual void beginDebugGroup(const std::string& name) = 0;
        virtual void endDebugGroup() = 0;
    };

    using RenderDevicePtr = std::shared_ptr<RenderDevice>;

} // namespace Graphics
} // namespace Engine
