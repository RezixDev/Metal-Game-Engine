// Engine/Core/Time.hpp
// Centralized time management system
#pragma once

#include <chrono>
#include <cstdint>
#include <thread>  // Added for std::this_thread::sleep_for
#include <string>   // Added for std::string
#include <algorithm> // Added for std::min/max
#include <cstdio>   // Added for printf

namespace Engine {
namespace Core {

    class Time {
    public:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;
        using Duration = std::chrono::duration<float>;

        // Singleton access
        static Time& getInstance() {
            static Time instance;
            return instance;
        }

        // Initialize the time system
        void initialize() {
            startTime_ = Clock::now();
            currentTime_ = startTime_;
            lastFrameTime_ = startTime_;
            deltaTime_ = 0.0f;
            unscaledDeltaTime_ = 0.0f;
            timeScale_ = 1.0f;
            frameCount_ = 0;
            fps_ = 0.0f;
            fpsUpdateTime_ = 0.0f;
            fpsFrameCount_ = 0;
        }

        // Update time (call once per frame)
        void update() {
            TimePoint now = Clock::now();

            // Calculate unscaled delta time
            Duration delta = now - lastFrameTime_;
            unscaledDeltaTime_ = delta.count();

            // Cap delta time to prevent large jumps
            unscaledDeltaTime_ = std::min(unscaledDeltaTime_, maxDeltaTime_);

            // Apply time scale
            deltaTime_ = unscaledDeltaTime_ * timeScale_;

            // Update times
            lastFrameTime_ = currentTime_;
            currentTime_ = now;

            // Update total time
            Duration totalDuration = currentTime_ - startTime_;
            time_ = totalDuration.count();

            // Update frame count
            frameCount_++;

            // Update FPS counter
            updateFPS();
        }

        // Getters
        float getDeltaTime() const { return deltaTime_; }
        float getUnscaledDeltaTime() const { return unscaledDeltaTime_; }
        float getTime() const { return time_; }
        float getTimeScale() const { return timeScale_; }
        uint64_t getFrameCount() const { return frameCount_; }
        float getFPS() const { return fps_; }
        float getTargetFPS() const { return targetFPS_; }

        // Setters
        void setTimeScale(float scale) {
            timeScale_ = std::max(0.0f, scale);
        }

        void setTargetFPS(float fps) {
            targetFPS_ = fps;
            if (fps > 0) {
                targetFrameTime_ = 1.0f / fps;
            }
        }

        // Frame rate limiting
        void waitForTargetFPS() {
            if (targetFPS_ <= 0) return;

            Duration elapsed = Clock::now() - currentTime_;
            float elapsedSeconds = elapsed.count();

            if (elapsedSeconds < targetFrameTime_) {
                float sleepTime = targetFrameTime_ - elapsedSeconds;
                std::this_thread::sleep_for(
                    std::chrono::duration<float>(sleepTime)
                );
            }
        }

        // Timing utilities
        class ScopedTimer {
        public:
            ScopedTimer(const std::string& name)
                : name_(name), start_(Clock::now()) {}

            ~ScopedTimer() {
                Duration elapsed = Clock::now() - start_;
                printf("[Timer] %s: %.3f ms\n",
                       name_.c_str(), elapsed.count() * 1000.0f);
            }

        private:
            std::string name_;
            TimePoint start_;
        };

        // Fixed update support
        bool shouldDoFixedUpdate() {
            fixedUpdateAccumulator_ += unscaledDeltaTime_;
            if (fixedUpdateAccumulator_ >= fixedDeltaTime_) {
                fixedUpdateAccumulator_ -= fixedDeltaTime_;
                return true;
            }
            return false;
        }

        float getFixedDeltaTime() const { return fixedDeltaTime_; }

        void setFixedDeltaTime(float dt) {
            fixedDeltaTime_ = std::max(0.001f, dt);
        }

        // Interpolation factor for fixed update
        float getInterpolation() const {
            return fixedUpdateAccumulator_ / fixedDeltaTime_;
        }

    private:
        Time() : time_(0.0f) {  // Initialize time_ to avoid uninitialized warning
            initialize();
        }

        void updateFPS() {
            fpsUpdateTime_ += unscaledDeltaTime_;
            fpsFrameCount_++;

            if (fpsUpdateTime_ >= 1.0f) {
                fps_ = fpsFrameCount_ / fpsUpdateTime_;
                fpsUpdateTime_ = 0.0f;
                fpsFrameCount_ = 0;
            }
        }

        // Time points
        TimePoint startTime_;
        TimePoint currentTime_;
        TimePoint lastFrameTime_;

        // Time values
        float deltaTime_;
        float unscaledDeltaTime_;
        float time_;
        float timeScale_;

        // Frame counting
        uint64_t frameCount_;

        // FPS calculation
        float fps_;
        float fpsUpdateTime_;
        int fpsFrameCount_;

        // Frame rate limiting
        float targetFPS_ = 0.0f;  // 0 = unlimited
        float targetFrameTime_ = 0.0f;

        // Fixed update
        float fixedDeltaTime_ = 1.0f / 60.0f;  // 60 Hz default
        float fixedUpdateAccumulator_ = 0.0f;

        // Safety
        float maxDeltaTime_ = 1.0f / 30.0f;  // Cap at 30 FPS minimum
    };

    // Convenience functions
    inline float DeltaTime() { return Time::getInstance().getDeltaTime(); }
    inline float UnscaledDeltaTime() { return Time::getInstance().getUnscaledDeltaTime(); }
    inline float CurrentTime() { return Time::getInstance().getTime(); }
    inline float FPS() { return Time::getInstance().getFPS(); }

} // namespace Core
} // namespace Engine