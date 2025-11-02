#pragma once

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

#include <cstddef>
#include <span>
#include <vector>
#include <algorithm>
#include <cassert>


namespace Util {

	class RingBuffer
	{
    private:
        // 실제 저장 공간
        std::vector<std::byte> buf_;
        // 전체 용량
        std::size_t cap_ = 0;
        // 읽기 위치
        std::size_t head_ = 0;
        // 쓰기 위치
        std::size_t tail_ = 0;
        // 보유 중 바이트
        std::size_t size_ = 0;

    public:
        explicit RingBuffer(std::size_t capacity)
            : buf_(capacity), cap_(capacity) {
        }

        std::size_t capacity() const noexcept { return cap_; }
        std::size_t size() const noexcept { return size_; }
        std::size_t free_space() const noexcept { return cap_ - size_; }
        bool empty() const noexcept { return size_ == 0; }
        void clear() noexcept { head_ = tail_ = size_ = 0; }

        // src에서 가능한 만큼(최대 free_space()) 링버퍼에 쓰기
        std::size_t write(std::span<const std::byte> src) {
            const std::size_t n = std::min(src.size(), free_space());
            const std::size_t first = std::min(n, cap_ - tail_);

            std::copy_n(src.data(), first, buf_.data() + tail_);
            std::copy_n(src.data() + first, n - first, buf_.data());

            tail_ = (tail_ + n) % cap_;
            size_ += n;

            return n;
        }

        // 링버퍼에서 out으로 가능한 만큼(최대 size()) 읽기
        std::size_t read(std::span<std::byte> out) {
            const std::size_t n = std::min(out.size(), size_);
            const std::size_t first = std::min(n, cap_ - head_);

            std::copy_n(buf_.data() + head_, first, out.data());
            std::copy_n(buf_.data(), n - first, out.data() + first);

            head_ = (head_ + n) % cap_;
            size_ -= n;

            return n;
        }

        // head에서 끝까지 연속된 데이터 구간을 보여줌 (하나에 다 있냐 / 끝에서 나눠져서 이어지냐)
        std::span<const std::byte> first_segment() const noexcept {
            const std::size_t n = std::min(size_, cap_ - head_);

            return { buf_.data() + head_, n };
        }
        std::span<const std::byte> second_segment() const noexcept {
            const std::size_t n1 = std::min(size_, cap_ - head_);
            const std::size_t n2 = size_ - n1;

            return { buf_.data(), n2 };
        }

        // 앞부분 n바이트 버리고 head를 앞으로 이동
        void consume(std::size_t n) noexcept {
            n = std::min(n, size_);
            head_ = (head_ + n) % cap_;
            size_ -= n;
        }

	};
}