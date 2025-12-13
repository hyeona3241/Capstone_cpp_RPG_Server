#pragma once
#include <cstdint>
#include "IPacket.h"

#include "BinaryWriter.h"
#include "BinaryReader.h"

#include "PacketIds.h"

// MainServer -> DBServer
class DBPingReq final : public IPacket
{
public:
    uint32_t seq = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_PING_REQ);
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

// DBServer -> MainServer
class DBPingAck final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t serverTick = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_PING_ACK);
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
