// Renderer.hpp
#pragma once

#include "Engine/Graphics/RenderDevice.hpp"
#include "Engine/Graphics/GeometryBuilder.hpp"
#include "Engine/Scene/Camera.hpp"

#include <memory>
#include <vector>

class Renderer {
public:
    explicit Renderer(Engine::Graphics::RenderDevicePtr device);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    void beginFrame();
    void render(const Engine::Camera& camera);
    void endFrame();

    void addMesh(Engine::Graphics::MeshPtr mesh,
                 const Engine::Math::Mat4& transform = Engine::Math::Matrix::identity());
    void clearScene();

    void resize(uint32_t width, uint32_t height);
    bool isValid() const { return device_ && device_->isValid(); }

private:
    struct RenderItem {
        Engine::Graphics::MeshPtr mesh;
        Engine::Math::Mat4 transform;
    };

    struct Uniforms {
        Engine::Math::Mat4 modelViewProjection;
        Engine::Math::Mat4 model;
        Engine::Math::Mat4 view;
        Engine::Math::Mat4 projection;
    };

    void createPipeline();
    void createUniformBuffer();

    Engine::Graphics::RenderDevicePtr device_;
    Engine::Graphics::PipelinePtr pipeline_;
    Engine::Graphics::BufferPtr uniformBuffer_;

    std::vector<RenderItem> renderItems_;

    uint32_t frameCount_;
    uint32_t width_;
    uint32_t height_;
};

using RendererPtr = std::shared_ptr<Renderer>;
