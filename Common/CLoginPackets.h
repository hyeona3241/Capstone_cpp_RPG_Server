#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

#include "ELoginResult.h"

// Client -> MainServer
// 아이디랑 패스워드 전달
class LoginReq final : public IPacket
{
public:
    std::string loginId;
    std::string plainPw;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::LOGIN_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteStringLp16(loginId);
        w.WriteStringLp16(plainPw);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        loginId = r.ReadString();  // LP16 호환
        plainPw = r.ReadString();  // LP16 호환
        return true;
    }
};


// MainServer -> Client
// 성공 여부랑, 결과 (실패 사유등)을 전달
class LoginAck final : public IPacket
{
public:
    uint32_t success = 0;     // 0=false, 1=true
    uint32_t resultCode = static_cast<uint32_t>(ELoginResult::INTERNAL_ERROR);

    uint64_t accountId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::LOGIN_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(success);
        w.WriteU32(resultCode);

        if (success != 0)
            w.WriteU64(accountId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        success = r.ReadUInt32();
        resultCode = r.ReadUInt32();

        if (success != 0)
            accountId = r.ReadUInt64();
        else
            accountId = 0;

        return true;
    }

    static LoginAck MakeSuccess(uint64_t accountId)
    {
        LoginAck a;
        a.success = 1;
        a.resultCode = static_cast<uint32_t>(ELoginResult::OK);
        a.accountId = accountId;
        return a;
    }

    static LoginAck MakeFail(ELoginResult reason)
    {
        LoginAck a;
        a.success = 0;
        a.resultCode = static_cast<uint32_t>(reason);
        a.accountId = 0;
        return a;
    }
};