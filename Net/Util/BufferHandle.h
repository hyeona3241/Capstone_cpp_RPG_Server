#pragma once
#include <cstddef>
#include <span>

namespace Util {

	class BufferPool;

	class BufferHandle
	{
    private:
        // 이 버퍼를 생성한 BufferPool 포인터
        BufferPool* owner_ = nullptr;
        // 버퍼 데이터 시작 주소
        std::byte* data_ = nullptr;
        // 버퍼 전체 용량
        std::size_t cap_ = 0;
        // 사용된 바이트 수
        std::size_t used_ = 0;

    public:
        BufferHandle() = default;
        ~BufferHandle();    // owner_가 존재하면 자동 반납

        BufferHandle(const BufferHandle&) = delete; // 복사 금지 (소유권 중복 방지)
        BufferHandle& operator=(const BufferHandle&) = delete;  // 복사 대입 금지

        // 다른 핸들 소유권을 이 핸들로 이전
        BufferHandle(BufferHandle&& rhs) noexcept { moveFrom(std::move(rhs)); }
        BufferHandle& operator=(BufferHandle&& rhs) noexcept {
            if (this != &rhs) { 
                reset(); moveFrom(std::move(rhs)); 
            }

            return *this;
        }

        // 버퍼가 유효한지
        explicit operator bool() const noexcept { return data_ != nullptr; }

        // 버퍼 시작 주소 반환
        std::byte* data() noexcept { return data_; }
        const std::byte* data() const noexcept { return data_; }
        // 버퍼 전체 용량 반환
        std::size_t capacity() const noexcept { return cap_; }

        // 사용 바이트 반환
        std::size_t& used() noexcept { return used_; }
        std::size_t  used() const noexcept { return used_; }

        // 현재 사용된 길이(used_) 또는 전체 용량(cap_)만큼의 쓰기 가능 범위를 span으로 반환
        std::span<std::byte> span()  noexcept { return { data_, used_ ? used_ : cap_ }; }
        std::span<const std::byte> cspan() const noexcept { return { data_, used_ ? used_ : cap_ }; }

        // 즉시 반납하고 비움
        void reset() noexcept;

    private:
        friend class BufferPool; // BufferPool만 생성/소유 변경 가능

        BufferHandle(BufferPool* owner, std::byte* data, std::size_t cap) noexcept
            : owner_(owner), data_(data), cap_(cap) {
        }

        // 이동 시 내부 포인터/소유권 이전하는 헬퍼 함수
        void moveFrom(BufferHandle&& rhs) noexcept {
            owner_ = rhs.owner_;
            data_ = rhs.data_;
            cap_ = rhs.cap_; 
            used_ = rhs.used_;

            // 원본 핸들 초기화
            rhs.owner_ = nullptr; 
            rhs.data_ = nullptr; 
            rhs.cap_ = 0; 
            rhs.used_ = 0;
        }

	};

}