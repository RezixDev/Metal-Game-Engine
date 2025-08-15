// ========================================
// Engine/Core/Base/Logger.hpp
// Enhanced logging system
// ========================================

#pragma once

#include "Types.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <vector>
#include <chrono>

namespace Engine {
namespace Core {

    enum class LogLevel : uint8 {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Fatal = 5
    };

    enum class LogCategory : uint8 {
        General = 0,
        Core = 1,
        Rendering = 2,
        Input = 3,
        Audio = 4,
        Network = 5,
        Game = 6,
        Count
    };

    class LogSink {
    public:
        virtual ~LogSink() = default;
        virtual void write(LogLevel level, LogCategory category,
                          const std::string& timestamp, const std::string& message) = 0;
    };

    class ConsoleSink : public LogSink {
    public:
        void write(LogLevel level, LogCategory category,
                  const std::string& timestamp, const std::string& message) override;
    };

    class FileSink : public LogSink {
    public:
        FileSink(const std::string& filename);
        ~FileSink();
        void write(LogLevel level, LogCategory category,
                  const std::string& timestamp, const std::string& message) override;

    private:
        std::ofstream file_;
        std::mutex mutex_;
    };

    class Logger {
    public:
        static Logger& getInstance() {
            static Logger instance;
            return instance;
        }

        // Configuration
        void setLevel(LogLevel level) { minLevel_ = level; }
        void setCategoryEnabled(LogCategory category, bool enabled);
        void addSink(std::unique_ptr<LogSink> sink);
        void clearSinks();

        // Logging
        template<typename... Args>
        void log(LogLevel level, LogCategory category, const char* format, Args&&... args) {
            if (level < minLevel_ || !isCategoryEnabled(category)) {
                return;
            }

            std::string message = formatString(format, std::forward<Args>(args)...);
            std::string timestamp = getCurrentTimestamp();

            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& sink : sinks_) {
                sink->write(level, category, timestamp, message);
            }
        }

        // Convenience methods
        template<typename... Args>
        void trace(LogCategory category, const char* format, Args&&... args) {
            log(LogLevel::Trace, category, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void debug(LogCategory category, const char* format, Args&&... args) {
            log(LogLevel::Debug, category, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void info(LogCategory category, const char* format, Args&&... args) {
            log(LogLevel::Info, category, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void warning(LogCategory category, const char* format, Args&&... args) {
            log(LogLevel::Warning, category, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void error(LogCategory category, const char* format, Args&&... args) {
            log(LogLevel::Error, category, format, std::forward<Args>(args)...);
        }

        template<typename... Args>
        void fatal(LogCategory category, const char* format, Args&&... args) {
            log(LogLevel::Fatal, category, format, std::forward<Args>(args)...);
        }

    private:
        Logger();

        template<typename... Args>
        std::string formatString(const char* format, Args&&... args) {
            size_t size = std::snprintf(nullptr, 0, format, args...) + 1;
            std::vector<char> buffer(size);
            std::snprintf(buffer.data(), size, format, args...);
            return std::string(buffer.data(), size - 1);
        }

        std::string getCurrentTimestamp();
        bool isCategoryEnabled(LogCategory category) const;

        LogLevel minLevel_;
        std::array<bool, static_cast<size_t>(LogCategory::Count)> categoryEnabled_;
        std::vector<std::unique_ptr<LogSink>> sinks_;
        std::mutex mutex_;
    };

    // Convenience macros
    #define LOG_TRACE(category, ...) ::Engine::Core::Logger::getInstance().trace(category, __VA_ARGS__)
    #define LOG_DEBUG(category, ...) ::Engine::Core::Logger::getInstance().debug(category, __VA_ARGS__)
    #define LOG_INFO(category, ...) ::Engine::Core::Logger::getInstance().info(category, __VA_ARGS__)
    #define LOG_WARNING(category, ...) ::Engine::Core::Logger::getInstance().warning(category, __VA_ARGS__)
    #define LOG_ERROR(category, ...) ::Engine::Core::Logger::getInstance().error(category, __VA_ARGS__)
    #define LOG_FATAL(category, ...) ::Engine::Core::Logger::getInstance().fatal(category, __VA_ARGS__)

    // Short-hand macros for common categories - use :: for global namespace
    #define LOG_CORE_INFO(...) LOG_INFO(::Engine::Core::LogCategory::Core, __VA_ARGS__)
    #define LOG_CORE_WARNING(...) LOG_WARNING(::Engine::Core::LogCategory::Core, __VA_ARGS__)
    #define LOG_CORE_ERROR(...) LOG_ERROR(::Engine::Core::LogCategory::Core, __VA_ARGS__)
    #define LOG_RENDER_INFO(...) LOG_INFO(::Engine::Core::LogCategory::Rendering, __VA_ARGS__)
    #define LOG_RENDER_ERROR(...) LOG_ERROR(::Engine::Core::LogCategory::Rendering, __VA_ARGS__)
} // namespace Core
} // namespace Engine