// Engine/Core/Events/EventSystem.hpp
// Type-safe event system for loose coupling
#pragma once

#include "../Base/Types.hpp"
#include <vector>
#include <functional>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <mutex>
#include <any>
#include <string>

namespace Engine {
namespace Core {

    // ========================================
    // Event Base Class
    // ========================================

    class Event {
    public:
        virtual ~Event() = default;

        bool isHandled() const { return handled_; }
        void setHandled(bool handled = true) { handled_ = handled; }

    private:
        bool handled_ = false;
    };

    // ========================================
    // Event Handler Interface
    // ========================================

    template<typename EventType>
    class IEventHandler {
    public:
        virtual ~IEventHandler() = default;
        virtual bool handle(const EventType& event) = 0;
    };

    // ========================================
    // Function-based Event Handler
    // ========================================

    template<typename EventType>
    class FunctionEventHandler : public IEventHandler<EventType> {
    public:
        using HandlerFunction = std::function<bool(const EventType&)>;

        explicit FunctionEventHandler(HandlerFunction handler)
            : handler_(std::move(handler)) {}

        bool handle(const EventType& event) override {
            return handler_(event);
        }

    private:
        HandlerFunction handler_;
    };

    // ========================================
    // Method-based Event Handler
    // ========================================

    template<typename T, typename EventType>
    class MethodEventHandler : public IEventHandler<EventType> {
    public:
        using HandlerMethod = bool (T::*)(const EventType&);

        MethodEventHandler(T* instance, HandlerMethod method)
            : instance_(instance), method_(method) {}

        bool handle(const EventType& event) override {
            return (instance_->*method_)(event);
        }

    private:
        T* instance_;
        HandlerMethod method_;
    };

    // ========================================
    // Event Bus
    // ========================================

    class EventBus {
    public:
        static EventBus& getInstance() {
            static EventBus instance;
            return instance;
        }

        // Subscribe with function
        template<typename EventType>
        uint32 subscribe(std::function<bool(const EventType&)> handler) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto handlerPtr = std::make_shared<FunctionEventHandler<EventType>>(std::move(handler));
            uint32 id = nextHandlerId_++;

            auto& handlers = getHandlers<EventType>();
            handlers[id] = handlerPtr;

            return id;
        }

        // Subscribe with method
        template<typename T, typename EventType>
        uint32 subscribe(T* instance, bool (T::*method)(const EventType&)) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto handlerPtr = std::make_shared<MethodEventHandler<T, EventType>>(instance, method);
            uint32 id = nextHandlerId_++;

            auto& handlers = getHandlers<EventType>();
            handlers[id] = handlerPtr;

            return id;
        }

