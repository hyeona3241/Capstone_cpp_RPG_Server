#pragma once
#include <cstddef>
#include <vector>
#include <span>
#include <algorithm>

namespace Util {

	class RingBuffer
	{
    private:
        // ���� ���� ����
        std::vector<std::byte> buf_;
        // ��ü �뷮
        std::size_t cap_ = 0;
        // �б� ��ġ
        std::size_t head_ = 0;
        // ���� ��ġ
        std::size_t tail_ = 0;
        // ���� �� ����Ʈ
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

        // src���� ������ ��ŭ(�ִ� free_space()) �����ۿ� ����
        std::size_t write(std::span<const std::byte> src) {
            const std::size_t n = std::min(src.size(), free_space());
            const std::size_t first = std::min(n, cap_ - tail_);

            std::copy_n(src.data(), first, buf_.data() + tail_);
            std::copy_n(src.data() + first, n - first, buf_.data());

            tail_ = (tail_ + n) % cap_;
            size_ += n;

            return n;
        }

        // �����ۿ��� out���� ������ ��ŭ(�ִ� size()) �б�
        std::size_t read(std::span<std::byte> out) {
            const std::size_t n = std::min(out.size(), size_);
            const std::size_t first = std::min(n, cap_ - head_);

            std::copy_n(buf_.data() + head_, first, out.data());
            std::copy_n(buf_.data(), n - first, out.data() + first);

            head_ = (head_ + n) % cap_;
            size_ -= n;

            return n;
        }

        // head���� ������ ���ӵ� ������ ������ ������ (�ϳ��� �� �ֳ� / ������ �������� �̾�����)
        std::span<const std::byte> first_segment() const noexcept {
            const std::size_t n = std::min(size_, cap_ - head_);

            return { buf_.data() + head_, n };
        }
        std::span<const std::byte> second_segment() const noexcept {
            const std::size_t n1 = std::min(size_, cap_ - head_);
            const std::size_t n2 = size_ - n1;

            return { buf_.data(), n2 };
        }

        // �պκ� n����Ʈ ������ head�� ������ �̵�
        void consume(std::size_t n) noexcept {
            n = std::min(n, size_);
            head_ = (head_ + n) % cap_;
            size_ -= n;
        }

	};
}