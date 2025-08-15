// Engine/Core/Base/Types.hpp
// Fundamental types and definitions for the engine
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <optional>
#include <variant>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <vector>      // Added - needed for std::vector
#include <array>       // Added - needed for std::array
#include <algorithm>   // Added - needed for std::remove

namespace Engine {
namespace Core {

    // ========================================
    // Fundamental Types
    // ========================================

    using uint8 = std::uint8_t;
    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;
    using uint64 = std::uint64_t;

    using int8 = std::int8_t;
    using int16 = std::int16_t;
    using int32 = std::int32_t;
    using int64 = std::int64_t;

    using float32 = float;
    using float64 = double;

    using byte = std::uint8_t;
    using size_t = std::size_t;

    // ========================================
    // Smart Pointers
    // ========================================

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;

    template<typename T, typename... Args>
    constexpr UniquePtr<T> MakeUnique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    constexpr SharedPtr<T> MakeShared(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    // ========================================
    // Result Type for Error Handling
    // ========================================

    template<typename T, typename E = std::string>
    class Result {
    public:
        // Success constructor
        Result(T&& value) : data_(std::move(value)) {}
        Result(const T& value) : data_(value) {}

        // Error constructor
        Result(E&& error) : data_(std::move(error)) {}
        Result(const E& error) : data_(error) {}

        // Copy/Move constructors
        Result(const Result&) = default;
        Result(Result&&) = default;
        Result& operator=(const Result&) = default;
        Result& operator=(Result&&) = default;

        // Check if result contains value
        bool isOk() const { return std::holds_alternative<T>(data_); }
        bool isError() const { return std::holds_alternative<E>(data_); }

        // Get value (throws if error)
        const T& getValue() const {
            if (isError()) {
                throw std::runtime_error("Attempted to get value from error result");
            }
            return std::get<T>(data_);
        }

        T& getValue() {
            if (isError()) {
                throw std::runtime_error("Attempted to get value from error result");
            }
            return std::get<T>(data_);
        }

        // Get error (throws if value)
        const E& getError() const {
            if (isOk()) {
                throw std::runtime_error("Attempted to get error from success result");
            }
            return std::get<E>(data_);
        }

        // Safe access
        const T* tryGetValue() const {
            return isOk() ? &std::get<T>(data_) : nullptr;
        }

        const E* tryGetError() const {
            return isError() ? &std::get<E>(data_) : nullptr;
        }

        // Functional interface
        template<typename F>
        auto map(F&& func) const -> Result<decltype(func(getValue())), E> {
            if (isOk()) {
                return func(getValue());
            }
            return getError();
        }

        template<typename F>
        Result<T, E> mapError(F&& func) const {
            if (isError()) {
                return func(getError());
            }
            return getValue();
        }

        // Unwrap with default
        T valueOr(const T& defaultValue) const {
            return isOk() ? getValue() : defaultValue;
        }

    private:
        std::variant<T, E> data_;
    };

    // Success/Error factory functions
    template<typename T>
    Result<T> Ok(T&& value) {
        return Result<T>(std::forward<T>(value));
    }

    template<typename E>
    auto Error(E&& error) {
        return [error = std::forward<E>(error)]<typename T>() {
            return Result<T, std::decay_t<E>>(error);
        };
    }

    // ========================================
    // Handle System for Resource Management
    // ========================================

    template<typename T>
    class Handle {
    public:
        using IdType = uint32;
        using GenerationType = uint16;

        Handle() : id_(0), generation_(0) {}
        Handle(IdType id, GenerationType generation) : id_(id), generation_(generation) {}

        bool isValid() const { return id_ != 0; }
        IdType getId() const { return id_; }
        GenerationType getGeneration() const { return generation_; }

        bool operator==(const Handle& other) const {
            return id_ == other.id_ && generation_ == other.generation_;
        }

        bool operator!=(const Handle& other) const {
            return !(*this == other);
        }

        bool operator<(const Handle& other) const {
            if (id_ != other.id_) return id_ < other.id_;
            return generation_ < other.generation_;
        }

    private:
        IdType id_;
        GenerationType generation_;
    };

    // ========================================
    // Configuration System
    // ========================================

    class Config {
    public:
        static Config& getInstance() {
            static Config instance;
            return instance;
        }

        // Set values
        void setBool(const std::string& key, bool value);
        void setInt(const std::string& key, int32 value);
        void setFloat(const std::string& key, float32 value);
        void setString(const std::string& key, const std::string& value);

        // Get values with defaults
        bool getBool(const std::string& key, bool defaultValue = false) const;
        int32 getInt(const std::string& key, int32 defaultValue = 0) const;
        float32 getFloat(const std::string& key, float32 defaultValue = 0.0f) const;
        std::string getString(const std::string& key, const std::string& defaultValue = "") const;

        // File operations
        bool loadFromFile(const std::string& filename);
        bool saveToFile(const std::string& filename) const;

        // Clear all settings
        void clear();

    private:
        Config() = default;
        std::unordered_map<std::string, std::string> values_;
    };

    // ========================================
    // Utility Macros
    // ========================================

    #define ENGINE_DISABLE_COPY(className) \
        className(const className&) = delete; \
        className& operator=(const className&) = delete;

    #define ENGINE_DISABLE_MOVE(className) \
        className(className&&) = delete; \
        className& operator=(className&&) = delete;

    #define ENGINE_DISABLE_COPY_AND_MOVE(className) \
        ENGINE_DISABLE_COPY(className) \
        ENGINE_DISABLE_MOVE(className)

    // Debug assertions
    #ifdef DEBUG
        #define ENGINE_ASSERT(condition, message) \
            do { \
                if (!(condition)) { \
                    printf("Assertion failed: %s\n", message); \
                    std::abort(); \
                } \
            } while(0)
    #else
        #define ENGINE_ASSERT(condition, message) ((void)0)
    #endif

    // Force inline
    #ifdef _MSC_VER
        #define ENGINE_FORCE_INLINE __forceinline
    #else
        #define ENGINE_FORCE_INLINE __attribute__((always_inline)) inline
    #endif

    // ========================================
    // Utility Functions
    // ========================================

    // Use size_t explicitly to avoid template instantiation issues
    constexpr size_t Kilobytes(size_t value) { return value * 1024; }
    constexpr size_t Megabytes(size_t value) { return Kilobytes(value) * 1024; }
    constexpr size_t Gigabytes(size_t value) { return Megabytes(value) * 1024; }

    // Alignment utilities - keep as templates for flexibility
    template<typename T>
    constexpr T AlignUp(T value, T alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    template<typename T>
    constexpr T AlignDown(T value, T alignment) {
        return value & ~(alignment - 1);
    }

    template<typename T>
    constexpr bool IsAligned(T value, T alignment) {
        return (value & (alignment - 1)) == 0;
    }

} // namespace Core
} // namespace Engine

