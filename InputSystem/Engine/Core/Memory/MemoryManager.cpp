// Engine/Core/Memory/MemoryManager.cpp
#include "MemoryManager.hpp"
#include <cstdlib>
#include <cstring>
#include <new>      // Added for std::bad_alloc
#include <cstdio>   // Added for printf

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

        printf("[MemoryPool] Created: %zu blocks of %zu bytes (total: %zu bytes)\n",
               blockCount, blockSize_, totalSize);
    }

    MemoryPool::~MemoryPool() {
        if (memory_) {
            std::free(memory_);
            printf("[MemoryPool] Destroyed with %zu used blocks\n", usedBlocks_.load());
        }
    }

    void* MemoryPool::allocate() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (freeBlocks_.empty()) {
            printf("[MemoryPool] ERROR: Pool exhausted! No free blocks available\n");
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

        printf("[StackAllocator] Created: %zu bytes\n", size);
    }

    StackAllocator::~StackAllocator() {
        if (start_) {
            std::free(start_);
            printf("[StackAllocator] Destroyed\n");
        }
    }

    void* StackAllocator::allocate(size_t size, size_t alignment) {
        // Ensure we have a valid alignment
        if (alignment == 0) {
            alignment = alignof(std::max_align_t);
        }

        // Align current pointer
        uintptr_t currentAddr = reinterpret_cast<uintptr_t>(current_);
        uintptr_t alignedAddr = AlignUp(currentAddr, alignment);
        char* aligned = reinterpret_cast<char*>(alignedAddr);

        // Check if we have enough space
        if (aligned + size > end_) {
            printf("[StackAllocator] ERROR: Out of memory! Requested: %zu, Available: %ld\n",
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
            printf("[MemoryManager] WARNING: Pool '%s' already exists\n", name.c_str());
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
            printf("[MemoryManager] WARNING: Stack allocator '%s' already exists\n", name.c_str());
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

        printf("=== Memory Manager Statistics ===\n");

        for (const auto& [name, pool] : pools_) {
            printf("Pool '%s': %zu/%zu blocks used (%zu bytes)\n",
                   name.c_str(), pool->getUsedBlocks(), pool->getTotalBlocks(),
                   pool->getUsedBlocks() * pool->getBlockSize());
        }

        for (const auto& [name, allocator] : stackAllocators_) {
            printf("Stack '%s': %zu/%zu bytes used\n",
                   name.c_str(), allocator->getUsed(), allocator->getSize());
        }
    }

    void MemoryManager::cleanup() {
        std::lock_guard<std::mutex> lock(mutex_);
        pools_.clear();
        stackAllocators_.clear();
        printf("[MemoryManager] Cleaned up\n");
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
            printf("[MemoryTracker] No memory leaks detected\n");
            return;
        }

        printf("[MemoryTracker] Memory leaks detected: %zu allocations\n", allocations_.size());
        for (const auto& [ptr, info] : allocations_) {
            printf("  Leak: %zu bytes at %p (%s:%d)\n",
                   info.size, ptr, info.file.c_str(), info.line);
        }
    }
    #endif

} // namespace Memory
} // namespace Core
} // namespace Engine