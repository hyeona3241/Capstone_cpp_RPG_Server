#pragma once
#include <cstdint>
#include <vector>
#include <span>

#include "BinaryWriter.h"
#include "BinaryReader.h"

class IPacket
{
public:
    virtual ~IPacket() = default;

    // 패킷 ID
    virtual std::uint16_t PacketId() const = 0;

    virtual void Write(Proto::BinaryWriter& w) const = 0;
    virtual bool Read(Proto::BinaryReader& r) = 0;

    std::vector<std::byte> Build() const
    {
        Proto::BinaryWriter writer;

        writer.BeginPacket(PacketId());

        Write(writer);
        writer.EndPacket();

        return writer.Buffer();
    }

    // OnRawPacket에서 header 분기 후 payload만 넘겨서 파싱
    bool ParsePayload(const std::byte* payload, size_t length)
    {
        Proto::BinaryReader reader(reinterpret_cast<const uint8_t*>(payload), length);
        return Read(reader);
    }
};

