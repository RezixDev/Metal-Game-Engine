// Engine/Graphics/RenderSystem.cpp
#include "RenderSystem.hpp"
#include <stdexcept>
#include <cstdio>

namespace Engine {
namespace Graphics {

void RenderSystem::initialize(RenderDevicePtr device) {
    if (device_) { printf("⚠️ RenderSystem already initialized\n"); return; }
    if (!device || !device->isValid()) throw std::runtime_error("Cannot initialize RenderSystem with invalid device");
    device_ = std::move(device);
    printf("✅ RenderSystem initialized with device: %s\n", device_->getDeviceName().c_str());
}

void RenderSystem::shutdown() {
    if (!device_) return;
    printf("🛑 Shutting down RenderSystem...\n");
    device_.reset();
    printf("✅ RenderSystem shutdown complete\n");
}

void RenderSystem::submitDrawCall(CommandBufferPtr cmd, const DrawCall& dc) {
    if (!cmd || !dc.pipeline || !dc.vertexBuffer || dc.indexCount == 0) {
        printf("❌ Invalid draw call parameters\n"); return;
    }
    cmd->setPipeline(dc.pipeline);
    cmd->setVertexBuffer(dc.vertexBuffer, 0);
    if (dc.indexBuffer) cmd->setIndexBuffer(dc.indexBuffer);
    for (size_t i = 0; i < dc.uniformBuffers.size(); ++i)
        if (dc.uniformBuffers[i]) cmd->setUniformBuffer(dc.uniformBuffers[i], static_cast<uint32_t>(i));
    for (size_t i = 0; i < dc.textures.size(); ++i)
        if (dc.textures[i]) cmd->setTexture(dc.textures[i], static_cast<uint32_t>(i));
    cmd->setUniform("u_Model", dc.modelMatrix);
    cmd->drawIndexed(PrimitiveType::Triangles, dc.indexCount, dc.firstIndex, 0);
}

PipelinePtr RenderSystem::createBasicPipeline(const std::string& vs, const std::string& fs) {
    if (!device_) throw std::runtime_error("RenderSystem not initialized");
    auto v = device_->createShader(ShaderStage::Vertex,   vs, "vs_main");
    auto f = device_->createShader(ShaderStage::Fragment, fs, "fs_main");
    PipelineDescriptor d;
    d.vertexShader = v;
    d.fragmentShader = f;
    d.vertexLayout = VertexLayout::Position3D();
    d.colorFormats = {TextureFormat::BGRA8};
    d.depthFormat = TextureFormat::Depth32Float;
    d.name = "BasicPipeline";
    return device_->createPipeline(d);
}

PipelinePtr RenderSystem::createColoredPipeline() {
    if (!device_) throw std::runtime_error("RenderSystem not initialized");

    // CHANGE: Load from default Metal library instead of embedded strings
    auto vs = device_->createShader(ShaderStage::Vertex, "", "vs_main");
    auto fs = device_->createShader(ShaderStage::Fragment, "", "fs_main"); // Basic unlit version

    PipelineDescriptor d;
    d.vertexShader = vs;
    d.fragmentShader = fs;
    d.vertexLayout = VertexLayout::PositionColor();
    d.colorFormats = {TextureFormat::BGRA8};
    d.depthFormat = TextureFormat::Depth32Float;
    d.name = "ColoredPipeline";
    return device_->createPipeline(d);
}

PipelinePtr RenderSystem::createTexturedPipeline() {
    if (!device_) throw std::runtime_error("RenderSystem not initialized");

    // CHANGE: Load from default Metal library
    auto vs = device_->createShader(ShaderStage::Vertex, "", "vs_main");
    auto fs = device_->createShader(ShaderStage::Fragment, "", "fs_textured"); // Add this to Shaders.metal

    PipelineDescriptor d;
    d.vertexShader = vs;
    d.fragmentShader = fs;
    d.vertexLayout = VertexLayout::PositionNormalTexCoord();
    d.colorFormats = {TextureFormat::BGRA8};
    d.depthFormat = TextureFormat::Depth32Float;
    d.name = "TexturedPipeline";
    return device_->createPipeline(d);
}

} // namespace Graphics
} // namespace Engine
