
#pragma once
#include <cstdint>
#include <cstddef>
#include <span>
#include <vector>
#include "Const.h"

// =============================
// ��Ŷ ��� �� ��� ����
// =============================

namespace Proto {

    // ��Ŷ ���� 4����Ʈ�� (��� ���� final)
    struct PacketHeader final {
        uint16_t size;
        uint16_t id;
    };

    // ����� �ݵ�� 4����Ʈ�� �����ϵ��� ���⼭ ������
    static_assert(sizeof(PacketHeader) == 4, "PacketHeader must be 4 bytes");

    

    inline constexpr std::size_t kHeaderSize = sizeof(PacketHeader);
    // �� ���� ��Ŷ�� ���� �� �ִ� �ִ� ũ�� ���� (64KB) �Ŀ� ũ�� ���� �ɼ���
    inline constexpr std::size_t kMaxPacket = 64 * 1024;    // �޸� �� ����
    inline constexpr std::size_t kMaxPayload = kMaxPacket - kHeaderSize;



    // ���� Ȯ�� �Լ� (Const���� ���� ��)
    inline constexpr uint32_t ProtocolVersion() noexcept 
    {
        return Const::kProtocolVersion;
    }

    // ��ȿ�� �˻�
    constexpr bool IsValidPacketSize(uint16_t size) noexcept 
    {
        return size >= kHeaderSize && size <= kMaxPacket;
    }

    // �׽�Ʈ�� BinaryWriter
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