// Engine/Graphics/RenderSystem.hpp
// Abstract rendering interface
#pragma once

#include "../Core/Math.hpp"
#include <memory>
#include <vector>
#include <string>

namespace Engine {
namespace Graphics {

    // Forward declarations
    class RenderDevice;
    class RenderPipeline;
    class VertexBuffer;
    class IndexBuffer;
    class UniformBuffer;
    class Texture;
    class Shader;
    class Material;
    class Mesh;

    // Type aliases
    using RenderDevicePtr = std::shared_ptr<RenderDevice>;
    using RenderPipelinePtr = std::shared_ptr<RenderPipeline>;
    using VertexBufferPtr = std::shared_ptr<VertexBuffer>;
    using IndexBufferPtr = std::shared_ptr<IndexBuffer>;
    using UniformBufferPtr = std::shared_ptr<UniformBuffer>;
    using TexturePtr = std::shared_ptr<Texture>;
    using ShaderPtr = std::shared_ptr<Shader>;
    using MaterialPtr = std::shared_ptr<Material>;
    using MeshPtr = std::shared_ptr<Mesh>;

    // ========================================
    // Vertex Formats
    // ========================================

    struct VertexAttribute {
        enum class Format {
            Float,
            Float2,
            Float3,
            Float4,
            UInt,
            UInt2,
            UInt3,
            UInt4
        };

        std::string name;
        Format format;
        uint32_t offset;
        uint32_t bufferIndex = 0;
    };

    struct VertexLayout {
        std::vector<VertexAttribute> attributes;
        uint32_t stride;

        static VertexLayout Position3D() {
            return {
                {{"position", VertexAttribute::Format::Float3, 0}},
                sizeof(Math::Vec3)
            };
        }

        static VertexLayout PositionNormalTexCoord() {
            return {
                {
                    {"position", VertexAttribute::Format::Float3, 0},
                    {"normal", VertexAttribute::Format::Float3, sizeof(Math::Vec3)},
                    {"texCoord", VertexAttribute::Format::Float2, sizeof(Math::Vec3) * 2}
                },
                sizeof(Math::Vec3) * 2 + sizeof(Math::Vec2)
            };
        }

        static VertexLayout PositionColorNormalTexCoord() {
            return {
                {
                    {"position", VertexAttribute::Format::Float3, 0},
                    {"color", VertexAttribute::Format::Float3, sizeof(Math::Vec3)},
                    {"normal", VertexAttribute::Format::Float3, sizeof(Math::Vec3) * 2},
                    {"texCoord", VertexAttribute::Format::Float2, sizeof(Math::Vec3) * 3}
                },
                sizeof(Math::Vec3) * 3 + sizeof(Math::Vec2)
            };
        }
    };

    // ========================================
    // Render State
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

    enum class DepthFunction {
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
        DepthFunction depthFunction = DepthFunction::Less;
        bool depthWriteEnabled = true;
        bool depthTestEnabled = true;
        BlendMode blendMode = BlendMode::Opaque;
    };

    // ========================================
    // Buffer Interfaces
    // ========================================

    class Buffer {
    public:
        virtual ~Buffer() = default;
        virtual void* map() = 0;
        virtual void unmap() = 0;
        virtual size_t getSize() const = 0;
    };

    class VertexBuffer : public Buffer {
    public:
        virtual const VertexLayout& getLayout() const = 0;
        virtual uint32_t getVertexCount() const = 0;
    };

    class IndexBuffer : public Buffer {
    public:
        enum class IndexType {
            UInt16,
            UInt32
        };

        virtual IndexType getIndexType() const = 0;
        virtual uint32_t getIndexCount() const = 0;
    };

    class UniformBuffer : public Buffer {
    public:
        virtual void update(const void* data, size_t size, size_t offset = 0) = 0;
    };

    // ========================================
    // Texture Interface
    // ========================================

    class Texture {
    public:
        enum class Format {
            RGBA8,
            BGRA8,
            RGB8,
            R8,
            Depth32Float,
            Depth24Stencil8
        };

        enum class Usage {
            ShaderRead,
            RenderTarget,
            DepthStencil
        };

        virtual ~Texture() = default;
        virtual uint32_t getWidth() const = 0;
        virtual uint32_t getHeight() const = 0;
        virtual Format getFormat() const = 0;
        virtual Usage getUsage() const = 0;
    };

    // ========================================
    // Shader Interface
    // ========================================

    class Shader {
    public:
        enum class Type {
            Vertex,
            Fragment,
            Compute
        };

        virtual ~Shader() = default;
        virtual Type getType() const = 0;
        virtual const std::string& getName() const = 0;
    };

    // ========================================
    // Material System
    // ========================================

    class Material {
    public:
        virtual ~Material() = default;

        virtual void setFloat(const std::string& name, float value) = 0;
        virtual void setVector2(const std::string& name, const Math::Vec2& value) = 0;
        virtual void setVector3(const std::string& name, const Math::Vec3& value) = 0;
        virtual void setVector4(const std::string& name, const Math::Vec4& value) = 0;
        virtual void setMatrix(const std::string& name, const Math::Mat4& value) = 0;
        virtual void setTexture(const std::string& name, TexturePtr texture) = 0;

