#include "PacketFramer.h"

namespace Proto {
    // 연속되어 있을 때
    PacketFramer::Result PacketFramer::TryPopFromContiguous(std::span<const std::byte> buf,
        size_t& consumed, std::vector<std::byte>& out) const noexcept 
    {
        consumed = 0;
        out.clear();

        // 헤더가 부족하면 더 필요
        if (buf.size() < kHeaderSize) {
            return Result::NeedMore;
        }

        // 헤더 읽기
        PacketHeader hdr{};
        std::memcpy(&hdr, buf.data(), kHeaderSize);

        // 크기 유효성 검사
        if (!IsValidPacketSize(hdr.size)) {
            return Result::Fatal; // 헤더 미만 또는 상한 초과
        }

        // 본문 부족
        if (buf.size() < hdr.size) {
            return Result::NeedMore;
        }

        //  패킷 1개 복사
        out.resize(hdr.size);
        std::memcpy(out.data(), buf.data(), hdr.size);

        consumed = hdr.size;
        return Result::Ok;
    }

    // 두 조각으로 나뉘었을 때
    PacketFramer::Result PacketFramer::TryPopFromTwoSegments(std::span<const std::byte> seg1, std::span<const std::byte> seg2,
        size_t& consumed, std::vector<std::byte>& out) const noexcept 
    {
        consumed = 0;
        out.clear();

        const size_t totalAvail = seg1.size() + seg2.size();

        if (totalAvail < kHeaderSize) {
            return Result::NeedMore;
        }

        // 헤더 조립/해석
        PacketHeader hdr{};
        if (!ReadHeaderFromTwo(seg1, seg2, hdr)) {
            return Result::Fatal;
        }

        if (!IsValidPacketSize(hdr.size)) {
            return Result::Fatal;
        }

        if (totalAvail < hdr.size) {
            return Result::NeedMore;
        }

        // 패킷 복사
        out.resize(hdr.size);

        // seg1에서 먼저 뽑을 양
        size_t firstTake = (hdr.size <= seg1.size()) ? hdr.size : seg1.size();

        if (firstTake)
            std::memcpy(out.data(), seg1.data(), firstTake);

        size_t remain = hdr.size - firstTake;
        if (remain) {
            std::memcpy(out.data() + firstTake, seg2.data(), remain);
        }

        consumed = hdr.size;
        return Result::Ok;
    }

    bool PacketFramer::PeekHeader(std::span<const std::byte> buf, PacketHeader& hdr) const noexcept {
        if (buf.size() < kHeaderSize) return false;
        std::memcpy(&hdr, buf.data(), kHeaderSize);

        return true;
    }

    bool PacketFramer::ReadHeaderFromTwo(std::span<const std::byte> a, std::span<const std::byte> b, PacketHeader& hdr) noexcept
    {
        // a만으로 충분하면 바로
        if (a.size() >= kHeaderSize) {
            std::memcpy(&hdr, a.data(), kHeaderSize);
            return true;
        }

        // a에 일부, b에서 나머지
        if (a.size() + b.size() < kHeaderSize) return false;

        std::byte tmp[kHeaderSize];

        if (!a.empty())
            std::memcpy(tmp, a.data(), a.size());

        std::memcpy(tmp + a.size(), b.data(), kHeaderSize - a.size());
        std::memcpy(&hdr, tmp, kHeaderSize);

        return true;
    }

}
