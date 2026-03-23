#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

enum class EChatAuthResult : uint32_t
{
    OK = 0,
    INVALID_TOKEN = 1,
    EXPIRED = 2,
    ALREADY_USED = 3,
    NOT_ALLOWED = 4,
    INTERNAL_ERROR = 100
};

// MainServer -> Client
// 채팅 서버 접속 정보(포트) + 토큰
class ChatConnectInfoAck final : public IPacket
{
public:
    std::string chatIp;
    uint32_t chatPort = 0;
    std::string token;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::MS_CL_CHAT_CONNECT_INFO_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteStringLp16(chatIp);
        w.WriteU32(chatPort);
        w.WriteStringLp16(token);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        chatIp = r.ReadString();
        chatPort = r.ReadUInt32();
        token = r.ReadString();
        return true;
    }
};

// MainServer -> ChatServer (내부)
// 토큰 허용 등록
class MSCSChatAllowTokenReq final : public IPacket
{
public:
    std::string token;
    uint64_t accountId = 0;
    uint32_t ttlSec = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::MS_CS_CHAT_ALLOW_TOKEN_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteStringLp16(token);
        w.WriteU64(accountId);
        w.WriteU32(ttlSec);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        token = r.ReadString();
        accountId = r.ReadUInt64();
        ttlSec = r.ReadUInt32();
        return true;
    }
};

// Client -> ChatServer
// 토큰으로 인증 요청
class CLCSChatAuthReq final : public IPacket
{
public:
    std::string token;
    std::string loginId;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CL_CS_CHAT_AUTH_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteStringLp16(token);
        w.WriteStringLp16(loginId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        token = r.ReadString();
        loginId = r.ReadString();
        return true;
    }
};

// ChatServer -> Client
// 인증 결과
class CSCLChatAuthAck final : public IPacket
{
public:
    uint32_t success = 0; // 0=false, 1=true
    uint32_t resultCode = static_cast<uint32_t>(EChatAuthResult::INTERNAL_ERROR);

    // success==1일 때만 유효(클라 재확인용)
    uint64_t accountId = 0;
    std::string nickname;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Chat::CS_CL_CHAT_AUTH_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(success);
        w.WriteU32(resultCode);

        if (success != 0)
        {
            w.WriteU64(accountId);
            w.WriteStringLp16(nickname);
        }
    }

    bool Read(Proto::BinaryReader& r) override
    {
        success = r.ReadUInt32();
        resultCode = r.ReadUInt32();

        if (success != 0)
        {
            accountId = r.ReadUInt64();
            nickname = r.ReadString();
        }
        else
        {
            accountId = 0;
            nickname.clear();
        }

        return true;
    }

    static CSCLChatAuthAck MakeOk(uint64_t aid, const std::string& nick)
    {
        CSCLChatAuthAck p;
        p.success = 1;
        p.resultCode = static_cast<uint32_t>(EChatAuthResult::OK);
        p.accountId = aid;
        p.nickname = nick;
        return p;
    }

    static CSCLChatAuthAck MakeFail(EChatAuthResult why)
    {
        CSCLChatAuthAck p;
        p.success = 0;
        p.resultCode = static_cast<uint32_t>(why);
        p.accountId = 0;
        p.nickname.clear();
        return p;
    }
};