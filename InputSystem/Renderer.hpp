// Renderer.hpp
// UPDATED VERSION - Modern renderer using graphics abstraction
#pragma once

#include "Engine/Graphics/RenderDevice.hpp"
#include "Engine/Graphics/GeometryBuilder.hpp"


#include <memory>
#include <vector>

class Renderer {
public:
    Renderer(Engine::Graphics::RenderDevicePtr device);
    ~Renderer();

    // Main rendering interface
    void beginFrame();
    void render(const Engine::Camera& camera);
    void endFrame();

    // Scene management
    void addMesh(Engine::Graphics::MeshPtr mesh, const Engine::Math::Mat4& transform = Engine::Math::Matrix::identity());
    void clearScene();

    // Utility
    void resize(uint32_t width, uint32_t height);
    bool isValid() const { return device_ && device_->isValid(); }

private:
    struct RenderItem {
        Engine::Graphics::MeshPtr mesh;
        Engine::Math::Mat4 transform;
    };

    // Core rendering system
    Engine::Graphics::RenderDevicePtr device_;
    Engine::Graphics::PipelinePtr pipeline_;
    Engine::Graphics::BufferPtr uniformBuffer_;

    // Scene data
    std::vector<RenderItem> renderItems_;

    // Uniform data structure
    struct Uniforms {
        Engine::Math::Mat4 modelViewProjection;
        Engine::Math::Mat4 model;
        Engine::Math::Mat4 view;
        Engine::Math::Mat4 projection;
    };

    // Setup methods
    void createPipeline();
    void createUniformBuffer();

    // Debug tracking
    uint32_t frameCount_;
    uint32_t width_, height_;
};

using RendererPtr = std::shared_ptr<Renderer>;
