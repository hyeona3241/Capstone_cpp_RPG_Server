#include "UidGenerator.h"
#include <thread> 
#include <limits>

namespace Ids {

    UidGenerator::UidGenerator(uint16_t node_id) : node_id_(node_id) {
        if (node_id_ > kNodeMask) {
            throw std::invalid_argument("UidGenerator: node_id out of range (0..1023)");
        }
    }

    // 64비트 ID 생성
    uint64_t UidGenerator::Next() {
        for (;;) {
            const uint64_t now = NowMillis();

            const uint64_t base = (now > kCustomEpochMs) ? (now - kCustomEpochMs) : 0ULL;

            bool need_sleep = false;
            uint64_t uid = 0;

            {
                std::lock_guard<std::mutex> lk(m_);

                if (now < last_ts_ms_) {
                    // now가 last_ts에 도달할 때까지 1ms씩 대기
                    need_sleep = true;
                }
                else if (now == last_ts_ms_) {
                    // 같은 ms는 시퀀스 증가
                    if (seq_ == kSeqMask) {
                        need_sleep = true;

                    }
                    // 4096개 초과시 다음 ms까지 대기
                    else {
                        ++seq_;
                        const uint64_t ts_delta = now - kCustomEpochMs;
                        uid = (ts_delta << kTimeShift) | (uint64_t(node_id_) << kNodeShift) | uint64_t(seq_);
                    }
                }
                else { // now > last_ts
                    last_ts_ms_ = now;
                    // 새로운 ms 시작
                    seq_ = 0;
                    const uint64_t ts_delta = now - kCustomEpochMs;
                    uid = (ts_delta << kTimeShift) | (uint64_t(node_id_) << kNodeShift) | uint64_t(seq_);
                }
            }

            if (uid) return uid;
            SleepFor1ms();
        }
    }

    // UID를 원래 구성 요소로 분해 (디버깅 / 로그 / 운영 도구용)
    IdParts UidGenerator::Decode(uint64_t uid) {
        IdParts p{};

        p.sequence = static_cast<uint16_t>(uid & kSeqMask);
        p.node_id = static_cast<uint16_t>((uid >> kNodeShift) & kNodeMask);

        const uint64_t ts_delta = (uid >> kTimeShift);

        p.timestamp_ms = ts_delta + kCustomEpochMs;
        return p;
    }


    // 현재 UTC 기준 밀리초 정수 변환
    uint64_t UidGenerator::NowMillis() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    // 1ms 대기
    void UidGenerator::SleepFor1ms() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}