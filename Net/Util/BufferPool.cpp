#include "BufferPool.h"
#include <new>

namespace Util {

    // 순환 의존 회피
    BufferHandle::~BufferHandle() { reset(); }

    void BufferHandle::reset() noexcept {
        if (!owner_ || !data_) return;

        BufferPool* pool = owner_;
        std::byte* p = data_;
        owner_ = nullptr; data_ = nullptr; cap_ = 0; used_ = 0;

        pool->Release(p); // 안전 반납
    }



    BufferPool::BufferPool(std::size_t blockSize, std::size_t initialCount)
        : blockSize_(blockSize) {
        std::lock_guard lk(mtx_);

        free_.reserve(initialCount);

        for (std::size_t i = 0; i < initialCount; ++i) {
            auto* raw = static_cast<std::byte*>(::operator new(blockSize_));
            free_.push_back(raw);

            ++stats_.totalAlloc;
        }
    }

    BufferPool::~BufferPool() {
        std::lock_guard lk(mtx_);

        for (auto* p : free_) ::operator delete(p);
        free_.clear();
    }

    BufferHandle BufferPool::Acquire() {
        auto* p = Allocate();

        return BufferHandle(this, p, blockSize_);
    }

    std::byte* BufferPool::Allocate() {
        std::lock_guard lk(mtx_);

        ++stats_.totalAcquire;

        if (free_.empty()) {
            ++stats_.totalAlloc;
            return static_cast<std::byte*>(::operator new(blockSize_));
        }

        auto* p = free_.back();
        free_.pop_back();
        return p;
    }

    void BufferPool::Release(std::byte* p) {
        if (!p) return; // 가드

        std::lock_guard lk(mtx_);

        free_.push_back(p);
        ++stats_.totalRelease;
    }

}
