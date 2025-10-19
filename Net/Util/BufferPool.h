#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include "BufferHandle.h"

// ���� ũ�� ���� ����� �����ϴ� ������ �޸� Ǯ

namespace Util {

	class BufferPool
	{
    public:
        // ��Ÿ�� ��� (������)
        struct Stats {
            std::atomic<uint64_t> totalAlloc{ 0 };  // ���� Ȯ���� ��� ��
            std::atomic<uint64_t> totalAcquire{ 0 };    // Allocate ȣ�� ���� Ƚ��
            std::atomic<uint64_t> totalRelease{ 0 };    // �ݳ� ���� Ƚ��
        };

    private:
        // free_ ��ȣ
        std::mutex mtx_;
        // ��� ����(�����) ��� ������ ����
        std::vector<std::byte*> free_;
        // ��� ���� ũ��
        std::size_t blockSize_;
        // ��� ī���� ��ü
        Stats stats_;

    public:
        explicit BufferPool(std::size_t blockSize, std::size_t initialCount = 0);
        ~BufferPool();

        // ���� �ϳ� ������ BufferHandle ���·� ��ȯ (�ڵ� �ݳ�)
        BufferHandle Acquire();

        // ���� ���� �� �����ͷ� ��ȯ (���� �ݳ��������)
        std::byte* Allocate();
        // �ݳ�
        void Release(std::byte* p);

        // ��� ���� ũ�� ��ȯ
        std::size_t block_size() const noexcept { return blockSize_; }
        // ��� �б�
        const Stats& stats() const noexcept { return stats_; }
	};
}