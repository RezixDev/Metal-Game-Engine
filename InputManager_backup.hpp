// InputManager_backup.hpp
// Enhanced Input System with better abstraction and features
#pragma once

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <vector>
#include <string>
#include <cstdint>
#include "Math.hpp"

namespace Engine {
namespace Input {

    // ========================================
    // Key and Button Definitions
    // ========================================
    
    using KeyCode = uint16_t;
    using MouseButton = uint8_t;
    
    namespace Keys {
        // Letters
        constexpr KeyCode A = 0;
        constexpr KeyCode B = 11;
        constexpr KeyCode C = 8;
        constexpr KeyCode D = 2;
        constexpr KeyCode E = 14;
        constexpr KeyCode F = 3;
        constexpr KeyCode G = 5;
        constexpr KeyCode H = 4;
        constexpr KeyCode I = 34;
        constexpr KeyCode J = 38;
        constexpr KeyCode K = 40;
        constexpr KeyCode L = 37;
        constexpr KeyCode M = 46;
        constexpr KeyCode N = 45;
        constexpr KeyCode O = 31;
        constexpr KeyCode P = 35;
        constexpr KeyCode Q = 12;
        constexpr KeyCode R = 15;
        constexpr KeyCode S = 1;
        constexpr KeyCode T = 17;
        constexpr KeyCode U = 32;
        constexpr KeyCode V = 9;
        constexpr KeyCode W = 13;
        constexpr KeyCode X = 7;
        constexpr KeyCode Y = 16;
        constexpr KeyCode Z = 6;
        
        // Numbers
        constexpr KeyCode Key0 = 29;
        constexpr KeyCode Key1 = 18;
        constexpr KeyCode Key2 = 19;
        constexpr KeyCode Key3 = 20;
        constexpr KeyCode Key4 = 21;
        constexpr KeyCode Key5 = 23;
        constexpr KeyCode Key6 = 22;
        constexpr KeyCode Key7 = 26;
        constexpr KeyCode Key8 = 28;
        constexpr KeyCode Key9 = 25;
        
        // Special keys
        constexpr KeyCode Space = 49;
        constexpr KeyCode Return = 36;
        constexpr KeyCode Escape = 53;
        constexpr KeyCode Backspace = 51;
        constexpr KeyCode Tab = 48;
        constexpr KeyCode LeftShift = 56;
        constexpr KeyCode RightShift = 60;
        constexpr KeyCode LeftControl = 59;
        constexpr KeyCode RightControl = 62;
        constexpr KeyCode LeftAlt = 58;
        constexpr KeyCode RightAlt = 61;
        constexpr KeyCode LeftCommand = 55;
        constexpr KeyCode RightCommand = 54;
        
        // Arrow keys
        constexpr KeyCode Left = 123;
        constexpr KeyCode Right = 124;
        constexpr KeyCode Down = 125;
        constexpr KeyCode Up = 126;
        
        // Function keys
        constexpr KeyCode F1 = 122;
        constexpr KeyCode F2 = 120;
        constexpr KeyCode F3 = 99;
        constexpr KeyCode F4 = 118;
        constexpr KeyCode F5 = 96;
        constexpr KeyCode F6 = 97;
        constexpr KeyCode F7 = 98;
        constexpr KeyCode F8 = 100;
        constexpr KeyCode F9 = 101;
        constexpr KeyCode F10 = 109;
        constexpr KeyCode F11 = 103;
        constexpr KeyCode F12 = 111;
    }
    
    namespace Mouse {
        constexpr MouseButton Left = 0;
        constexpr MouseButton Right = 1;
        constexpr MouseButton Middle = 2;
        constexpr MouseButton Button4 = 3;
        constexpr MouseButton Button5 = 4;
    }
    
    // ========================================
    // Input State Structures
    // ========================================
    
    enum class InputState {
        Released,
        Pressed,
        Held
    };
    
    struct KeyState {
        InputState state = InputState::Released;
        float duration = 0.0f;  // How long the key has been in current state
        bool consumed = false;  // For input handling priority
    };
    