        // Unsubscribe
        template<typename EventType>
        void unsubscribe(uint32 handlerId) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& handlers = getHandlers<EventType>();
            handlers.erase(handlerId);
        }

        // Publish event
        template<typename EventType>
        void publish(const EventType& event) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto& handlers = getHandlers<EventType>();
            for (auto& [id, handler] : handlers) {
                if (handler && handler->handle(event)) {
                    break; // Event was handled, stop propagation
                }
            }
        }

        // Publish event (move semantics)
        template<typename EventType>
        void publish(EventType&& event) {
            publish(static_cast<const EventType&>(event));
        }

        // Clear all handlers for an event type
        template<typename EventType>
        void clearHandlers() {
            std::lock_guard<std::mutex> lock(mutex_);
            auto& handlers = getHandlers<EventType>();
            handlers.clear();
        }

        // Clear all handlers
        void clearAllHandlers() {
            std::lock_guard<std::mutex> lock(mutex_);
            handlerStorage_.clear();
        }

    private:
        template<typename EventType>
        using HandlerMapT = std::unordered_map<uint32, std::shared_ptr<IEventHandler<EventType>>>;

        template<typename EventType>
        HandlerMapT<EventType>& getHandlers() {
            std::type_index typeIndex = std::type_index(typeid(EventType));
            auto it = handlerStorage_.find(typeIndex);
            if (it == handlerStorage_.end()) {
                // Create a shared_ptr to the handler map and store it in std::any
                auto mapPtr = std::make_shared<HandlerMapT<EventType>>();
                handlerStorage_[typeIndex] = mapPtr;
                return *mapPtr;
            }
            // Cast back to shared_ptr and dereference
            return *std::any_cast<std::shared_ptr<HandlerMapT<EventType>>>(it->second);
        }

        std::unordered_map<std::type_index, std::any> handlerStorage_;

        uint32 nextHandlerId_ = 1;
        std::mutex mutex_;
    };

    // ========================================
    // Common Engine Events
    // ========================================

    // Application Events
    struct ApplicationStartEvent : public Event {
        std::string applicationName;

        // Add explicit constructor
        explicit ApplicationStartEvent(const std::string& name)
            : applicationName(name) {}

        // Default constructor if needed
        ApplicationStartEvent() = default;
    };

    struct ApplicationShutdownEvent : public Event {
        int32 exitCode = 0;

        ApplicationShutdownEvent() = default;
        explicit ApplicationShutdownEvent(int32 code)
            : exitCode(code) {}
    };

    struct WindowResizeEvent : public Event {
        uint32 width;
        uint32 height;
        uint32 oldWidth;
        uint32 oldHeight;

        WindowResizeEvent() = default;
        WindowResizeEvent(uint32 w, uint32 h, uint32 ow, uint32 oh)
            : width(w), height(h), oldWidth(ow), oldHeight(oh) {}
    };

    struct WindowCloseEvent : public Event {};

    struct KeyPressedEvent : public Event {
        uint16 keyCode;
        bool isRepeat;

        KeyPressedEvent() = default;
        KeyPressedEvent(uint16 key, bool repeat) : keyCode(key), isRepeat(repeat) {}
    };

    struct KeyReleasedEvent : public Event {
        uint16 keyCode;

        KeyReleasedEvent() = default;
        explicit KeyReleasedEvent(uint16 key) : keyCode(key) {}
    };

    struct MouseButtonPressedEvent : public Event {
        uint8 button;
        float x, y;

        MouseButtonPressedEvent() = default;
        MouseButtonPressedEvent(uint8 btn, float px, float py) : button(btn), x(px), y(py) {}
    };

    struct MouseButtonReleasedEvent : public Event {
        uint8 button;
        float x, y;

        MouseButtonReleasedEvent() = default;
        MouseButtonReleasedEvent(uint8 btn, float px, float py) : button(btn), x(px), y(py) {}
    };

    struct MouseMovedEvent : public Event {
        float x, y;
        float deltaX, deltaY;

        MouseMovedEvent() = default;
        MouseMovedEvent(float px, float py, float dx, float dy) : x(px), y(py), deltaX(dx), deltaY(dy) {}
    };

    struct MouseScrolledEvent : public Event {
        float deltaX, deltaY;

        MouseScrolledEvent() = default;
        MouseScrolledEvent(float dx, float dy) : deltaX(dx), deltaY(dy) {}
    };

    struct FrameBeginEvent : public Event {
        float deltaTime;
        uint64 frameNumber;

        FrameBeginEvent() = default;
        FrameBeginEvent(float dt, uint64 frame) : deltaTime(dt), frameNumber(frame) {}
    };

    struct FrameEndEvent : public Event {
        float frameTime;
        uint64 frameNumber;

        FrameEndEvent() = default;
        FrameEndEvent(float ft, uint64 frame) : frameTime(ft), frameNumber(frame) {}
    };

    struct RenderDeviceCreatedEvent : public Event {
        void* device;

        RenderDeviceCreatedEvent() = default;
        explicit RenderDeviceCreatedEvent(void* dev) : device(dev) {}
    };

    struct RenderDeviceDestroyedEvent : public Event {};

    struct ResourceLoadedEvent : public Event {
        std::string resourcePath;
        void* resource;

        ResourceLoadedEvent() = default;
        ResourceLoadedEvent(const std::string& path, void* res) : resourcePath(path), resource(res) {}
    };

    struct ResourceUnloadedEvent : public Event {
        std::string resourcePath;

        ResourceUnloadedEvent() = default;
        explicit ResourceUnloadedEvent(const std::string& path) : resourcePath(path) {}
    };

    // ========================================
    // Convenience Macros
    // ========================================

    #define SUBSCRIBE_EVENT(eventType, handler) \
        Engine::Core::EventBus::getInstance().subscribe<eventType>(handler)

    #define SUBSCRIBE_METHOD(eventType, instance, method) \
        Engine::Core::EventBus::getInstance().subscribe<eventType>(instance, method)

    #define UNSUBSCRIBE_EVENT(eventType, handlerId) \
        Engine::Core::EventBus::getInstance().unsubscribe<eventType>(handlerId)

    #define PUBLISH_EVENT(event) \
        Engine::Core::EventBus::getInstance().publish(event)

} // namespace Core
} // namespace Engine

