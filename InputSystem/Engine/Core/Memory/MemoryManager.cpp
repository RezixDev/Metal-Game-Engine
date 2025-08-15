// ========================================
// Engine/Core/Memory/MemoryManager.cpp
// Memory manager implementation
// ========================================

#include "MemoryManager.hpp"
#include <cstdlib>
#include <cstring>
#include <new>  // Added for std::align

namespace Engine {
namespace Core {
namespace Memory {

    // MemoryPool implementation
    MemoryPool::MemoryPool(size_t blockSize, size_t blockCount)
        : blockSize_(blockSize), blockCount_(blockCount), usedBlocks_(0) {

        // Ensure minimum alignment for any type
        size_t alignment = alignof(std::max_align_t);
        blockSize_ = AlignUp(blockSize, alignment);

        size_t totalSize = blockSize_ * blockCount;

        // Use aligned allocation instead of malloc
        memory_ = std::aligned_alloc(alignment, totalSize);

        if (!memory_) {
            throw std::bad_alloc();
        }

        // Initialize free blocks
        freeBlocks_.reserve(blockCount);
        char* ptr = static_cast<char*>(memory_);
        for (size_t i = 0; i < blockCount; ++i) {
            freeBlocks_.push_back(ptr + i * blockSize_);
        }

        LOG_CORE_INFO("Created memory pool: %zu blocks of %zu bytes (total: %zu bytes)",
                      blockCount, blockSize_, totalSize);
    }

    MemoryPool::~MemoryPool() {
        if (memory_) {
            std::free(memory_);
            LOG_CORE_INFO("Destroyed memory pool with %zu used blocks", usedBlocks_.load());
        }
    }

    void* MemoryPool::allocate() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (freeBlocks_.empty()) {
            LOG_CORE_ERROR("Memory pool exhausted! No free blocks available");
            return nullptr;
        }

        void* ptr = freeBlocks_.back();
        freeBlocks_.pop_back();
        usedBlocks_++;

        return ptr;
    }

    void MemoryPool::deallocate(void* ptr) {
        if (!ptr) return;

        std::lock_guard<std::mutex> lock(mutex_);
        freeBlocks_.push_back(ptr);
        usedBlocks_--;
    }

    // StackAllocator implementation
    StackAllocator::StackAllocator(size_t size) : size_(size) {
        // Use aligned allocation for stack allocator too
        size_t alignment = alignof(std::max_align_t);
        start_ = static_cast<char*>(std::aligned_alloc(alignment, size));

        if (!start_) {
            throw std::bad_alloc();
        }

        current_ = start_;
        end_ = start_ + size;

        LOG_CORE_INFO("Created stack allocator: %zu bytes", size);
    }

    StackAllocator::~StackAllocator() {
        if (start_) {
            std::free(start_);
            LOG_CORE_INFO("Destroyed stack allocator");
        }
    }

    void* StackAllocator::allocate(size_t size, size_t alignment) {
        // Ensure we have a valid alignment
        if (alignment == 0) {
            alignment = alignof(std::max_align_t);
        }

        // Align current pointer
        char* aligned = reinterpret_cast<char*>(
            AlignUp(reinterpret_cast<uintptr_t>(current_), alignment)
        );

        // Check if we have enough space
        if (aligned + size > end_) {
            LOG_CORE_ERROR("Stack allocator out of memory! Requested: %zu, Available: %zu",
                          size, end_ - aligned);
            return nullptr;
        }

        current_ = aligned + size;
        return aligned;
    }

    void StackAllocator::reset() {
        current_ = start_;
    }

    // MemoryManager implementation
    void MemoryManager::createPool(const std::string& name, size_t blockSize, size_t blockCount) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (pools_.find(name) != pools_.end()) {
            LOG_CORE_WARNING("Memory pool '%s' already exists", name.c_str());
            return;
        }

        pools_[name] = MakeUnique<MemoryPool>(blockSize, blockCount);
    }

    MemoryPool* MemoryManager::getPool(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = pools_.find(name);
        return (it != pools_.end()) ? it->second.get() : nullptr;
    }

    void MemoryManager::destroyPool(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        pools_.erase(name);
    }

    void MemoryManager::createStackAllocator(const std::string& name, size_t size) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (stackAllocators_.find(name) != stackAllocators_.end()) {
            LOG_CORE_WARNING("Stack allocator '%s' already exists", name.c_str());
            return;
        }

        stackAllocators_[name] = MakeUnique<StackAllocator>(size);
    }

    StackAllocator* MemoryManager::getStackAllocator(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = stackAllocators_.find(name);
        return (it != stackAllocators_.end()) ? it->second.get() : nullptr;
    }

    void MemoryManager::destroyStackAllocator(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        stackAllocators_.erase(name);
    }

    MemoryStats MemoryManager::getGlobalStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        MemoryStats stats = {};

        for (const auto& [name, pool] : pools_) {
            stats.currentUsage += pool->getUsedBlocks() * pool->getBlockSize();
        }

        for (const auto& [name, allocator] : stackAllocators_) {
            stats.currentUsage += allocator->getUsed();
        }

        return stats;
    }

    void MemoryManager::logStats() const {
        std::lock_guard<std::mutex> lock(mutex_);

        LOG_CORE_INFO("=== Memory Manager Statistics ===");

        for (const auto& [name, pool] : pools_) {
            LOG_CORE_INFO("Pool '%s': %zu/%zu blocks used (%zu bytes)",
                         name.c_str(), pool->getUsedBlocks(), pool->getTotalBlocks(),
                         pool->getUsedBlocks() * pool->getBlockSize());
        }

        for (const auto& [name, allocator] : stackAllocators_) {
            LOG_CORE_INFO("Stack '%s': %zu/%zu bytes used",
                         name.c_str(), allocator->getUsed(), allocator->getSize());
        }
    }

    void MemoryManager::cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        pools_.clear();
        stackAllocators_.clear();
        LOG_CORE_INFO("Memory manager cleaned up");
    }

    // ========================================
    // Memory Debugging Implementation (if DEBUG defined)
    // ========================================

    #ifdef DEBUG
    void MemoryTracker::recordAllocation(void* ptr, size_t size, const char* file, int line) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_[ptr] = {size, file, line};
    }

    void MemoryTracker::recordDeallocation(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        allocations_.erase(ptr);
    }

    void MemoryTracker::dumpLeaks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (allocations_.empty()) {
            LOG_CORE_INFO("No memory leaks detected");
            return;
        }

        LOG_CORE_ERROR("Memory leaks detected: %zu allocations", allocations_.size());
        for (const auto& [ptr, info] : allocations_) {
            LOG_CORE_ERROR("  Leak: %zu bytes at %p (%s:%d)",
                          info.size, ptr, info.file.c_str(), info.line);
        }
    }
    #endif

} // namespace Memory
} // namespace Core
} // namespace Engine