    struct MouseState {
        Math::Vec2 position = {0, 0};
        Math::Vec2 deltaPosition = {0, 0};
        Math::Vec2 scrollDelta = {0, 0};
        std::unordered_map<MouseButton, KeyState> buttons;
        bool isOverWindow = true;
    };
    
    // ========================================
    // Input Events
    // ========================================
    
    enum class EventType {
        KeyPressed,
        KeyReleased,
        MousePressed,
        MouseReleased,
        MouseMoved,
        MouseScrolled,
        WindowFocused,
        WindowUnfocused
    };
    
    struct InputEvent {
        EventType type;
        union {
            struct { KeyCode key; } keyboard;
            struct { MouseButton button; Math::Vec2 position; } mouse;
            struct { Math::Vec2 position; Math::Vec2 delta; } mouseMoved;
            struct { Math::Vec2 delta; } mouseScrolled;
        } data;
        float timestamp;
        bool consumed = false;
    };
    
    using InputCallback = std::function<bool(const InputEvent&)>;
    
    // ========================================
    // Input Binding System
    // ========================================
    
    enum class BindingType {
        Key,
        MouseButton,
        MouseAxis,
        Combination
    };
    
    struct InputBinding {
        std::string name;
        BindingType type;
        union {
            KeyCode key;
            MouseButton mouseButton;
            struct {
                KeyCode primary;
                KeyCode modifier;
            } combination;
        } input;
        float sensitivity = 1.0f;
        bool enabled = true;
    };
    
    // ========================================
    // Main Input Manager Class
    // ========================================
    
    class InputManager {
    public:
        static InputManager& getInstance() {
            static InputManager instance;
            return instance;
        }
        
        // ========================================
        // Core Input Processing
        // ========================================
        
        void update(float deltaTime) {
            deltaTime_ = deltaTime;
            
            // Update key durations
            for (auto& [key, state] : keyStates_) {
                if (state.state != InputState::Released) {
                    state.duration += deltaTime;
                }
                state.consumed = false; // Reset consumed state each frame
            }
            
            // Update mouse button durations
            for (auto& [button, state] : mouseState_.buttons) {
                if (state.state != InputState::Released) {
                    state.duration += deltaTime;
                }
                state.consumed = false;
            }
            
            // Reset mouse delta and scroll
            mouseState_.deltaPosition = {0, 0};
            mouseState_.scrollDelta = {0, 0};
            
            // Clear events from previous frame
            events_.clear();
        }
        
        // ========================================
        // Keyboard Input
        // ========================================
        
        void keyDown(KeyCode key) {
            auto& state = keyStates_[key];
            if (state.state == InputState::Released) {
                state.state = InputState::Pressed;
                state.duration = 0.0f;
                
                // Add event
                InputEvent event;
                event.type = EventType::KeyPressed;
                event.data.keyboard.key = key;
                event.timestamp = getCurrentTime();
                events_.push_back(event);
            } else if (state.state == InputState::Pressed) {
                state.state = InputState::Held;
            }
        }
        
        void keyUp(KeyCode key) {
            auto& state = keyStates_[key];
            if (state.state != InputState::Released) {
                state.state = InputState::Released;
                state.duration = 0.0f;
                
                // Add event
                InputEvent event;
                event.type = EventType::KeyReleased;
                event.data.keyboard.key = key;
                event.timestamp = getCurrentTime();
                events_.push_back(event);
            }
        }
        
        bool isKeyPressed(KeyCode key) const {
            auto it = keyStates_.find(key);
            return it != keyStates_.end() && it->second.state == InputState::Pressed && !it->second.consumed;
        }
        
        bool isKeyHeld(KeyCode key) const {
            auto it = keyStates_.find(key);
            return it != keyStates_.end() &&
                   (it->second.state == InputState::Pressed || it->second.state == InputState::Held) &&
                   !it->second.consumed;
        }
        
        bool isKeyReleased(KeyCode key) const {
            auto it = keyStates_.find(key);
            return it != keyStates_.end() && it->second.state == InputState::Released;
        }
        
