// Engine/Core/Engine.hpp
// Main engine initialization and coordination
#pragma once

#include "Base/Types.hpp"
#include "Base/Logger.hpp"
#include "Events/EventSystem.hpp"
#include "Memory/MemoryManager.hpp"
#include "Time.hpp"
#include <string>

namespace Engine {
namespace Core {

    struct EngineConfig {
        // Logging
        LogLevel logLevel = LogLevel::Info;
        bool enableFileLogging = true;
        std::string logFileName = "engine.log";

        // Memory
        size_t generalPoolSize = Megabytes(16);
        size_t renderPoolSize = Megabytes(64);
        size_t frameStackSize = Megabytes(4);

        // Performance
        bool enableProfiling = false;
        bool enableMemoryTracking = false;

        // Application
        std::string applicationName = "Engine Application";
        std::string configFileName = "engine.cfg";
    };

    class Engine {
    public:
        static Engine& getInstance() {
            static Engine instance;
            return instance;
        }

        bool initialize(const EngineConfig& config = {});
        void shutdown();

        bool isInitialized() const { return initialized_; }
        const EngineConfig& getConfig() const { return config_; }

        // Subsystem access
        Logger& getLogger() { return Logger::getInstance(); }
        EventBus& getEventBus() { return EventBus::getInstance(); }
        Memory::MemoryManager& getMemoryManager() { return Memory::MemoryManager::getInstance(); }
        Time& getTime() { return Time::getInstance(); }

    private:
        Engine() = default;
        ~Engine() { shutdown(); }

        bool initializeLogging();
        bool initializeMemory();
        bool initializeConfig();

        EngineConfig config_;
        bool initialized_ = false;
    };

} // namespace Core
} // namespace Engine

// Convenience macros for global access - Fixed to use correct namespace path
#define ENGINE Engine::Core::Engine::getInstance()
#define ENGINE_LOG Engine::Core::Engine::getInstance().getLogger()
#define ENGINE_EVENTS Engine::Core::Engine::getInstance().getEventBus()
#define ENGINE_MEMORY Engine::Core::Engine::getInstance().getMemoryManager()
#define ENGINE_TIME Engine::Core::Engine::getInstance().getTime()