// Renderer.mm
// UPDATED VERSION - Clean implementation using graphics abstraction
#include "Renderer.hpp"
#include <stdexcept>
#include <cstdio>

Renderer::Renderer(Engine::Graphics::RenderDevicePtr device)
    : device_(device), frameCount_(0), width_(0), height_(0) {

    if (!device_ || !device_->isValid()) {
        throw std::runtime_error("Invalid render device provided to Renderer");
    }

    printf("🎯 Creating modern Renderer with device: %s\n", device_->getDeviceName().c_str());

    createPipeline();
    createUniformBuffer();

    printf("✅ Modern Renderer initialized successfully\n");
}

Renderer::~Renderer() {
    printf("🧹 Renderer destroyed\n");
}

void Renderer::createPipeline() {
    // Create shaders using the built-in shader source
    std::string vertexShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float3 color    [[attribute(1)]];
    float3 normal   [[attribute(2)]];
    float2 texCoord [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float3 color;
    float3 normal;
    float2 texCoord;
    float3 worldPosition;
    float3 viewPosition;
};

struct Uniforms {
    float4x4 modelViewProjection;
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

vertex VertexOut vs_main(VertexIn in [[stage_in]],
                         constant Uniforms& uniforms [[buffer(1)]],
                         uint vertexID [[vertex_id]]) {
    VertexOut out;

    // Transform position to world space
    float4 worldPos = uniforms.model * float4(in.position, 1.0);
    out.worldPosition = worldPos.xyz;

    // Transform to view space
    float4 viewPos = uniforms.view * worldPos;
    out.viewPosition = viewPos.xyz;

    // Transform to clip space
    out.position = uniforms.projection * viewPos;

    // Transform normal to world space (assumes uniform scaling)
    float3 worldNormal = (uniforms.model * float4(in.normal, 0.0)).xyz;
    out.normal = normalize(worldNormal);

    // Pass through other attributes
    out.color = in.color;
    out.texCoord = in.texCoord;

    return out;
}
)";

    std::string fragmentShaderSource = R"(
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float3 color;
    float3 normal;
    float2 texCoord;
    float3 worldPosition;
    float3 viewPosition;
};

fragment float4 fs_main(VertexOut in [[stage_in]]) {
    // Simple lighting calculation
    float3 lightDirection = normalize(float3(0.5, -0.7, -0.5));
    float3 lightColor = float3(1.0, 1.0, 1.0);
    float3 ambientColor = float3(0.2, 0.2, 0.2);

    // Normalize the normal
    float3 normal = normalize(in.normal);

    // Calculate diffuse lighting
    float diffuseStrength = max(dot(normal, -lightDirection), 0.0);
    float3 diffuse = diffuseStrength * lightColor;

    // Combine lighting with vertex color
    float3 finalColor = in.color * (ambientColor + diffuse);

    return float4(finalColor, 1.0);
}
)";

    try {
        // Create shaders
        auto vertexShader = device_->createShader(
            Engine::Graphics::ShaderStage::Vertex,
            vertexShaderSource,
            "vs_main"
        );

        auto fragmentShader = device_->createShader(
            Engine::Graphics::ShaderStage::Fragment,
            fragmentShaderSource,
            "fs_main"
        );

        // Create pipeline descriptor
        Engine::Graphics::PipelineDescriptor pipelineDesc;
        pipelineDesc.vertexShader = vertexShader;
        pipelineDesc.fragmentShader = fragmentShader;
        pipelineDesc.vertexLayout = Engine::Graphics::VertexLayout::PositionColorNormalTexCoord();
        pipelineDesc.colorFormats = {Engine::Graphics::TextureFormat::BGRA8};
        pipelineDesc.depthFormat = Engine::Graphics::TextureFormat::Depth32Float;
        pipelineDesc.name = "MainPipeline";

        // Configure render state
        pipelineDesc.renderState.cullMode = Engine::Graphics::CullMode::Back;
        pipelineDesc.renderState.windingOrder = Engine::Graphics::WindingOrder::CounterClockwise;
        pipelineDesc.renderState.depthFunction = Engine::Graphics::CompareFunction::Less;
        pipelineDesc.renderState.depthWriteEnabled = true;
        pipelineDesc.renderState.depthTestEnabled = true;

        // Create pipeline
        pipeline_ = device_->createPipeline(pipelineDesc);

        if (!pipeline_ || !pipeline_->isValid()) {
            throw std::runtime_error("Failed to create render pipeline");
        }

        printf("✅ Render pipeline created successfully\n");

    } catch (const std::exception& e) {
        printf("❌ Failed to create pipeline: %s\n", e.what());
        throw;
    }
}

