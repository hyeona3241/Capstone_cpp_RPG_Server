#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

// Client -> ChatServer
// 채널 안에서 메시지 보내기
class CLCSChatSendReq final : public IPacket
{
public:
    uint32_t channelId = 0;
    std::string message;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CL_CS_CHAT_SEND_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(channelId);
        w.WriteStringLp16(message);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        channelId = r.ReadUInt32();
        message = r.ReadString();
        return true;
    }
};

// ChatServer -> Client (브로드캐스트)
// 같은 채널에 있는 모든 클라에게 전달
class CSCLChatBroadcastNfy final : public IPacket
{
public:
    uint32_t channelId = 0;
    std::string senderLoginId; // 서버가 세션에서 채워줌
    std::string message;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_CL_CHAT_BROADCAST_NFY);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(channelId);
        w.WriteStringLp16(senderLoginId);
        w.WriteStringLp16(message);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        channelId = r.ReadUInt32();
        senderLoginId = r.ReadString();
        message = r.ReadString();
        return true;
    }
};