        virtual ShaderPtr getVertexShader() const = 0;
        virtual ShaderPtr getFragmentShader() const = 0;
        virtual const RenderState& getRenderState() const = 0;
        virtual void setRenderState(const RenderState& state) = 0;
    };

    // ========================================
    // Mesh Interface
    // ========================================

    class Mesh {
    public:
        struct SubMesh {
            uint32_t indexStart;
            uint32_t indexCount;
            MaterialPtr material;
        };

        virtual ~Mesh() = default;

        virtual VertexBufferPtr getVertexBuffer() const = 0;
        virtual IndexBufferPtr getIndexBuffer() const = 0;
        virtual const std::vector<SubMesh>& getSubMeshes() const = 0;

        virtual void addSubMesh(uint32_t indexStart, uint32_t indexCount, MaterialPtr material) = 0;
    };

    // ========================================
    // Render Command Interface
    // ========================================

    class RenderCommand {
    public:
        virtual ~RenderCommand() = default;

        virtual void setViewport(int x, int y, int width, int height) = 0;
        virtual void setScissor(int x, int y, int width, int height) = 0;

        virtual void setPipeline(RenderPipelinePtr pipeline) = 0;
        virtual void setVertexBuffer(VertexBufferPtr buffer, uint32_t slot = 0) = 0;
        virtual void setIndexBuffer(IndexBufferPtr buffer) = 0;
        virtual void setUniformBuffer(UniformBufferPtr buffer, uint32_t slot) = 0;
        virtual void setTexture(TexturePtr texture, uint32_t slot) = 0;

        virtual void draw(uint32_t vertexCount, uint32_t firstVertex = 0) = 0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex = 0, int32_t vertexOffset = 0) = 0;
        virtual void drawInstanced(uint32_t vertexCount, uint32_t instanceCount,
                                  uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount,
                                         uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                                         uint32_t firstInstance = 0) = 0;
    };

    using RenderCommandPtr = std::shared_ptr<RenderCommand>;

    // ========================================
    // Render Pipeline Interface
    // ========================================

    class RenderPipeline {
    public:
        struct Descriptor {
            ShaderPtr vertexShader;
            ShaderPtr fragmentShader;
            VertexLayout vertexLayout;
            RenderState renderState;
            std::vector<Texture::Format> colorFormats;
            Texture::Format depthFormat = Texture::Format::Depth32Float;
        };

        virtual ~RenderPipeline() = default;
        virtual const Descriptor& getDescriptor() const = 0;
    };

    // ========================================
    // Main Render Device Interface
    // ========================================

    class RenderDevice {
    public:
        virtual ~RenderDevice() = default;

        // Resource creation
        virtual VertexBufferPtr createVertexBuffer(const void* data, size_t size,
                                                   const VertexLayout& layout) = 0;
        virtual IndexBufferPtr createIndexBuffer(const void* data, size_t size,
                                                IndexBuffer::IndexType type) = 0;
        virtual UniformBufferPtr createUniformBuffer(size_t size) = 0;
        virtual TexturePtr createTexture(uint32_t width, uint32_t height,
                                        Texture::Format format, Texture::Usage usage,
                                        const void* data = nullptr) = 0;
        virtual ShaderPtr createShader(Shader::Type type, const std::string& source,
                                      const std::string& name) = 0;
        virtual RenderPipelinePtr createRenderPipeline(const RenderPipeline::Descriptor& desc) = 0;
        virtual MaterialPtr createMaterial(ShaderPtr vertexShader, ShaderPtr fragmentShader) = 0;

        // Frame management
        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
        virtual void present() = 0;

        // Render pass management
        virtual RenderCommandPtr beginRenderPass(TexturePtr colorTarget, TexturePtr depthTarget,
                                                const Math::Vec4& clearColor = {0, 0, 0, 1},
                                                float clearDepth = 1.0f) = 0;
        virtual void endRenderPass(RenderCommandPtr command) = 0;

        // Default framebuffer
        virtual TexturePtr getBackBuffer() = 0;
        virtual TexturePtr getDepthBuffer() = 0;

        // Device capabilities
        virtual uint32_t getMaxTextureSize() const = 0;
        virtual uint32_t getMaxVertexAttributes() const = 0;
        virtual bool supportsComputeShaders() const = 0;
        virtual bool supportsGeometryShaders() const = 0;
    };

    // ========================================
    // Render System Manager
    // ========================================

    class RenderSystem {
    public:
        static RenderSystem& getInstance() {
            static RenderSystem instance;
            return instance;
        }

        void initialize(RenderDevicePtr device) {
            device_ = device;
        }

        RenderDevicePtr getDevice() const { return device_; }

        // High-level rendering operations
        void renderMesh(RenderCommandPtr cmd, MeshPtr mesh, const Math::Mat4& transform) {
            // Set transform uniform
            // This would typically update a uniform buffer with the transform

            auto vertexBuffer = mesh->getVertexBuffer();
            auto indexBuffer = mesh->getIndexBuffer();

            cmd->setVertexBuffer(vertexBuffer);
            cmd->setIndexBuffer(indexBuffer);

            for (const auto& subMesh : mesh->getSubMeshes()) {
                // Apply material settings here
                // cmd->setPipeline(subMesh.material->getPipeline());

                cmd->drawIndexed(subMesh.indexCount, subMesh.indexStart);
            }
        }

    private:
        RenderDevicePtr device_;
    };

} // namespace Graphics
} // namespace Engine