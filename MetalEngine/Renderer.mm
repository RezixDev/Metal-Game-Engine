// Renderer.mm
#include "Renderer.hpp"
#include <stdexcept>

Renderer::Renderer(Engine::Graphics::RenderDevicePtr device)
    : device_(device), frameCount_(0), width_(0), height_(0) {

    if (!device_ || !device_->isValid()) {
        throw std::runtime_error("Invalid render device provided to Renderer");
    }

    createPipeline();
    createUniformBuffer();
}

Renderer::~Renderer() = default;

void Renderer::createPipeline() {
    auto vertexShader = device_->createShader(
        Engine::Graphics::ShaderStage::Vertex, "", "vs_main");
    auto fragmentShader = device_->createShader(
        Engine::Graphics::ShaderStage::Fragment, "", "fs_main_lit");

    Engine::Graphics::PipelineDescriptor desc;
    desc.vertexShader   = vertexShader;
    desc.fragmentShader = fragmentShader;
    desc.vertexLayout   = Engine::Graphics::VertexLayout::PositionColorNormalTexCoord();
    desc.colorFormats   = {Engine::Graphics::TextureFormat::BGRA8};
    desc.depthFormat    = Engine::Graphics::TextureFormat::Depth32Float;
    desc.name           = "MainPipeline";

    desc.renderState.cullMode          = Engine::Graphics::CullMode::Back;
    desc.renderState.windingOrder      = Engine::Graphics::WindingOrder::CounterClockwise;
    desc.renderState.depthFunction     = Engine::Graphics::CompareFunction::Less;
    desc.renderState.depthWriteEnabled = true;
    desc.renderState.depthTestEnabled  = true;

    pipeline_ = device_->createPipeline(desc);
    if (!pipeline_ || !pipeline_->isValid()) {
        throw std::runtime_error("Failed to create render pipeline");
    }
}

void Renderer::createUniformBuffer() {
    uniformBuffer_ = device_->createBuffer(
        Engine::Graphics::BufferType::Uniform,
        Engine::Graphics::BufferUsage::Dynamic,
        sizeof(Uniforms));
    if (!uniformBuffer_) {
        throw std::runtime_error("Failed to create uniform buffer");
    }
}

void Renderer::beginFrame() {
    device_->beginFrame();
    ++frameCount_;
}

void Renderer::render(const Engine::Camera& camera) {
    if (renderItems_.empty()) return;

    const float aspect = (width_ > 0 && height_ > 0)
        ? static_cast<float>(width_) / static_cast<float>(height_)
        : 16.0f / 9.0f;

    const Engine::Math::Mat4 projection = camera.getProjectionMatrix(aspect);
    const Engine::Math::Mat4 view       = camera.getViewMatrix();

    Engine::Graphics::RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorTarget       = nullptr; // backbuffer / current drawable
    renderPassDesc.depthTarget       = device_->getDepthBuffer();
    renderPassDesc.colorLoadAction   = Engine::Graphics::LoadAction::Clear;
    renderPassDesc.colorStoreAction  = Engine::Graphics::StoreAction::Store;
    renderPassDesc.depthLoadAction   = Engine::Graphics::LoadAction::Clear;
    renderPassDesc.depthStoreAction  = Engine::Graphics::StoreAction::DontCare;
    renderPassDesc.clearValue.color  = {0.05f, 0.08f, 0.12f, 1.0f};
    renderPassDesc.clearValue.depth  = 1.0f;

    auto commandBuffer = device_->beginRenderPass(renderPassDesc);

    if (width_ > 0 && height_ > 0) {
        commandBuffer->setViewport(0, 0,
                                   static_cast<int>(width_),
                                   static_cast<int>(height_));
    }
    commandBuffer->setPipeline(pipeline_);

    for (const auto& item : renderItems_) {
        const Engine::Math::Mat4 model     = item.transform;
        const Engine::Math::Mat4 modelView = Engine::Math::Matrix::multiply(view, model);
        const Engine::Math::Mat4 mvp       = Engine::Math::Matrix::multiply(projection, modelView);

        Uniforms uniforms{
            .modelViewProjection = mvp,
            .model               = model,
            .view                = view,
            .projection          = projection,
        };
        uniformBuffer_->update(&uniforms, sizeof(uniforms));

        commandBuffer->setUniformBuffer(uniformBuffer_, 1);
        item.mesh->draw(commandBuffer);
    }

    device_->endRenderPass(commandBuffer);
}

void Renderer::endFrame() {
    device_->endFrame();
}

void Renderer::addMesh(Engine::Graphics::MeshPtr mesh,
                       const Engine::Math::Mat4& transform) {
    if (mesh) {
        renderItems_.push_back({mesh, transform});
    }
}

void Renderer::clearScene() {
    renderItems_.clear();
}

void Renderer::resize(uint32_t width, uint32_t height) {
    width_  = width;
    height_ = height;
    device_->resizeBackBuffer(width, height);
}
