// ========================================
// Engine/Core/Memory/MemoryManager.hpp
// Advanced memory management system
// ========================================

#pragma once

#include "../Base/Types.hpp"
#include "../Base/Logger.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <string>
#include <cstdlib>
#include <cstring>

namespace Engine {
namespace Core {
namespace Memory {

    // ========================================
    // Memory Statistics
    // ========================================

    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t totalDeallocated = 0;
        size_t currentUsage = 0;
        size_t peakUsage = 0;
        size_t allocationCount = 0;
        size_t deallocationCount = 0;
    };

    // ========================================
    // Memory Pool
    // ========================================

    class MemoryPool {
    public:
        MemoryPool(size_t blockSize, size_t blockCount);
        ~MemoryPool();

        void* allocate();
        void deallocate(void* ptr);

        size_t getBlockSize() const { return blockSize_; }
        size_t getTotalBlocks() const { return blockCount_; }
        size_t getUsedBlocks() const { return usedBlocks_; }
        size_t getAvailableBlocks() const { return blockCount_ - usedBlocks_; }

    private:
        size_t blockSize_;
        size_t blockCount_;
        std::atomic<size_t> usedBlocks_;

        void* memory_;
        std::vector<void*> freeBlocks_;
        std::mutex mutex_;
    };

    // ========================================
    // Stack Allocator
    // ========================================

    class StackAllocator {
    public:
        explicit StackAllocator(size_t size);
        ~StackAllocator();

        void* allocate(size_t size, size_t alignment = sizeof(void*));
        void reset();

        size_t getSize() const { return size_; }
        size_t getUsed() const { return current_ - start_; }
        size_t getRemaining() const { return size_ - getUsed(); }

        // RAII marker for automatic reset
        class Marker {
        public:
            explicit Marker(StackAllocator& allocator)
                : allocator_(allocator), position_(allocator.current_) {}

            ~Marker() { allocator_.current_ = position_; }

        private:
            StackAllocator& allocator_;
            char* position_;
        };

        Marker getMarker() { return Marker(*this); }

    private:
        size_t size_;
        char* start_;
        char* current_;
        char* end_;

        friend class Marker;  // Allow Marker to access private members
    };

    // ========================================
    // Resource Manager
    // ========================================

    template<typename T>
    class ResourceManager {
    public:
        using ResourceHandle = Handle<T>;
        using ResourcePtr = SharedPtr<T>;

        ResourceHandle create(const std::string& name = "") {
            std::lock_guard<std::mutex> lock(mutex_);

            ResourceHandle handle(nextId_++, 0);
            auto resource = MakeShared<T>();

            resources_[handle.getId()] = resource;
            if (!name.empty()) {
                nameToHandle_[name] = handle;
            }

            stats_.allocationCount++;

            return handle;
        }

        template<typename... Args>
        ResourceHandle create(const std::string& name, Args&&... args) {
            std::lock_guard<std::mutex> lock(mutex_);

            ResourceHandle handle(nextId_++, 0);
            auto resource = MakeShared<T>(std::forward<Args>(args)...);

            resources_[handle.getId()] = resource;
            if (!name.empty()) {
                nameToHandle_[name] = handle;
            }

            stats_.allocationCount++;

            return handle;
        }

        ResourcePtr get(ResourceHandle handle) const {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = resources_.find(handle.getId());
            return (it != resources_.end()) ? it->second : nullptr;
        }

        ResourcePtr get(const std::string& name) const {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = nameToHandle_.find(name);
            if (it != nameToHandle_.end()) {
                return get(it->second);
            }
            return nullptr;
        }

        bool destroy(ResourceHandle handle) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = resources_.find(handle.getId());
            if (it != resources_.end()) {
                resources_.erase(it);
                stats_.deallocationCount++;
                return true;
            }
            return false;
        }

        bool destroy(const std::string& name) {
            std::lock_guard<std::mutex> lock(mutex_);

            auto it = nameToHandle_.find(name);
            if (it != nameToHandle_.end()) {
                ResourceHandle handle = it->second;
                nameToHandle_.erase(it);
                return destroy(handle);
            }
            return false;
        }

        void clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            resources_.clear();
            nameToHandle_.clear();
            stats_.deallocationCount += stats_.allocationCount;
            stats_.allocationCount = 0;
        }

