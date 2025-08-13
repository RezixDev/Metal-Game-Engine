// MetalApplication.hpp
// Bridges the new architecture with existing Metal implementation
#pragma once

#include "Engine/Application/Application.hpp"
#include "Engine/Scene/Scene.hpp"  // Include the Scene header
#include "Engine/Graphics/RenderSystem.hpp"
#include "Engine/Camera.hpp"
#include <memory>

class MetalApplication : public Engine::Application {
public:
    MetalApplication()
        : Engine::Application({
            .title = "Metal + Clean Architecture",
            .windowWidth = 960,
            .windowHeight = 600,
            .vsync = true,
            .targetFPS = 0  // Unlimited
        })
    {}

    // Override lifecycle methods
    bool onInitialize() override {
        // Create scene
        scene_ = std::make_shared<Engine::SceneSystem::Scene>("Main Scene");

        // Initialize camera with your existing Camera class
        // We'll wrap it in a SceneNode-compatible class
        initializeCamera();

        // Create test cubes
        createTestScene();

        // Initialize input bindings
        setupInputBindings();

        return true;
    }

    void onUpdate(float deltaTime) override {
        // Process camera movement
        updateCameraMovement(deltaTime);

        // Update scene
        scene_->update(deltaTime);
    }

    void onRender() override {
        // This will be called by your existing renderer
        // For now, we keep using your current render system

        // The camera provides view and projection matrices
        if (camera_) {
            float aspect = static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_);
            auto viewMatrix = camera_->getViewMatrix();
            auto projMatrix = camera_->getProjectionMatrix(aspect);

            // These matrices would be passed to your renderer
            // We'll integrate this in the next step
        }
    }

    void onResize(int width, int height) override {
        // Handle window resize
        windowWidth_ = width;
        windowHeight_ = height;
    }

    // Camera access for renderer
    Camera* getCamera() { return camera_.get(); }

    // Input state for MyMetalView integration
    struct InputState {
        bool moveForward = false;
        bool moveBackward = false;
        bool moveLeft = false;
        bool moveRight = false;
        bool moveUp = false;
        bool moveDown = false;
        bool mouseLookEnabled = false;
        float mouseDeltaX = 0;
        float mouseDeltaY = 0;
    };

    InputState& getInputState() { return inputState_; }

    void processKeyDown(uint16_t keyCode) {
        switch(keyCode) {
            case 13: inputState_.moveForward = true; break;   // W
            case 1:  inputState_.moveBackward = true; break;  // S
            case 0:  inputState_.moveLeft = true; break;      // A
            case 2:  inputState_.moveRight = true; break;     // D
            case 49: inputState_.moveUp = true; break;        // Space
            case 12: inputState_.moveDown = true; break;      // Q
            case 15: resetCamera(); break;                    // R
        }
    }

    void processKeyUp(uint16_t keyCode) {
        switch(keyCode) {
            case 13: inputState_.moveForward = false; break;
            case 1:  inputState_.moveBackward = false; break;
            case 0:  inputState_.moveLeft = false; break;
            case 2:  inputState_.moveRight = false; break;
            case 49: inputState_.moveUp = false; break;
            case 12: inputState_.moveDown = false; break;
        }
    }

    void processMouseMove(float deltaX, float deltaY) {
        if (inputState_.mouseLookEnabled) {
            inputState_.mouseDeltaX += deltaX;
            inputState_.mouseDeltaY += deltaY;
        }
    }

    void setMouseLook(bool enabled) {
        inputState_.mouseLookEnabled = enabled;
    }

protected:  // Changed from private to protected to allow access from derived class
    // Helper method to get aspect ratio
    float getAspectRatio() const {
        if (windowHeight_ == 0) return 1.0f;
        return static_cast<float>(windowWidth_) / static_cast<float>(windowHeight_);
    }

    void initializeCamera() {
        // Create camera with your existing implementation
        vector_float3 cameraPos = {-5.0f, 3.0f, 10.0f};
        vector_float3 cameraTarget = {0.0f, 0.0f, 0.0f};
        vector_float3 cameraUp = {0.0f, 1.0f, 0.0f};

        camera_ = std::make_unique<Camera>(cameraPos, cameraTarget, cameraUp);
        camera_->setMovementSpeed(3.0f);
    }

    void createTestScene() {
        // This would create scene nodes for your cubes
        // For now, we keep the cube rendering in your existing renderer
    }

    void setupInputBindings() {
        // Future: Set up proper input bindings using the new input system
    }

    void updateCameraMovement(float deltaTime) {
        if (!camera_) return;

        // Process movement
        camera_->processMovement(
            deltaTime,
            inputState_.moveForward,
            inputState_.moveBackward,
            inputState_.moveLeft,
            inputState_.moveRight,
            inputState_.moveUp,
            inputState_.moveDown
        );

        // Process mouse look
        if (inputState_.mouseDeltaX != 0 || inputState_.mouseDeltaY != 0) {
            camera_->processMouseMovement(
                inputState_.mouseDeltaX,
                inputState_.mouseDeltaY,
                0.002f
            );
            inputState_.mouseDeltaX = 0;
            inputState_.mouseDeltaY = 0;
        }
    }

    void resetCamera() {
        vector_float3 cameraPos = {-5.0f, 3.0f, 10.0f};
        vector_float3 cameraTarget = {0.0f, 0.0f, 0.0f};
        vector_float3 cameraUp = {0.0f, 1.0f, 0.0f};

        camera_->lookAt(cameraPos, cameraTarget, cameraUp);
    }

    std::shared_ptr<Engine::SceneSystem::Scene> scene_;
    std::unique_ptr<Camera> camera_;
    InputState inputState_;
    int windowWidth_ = 960;   // Added member variables
    int windowHeight_ = 600;  // Added member variables
};

// Global instance for easy access from Objective-C
inline MetalApplication* getMetalApplication() {
    static MetalApplication app;
    return &app;
}