        float getKeyDuration(KeyCode key) const {
            auto it = keyStates_.find(key);
            return it != keyStates_.end() ? it->second.duration : 0.0f;
        }
        
        void consumeKey(KeyCode key) {
            auto it = keyStates_.find(key);
            if (it != keyStates_.end()) {
                it->second.consumed = true;
            }
        }
        
        // ========================================
        // Mouse Input
        // ========================================
        
        void mouseDown(MouseButton button, const Math::Vec2& position) {
            auto& state = mouseState_.buttons[button];
            if (state.state == InputState::Released) {
                state.state = InputState::Pressed;
                state.duration = 0.0f;
                
                // Add event
                InputEvent event;
                event.type = EventType::MousePressed;
                event.data.mouse.button = button;
                event.data.mouse.position = position;
                event.timestamp = getCurrentTime();
                events_.push_back(event);
            } else if (state.state == InputState::Pressed) {
                state.state = InputState::Held;
            }
        }
        
        void mouseUp(MouseButton button, const Math::Vec2& position) {
            auto& state = mouseState_.buttons[button];
            if (state.state != InputState::Released) {
                state.state = InputState::Released;
                state.duration = 0.0f;
                
                // Add event
                InputEvent event;
                event.type = EventType::MouseReleased;
                event.data.mouse.button = button;
                event.data.mouse.position = position;
                event.timestamp = getCurrentTime();
                events_.push_back(event);
            }
        }
        
        void mouseMoved(const Math::Vec2& position) {
            Math::Vec2 delta = position - mouseState_.position;
            mouseState_.deltaPosition += delta;
            mouseState_.position = position;
            
            // Add event
            InputEvent event;
            event.type = EventType::MouseMoved;
            event.data.mouseMoved.position = position;
            event.data.mouseMoved.delta = delta;
            event.timestamp = getCurrentTime();
            events_.push_back(event);
        }
        
        void mouseScrolled(const Math::Vec2& delta) {
            mouseState_.scrollDelta += delta;
            
            // Add event
            InputEvent event;
            event.type = EventType::MouseScrolled;
            event.data.mouseScrolled.delta = delta;
            event.timestamp = getCurrentTime();
            events_.push_back(event);
        }
        
        bool isMousePressed(MouseButton button) const {
            auto it = mouseState_.buttons.find(button);
            return it != mouseState_.buttons.end() && it->second.state == InputState::Pressed && !it->second.consumed;
        }
        
        bool isMouseHeld(MouseButton button) const {
            auto it = mouseState_.buttons.find(button);
            return it != mouseState_.buttons.end() &&
                   (it->second.state == InputState::Pressed || it->second.state == InputState::Held) &&
                   !it->second.consumed;
        }
        
        bool isMouseReleased(MouseButton button) const {
            auto it = mouseState_.buttons.find(button);
            return it != mouseState_.buttons.end() && it->second.state == InputState::Released;
        }
        
        const Math::Vec2& getMousePosition() const { return mouseState_.position; }
        const Math::Vec2& getMouseDelta() const { return mouseState_.deltaPosition; }
        const Math::Vec2& getScrollDelta() const { return mouseState_.scrollDelta; }
        
        void consumeMouse(MouseButton button) {
            auto it = mouseState_.buttons.find(button);
            if (it != mouseState_.buttons.end()) {
                it->second.consumed = true;
            }
        }
        
        // ========================================
        // Input Combinations
        // ========================================
        
        bool isKeyCombo(KeyCode primary, KeyCode modifier) const {
            return isKeyHeld(modifier) && isKeyPressed(primary);
        }
        
        bool isKeyComboHeld(KeyCode primary, KeyCode modifier) const {
            return isKeyHeld(modifier) && isKeyHeld(primary);
        }
        
        // ========================================
        // Input Bindings
        // ========================================
        
        void addBinding(const std::string& name, KeyCode key, float sensitivity = 1.0f) {
            InputBinding binding;
            binding.name = name;
            binding.type = BindingType::Key;
            binding.input.key = key;
            binding.sensitivity = sensitivity;
            bindings_[name] = binding;
        }
        
