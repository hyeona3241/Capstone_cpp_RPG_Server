#include "BinaryWriter.h"

namespace Proto {

    void BinaryWriter::BeginPacket(uint16_t id) {
        // 이미 빌드 중이면 중첩 금지
        if (hdrPos_ != npos) {
            // 개발 중 실수 방지용
            throw std::logic_error("BeginPacket called while a packet is already open");
        }

        // 헤더 자리 확보(4B)
        hdrPos_ = buf_.size();
        buf_.resize(hdrPos_ + kHeaderSize);

        // id 기록
        PacketHeader tmp{};
        tmp.size = 0;   // 아직 payload 길이를 모름
        tmp.id = id;

        // 헤더 영역에 쓰기 [size(2)][id(2)]
        std::memcpy(buf_.data() + hdrPos_, &tmp, sizeof(tmp));
    }

    void BinaryWriter::EndPacket() {
        // BeginPacket 없이 EndPacket 먼저 사용
        if (hdrPos_ == npos) {
            throw std::logic_error("EndPacket called without BeginPacket");
        }

        // 총 길이 = 현재 버퍼 길이 - 헤더 시작 위치의 오프셋
        const size_t total = buf_.size() - hdrPos_;
        if (total > kMaxPacket) {
            // kMaxPacket를 넘는 패킷 크기
            throw std::out_of_range("EndPacket: packet too large");
        }
        if (total < kHeaderSize) {
            // 헤더보다 작은 패킷 크기
            throw std::out_of_range("EndPacket: invalid packet size");
        }

        uint16_t size16 = static_cast<uint16_t>(total);
        std::memcpy(buf_.data() + hdrPos_, &size16, sizeof(uint16_t));

        hdrPos_ = npos;
    }

    void BinaryWriter::WriteHeader(uint16_t id, uint16_t totalSize) {
        // 즉시 기록 (payload 길이를 이미 알고 있을 때 사용)
        if (totalSize < kHeaderSize || totalSize > kMaxPacket) {
            // 패킷 크기
            throw std::out_of_range("WriteHeader: invalid totalSize");
        }
        PacketHeader h{ totalSize, id };
        const size_t old = buf_.size();
        buf_.resize(old + kHeaderSize);
        std::memcpy(buf_.data() + old, &h, sizeof(h));
    }

    void BinaryWriter::WriteBytes(std::span<const std::byte> bytes) {
        if (bytes.empty()) return;
        const size_t old = buf_.size();
        buf_.resize(old + bytes.size());
        std::memcpy(buf_.data() + old, bytes.data(), bytes.size());
    }

    void BinaryWriter::WriteStringLp16(std::string_view s) {
        if (s.size() > 0xFFFF) {
            // 길이 초과 문자열
            throw std::out_of_range("WriteStringLp16: string too long");
        }
        WriteU16(static_cast<uint16_t>(s.size()));
        if (!s.empty()) {
            const size_t old = buf_.size();
            buf_.resize(old + s.size());
            std::memcpy(buf_.data() + old, s.data(), s.size());
        }
    }

    void BinaryWriter::WriteStringLp32(std::string_view s) {
        WriteU32(static_cast<uint32_t>(s.size()));
        if (!s.empty()) {
            const size_t old = buf_.size();
            buf_.resize(old + s.size());
            std::memcpy(buf_.data() + old, s.data(), s.size());
        }
    }

}