        size_t getResourceCount() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return resources_.size();
        }

        const MemoryStats& getStats() const { return stats_; }

        // Iteration support
        template<typename Func>
        void forEach(Func&& func) const {
            std::lock_guard<std::mutex> lock(mutex_);
            for (const auto& [id, resource] : resources_) {
                if (resource) {
                    func(*resource);
                }
            }
        }

    private:
        mutable std::mutex mutex_;
        std::unordered_map<uint32, ResourcePtr> resources_;
        std::unordered_map<std::string, ResourceHandle> nameToHandle_;
        std::atomic<uint32> nextId_{1};
        MemoryStats stats_;
    };

    // ========================================
    // Global Memory Manager
    // ========================================

    class MemoryManager {
    public:
        static MemoryManager& getInstance() {
            static MemoryManager instance;
            return instance;
        }

        // Pool management
        void createPool(const std::string& name, size_t blockSize, size_t blockCount);
        MemoryPool* getPool(const std::string& name);
        void destroyPool(const std::string& name);

        // Stack allocator management
        void createStackAllocator(const std::string& name, size_t size);
        StackAllocator* getStackAllocator(const std::string& name);
        void destroyStackAllocator(const std::string& name);

        // Statistics
        MemoryStats getGlobalStats() const;
        void logStats() const;

        // Cleanup
        void cleanup();

    private:
        MemoryManager() = default;
        ~MemoryManager() { cleanup(); }

        std::unordered_map<std::string, UniquePtr<MemoryPool>> pools_;
        std::unordered_map<std::string, UniquePtr<StackAllocator>> stackAllocators_;
        mutable std::mutex mutex_;
    };

    // ========================================
    // RAII Memory Helpers
    // ========================================

    template<typename T>
    class ScopedResource {
    public:
        template<typename... Args>
        ScopedResource(Args&&... args) : resource_(std::forward<Args>(args)...) {}

        ~ScopedResource() = default;

        T& get() { return resource_; }
        const T& get() const { return resource_; }

        T& operator*() { return resource_; }
        const T& operator*() const { return resource_; }

        T* operator->() { return &resource_; }
        const T* operator->() const { return &resource_; }

    private:
        T resource_;
    };

    // ========================================
    // Memory Debugging
    // ========================================

    #ifdef DEBUG
    class MemoryTracker {
    public:
        static MemoryTracker& getInstance() {
            static MemoryTracker instance;
            return instance;
        }

        void recordAllocation(void* ptr, size_t size, const char* file, int line);
        void recordDeallocation(void* ptr);
        void dumpLeaks() const;

    private:
        struct AllocationInfo {
            size_t size;
            std::string file;
            int line;
        };

        std::unordered_map<void*, AllocationInfo> allocations_;
        mutable std::mutex mutex_;
    };

    #define TRACK_ALLOCATION(ptr, size) \
        Engine::Core::Memory::MemoryTracker::getInstance().recordAllocation(ptr, size, __FILE__, __LINE__)

    #define TRACK_DEALLOCATION(ptr) \
        Engine::Core::Memory::MemoryTracker::getInstance().recordDeallocation(ptr)
    #else
    #define TRACK_ALLOCATION(ptr, size) ((void)0)
    #define TRACK_DEALLOCATION(ptr) ((void)0)
    #endif

    // ========================================
    // Custom Aligned Deleter for unique_ptr
    // ========================================

    template<typename T>
    struct AlignedDeleter {
        void operator()(T* ptr) const {
            if (ptr) {
                ptr->~T();  // Call destructor
                std::free(ptr);  // Free the aligned memory
            }
        }
    };

    // ========================================
    // Utility Functions
    // ========================================

    template<typename T>
    std::unique_ptr<T, AlignedDeleter<T>> MakeUniqueAligned(size_t alignment) {
        void* ptr = std::aligned_alloc(alignment, sizeof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }
        T* obj = new(ptr) T();  // Placement new
        return std::unique_ptr<T, AlignedDeleter<T>>(obj);
    }

    template<typename T, typename... Args>
    std::unique_ptr<T, AlignedDeleter<T>> MakeUniqueAligned(size_t alignment, Args&&... args) {
        void* ptr = std::aligned_alloc(alignment, sizeof(T));
        if (!ptr) {
            throw std::bad_alloc();
        }
        T* obj = new(ptr) T(std::forward<Args>(args)...);  // Placement new with arguments
        return std::unique_ptr<T, AlignedDeleter<T>>(obj);
    }

} // namespace Memory
} // namespace Core
} // namespace Engine