        void addBinding(const std::string& name, MouseButton button, float sensitivity = 1.0f) {
            InputBinding binding;
            binding.name = name;
            binding.type = BindingType::MouseButton;
            binding.input.mouseButton = button;
            binding.sensitivity = sensitivity;
            bindings_[name] = binding;
        }
        
        void addComboBinding(const std::string& name, KeyCode primary, KeyCode modifier, float sensitivity = 1.0f) {
            InputBinding binding;
            binding.name = name;
            binding.type = BindingType::Combination;
            binding.input.combination.primary = primary;
            binding.input.combination.modifier = modifier;
            binding.sensitivity = sensitivity;
            bindings_[name] = binding;
        }
        
        void removeBinding(const std::string& name) {
            bindings_.erase(name);
        }
        
        bool isBindingPressed(const std::string& name) const {
            auto it = bindings_.find(name);
            if (it == bindings_.end() || !it->second.enabled) return false;
            
            const auto& binding = it->second;
            switch (binding.type) {
                case BindingType::Key:
                    return isKeyPressed(binding.input.key);
                case BindingType::MouseButton:
                    return isMousePressed(binding.input.mouseButton);
                case BindingType::Combination:
                    return isKeyCombo(binding.input.combination.primary, binding.input.combination.modifier);
                default:
                    return false;
            }
        }
        
        bool isBindingHeld(const std::string& name) const {
            auto it = bindings_.find(name);
            if (it == bindings_.end() || !it->second.enabled) return false;
            
            const auto& binding = it->second;
            switch (binding.type) {
                case BindingType::Key:
                    return isKeyHeld(binding.input.key);
                case BindingType::MouseButton:
                    return isMouseHeld(binding.input.mouseButton);
                case BindingType::Combination:
                    return isKeyComboHeld(binding.input.combination.primary, binding.input.combination.modifier);
                default:
                    return false;
            }
        }
        
        float getBindingValue(const std::string& name) const {
            auto it = bindings_.find(name);
            if (it == bindings_.end() || !it->second.enabled) return 0.0f;
            
            const auto& binding = it->second;
            float value = 0.0f;
            
            switch (binding.type) {
                case BindingType::Key:
                    value = isKeyHeld(binding.input.key) ? 1.0f : 0.0f;
                    break;
                case BindingType::MouseButton:
                    value = isMouseHeld(binding.input.mouseButton) ? 1.0f : 0.0f;
                    break;
                case BindingType::MouseAxis:
                    // For mouse axis bindings, return delta
                    value = mouseState_.deltaPosition.x; // This could be made more sophisticated
                    break;
                case BindingType::Combination:
                    value = isKeyComboHeld(binding.input.combination.primary, binding.input.combination.modifier) ? 1.0f : 0.0f;
                    break;
            }
            
            return value * binding.sensitivity;
        }
        
        void setBindingEnabled(const std::string& name, bool enabled) {
            auto it = bindings_.find(name);
            if (it != bindings_.end()) {
                it->second.enabled = enabled;
            }
        }
        
        // ========================================
        // Event System
        // ========================================
        
        void addEventCallback(const std::string& name, InputCallback callback) {
            callbacks_[name] = callback;
        }
        
        void removeEventCallback(const std::string& name) {
            callbacks_.erase(name);
        }
        
        void processEvents() {
            for (auto& event : events_) {
                if (event.consumed) continue;
                
                // Process callbacks in registration order
                for (auto& [name, callback] : callbacks_) {
                    if (callback(event)) {
                        event.consumed = true;
                        break; // Event was handled, stop processing
                    }
                }
            }
        }
        
        const std::vector<InputEvent>& getEvents() const { return events_; }
        
        // ========================================
        // Utility Functions
        // ========================================
        
        void setMouseCursorVisible(bool visible) {
            cursorVisible_ = visible;
            // Platform-specific cursor visibility code would go here
        }
        
