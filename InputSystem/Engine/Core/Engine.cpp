// Engine/Core/Engine.cpp
// Engine implementation
#include "Engine.hpp"
#include "Base/Types.hpp"

namespace Engine {
namespace Core {

    bool Engine::initialize(const EngineConfig& config) {
        if (initialized_) {
            return true;
        }

        config_ = config;

        // Initialize core systems in order
        if (!initializeLogging()) {
            return false;
        }

        LOG_CORE_INFO("=== Engine Initialization Started ===");
        LOG_CORE_INFO("Application: %s", config_.applicationName.c_str());


        if (!initializeMemory()) {
            LOG_CORE_ERROR("Failed to initialize memory management");
            return false;
        }

        // Initialize time system
        Time::getInstance().initialize();

        // Publish initialization event
        // Use direct access instead of macro from within the class
        EventBus::getInstance().publish(ApplicationStartEvent{config_.applicationName});

        initialized_ = true;
        LOG_CORE_INFO("=== Engine Initialization Complete ===");

        return true;
    }

    void Engine::shutdown() {
        if (!initialized_) {
            return;
        }

        LOG_CORE_INFO("=== Engine Shutdown Started ===");

        // Publish shutdown event
        // Use direct access instead of macro from within the class
        EventBus::getInstance().publish(ApplicationShutdownEvent{0});

        // Cleanup subsystems in reverse order
        Memory::MemoryManager::getInstance().cleanup();
        EventBus::getInstance().clearAllHandlers();

        LOG_CORE_INFO("=== Engine Shutdown Complete ===");

        initialized_ = false;
    }

    bool Engine::initializeLogging() {
        auto& logger = Logger::getInstance();

        // Set log level
        logger.setLevel(config_.logLevel);

        // Add file sink if requested
        if (config_.enableFileLogging) {
            try {
                logger.addSink(std::make_unique<FileSink>(config_.logFileName));
            } catch (const std::exception& e) {
                // Can't log yet, so use printf
                printf("Warning: Failed to create log file '%s': %s\n",
                       config_.logFileName.c_str(), e.what());
            }
        }

        return true;
    }

    bool Engine::initializeMemory() {
        // Use direct access instead of macro from within the class
        auto& memMgr = Memory::MemoryManager::getInstance();

        // Create standard memory pools
        memMgr.createPool("General", 64, config_.generalPoolSize / 64);
        memMgr.createPool("Rendering", 256, config_.renderPoolSize / 256);

        // Create frame stack allocator
        memMgr.createStackAllocator("Frame", config_.frameStackSize);

        LOG_CORE_INFO("Memory management initialized");
        return true;
    }


} // namespace Core
} // namespace Engine
