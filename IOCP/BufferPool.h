#pragma once

#include "Pch.h"
#include <stack>
#include <mutex>
#include "Buffer.h"

class BufferPool
{
public:
    BufferPool(std::size_t poolSize, std::size_t bufferSize)
    {
        buffers_.reserve(poolSize);

        for (std::size_t i = 0; i < poolSize; ++i)
        {
            buffers_.emplace_back(bufferSize);
            freeList_.push(i);
        }
    }

    // 버퍼 빌리기
    Buffer* Acquire()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (freeList_.empty())
        {
            // 사용 가능 버퍼 없음
            // 예외 처리 필요
            return nullptr;
        }

        std::size_t index = freeList_.top();
        freeList_.pop();

        return &buffers_[index];
    }

    // 버퍼 반납
    void Release(Buffer* buffer)
    {
        if (!buffer)
            return;

        std::lock_guard<std::mutex> lock(mutex_);

        buffer->Reset();

        std::size_t index = static_cast<std::size_t>(buffer - &buffers_[0]);

        freeList_.push(index);
    }

private:
    std::vector<Buffer> buffers_;
    std::stack<std::size_t> freeList_; // 사용 가능한 인덱스 스텍
    std::mutex mutex_;
};

