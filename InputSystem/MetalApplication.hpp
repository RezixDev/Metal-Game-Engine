// MetalApplication.hpp
// UPDATED VERSION - Clean application using graphics abstraction
#pragma once

#include "Engine/Camera.hpp"
#include "Engine/Graphics/RenderDevice.hpp"
#include "Engine/Graphics/GeometryBuilder.hpp"
#include "Renderer.hpp"
#include <memory>
#include <vector>

// Input state structure
struct InputState {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    bool mouseLookEnabled = false;
};

// Modern Metal Application class
class MetalApplication {
public:
    MetalApplication();
    ~MetalApplication();

    // Lifecycle methods
    void onInitialize(Engine::Graphics::RenderDevicePtr device);
    void onUpdate(float deltaTime);
    void onRender();
    void onResize(int width, int height);
    void onShutdown();

    // Input processing
    void processKeyDown(unsigned short keyCode);
    void processKeyUp(unsigned short keyCode);
    void processMouseMove(float deltaX, float deltaY);
    void setMouseLook(bool enabled);

    // Getters
    Engine::Camera* getCamera() { return camera_.get(); }
    const InputState& getInputState() const { return inputState_; }

    // Scene management
    void setupScene();

private:
    // Core systems
    Engine::Graphics::RenderDevicePtr renderDevice_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Engine::Camera> camera_;

    // Scene objects
    std::vector<Engine::Graphics::MeshPtr> sceneMeshes_;

    // Input state
    InputState inputState_;

    // Camera control parameters
    float moveSpeed_ = 5.0f;
    float mouseSensitivity_ = 0.002f;

    // Application state
    bool isInitialized_ = false;
};

// Global function to get the application instance
MetalApplication* getMetalApplication();
