// ========================================
// Engine/Core/Base/Logger.cpp
// Logger implementation
// ========================================

#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <chrono>

namespace Engine {
namespace Core {

    // ConsoleSink implementation
    void ConsoleSink::write(LogLevel level, LogCategory category,
                           const std::string& timestamp, const std::string& message) {

        const char* levelStr = "UNKNOWN";
        const char* categoryStr = "GENERAL";
        const char* colorCode = "\033[0m"; // Reset

        switch (level) {
            case LogLevel::Trace:   levelStr = "TRACE"; colorCode = "\033[37m"; break; // White
            case LogLevel::Debug:   levelStr = "DEBUG"; colorCode = "\033[36m"; break; // Cyan
            case LogLevel::Info:    levelStr = "INFO";  colorCode = "\033[32m"; break; // Green
            case LogLevel::Warning: levelStr = "WARN";  colorCode = "\033[33m"; break; // Yellow
            case LogLevel::Error:   levelStr = "ERROR"; colorCode = "\033[31m"; break; // Red
            case LogLevel::Fatal:   levelStr = "FATAL"; colorCode = "\033[35m"; break; // Magenta
        }

        switch (category) {
            case LogCategory::General:  categoryStr = "GENERAL"; break;
            case LogCategory::Core:     categoryStr = "CORE"; break;
            case LogCategory::Rendering: categoryStr = "RENDER"; break;
            case LogCategory::Input:    categoryStr = "INPUT"; break;
            case LogCategory::Audio:    categoryStr = "AUDIO"; break;
            case LogCategory::Network:  categoryStr = "NETWORK"; break;
            case LogCategory::Game:     categoryStr = "GAME"; break;
            case LogCategory::Count:    categoryStr = "UNKNOWN"; break; // Added missing case
        }

        std::cout << colorCode << "[" << timestamp << "] [" << levelStr << "] [" << categoryStr << "] "
                  << message << "\033[0m" << std::endl;
    }

    // FileSink implementation
    FileSink::FileSink(const std::string& filename) : file_(filename, std::ios::app) {
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filename);
        }
    }

    FileSink::~FileSink() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    void FileSink::write(LogLevel level, LogCategory category,
                        const std::string& timestamp, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);

        const char* levelStr = "UNKNOWN";
        const char* categoryStr = "GENERAL";

        switch (level) {
            case LogLevel::Trace:   levelStr = "TRACE"; break;
            case LogLevel::Debug:   levelStr = "DEBUG"; break;
            case LogLevel::Info:    levelStr = "INFO"; break;
            case LogLevel::Warning: levelStr = "WARN"; break;
            case LogLevel::Error:   levelStr = "ERROR"; break;
            case LogLevel::Fatal:   levelStr = "FATAL"; break;
        }

        switch (category) {
            case LogCategory::General:  categoryStr = "GENERAL"; break;
            case LogCategory::Core:     categoryStr = "CORE"; break;
            case LogCategory::Rendering: categoryStr = "RENDER"; break;
            case LogCategory::Input:    categoryStr = "INPUT"; break;
            case LogCategory::Audio:    categoryStr = "AUDIO"; break;
            case LogCategory::Network:  categoryStr = "NETWORK"; break;
            case LogCategory::Game:     categoryStr = "GAME"; break;
            case LogCategory::Count:    categoryStr = "UNKNOWN"; break; // Added missing case
        }

        file_ << "[" << timestamp << "] [" << levelStr << "] [" << categoryStr << "] "
              << message << std::endl;
        file_.flush();
    }

    // Logger implementation
    Logger::Logger() : minLevel_(LogLevel::Info) {
        // Enable all categories by default
        categoryEnabled_.fill(true);

        // Add default console sink
        addSink(std::make_unique<ConsoleSink>());
    }

    void Logger::setCategoryEnabled(LogCategory category, bool enabled) {
        if (category < LogCategory::Count) {  // Added bounds check
        categoryEnabled_[static_cast<size_t>(category)] = enabled;
    }
    }

    void Logger::addSink(std::unique_ptr<LogSink> sink) {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.push_back(std::move(sink));
    }

    void Logger::clearSinks() {
        std::lock_guard<std::mutex> lock(mutex_);
        sinks_.clear();
    }

    std::string Logger::getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    bool Logger::isCategoryEnabled(LogCategory category) const {
        if (category >= LogCategory::Count) {  // Added bounds check
            return false;
        }
        return categoryEnabled_[static_cast<size_t>(category)];
    }

} // namespace Core
} // namespace Engine