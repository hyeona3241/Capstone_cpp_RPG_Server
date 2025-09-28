#include "UidGenerator.h"
#include <thread> 
#include <limits>

namespace Ids {

    UidGenerator::UidGenerator(uint16_t node_id) : node_id_(node_id) {
        if (node_id_ > kNodeMask) {
            throw std::invalid_argument("UidGenerator: node_id out of range (0..1023)");
        }
    }

    // 64��Ʈ ID ����
    uint64_t UidGenerator::Next() {
        for (;;) {
            const uint64_t now = NowMillis();

            const uint64_t base = (now > kCustomEpochMs) ? (now - kCustomEpochMs) : 0ULL;

            bool need_sleep = false;
            uint64_t uid = 0;

            {
                std::lock_guard<std::mutex> lk(m_);

                if (now < last_ts_ms_) {
                    // now�� last_ts�� ������ ������ 1ms�� ���
                    need_sleep = true;
                }
                else if (now == last_ts_ms_) {
                    // ���� ms�� ������ ����
                    if (seq_ == kSeqMask) {
                        need_sleep = true;

                    }
                    // 4096�� �ʰ��� ���� ms���� ���
                    else {
                        ++seq_;
                        const uint64_t ts_delta = now - kCustomEpochMs;
                        uid = (ts_delta << kTimeShift) | (uint64_t(node_id_) << kNodeShift) | uint64_t(seq_);
                    }
                }
                else { // now > last_ts
                    last_ts_ms_ = now;
                    // ���ο� ms ����
                    seq_ = 0;
                    const uint64_t ts_delta = now - kCustomEpochMs;
                    uid = (ts_delta << kTimeShift) | (uint64_t(node_id_) << kNodeShift) | uint64_t(seq_);
                }
            }

            if (uid) return uid;
            SleepFor1ms();
        }
    }

    // UID�� ���� ���� ��ҷ� ���� (����� / �α� / � ������)
    IdParts UidGenerator::Decode(uint64_t uid) {
        IdParts p{};

        p.sequence = static_cast<uint16_t>(uid & kSeqMask);
        p.node_id = static_cast<uint16_t>((uid >> kNodeShift) & kNodeMask);

        const uint64_t ts_delta = (uid >> kTimeShift);

        p.timestamp_ms = ts_delta + kCustomEpochMs;
        return p;
    }


    // ���� UTC ���� �и��� ���� ��ȯ
    uint64_t UidGenerator::NowMillis() {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    // 1ms ���
    void UidGenerator::SleepFor1ms() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}