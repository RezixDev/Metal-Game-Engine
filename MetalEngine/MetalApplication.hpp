// MetalApplication.hpp
#pragma once

#include "Renderer.hpp"
#include <memory>
#include <vector>

struct InputState {
    bool moveForward      = false;
    bool moveBackward     = false;
    bool moveLeft         = false;
    bool moveRight        = false;
    bool moveUp           = false;
    bool moveDown         = false;
    bool mouseLookEnabled = false;
};

class MetalApplication {
public:
    static MetalApplication& shared();

    MetalApplication();
    ~MetalApplication();

    MetalApplication(const MetalApplication&) = delete;
    MetalApplication& operator=(const MetalApplication&) = delete;

    void onInitialize(Engine::Graphics::RenderDevicePtr device);
    void onUpdate(float deltaTime);
    void onRender();
    void onResize(int width, int height);
    void onShutdown();

    void processKeyDown(unsigned short keyCode);
    void processKeyUp(unsigned short keyCode);
    void processMouseMove(float deltaX, float deltaY);
    void setMouseLook(bool enabled);

    Engine::Camera* getCamera() { return camera_.get(); }
    const InputState& getInputState() const { return inputState_; }

private:
    struct SceneObject {
        Engine::Graphics::MeshPtr mesh;
        Engine::Math::Mat4 transform;
    };

    void setupScene();

    Engine::Graphics::RenderDevicePtr renderDevice_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<Engine::Camera> camera_;

    std::vector<SceneObject> sceneObjects_;
    InputState inputState_;

    float moveSpeed_        = 5.0f;
    float mouseSensitivity_ = 0.002f;
    bool isInitialized_     = false;
};

// Backwards-compatible accessor used by Objective-C glue code.
MetalApplication* getMetalApplication();
