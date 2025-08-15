// Engine/Core/Engine.cpp
#include "Engine.hpp"
#include "Base/Types.hpp"
#include <cstdio>  // Added for printf

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

        printf("=== Engine Initialization Started ===\n");
        printf("Application: %s\n", config_.applicationName.c_str());

        if (!initializeMemory()) {
            printf("ERROR: Failed to initialize memory management\n");
            return false;
        }

        // Initialize time system
        Time::getInstance().initialize();

        // Publish initialization event
        EventBus::getInstance().publish(ApplicationStartEvent{config_.applicationName});

        initialized_ = true;
        printf("=== Engine Initialization Complete ===\n");

        return true;
    }

    void Engine::shutdown() {
        if (!initialized_) {
            return;
        }

        printf("=== Engine Shutdown Started ===\n");

        // Publish shutdown event
        EventBus::getInstance().publish(ApplicationShutdownEvent{0});

        // Cleanup subsystems in reverse order
        Memory::MemoryManager::getInstance().cleanup();
        EventBus::getInstance().clearAllHandlers();

        printf("=== Engine Shutdown Complete ===\n");

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
        auto& memMgr = Memory::MemoryManager::getInstance();

        // Create standard memory pools
        memMgr.createPool("General", 64, config_.generalPoolSize / 64);
        memMgr.createPool("Rendering", 256, config_.renderPoolSize / 256);

        // Create frame stack allocator
        memMgr.createStackAllocator("Frame", config_.frameStackSize);

        printf("Memory management initialized\n");
        return true;
    }

    bool Engine::initializeConfig() {
        // Load configuration from file if it exists
        Config& config = Config::getInstance();

        if (!config_.configFileName.empty()) {
            if (config.loadFromFile(config_.configFileName)) {
                printf("Configuration loaded from: %s\n", config_.configFileName.c_str());
            } else {
                printf("Configuration file not found: %s (using defaults)\n", config_.configFileName.c_str());
            }
        }

        return true;
    }

} // namespace Core
} // namespace Engine