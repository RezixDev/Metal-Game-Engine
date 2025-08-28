// MetalCppShim.mm
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "MetalApplication.hpp"
#include "Engine/Graphics/Metal/MetalRenderDevice.hpp"
#include "Engine/Graphics/RenderSystem.hpp"

#include "Engine/Core/Time.hpp"
#include <cmath>
#include <cstdio>

// macOS virtual key codes
enum {
    kVK_ANSI_W = 0x0D,
    kVK_ANSI_A = 0x00,
    kVK_ANSI_S = 0x01,
    kVK_ANSI_D = 0x02,
    kVK_ANSI_Q = 0x0C,
    kVK_ANSI_R = 0x0F,
    kVK_Space = 0x31
};

// Singleton instance
static std::unique_ptr<MetalApplication> g_metalApplication = nullptr;

MetalApplication* getMetalApplication() {
    if (!g_metalApplication) {
        g_metalApplication = std::make_unique<MetalApplication>();
    }
    return g_metalApplication.get();
}

MetalApplication::MetalApplication() {
    printf("🚀 Creating MetalApplication with modern architecture\n");

    // Create camera with initial position and target
    camera_ = std::make_unique<Engine::Camera>(
        Engine::Math::Vector::make(0.0f, 2.0f, 5.0f),  // position
        Engine::Math::Vector::make(0.0f, 0.0f, 0.0f),  // target
        Engine::Math::Vector::Constants::UP              // up vector
    );

    printf("✅ MetalApplication created\n");
}

MetalApplication::~MetalApplication() {
    onShutdown();
    printf("🧹 MetalApplication destroyed\n");
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
}

// bridge you call from Cocoa:
extern "C" void AppInitWithMetal(id<MTLDevice> device, CAMetalLayer* layer) {
    auto* app = getMetalApplication();
    auto rd = Engine::Graphics::Metal::createMetalRenderDevice(device, layer);
    app->onInitialize(std::move(rd));
}

void MetalApplication::setupScene() {
    printf("🎬 Setting up scene...\n");

    // Create various test meshes to demonstrate the system

    // Central cube at origin
    auto cubeMesh = Engine::Graphics::GeometryBuilder::createCubeMesh(renderDevice_, 1.0f);
    sceneMeshes_.push_back(cubeMesh);

    // Create additional cubes at different positions
    auto rightCube = Engine::Graphics::GeometryBuilder::createCubeMesh(renderDevice_, 0.8f);
    sceneMeshes_.push_back(rightCube);

    auto leftCube = Engine::Graphics::GeometryBuilder::createCubeMesh(renderDevice_, 0.8f);
    sceneMeshes_.push_back(leftCube);

    auto topCube = Engine::Graphics::GeometryBuilder::createCubeMesh(renderDevice_, 0.8f);
    sceneMeshes_.push_back(topCube);

    auto bottomCube = Engine::Graphics::GeometryBuilder::createCubeMesh(renderDevice_, 0.8f);
    sceneMeshes_.push_back(bottomCube);

    // Create a sphere for variety
    auto sphereMesh = Engine::Graphics::GeometryBuilder::createSphereMesh(renderDevice_, 0.7f, 16, 12);
    sceneMeshes_.push_back(sphereMesh);

    // Create a ground plane
    auto planeMesh = Engine::Graphics::GeometryBuilder::createPlaneMesh(renderDevice_, 20.0f, 20.0f, 1);
    sceneMeshes_.push_back(planeMesh);

    printf("✅ Scene setup complete: %zu meshes created\n", sceneMeshes_.size());
}

void MetalApplication::onUpdate(float deltaTime) {
    if (!isInitialized_) {
        return;
    }

    // Process camera movemenwt based on input state
    camera_->processMovement(
        deltaTime,
        inputState_.moveForward,
        inputState_.moveBackward,
        inputState_.moveLeft,
        inputState_.moveRight,
        inputState_.moveUp,
        inputState_.moveDown
    );
}

