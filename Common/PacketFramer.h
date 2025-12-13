#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <cstring>
#include "PacketDef.h"

// TCP 바이트 스트림에서 패킷을 완성 단위로 분리
// 링버퍼에서 패킷 분리

// RecvRing에서 만든 비슷한 구조여서 나중에 통합처리 하면 될듯(지금 사용 안하고 있음)
namespace Proto {
    class PacketFramer final {
    public:
        enum class Result {
            NeedMore, // 아직 패킷 다 안들어옴(헤더/본문 부족)
            Ok,       // 패킷 1개 완성(out에 채움), consumed만큼 버퍼에서 삭제
            Fatal     // 비정상 size(헤더 미만/상한 초과). 세션 종료 권장
        };

        // 완성된 패킷이 잇는지 확인
        Result TryPopFromContiguous(std::span<const std::byte> buf, size_t& consumed, std::vector<std::byte>& out) const noexcept;
        // 두 조각으로 나뉜 버퍼 조립
        Result TryPopFromTwoSegments(std::span<const std::byte> seg1, std::span<const std::byte> seg2, size_t& consumed, std::vector<std::byte>& out) const noexcept;
        // 헤더만 미리 확인
        bool PeekHeader(std::span<const std::byte> buf, PacketHeader& hdr) const noexcept;

    private:
        // 두 조각으로 나뉜 헤더를 복사한 뒤 해석
        static bool ReadHeaderFromTwo(std::span<const std::byte> a, std::span<const std::byte> b, PacketHeader& hdr) noexcept;
    };
}