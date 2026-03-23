#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

namespace ChatProtocol
{
    struct ChannelInfo
    {
        uint32_t channelId = 0;
        uint16_t userCount = 0;
        uint16_t maxUsers = 0;
        uint8_t  needPassword = 0; // 0/1
        std::string name; 
    };
}


// Client -> ChatServer
// 채널 목록 요청
class CLCSChannelListReq final : public IPacket
{
public:

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CL_CS_CHANNEL_LIST_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        
    }

    bool Read(Proto::BinaryReader& r) override
    {
       
        (void)r;
        return true;
    }
};

class CSCLChannelListAck final : public IPacket
{
public:
    uint8_t success = 0;     // 0/1
    uint16_t resultCode = 0; // 0 OK, 기타 에러 코드
    std::vector<ChatProtocol::ChannelInfo> channels;

    static CSCLChannelListAck MakeOk(const std::vector<ChatProtocol::ChannelInfo>& list)
    {
        CSCLChannelListAck p;
        p.success = 1;
        p.resultCode = 0;
        p.channels = list;
        return p;
    }

    static CSCLChannelListAck MakeFail(uint16_t code)
    {
        CSCLChannelListAck p;
        p.success = 0;
        p.resultCode = code;
        return p;
    }

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_CL_CHANNEL_LIST_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU8(success);
        w.WriteU16(resultCode);

        // count
        const uint16_t cnt = static_cast<uint16_t>(channels.size());
        w.WriteU16(cnt);

        for (const auto& c : channels)
        {
            w.WriteU32(c.channelId);
            w.WriteU16(c.userCount);
            w.WriteU16(c.maxUsers);
            w.WriteU8(c.needPassword);

            // name: 표시용(없으면 빈 문자열)
            w.WriteStringLp16(c.name);
        }
    }

    bool Read(Proto::BinaryReader& r) override
    {
        success = r.ReadUInt8();
        resultCode = r.ReadUInt16();

        const uint16_t cnt = r.ReadUInt16();
        channels.clear();
        channels.reserve(cnt);

        for (uint16_t i = 0; i < cnt; ++i)
        {
            ChatProtocol::ChannelInfo c;
            c.channelId = r.ReadUInt32();
            c.userCount = r.ReadUInt16();
            c.maxUsers = r.ReadUInt16();
            c.needPassword = r.ReadUInt8();
            c.name = r.ReadString();

            channels.push_back(std::move(c));
        }
        return true;
    }
};


// Client -> ChatServer
// 채널 입장 요청
class CLCSChannelEnterReq final : public IPacket
{
public:
    uint32_t channelId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CL_CS_CHANNEL_ENTER_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(channelId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        channelId = r.ReadUInt32();
        return true;
    }
};


// ChatServer -> Client
// 채널 입장 요청 응답
class CSCLChannelEnterAck final : public IPacket
{
public:
    uint8_t success = 0;
    uint16_t resultCode = 0;
    uint32_t channelId = 0;

    static CSCLChannelEnterAck MakeOk(uint32_t cid)
    {
        CSCLChannelEnterAck p;
        p.success = 1;
        p.resultCode = 0;
        p.channelId = cid;
        return p;
    }

    static CSCLChannelEnterAck MakeFail(uint16_t code)
    {
        CSCLChannelEnterAck p;
        p.success = 0;
        p.resultCode = code;
        return p;
    }

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_CL_CHANNEL_ENTER_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU8(success);
        w.WriteU16(resultCode);
        w.WriteU32(channelId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        success = r.ReadUInt8();
        resultCode = r.ReadUInt16();
        channelId = r.ReadUInt32();
        return true;
    }
};


// Client -> ChatServer
// 채널 나가기
class CLCSChannelLeaveReq final : public IPacket
{
public:
    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CL_CS_CHANNEL_LEAVE_REQ);
    }

    void Write(Proto::BinaryWriter&) const override {}
    bool Read(Proto::BinaryReader&) override { return true; }
};


// ChatServer -> Client
class CSCLChannelLeaveAck final : public IPacket
{
public:
    uint8_t success = 0;
    uint16_t resultCode = 0;
    uint32_t channelId = 0;

    static CSCLChannelLeaveAck MakeOk(uint32_t cid)
    {
        CSCLChannelLeaveAck p;
        p.success = 1;
        p.resultCode = 0;
        p.channelId = cid;
        return p;
    }

    static CSCLChannelLeaveAck MakeFail(uint16_t code)
    {
        CSCLChannelLeaveAck p;
        p.success = 0;
        p.resultCode = code;
        return p;
    }

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_CL_CHANNEL_LEAVE_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU8(success);
        w.WriteU16(resultCode);
        w.WriteU32(channelId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        success = r.ReadUInt8();
        resultCode = r.ReadUInt16();
        channelId = r.ReadUInt32();
        return true;
    }
};