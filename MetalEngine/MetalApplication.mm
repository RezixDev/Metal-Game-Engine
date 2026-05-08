// MetalApplication.mm
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "MetalApplication.hpp"
#include "Engine/Graphics/Metal/MetalRenderDevice.hpp"
#include "Engine/Graphics/RenderSystem.hpp"

#include <cstdio>

// macOS virtual key codes (subset used by the app).
namespace {
constexpr unsigned short kVK_ANSI_A = 0x00;
constexpr unsigned short kVK_ANSI_S = 0x01;
constexpr unsigned short kVK_ANSI_D = 0x02;
constexpr unsigned short kVK_ANSI_Q = 0x0C;
constexpr unsigned short kVK_ANSI_W = 0x0D;
constexpr unsigned short kVK_ANSI_R = 0x0F;
constexpr unsigned short kVK_Space  = 0x31;

constexpr Engine::Math::Vec3 kInitialCameraPosition = {0.0f, 2.0f, 5.0f};
constexpr Engine::Math::Vec3 kInitialCameraTarget   = {0.0f, 0.0f, 0.0f};
}

MetalApplication& MetalApplication::shared() {
    static MetalApplication instance;
    return instance;
}

MetalApplication* getMetalApplication() {
    return &MetalApplication::shared();
}

MetalApplication::MetalApplication() {
    camera_ = std::make_unique<Engine::Camera>(
        kInitialCameraPosition,
        kInitialCameraTarget,
        Engine::Math::Vector::Constants::UP);
}

MetalApplication::~MetalApplication() {
    onShutdown();
}

void MetalApplication::onInitialize(Engine::Graphics::RenderDevicePtr device) {
    if (isInitialized_) return;

    renderDevice_ = std::move(device);
    Engine::Graphics::RenderSystem::getInstance().initialize(renderDevice_);
    renderer_ = std::make_unique<Renderer>(renderDevice_);

    camera_->setFieldOfView(60.0f);
    camera_->setNearPlane(0.1f);
    camera_->setFarPlane(100.0f);
    camera_->setMovementSpeed(moveSpeed_);
    camera_->setMouseSensitivity(mouseSensitivity_);
    camera_->setMovementType(Engine::CameraMovementType::FPS);

    setupScene();
    isInitialized_ = true;
    printf("✅ MetalApplication initialized\n");
}

extern "C" void AppInitWithMetal(id<MTLDevice> device, CAMetalLayer* layer) {
    auto rd = Engine::Graphics::Metal::createMetalRenderDevice(device, layer);
    MetalApplication::shared().onInitialize(std::move(rd));
}

void MetalApplication::setupScene() {
    using namespace Engine::Graphics;

    sceneObjects_.push_back({GeometryBuilder::createCubeMesh(renderDevice_, 1.0f),
                             Engine::Math::Matrix::identity()});
    sceneObjects_.push_back({GeometryBuilder::createCubeMesh(renderDevice_, 0.8f),
                             Engine::Math::Matrix::translation(5.0f, 0.0f, 0.0f)});
    sceneObjects_.push_back({GeometryBuilder::createCubeMesh(renderDevice_, 0.8f),
                             Engine::Math::Matrix::translation(-3.0f, 0.0f, 0.0f)});
    sceneObjects_.push_back({GeometryBuilder::createCubeMesh(renderDevice_, 0.8f),
                             Engine::Math::Matrix::translation(0.0f, 3.0f, 0.0f)});
    sceneObjects_.push_back({GeometryBuilder::createCubeMesh(renderDevice_, 0.8f),
                             Engine::Math::Matrix::translation(0.0f, -3.0f, 0.0f)});
    sceneObjects_.push_back({GeometryBuilder::createSphereMesh(renderDevice_, 0.7f, 16, 12),
                             Engine::Math::Matrix::translation(0.0f, 0.0f, -5.0f)});
    sceneObjects_.push_back({GeometryBuilder::createPlaneMesh(renderDevice_, 20.0f, 20.0f, 1),
                             Engine::Math::Matrix::translation(0.0f, -5.0f, 0.0f)});
}

void MetalApplication::onUpdate(float deltaTime) {
    if (!isInitialized_) return;

    camera_->processMovement(
        deltaTime,
        inputState_.moveForward, inputState_.moveBackward,
        inputState_.moveLeft,    inputState_.moveRight,
        inputState_.moveUp,      inputState_.moveDown);
}

void MetalApplication::onRender() {
    if (!isInitialized_ || !renderer_) return;

    try {
        renderer_->beginFrame();
        renderer_->clearScene();
        for (const auto& obj : sceneObjects_) {
            renderer_->addMesh(obj.mesh, obj.transform);
        }
        renderer_->render(*camera_);
        renderer_->endFrame();
    } catch (const std::exception& e) {
        printf("❌ Render error: %s\n", e.what());
    }
}

void MetalApplication::onResize(int width, int height) {
    if (!isInitialized_ || !renderer_) return;
    renderer_->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

void MetalApplication::onShutdown() {
    if (!isInitialized_) return;
    sceneObjects_.clear();
    renderer_.reset();
    Engine::Graphics::RenderSystem::getInstance().shutdown();
    renderDevice_.reset();
    isInitialized_ = false;
}

void MetalApplication::processKeyDown(unsigned short keyCode) {
    switch (keyCode) {
        case kVK_ANSI_W: inputState_.moveForward  = true; break;
        case kVK_ANSI_S: inputState_.moveBackward = true; break;
        case kVK_ANSI_A: inputState_.moveLeft     = true; break;
        case kVK_ANSI_D: inputState_.moveRight    = true; break;
        case kVK_Space:  inputState_.moveUp       = true; break;
        case kVK_ANSI_Q: inputState_.moveDown     = true; break;
        case kVK_ANSI_R:
            camera_->lookAt(kInitialCameraPosition,
                            kInitialCameraTarget,
                            Engine::Math::Vector::Constants::UP);
            break;
    }
}

void MetalApplication::processKeyUp(unsigned short keyCode) {
    switch (keyCode) {
        case kVK_ANSI_W: inputState_.moveForward  = false; break;
        case kVK_ANSI_S: inputState_.moveBackward = false; break;
        case kVK_ANSI_A: inputState_.moveLeft     = false; break;
        case kVK_ANSI_D: inputState_.moveRight    = false; break;
        case kVK_Space:  inputState_.moveUp       = false; break;
        case kVK_ANSI_Q: inputState_.moveDown     = false; break;
    }
}

void MetalApplication::processMouseMove(float deltaX, float deltaY) {
    if (!inputState_.mouseLookEnabled || !isInitialized_) return;
    camera_->processMouseMovement(deltaX, deltaY);
}

void MetalApplication::setMouseLook(bool enabled) {
    inputState_.mouseLookEnabled = enabled;
}
