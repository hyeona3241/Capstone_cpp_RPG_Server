#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include "BufferHandle.h"

// 단일 크기 고정 블록을 재사용하는 간단한 메모리 풀

namespace Util {

	class BufferPool
	{
    public:
        // 런타임 통계 (디버깅용)
        struct Stats {
            std::atomic<uint64_t> totalAlloc{ 0 };  // 새로 확보한 블록 수
            std::atomic<uint64_t> totalAcquire{ 0 };    // Allocate 호출 누적 횟수
            std::atomic<uint64_t> totalRelease{ 0 };    // 반납 누적 횟수
        };

    private:
        // free_ 보호
        std::mutex mtx_;
        // 사용 가능(대기중) 블록 포인터 스택
        std::vector<std::byte*> free_;
        // 블록 고정 크기
        std::size_t blockSize_;
        // 통계 카운터 객체
        Stats stats_;

    public:
        explicit BufferPool(std::size_t blockSize, std::size_t initialCount = 0);
        ~BufferPool();

        // 버퍼 하나 빌려서 BufferHandle 형태로 반환 (자동 반납)
        BufferHandle Acquire();

        // 버퍼 빌릴 때 포인터로 반환 (직접 반납해줘야함)
        std::byte* Allocate();
        // 반납
        void Release(std::byte* p);

        // 블록 고정 크기 반환
        std::size_t block_size() const noexcept { return blockSize_; }
        // 통계 읽기
        const Stats& stats() const noexcept { return stats_; }
	};
}