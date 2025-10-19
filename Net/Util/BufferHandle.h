#pragma once
#include <cstddef>
#include <span>

namespace Util {

	class BufferPool;

	class BufferHandle
	{
    private:
        // �� ���۸� ������ BufferPool ������
        BufferPool* owner_ = nullptr;
        // ���� ������ ���� �ּ�
        std::byte* data_ = nullptr;
        // ���� ��ü �뷮
        std::size_t cap_ = 0;
        // ���� ����Ʈ ��
        std::size_t used_ = 0;

    public:
        BufferHandle() = default;
        ~BufferHandle();    // owner_�� �����ϸ� �ڵ� �ݳ�

        BufferHandle(const BufferHandle&) = delete; // ���� ���� (������ �ߺ� ����)
        BufferHandle& operator=(const BufferHandle&) = delete;  // ���� ���� ����

        // �ٸ� �ڵ� �������� �� �ڵ�� ����
        BufferHandle(BufferHandle&& rhs) noexcept { moveFrom(std::move(rhs)); }
        BufferHandle& operator=(BufferHandle&& rhs) noexcept {
            if (this != &rhs) { 
                reset(); moveFrom(std::move(rhs)); 
            }

            return *this;
        }

        // ���۰� ��ȿ����
        explicit operator bool() const noexcept { return data_ != nullptr; }

        // ���� ���� �ּ� ��ȯ
        std::byte* data() noexcept { return data_; }
        const std::byte* data() const noexcept { return data_; }
        // ���� ��ü �뷮 ��ȯ
        std::size_t capacity() const noexcept { return cap_; }

        // ��� ����Ʈ ��ȯ
        std::size_t& used() noexcept { return used_; }
        std::size_t  used() const noexcept { return used_; }

        // ���� ���� ����(used_) �Ǵ� ��ü �뷮(cap_)��ŭ�� ���� ���� ������ span���� ��ȯ
        std::span<std::byte> span()  noexcept { return { data_, used_ ? used_ : cap_ }; }
        std::span<const std::byte> cspan() const noexcept { return { data_, used_ ? used_ : cap_ }; }

        // ��� �ݳ��ϰ� ���
        void reset() noexcept;

    private:
        friend class BufferPool; // BufferPool�� ����/���� ���� ����

        BufferHandle(BufferPool* owner, std::byte* data, std::size_t cap) noexcept
            : owner_(owner), data_(data), cap_(cap) {
        }

        // �̵� �� ���� ������/������ �����ϴ� ���� �Լ�
        void moveFrom(BufferHandle&& rhs) noexcept {
            owner_ = rhs.owner_;
            data_ = rhs.data_;
            cap_ = rhs.cap_; 
            used_ = rhs.used_;

            // ���� �ڵ� �ʱ�ȭ
            rhs.owner_ = nullptr; 
            rhs.data_ = nullptr; 
            rhs.cap_ = 0; 
            rhs.used_ = 0;
        }

	};

}