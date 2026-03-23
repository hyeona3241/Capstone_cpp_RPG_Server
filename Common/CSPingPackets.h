#pragma once
#include <cstdint>
#include "IPacket.h"

#include "BinaryWriter.h"
#include "BinaryReader.h"

#include "PacketIds.h"

// MainServer -> ChatServer
class CSPingReq final : public IPacket
{
public:
    uint32_t seq = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_MS_PING_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        return true;
    }
};

// ChatServer -> MainServer
class CSPingAck final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t serverTick = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_MS_PING_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU64(serverTick);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        serverTick = r.ReadUInt64();
        return true;
    }
};
