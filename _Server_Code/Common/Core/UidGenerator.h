#pragma once

#include <cstdint>
#include <atomic>
#include <mutex>
#include <chrono>
#include <stdexcept>

namespace Ids {

    // 중복 없는 전역 고유 ID를 발급
    struct IdParts {
        uint64_t timestamp_ms;  // 시간
        uint16_t node_id;   // 노드 id
        uint16_t sequence;  // 1ms 안에서 몇 번째로 발급된 UID인지
    };

    class UidGenerator
    {
    public:
        // 커스텀 기준시각 UTC (2025-01-01 00:00:00)
        static constexpr uint64_t kCustomEpochMs = 1735689600000ULL;

        // 시간 필드 비트수 (69.7년)
        static constexpr int kTimestampBits = 41;
        // 노드ID 비트수 (1024개)
        static constexpr int kNodeBits = 10;
        // 시퀀스 비트수 (ms당 4096개)
        static constexpr int kSeqBits = 12;

        // 시퀀스 범위 마스킹 (0xFFF) 오버플로 방지
        static constexpr uint64_t kSeqMask = (1ULL << kSeqBits) - 1ULL;
        // 노드ID 범위 마스킹 (0x3FF)
        static constexpr uint64_t kNodeMask = (1ULL << kNodeBits) - 1ULL;

        // 조합 시 쉬프트 폭 (필드들이 서로 겹치지 않게)
        static constexpr int kNodeShift = kSeqBits;
        static constexpr int kTimeShift = kSeqBits + kNodeBits;

    private:
        // 노드 아이디
        uint16_t node_id_;
        // 갱신 구간 보호 뮤텍스
        std::mutex m_;
        // 마지막으로 사용한 시간
        uint64_t last_ts_ms_{ 0 };
        // 마지막 ms에서 사용한 시퀀스
        uint16_t seq_{ 0 };

    public:
        // node_id 식별값 보내서 생성기 개체 초기화
        explicit UidGenerator(uint16_t node_id);

        // 64비트 UID 발급
        uint64_t Next();

        // UID를 구성요소로 분해(시간 / 노드id / 시퀀스)
        static IdParts Decode(uint64_t uid);

        // 현재 UTC 시각을 밀리초 단위 정수로 반환
        static uint64_t NowMillis();

    private:
        // 1ms 시간 대기
        static void SleepFor1ms();
    };

}