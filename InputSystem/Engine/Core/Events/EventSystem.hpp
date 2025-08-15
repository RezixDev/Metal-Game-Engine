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

            auto handlerPtr = std::make_unique<FunctionEventHandler<EventType>>(std::move(handler));
            uint32 id = nextHandlerId_++;

            auto& handlers = getHandlers<EventType>();
            handlers[id] = std::move(handlerPtr);

            return id;
        }

        // Subscribe with method
        template<typename T, typename EventType>
        uint32 subscribe(T* instance, bool (T::*method)(const EventType&)) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto handlerPtr = std::make_unique<MethodEventHandler<T, EventType>>(instance, method);
            uint32 id = nextHandlerId_++;

            auto& handlers = getHandlers<EventType>();
            handlers[id] = std::move(handlerPtr);

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
                if (handler->handle(event)) {
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
    };

    struct WindowCloseEvent : public Event {};

    struct KeyPressedEvent : public Event { uint16 keyCode; bool isRepeat; };
    struct KeyReleasedEvent : public Event { uint16 keyCode; };
    struct MouseButtonPressedEvent : public Event { uint8 button; float x, y; };
    struct MouseButtonReleasedEvent : public Event { uint8 button; float x, y; };
    struct MouseMovedEvent : public Event { float x, y; float deltaX, deltaY; };
    struct MouseScrolledEvent : public Event { float deltaX, deltaY; };

    struct FrameBeginEvent : public Event { float deltaTime; uint64 frameNumber; };
    struct FrameEndEvent : public Event { float frameTime; uint64 frameNumber; };
    struct RenderDeviceCreatedEvent : public Event { void* device; };
    struct RenderDeviceDestroyedEvent : public Event {};

    struct ResourceLoadedEvent : public Event { std::string resourcePath; void* resource; };
    struct ResourceUnloadedEvent : public Event { std::string resourcePath; };

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

