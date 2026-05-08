// Engine/Core/Time.hpp
#pragma once

#include <chrono>
#include <algorithm>

namespace Engine {
namespace Core {

// Single-frame delta-time clock. Initialize once, then call update() at the top
// of each frame and read getDeltaTime() during the frame.
class Time {
public:
    static Time& getInstance() {
        static Time instance;
        return instance;
    }

    void initialize() {
        lastFrameTime_ = Clock::now();
        deltaTime_     = 0.0f;
    }

    void update() {
        const TimePoint now = Clock::now();
        const Duration  dt  = now - lastFrameTime_;
        // Cap the delta at 1/30 s so a long pause doesn't blow up the next
        // physics or movement step.
        deltaTime_     = std::min(dt.count(), 1.0f / 30.0f);
        lastFrameTime_ = now;
    }

    float getDeltaTime() const { return deltaTime_; }

private:
    using Clock     = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration  = std::chrono::duration<float>;

    TimePoint lastFrameTime_ = Clock::now();
    float     deltaTime_     = 0.0f;
};

} // namespace Core
} // namespace Engine
