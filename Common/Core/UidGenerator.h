#pragma once

#include <cstdint>
#include <atomic>
#include <mutex>
#include <chrono>
#include <stdexcept>

namespace Ids {

    // �ߺ� ���� ���� ���� ID�� �߱�
    struct IdParts {
        uint64_t timestamp_ms;  // �ð�
        uint16_t node_id;   // ��� id
        uint16_t sequence;  // 1ms �ȿ��� �� ��°�� �߱޵� UID����
    };

    class UidGenerator
    {
    public:
        // Ŀ���� ���ؽð� UTC (2025-01-01 00:00:00)
        static constexpr uint64_t kCustomEpochMs = 1735689600000ULL;

        // �ð� �ʵ� ��Ʈ�� (69.7��)
        static constexpr int kTimestampBits = 41;
        // ���ID ��Ʈ�� (1024��)
        static constexpr int kNodeBits = 10;
        // ������ ��Ʈ�� (ms�� 4096��)
        static constexpr int kSeqBits = 12;

        // ������ ���� ����ŷ (0xFFF) �����÷� ����
        static constexpr uint64_t kSeqMask = (1ULL << kSeqBits) - 1ULL;
        // ���ID ���� ����ŷ (0x3FF)
        static constexpr uint64_t kNodeMask = (1ULL << kNodeBits) - 1ULL;

        // ���� �� ����Ʈ �� (�ʵ���� ���� ��ġ�� �ʰ�)
        static constexpr int kNodeShift = kSeqBits;
        static constexpr int kTimeShift = kSeqBits + kNodeBits;

    private:
        // ��� ���̵�
        uint16_t node_id_;
        // ���� ���� ��ȣ ���ؽ�
        std::mutex m_;
        // ���������� ����� �ð�
        uint64_t last_ts_ms_{ 0 };
        // ������ ms���� ����� ������
        uint16_t seq_{ 0 };

    public:
        // node_id �ĺ��� ������ ������ ��ü �ʱ�ȭ
        explicit UidGenerator(uint16_t node_id);

        // 64��Ʈ UID �߱�
        uint64_t Next();

        // UID�� ������ҷ� ����(�ð� / ���id / ������)
        static IdParts Decode(uint64_t uid);

        // ���� UTC �ð��� �и��� ���� ������ ��ȯ
        static uint64_t NowMillis();

    private:
        // 1ms �ð� ���
        static void SleepFor1ms();
    };

}