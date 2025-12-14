#pragma once
#include <cstdint>
#include "IPacket.h"

#include "BinaryWriter.h"
#include "BinaryReader.h"

#include "PacketIds.h"

// Client -> MainServer
class CPingReq final : public IPacket
{
public:
    uint32_t seq = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::C_PING_REQ);
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

// MainServer -> Client
class CPingAck final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t serverTick = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::C_PING_ACK);
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

// MainServer -> Client
class SPingReq final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t serverTick = 0; // ¼­¹ö ½Ã°£/Æ½

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::S_PING_REQ);
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

// Client -> MainServer
class SPingAck final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t clientTick = 0; // Å¬¶ó ½Ã°£/Æ½

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::S_PING_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU64(clientTick);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        clientTick = r.ReadUInt64();
        return true;
    }
};