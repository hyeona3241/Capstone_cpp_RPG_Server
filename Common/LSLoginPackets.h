#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

// MainServer -> LoginServer
// 아이디, 비번, 클라세션ID 전달
class LSLoginReq final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t clientSessionId = 0;

    std::string loginId;
    std::string plainPw;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Login::LS_LOGIN_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU64(clientSessionId);

        w.WriteStringLp16(loginId);
        w.WriteStringLp16(plainPw);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        clientSessionId = r.ReadUInt64();

        loginId = r.ReadString();   // LP16 호환
        plainPw = r.ReadString();   // LP16 호환
        return true;
    }
};


// LoginServer -> MainServer
// 성공 여부, 클라세션ID 전달
class LSLoginAck final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t clientSessionId = 0;

    // bool 대신 U32(0/1)로 통일
    uint32_t success = 0; // 0=false, 1=true

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Login::LS_LOGIN_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU64(clientSessionId);
        w.WriteU32(success);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        clientSessionId = r.ReadUInt64();
        success = r.ReadUInt32();
        return true;
    }
};