// MetalApplication.cpp
#include "MetalApplication.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>
#include <cfloat>
#include "Engine/Camera.hpp"

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
static MetalApplication* g_metalApplication = nullptr;

MetalApplication* getMetalApplication() {
    if (!g_metalApplication) {
        g_metalApplication = new MetalApplication();
    }
    return g_metalApplication;
}

MetalApplication::MetalApplication()
    : camera_(std::make_unique<Engine::Camera>(
          Math::Vector::make(0.0f, 2.0f, 5.0f),
          Math::Vector::make(0.0f, 0.0f, 0.0f),
          Math::Vector::Constants::UP)) {}

MetalApplication::~MetalApplication() {
    if (g_metalApplication == this) {
        g_metalApplication = nullptr;
    }
}

void MetalApplication::onUpdate(float deltaTime) {
    // Process camera movement based on input state
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
    // Future: Handle any application-specific rendering
}

void MetalApplication::onResize(int width, int height) {
    // The new camera system handles aspect ratio internally
    // when generating projection matrices, so we don't need
    // to explicitly set it here
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
            camera_->lookAt(
                Math::Vector::make(0.0f, 2.0f, 5.0f),  // position
                Math::Vector::make(0.0f, 0.0f, 0.0f),  // target
                Math::Vector::Constants::UP             // up
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
    if (!inputState_.mouseLookEnabled) return;

    // The new camera system handles mouse movement internally
    camera_->processMouseMovement(deltaX, deltaY);
}

void MetalApplication::setMouseLook(bool enabled) {
    inputState_.mouseLookEnabled = enabled;
}
