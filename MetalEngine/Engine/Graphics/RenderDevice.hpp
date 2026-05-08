// Engine/Graphics/RenderDevice.hpp
// Backend-agnostic graphics interface. The Metal backend lives under
// Engine/Graphics/Metal/. Everything in this header is abstract — concrete
// resources are produced via RenderDevicePtr.
#pragma once

#include "../Math/Math.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Engine {
namespace Graphics {

class Buffer;
class Texture;
class Shader;
class Pipeline;
class CommandBuffer;

using BufferPtr        = std::shared_ptr<Buffer>;
using TexturePtr       = std::shared_ptr<Texture>;
using ShaderPtr        = std::shared_ptr<Shader>;
using PipelinePtr      = std::shared_ptr<Pipeline>;
using CommandBufferPtr = std::shared_ptr<CommandBuffer>;

// ---- Vertex format / layout -----------------------------------------------

enum class VertexFormat {
    Float, Float2, Float3, Float4,
    UInt,  UInt2,  UInt3,  UInt4,
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

    static VertexLayout PositionColorNormalTexCoord();
};

// ---- Render state ---------------------------------------------------------

enum class CullMode        { None, Front, Back };
enum class WindingOrder    { Clockwise, CounterClockwise };
enum class CompareFunction { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };
enum class BlendMode       { Opaque, AlphaBlend, Additive, Multiply };

struct RenderState {
    CullMode cullMode               = CullMode::Back;
    WindingOrder windingOrder       = WindingOrder::CounterClockwise;
    CompareFunction depthFunction   = CompareFunction::Less;
    bool depthWriteEnabled          = true;
    bool depthTestEnabled           = true;
    BlendMode blendMode             = BlendMode::Opaque;
};

// ---- Buffers --------------------------------------------------------------

enum class BufferUsage { Static, Dynamic, Stream };
enum class BufferType  { Vertex, Index, Uniform };

class Buffer {
public:
    virtual ~Buffer() = default;
    virtual void update(const void* data, size_t size, size_t offset = 0) = 0;
    virtual size_t getSize() const = 0;
};

// ---- Textures -------------------------------------------------------------

enum class TextureFormat {
    RGBA8, BGRA8, RGB8, R8,
    RGBA16F, RGBA32F,
    Depth32Float, Depth24Stencil8,
};
enum class TextureUsage  { ShaderRead, RenderTarget, DepthStencil };

class Texture {
public:
    virtual ~Texture() = default;
    virtual uint32_t getWidth()  const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual TextureFormat getFormat() const = 0;
};

// ---- Shaders --------------------------------------------------------------

enum class ShaderStage { Vertex, Fragment };

class Shader {
public:
    virtual ~Shader() = default;
    virtual ShaderStage getStage() const = 0;
    virtual bool isValid() const = 0;
};

// ---- Pipelines ------------------------------------------------------------

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
    virtual bool isValid() const = 0;
};

// ---- Command recording ----------------------------------------------------

enum class PrimitiveType { Points, Lines, LineStrip, Triangles, TriangleStrip };

class CommandBuffer {
public:
    virtual ~CommandBuffer() = default;

    virtual void setViewport(int x, int y, int width, int height) = 0;
    virtual void setPipeline(PipelinePtr pipeline) = 0;

    virtual void setVertexBuffer(BufferPtr buffer, uint32_t slot = 0) = 0;
    virtual void setIndexBuffer(BufferPtr buffer) = 0;
    virtual void setUniformBuffer(BufferPtr buffer, uint32_t slot) = 0;

    virtual void drawIndexed(PrimitiveType primitive,
                             uint32_t indexCount,
                             uint32_t firstIndex = 0,
                             int32_t vertexOffset = 0) = 0;
};

// ---- Render pass ---------------------------------------------------------

struct ClearValue {
    Math::Vec4 color = {0.0f, 0.0f, 0.0f, 1.0f};
    float depth = 1.0f;
};

enum class LoadAction  { DontCare, Load, Clear };
enum class StoreAction { DontCare, Store };

struct RenderPassDescriptor {
    TexturePtr colorTarget;        // nullptr = use current backbuffer / drawable
    TexturePtr depthTarget;
    LoadAction  colorLoadAction  = LoadAction::Clear;
    StoreAction colorStoreAction = StoreAction::Store;
    LoadAction  depthLoadAction  = LoadAction::Clear;
    StoreAction depthStoreAction = StoreAction::DontCare;
    ClearValue  clearValue;
};

// ---- Device --------------------------------------------------------------

class RenderDevice {
public:
    virtual ~RenderDevice() = default;

    virtual const std::string& getDeviceName() const = 0;
    virtual bool isValid() const = 0;

    virtual BufferPtr   createBuffer(BufferType type, BufferUsage usage,
                                     size_t size, const void* data = nullptr) = 0;
    virtual TexturePtr  createTexture(uint32_t width, uint32_t height,
                                      TextureFormat format, TextureUsage usage,
                                      const void* data = nullptr) = 0;
    virtual ShaderPtr   createShader(ShaderStage stage,
                                     const std::string& source,
                                     const std::string& name = "") = 0;
    virtual PipelinePtr createPipeline(const PipelineDescriptor& descriptor) = 0;

    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;

    virtual CommandBufferPtr beginRenderPass(const RenderPassDescriptor& descriptor) = 0;
    virtual void             endRenderPass(CommandBufferPtr commandBuffer) = 0;

    virtual TexturePtr getDepthBuffer() = 0;
    virtual void       resizeBackBuffer(uint32_t width, uint32_t height) = 0;
};

using RenderDevicePtr = std::shared_ptr<RenderDevice>;

} // namespace Graphics
} // namespace Engine