void MetalApplication::onRender() {
    if (!isInitialized_ || !renderer_) {
        return;
    }

    try {
        // Begin frame
        renderer_->beginFrame();

        // Clear previous frame's render items
        renderer_->clearScene();

        // Add all scene objects with their transforms
        Engine::Math::Mat4 identity = Engine::Math::Matrix::identity();

        // Central cube (red from GeometryBuilder colors)
        renderer_->addMesh(sceneMeshes_[0], identity);

        // Right cube
        Engine::Math::Mat4 rightTransform = Engine::Math::Matrix::translation(3.0f, 0.0f, 0.0f);
        renderer_->addMesh(sceneMeshes_[1], rightTransform);

        // Left cube
        Engine::Math::Mat4 leftTransform = Engine::Math::Matrix::translation(-3.0f, 0.0f, 0.0f);
        renderer_->addMesh(sceneMeshes_[2], leftTransform);

        // Top cube
        Engine::Math::Mat4 topTransform = Engine::Math::Matrix::translation(0.0f, 3.0f, 0.0f);
        renderer_->addMesh(sceneMeshes_[3], topTransform);

        // Bottom cube
        Engine::Math::Mat4 bottomTransform = Engine::Math::Matrix::translation(0.0f, -3.0f, 0.0f);
        renderer_->addMesh(sceneMeshes_[4], bottomTransform);

        // Sphere at forward position
        Engine::Math::Mat4 sphereTransform = Engine::Math::Matrix::translation(0.0f, 0.0f, -5.0f);
        renderer_->addMesh(sceneMeshes_[5], sphereTransform);

        // Ground plane
        Engine::Math::Mat4 planeTransform = Engine::Math::Matrix::translation(0.0f, -5.0f, 0.0f);
        renderer_->addMesh(sceneMeshes_[6], planeTransform);

        // Render the scene
        renderer_->render(*camera_);

        // End frame
        renderer_->endFrame();

    } catch (const std::exception& e) {
        printf("❌ Render error: %s\n", e.what());
    }
}

void MetalApplication::onResize(int width, int height) {
    if (!isInitialized_ || !renderer_) {
        return;
    }

    printf("📏 Application resize: %dx%d\n", width, height);
    renderer_->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

void MetalApplication::onShutdown() {
    if (!isInitialized_) {
        return;
    }

    printf("🛑 Shutting down MetalApplication...\n");

    // Clear scene meshes
    sceneMeshes_.clear();

    // Clean up renderer
    renderer_.reset();

    // Shutdown render system
    Engine::Graphics::RenderSystem::getInstance().shutdown();

    // Clean up render device
    renderDevice_.reset();

    isInitialized_ = false;
    printf("✅ MetalApplication shutdown complete\n");
}

void MetalApplication::processKeyDown(unsigned short keyCode) {
    switch (keyCode) {
        case kVK_ANSI_W:
            inputState_.moveForward = true;
            break;
        case kVK_ANSI_S:
            inputState_.moveBackward = true;
            break;
        case kVK_ANSI_A:
            inputState_.moveLeft = true;
            break;
        case kVK_ANSI_D:
            inputState_.moveRight = true;
            break;
        case kVK_Space:
            inputState_.moveUp = true;
            break;
        case kVK_ANSI_Q:
            inputState_.moveDown = true;
            break;
        case kVK_ANSI_R:
            // Reset camera to initial position
            printf("🔄 Resetting camera position\n");
            camera_->lookAt(
                Engine::Math::Vector::make(0.0f, 2.0f, 5.0f),  // position
                Engine::Math::Vector::make(0.0f, 0.0f, 0.0f),  // target
                Engine::Math::Vector::Constants::UP             // up
            );
            break;
    }
}

void MetalApplication::processKeyUp(unsigned short keyCode) {
    switch (keyCode) {
        case kVK_ANSI_W:
            inputState_.moveForward = false;
            break;
        case kVK_ANSI_S:
            inputState_.moveBackward = false;
            break;
        case kVK_ANSI_A:
            inputState_.moveLeft = false;
            break;
        case kVK_ANSI_D:
            inputState_.moveRight = false;
            break;
        case kVK_Space:
            inputState_.moveUp = false;
            break;
        case kVK_ANSI_Q:
            inputState_.moveDown = false;
            break;
    }
}

void MetalApplication::processMouseMove(float deltaX, float deltaY) {
    if (!inputState_.mouseLookEnabled || !isInitialized_) {
        return;
    }

    camera_->processMouseMovement(deltaX, deltaY);
}

void MetalApplication::setMouseLook(bool enabled) {
    inputState_.mouseLookEnabled = enabled;
    if (enabled) {
        printf("🖱️ Mouse look enabled\n");
    } else {
        printf("🖱️ Mouse look disabled\n");
    }
}
