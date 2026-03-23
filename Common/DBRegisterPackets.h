#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

#include "ERegisterResult.h"

// MainServer -> DBServer
// 클라세션아이디, 아이디, 패스워드 보냄
class DbRegisterReq final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t clientSessionId = 0;

    std::string loginId;
    std::string plainPw;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_REGISTER_REQ);
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

        loginId = r.ReadString();
        plainPw = r.ReadString();
        return true;
    }
};

// DBServer -> MainServer
class DbRegisterAck final : public IPacket
{
public:
    uint32_t seq = 0;
    uint64_t clientSessionId = 0;

    uint32_t success = 0; // 0=false, 1=true
    uint32_t resultCode = static_cast<uint32_t>(ERegisterResult::INTERNAL_ERROR);

    // success==1 일 때만 유효
    uint64_t accountId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_REGISTER_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU64(clientSessionId);
        w.WriteU32(success);
        w.WriteU32(resultCode);

        if (success != 0)
            w.WriteU64(accountId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        clientSessionId = r.ReadUInt64();
        success = r.ReadUInt32();
        resultCode = r.ReadUInt32();

        if (success != 0)
            accountId = r.ReadUInt64();
        else
            accountId = 0;

        return true;
    }

    static DbRegisterAck MakeOk(uint32_t seq, uint64_t clientSessionId, uint64_t newAccountId)
    {
        DbRegisterAck p;
        p.seq = seq;
        p.clientSessionId = clientSessionId;
        p.success = 1;
        p.resultCode = static_cast<uint32_t>(ERegisterResult::OK);
        p.accountId = newAccountId;
        return p;
    }

    static DbRegisterAck MakeFail(uint32_t seq, uint64_t clientSessionId, ERegisterResult why)
    {
        DbRegisterAck p;
        p.seq = seq;
        p.clientSessionId = clientSessionId;
        p.success = 0;
        p.resultCode = static_cast<uint32_t>(why);
        p.accountId = 0;
        return p;
    }
};