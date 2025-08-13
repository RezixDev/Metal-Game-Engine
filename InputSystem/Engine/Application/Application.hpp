// Engine/Application/Application.hpp
// Base application framework
#pragma once

#include "../Core/Time.hpp"
#include "../Core/Math.hpp"
#include <memory>
#include <string>

namespace Engine {

    // Forward declarations
    class Scene;
    class RenderDevice;
    class InputManager;

    struct ApplicationConfig {
        std::string title = "Engine Application";
        int windowWidth = 960;
        int windowHeight = 600;
        bool fullscreen = false;
        bool vsync = true;
        float targetFPS = 0.0f;  // 0 = unlimited
        bool resizable = true;
    };

    class Application {
    public:
        Application(const ApplicationConfig& config = {})
            : config_(config)
            , isRunning_(false)
            , isPaused_(false)
        {}

        virtual ~Application() = default;

        // Lifecycle methods (override in derived class)
        virtual bool onInitialize() { return true; }
        virtual void onShutdown() {}
        virtual void onUpdate(float deltaTime) {}
        virtual void onFixedUpdate(float fixedDeltaTime) {}
        virtual void onRender() {}
        virtual void onGUI() {}
        virtual void onResize(int width, int height) {}
        virtual void onPause() {}
        virtual void onResume() {}

        // Main run method
        int run() {
            if (!initialize()) {
                return -1;
            }

            mainLoop();

            shutdown();
            return 0;
        }

        // Control methods
        void quit() { isRunning_ = false; }
        void pause() { isPaused_ = true; onPause(); }
        void resume() { isPaused_ = false; onResume(); }
        bool isRunning() const { return isRunning_; }
        bool isPaused() const { return isPaused_; }

        // Window info
        int getWindowWidth() const { return windowWidth_; }
        int getWindowHeight() const { return windowHeight_; }
        float getAspectRatio() const {
            return (float)windowWidth_ / (float)windowHeight_;
        }

    protected:
        // Internal lifecycle
        bool initialize() {
            Core::Time::getInstance().initialize();
            Core::Time::getInstance().setTargetFPS(config_.targetFPS);

            windowWidth_ = config_.windowWidth;
            windowHeight_ = config_.windowHeight;

            if (!onInitialize()) {
                return false;
            }

            isRunning_ = true;
            return true;
        }

        void mainLoop() {
            while (isRunning_) {
                // Update time
                Core::Time::getInstance().update();
                float deltaTime = Core::Time::getInstance().getDeltaTime();

                // Process events (will be handled by platform layer)
                processEvents();

                // Fixed update
                while (Core::Time::getInstance().shouldDoFixedUpdate()) {
                    if (!isPaused_) {
                        onFixedUpdate(Core::Time::getInstance().getFixedDeltaTime());
                    }
                }

                // Variable update
                if (!isPaused_) {
                    onUpdate(deltaTime);
                }

                // Render
                onRender();

                // GUI
                onGUI();

                // Present (will be handled by platform layer)
                present();

                // Frame rate limiting
                Core::Time::getInstance().waitForTargetFPS();

                // Debug output every 60 frames
                static int frameCounter = 0;
                if (++frameCounter >= 60) {
                    frameCounter = 0;
                    if (showDebugInfo_) {
                        debugOutput();
                    }
                }
            }
        }

        void shutdown() {
            onShutdown();
        }

        virtual void processEvents() {
            // Override in platform-specific implementation
        }

        virtual void present() {
            // Override in platform-specific implementation
        }

        void debugOutput() {
            printf("[App] FPS: %.1f | Frame: %llu | Time: %.2f\n",
                   Core::Time::getInstance().getFPS(),
                   Core::Time::getInstance().getFrameCount(),
                   Core::Time::getInstance().getTime());
        }

        // Configuration
        ApplicationConfig config_;

        // State
        bool isRunning_;
        bool isPaused_;
        bool showDebugInfo_ = true;

        // Window dimensions
        int windowWidth_;
        int windowHeight_;
    };

    // Singleton for global access (optional)
    class ApplicationInstance {
    public:
        static void set(Application* app) {
            instance_ = app;
        }

        static Application* get() {
            return instance_;
        }

    private:
        static Application* instance_;
    };

} // namespace Engine