void Renderer::createUniformBuffer() {
    size_t uniformSize = sizeof(Uniforms);
    uniformBuffer_ = device_->createBuffer(
        Engine::Graphics::BufferType::Uniform,
        Engine::Graphics::BufferUsage::Dynamic,
        uniformSize
    );

    if (!uniformBuffer_) {
        throw std::runtime_error("Failed to create uniform buffer");
    }

    printf("✅ Uniform buffer created: %zu bytes\n", uniformSize);
}

void Renderer::beginFrame() {
    device_->beginFrame();
    frameCount_++;
}

void Renderer::render(const Engine::Camera& camera) {
    if (renderItems_.empty()) {
        return; // Nothing to render
    }

 	auto depthBuffer = device_->getDepthBuffer();

    // Calculate aspect ratio (use current window size or default)
    float aspect = (width_ > 0 && height_ > 0) ?
        static_cast<float>(width_) / static_cast<float>(height_) :
        16.0f / 9.0f; // Default aspect ratio

    // Get camera matrices
    Engine::Math::Mat4 projection = camera.getProjectionMatrix(aspect);
    Engine::Math::Mat4 view = camera.getViewMatrix();

    // Setup render pass
    Engine::Graphics::RenderPassDescriptor renderPassDesc;
    renderPassDesc.colorTarget = nullptr;
    renderPassDesc.depthTarget = depthBuffer;
    renderPassDesc.colorLoadAction = Engine::Graphics::LoadAction::Clear;
    renderPassDesc.colorStoreAction = Engine::Graphics::StoreAction::Store;
    renderPassDesc.depthLoadAction = Engine::Graphics::LoadAction::Clear;
    renderPassDesc.depthStoreAction = Engine::Graphics::StoreAction::DontCare;
    renderPassDesc.clearValue.color = {0.0f, 0.1f, 0.3f, 1.0f}; // Dark blue background
    renderPassDesc.clearValue.depth = 1.0f;

    // Begin render pass
    auto commandBuffer = device_->beginRenderPass(renderPassDesc);

    // Set viewport
    commandBuffer->setViewport(0, 0,
        width_ > 0 ? width_ : 960,
        height_ > 0 ? height_ : 600);

    // Set pipeline
    commandBuffer->setPipeline(pipeline_);

    // Render all items
    for (const auto& item : renderItems_) {
        // Calculate matrices for this item
        Engine::Math::Mat4 model = item.transform;
        Engine::Math::Mat4 viewModel = Engine::Math::Matrix::multiply(view, model);
        Engine::Math::Mat4 mvp = Engine::Math::Matrix::multiply(projection, viewModel);

        // Update uniforms
        Uniforms uniforms = {
            .modelViewProjection = mvp,
            .model = model,
            .view = view,
            .projection = projection
        };

        uniformBuffer_->update(&uniforms, sizeof(uniforms));

        // Set uniform buffer
        commandBuffer->setUniformBuffer(uniformBuffer_, 1);

        // Draw the mesh
        item.mesh->draw(commandBuffer);
    }

    // End render pass
    device_->endRenderPass(commandBuffer);

    // Debug output every 60 frames
    if (frameCount_ % 60 == 0) {
        Engine::Math::Vec3 pos = camera.getPosition();
        Engine::Math::Vec3 forward = camera.getForward();

        printf("🎯 Frame %d - Camera pos: (%.2f, %.2f, %.2f)\n",
               frameCount_, pos.x, pos.y, pos.z);
        printf("👁️ Looking direction: (%.3f, %.3f, %.3f)\n",
               forward.x, forward.y, forward.z);
        printf("📦 Rendering %zu meshes\n", renderItems_.size());
    }
}

void Renderer::endFrame() {
    device_->endFrame();
}

void Renderer::addMesh(Engine::Graphics::MeshPtr mesh, const Engine::Math::Mat4& transform) {
    if (mesh) {
        renderItems_.push_back({mesh, transform});
    }
}

void Renderer::clearScene() {
    renderItems_.clear();
}

void Renderer::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    device_->resizeBackBuffer(width, height);
    printf("📏 Renderer resized to %dx%d\n", width, height);
}