        bool isMouseCursorVisible() const { return cursorVisible_; }
        
        void centerMouseCursor() {
            // Platform-specific cursor centering code would go here
        }
        
        void reset() {
            keyStates_.clear();
            mouseState_ = MouseState{};
            events_.clear();
        }
        
        void debugPrint() const {
            printf("Input Manager Debug:\n");
            printf("  Active Keys: %zu\n", keyStates_.size());
            printf("  Mouse Position: (%.2f, %.2f)\n", mouseState_.position.x, mouseState_.position.y);
            printf("  Mouse Delta: (%.2f, %.2f)\n", mouseState_.deltaPosition.x, mouseState_.deltaPosition.y);
            printf("  Active Bindings: %zu\n", bindings_.size());
            printf("  Event Callbacks: %zu\n", callbacks_.size());
            printf("  Events This Frame: %zu\n", events_.size());
        }
        
        // ========================================
        // Common Input Patterns
        // ========================================
        
        struct WASDInput {
            bool forward = false;
            bool backward = false;
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        };
        
        WASDInput getWASDInput() const {
            WASDInput input;
            input.forward = isKeyHeld(Keys::W);
            input.backward = isKeyHeld(Keys::S);
            input.left = isKeyHeld(Keys::A);
            input.right = isKeyHeld(Keys::D);
            input.up = isKeyHeld(Keys::Space);
            input.down = isKeyHeld(Keys::Q);
            return input;
        }
        
        Math::Vec2 getMovementVector() const {
            Math::Vec2 movement = {0, 0};
            if (isKeyHeld(Keys::W)) movement.y += 1.0f;
            if (isKeyHeld(Keys::S)) movement.y -= 1.0f;
            if (isKeyHeld(Keys::A)) movement.x -= 1.0f;
            if (isKeyHeld(Keys::D)) movement.x += 1.0f;
            
            // Normalize diagonal movement
            float length = Math::Vector::length(movement);
            if (length > 1.0f) {
                movement = Math::Vector::make(movement.x / length, movement.y / length);
            }
            
            return movement;
        }
        
    private:
        InputManager() = default;
        
        std::unordered_map<KeyCode, KeyState> keyStates_;
        MouseState mouseState_;
        std::vector<InputEvent> events_;
        std::unordered_map<std::string, InputBinding> bindings_;
        std::unordered_map<std::string, InputCallback> callbacks_;
        
        float deltaTime_ = 0.0f;
        bool cursorVisible_ = true;
        
        float getCurrentTime() const {
            // This would typically use a high-precision timer
            // For now, return a simple incrementing value
            static float time = 0.0f;
            time += deltaTime_;
            return time;
        }
    };
    
    // ========================================
    // Convenience Functions
    // ========================================
    
    inline InputManager& Input() {
        return InputManager::getInstance();
    }
    
    // Quick access functions for common operations
    inline bool IsKeyPressed(KeyCode key) { return Input().isKeyPressed(key); }
    inline bool IsKeyHeld(KeyCode key) { return Input().isKeyHeld(key); }
    inline bool IsMousePressed(MouseButton button) { return Input().isMousePressed(button); }
    inline bool IsMouseHeld(MouseButton button) { return Input().isMouseHeld(button); }
    inline Math::Vec2 GetMousePosition() { return Input().getMousePosition(); }
    inline Math::Vec2 GetMouseDelta() { return Input().getMouseDelta(); }
    inline Math::Vec2 GetScrollDelta() { return Input().getScrollDelta(); }

} // namespace Input
} // namespace Engine

// ========================================
// Backward Compatibility
// ========================================

// For existing code that uses the old InputManager interface
class InputManager {
public:
    static InputManager& instance() {
        static InputManager inst;
        return inst;
    }
    
    void keyDown(uint16_t key) {
        Engine::Input::Input().keyDown(key);
    }
    
    void keyUp(uint16_t key) {
        Engine::Input::Input().keyUp(key);
    }
    
    bool down(uint16_t key) const {
        return Engine::Input::Input().isKeyHeld(key);
    }
};
