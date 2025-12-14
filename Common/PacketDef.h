
#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>

#include "Core/Const.h"

// =============================
// 패킷 헤더 및 상수 정의
// =============================

namespace Proto {

    // 패킷 길이 4바이트로 (상속 금지 final)
    struct PacketHeader final {
        uint16_t size;
        uint16_t id;    //type
    };

    // 헤더는 반드시 4바이트로 직렬하도록 여기서 막아줌
    static_assert(sizeof(PacketHeader) == 4, "PacketHeader must be 4 bytes");

    

    inline constexpr std::size_t kHeaderSize = sizeof(PacketHeader);
    // 한 번의 패킷이 가질 수 있는 최대 크기 상한 (64KB) 후에 크기 수정 될수도
    inline constexpr std::size_t kMaxPacket = 64 * 1024;    // 메모리 고갈 방지 (요거 Const에서도 정의 해놈)
    inline constexpr std::size_t kMaxPayload = kMaxPacket - kHeaderSize;



    // 버전 확인 함수 (Const에서 관리 중)
    inline constexpr uint32_t ProtocolVersion() noexcept 
    {
        return Const::kProtocolVersion;
    }

    // 유효성 검사
    constexpr bool IsValidPacketSize(uint16_t size) noexcept 
    {
        return size >= kHeaderSize && size <= kMaxPacket;
    }

    // 테스트용 BinaryWriter
    inline std::vector<std::byte>
        BuildPacket(uint16_t id, std::span<const std::byte> payload) {
        const std::size_t total = kHeaderSize + payload.size();
        if (total > kMaxPacket) return {};

        std::vector<std::byte> out(total);
        auto* hdr = reinterpret_cast<PacketHeader*>(out.data());
        hdr->size = static_cast<uint16_t>(total);
        hdr->id = id;

        if (!payload.empty())
            std::memcpy(out.data() + kHeaderSize, payload.data(), payload.size());

        return out;
    }

}