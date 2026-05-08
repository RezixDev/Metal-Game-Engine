// Engine/Graphics/RenderSystem.hpp
#pragma once
#include "RenderDevice.hpp"
#include "GeometryBuilder.hpp"
#include "../Math/Math.hpp"
#include <memory>
#include <vector>
#include <string>

namespace Engine {
namespace Graphics {

class RenderSystem {
public:
    static RenderSystem& getInstance() {
        static RenderSystem instance;
        return instance;
    }

    void initialize(RenderDevicePtr device);
    void shutdown();

    RenderDevicePtr getDevice() const { return device_; }
    bool isInitialized() const { return device_ != nullptr; }

    struct DrawCall {
        PipelinePtr pipeline;
        BufferPtr vertexBuffer;
        BufferPtr indexBuffer;
        std::vector<BufferPtr> uniformBuffers;
        std::vector<TexturePtr> textures;
        uint32_t indexCount = 0;
        uint32_t firstIndex = 0;
        Math::Mat4 modelMatrix = Math::Matrix::identity();
    };

    void submitDrawCall(CommandBufferPtr cmd, const DrawCall& dc);

    void renderMesh(CommandBufferPtr cmd, MeshPtr mesh, const Math::Mat4& model) {
        if (!cmd || !mesh) return;
        cmd->setUniform("u_Model", model);
        mesh->draw(cmd);
    }

    // Declarations for helpers implemented in .cpp:
    PipelinePtr createBasicPipeline(const std::string& vs, const std::string& fs);
    PipelinePtr createColoredPipeline();
    PipelinePtr createTexturedPipeline();

private:
    RenderDevicePtr device_;
};

} // namespace Graphics
} // namespace Engine
