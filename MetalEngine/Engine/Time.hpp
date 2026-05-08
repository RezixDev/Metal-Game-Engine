// Engine/Time.hpp
#pragma once

#include <chrono>
#include <algorithm>

namespace Engine {

// Per-frame delta-time clock. Construct, call tick() once per frame, get dt.
class FrameClock {
public:
    float tick() {
        const auto now = Clock::now();
        const float dt = std::chrono::duration<float>(now - last_).count();
        last_ = now;
        // Cap dt so a long pause doesn't catapult the camera across the scene.
        return std::min(dt, 1.0f / 30.0f);
    }

private:
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point last_ = Clock::now();
};

} // namespace Engine
