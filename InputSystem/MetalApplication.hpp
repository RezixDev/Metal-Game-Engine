// MetalApplication.hpp
#pragma once

#include "Engine/Camera.hpp"
#include <simd/simd.h>

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

// Metal Application class - handles input and camera management
class MetalApplication {
public:
    MetalApplication();
    ~MetalApplication();

    // Lifecycle methods
    void onInitialize();
    void onUpdate(float deltaTime);
    void onRender();
    void onResize(int width, int height);

    // Input processing
    void processKeyDown(unsigned short keyCode);
    void processKeyUp(unsigned short keyCode);
    void processMouseMove(float deltaX, float deltaY);
    void setMouseLook(bool enabled);

    // Getters
    Engine::Camera* getCamera() { return camera_; }
    const InputState& getInputState() const { return inputState_; }

private:
    Engine::Camera* camera_;
    InputState inputState_;

    // Camera control parameters
    float moveSpeed_ = 5.0f;
    float mouseSensitivity_ = 0.002f;
};

// Global function to get the application instance
MetalApplication* getMetalApplication();