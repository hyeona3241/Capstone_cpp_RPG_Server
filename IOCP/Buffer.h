#pragma once

#include "Pch.h"
#include <memory>

class Buffer
{
public:
    explicit Buffer(std::size_t capacity)
        : capacity_(capacity), size_(0), data_(std::make_unique<char[]>(capacity))
    {
    }

    // 시작 주소 반환
    char* Data()
    {
        return data_.get();
    }

    const char* Data() const
    {
        return data_.get();
    }

    std::size_t Size() const
    {
        return size_;
    }

    std::size_t Capacity() const
    {
        return capacity_;
    }

    // 사용 가능한 영역의 시작 주소
    char* WritePtr()
    {
        return data_.get() + size_;
    }

    // 사용 가능한 남은 공간 크기
    std::size_t WritableSize() const
    {
        return capacity_ - size_;
    }

    // 비우고(Send) 들어올 때(Recv) 사용 가능 크기 반영
    void AdvanceWrite(std::size_t len)
    {
        size_ += len;
        if (size_ > capacity_)
        {
            size_ = capacity_;
        }
    }

    void Reset() 
    {
        size_ = 0;
    }

private:
    std::size_t capacity_; // 전체 크기
    std::size_t size_; // 사용 가능 데이터 크기
    std::unique_ptr<char[]> data_; // 실제 메모리 블록
};

