#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <span>

class RingBuffer
{
public:
    struct Region
    {
        const std::byte* ptr{ nullptr };
        std::size_t len{ 0 };
    };

    explicit RingBuffer(std::size_t capacity)
        : buf_(capacity), cap_(capacity)
    {
    }

    std::size_t capacity() const noexcept { return cap_; }
    std::size_t size() const noexcept { return size_; }
    std::size_t free_space() const noexcept { return cap_ - size_; }
    bool empty() const noexcept { return size_ == 0; }

    void clear() noexcept
    {
        head_ = 0;
        tail_ = 0;
        size_ = 0;
    }

    // n 바이트 읽기 / 쓰기
    bool can_read(std::size_t n) const noexcept
    {
        return size_ >= n;
    }

    bool can_write(std::size_t n) const noexcept
    {
        return free_space() >= n;
    }

    std::size_t write(const std::byte* data, std::size_t len)
    {
        if (!data || len == 0)
            return 0;

        const std::size_t n = std::min(len, free_space());
        const std::size_t first = std::min(n, cap_ - tail_);
        const std::size_t second = n - first;

        if (first > 0)
            std::memcpy(buf_.data() + tail_, data, first);
        if (second > 0)
            std::memcpy(buf_.data(), data + first, second);

        tail_ = (tail_ + n) % cap_;
        size_ += n;

        return n;
    }

    std::size_t read(std::byte* out, std::size_t len)
    {
        if (!out || len == 0 || size_ == 0)
            return 0;

        const std::size_t n = std::min(len, size_);
        const std::size_t first = std::min(n, cap_ - head_);
        const std::size_t second = n - first;

        if (first > 0)
            std::memcpy(out, buf_.data() + head_, first);
        if (second > 0)
            std::memcpy(out + first, buf_.data(), second);

        head_ = (head_ + n) % cap_;
        size_ -= n;

        if (size_ == 0)
        {
            head_ = 0;
            tail_ = 0;
        }

        return n;
    }

    // 복사해서 확인만
    std::size_t peek(std::byte* out, std::size_t len) const
    {
        if (!out || len == 0 || size_ == 0)
            return 0;

        const std::size_t n = std::min(len, size_);
        const std::size_t first = std::min(n, cap_ - head_);
        const std::size_t second = n - first;

        if (first > 0)
            std::memcpy(out, buf_.data() + head_, first);
        if (second > 0)
            std::memcpy(out + first, buf_.data(), second);

        return n;
    }

    // 읽어야 된 구간이 쪼개진 경우
    Region first_segment() const noexcept
    {
        Region r;
        if (size_ == 0)
            return r;

        r.len = std::min(size_, cap_ - head_);
        r.ptr = reinterpret_cast<const std::byte*>(buf_.data() + head_);
        return r;
    }

    Region second_segment() const noexcept
    {
        Region r;
        if (size_ == 0)
            return r;

        const std::size_t n1 = std::min(size_, cap_ - head_);
        const std::size_t n2 = size_ - n1;

        r.ptr = reinterpret_cast<const std::byte*>(buf_.data());
        r.len = n2;
        return r;
    }

    // 버퍼에서 데이터 제거
    void consume(std::size_t n) noexcept
    {
        if (n == 0 || size_ == 0)
            return;

        n = std::min(n, size_);
        head_ = (head_ + n) % cap_;
        size_ -= n;

        if (size_ == 0)
        {
            head_ = 0;
            tail_ = 0;
        }
    }

private:
    std::vector<char> buf_;
    std::size_t cap_{ 0 };

    std::size_t head_{ 0 };
    std::size_t tail_{ 0 };
    std::size_t size_{ 0 };

};

