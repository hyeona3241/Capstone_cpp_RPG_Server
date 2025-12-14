#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

#include "ELoginResult.h"

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
    uint32_t resultCode = static_cast<uint32_t>(ELoginResult::INTERNAL_ERROR);

    // success==1 일 때만 유효
    uint64_t accountId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Login::LS_LOGIN_ACK);
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
};


// LoginServer -> MainServer
// 아이디를 전달해서 디비 검색 요청
class LSDbFindAccountReq final : public IPacket
{
public:
    uint32_t seq = 0;
    std::string loginId;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Login::LS_DB_FIND_ACCOUNT_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteStringLp16(loginId);   // LP16
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        loginId = r.ReadString();     // ReadString()은 LP16 포맷과 호환
        return true;
    }
};

// MainServer -> LoginServe
// 결과 전달 (Id 랑 패스워드 해시, 솔트)
class LSDbFindAccountAck final : public IPacket
{
public:
    uint32_t seq = 0;

    uint32_t exists = 0;
    uint32_t resultCode = static_cast<uint32_t>(ELoginResult::INTERNAL_ERROR);

    // exists == 1일 때만 유효
    uint64_t accountId = 0;
    std::string passwordHash;
    std::string passwordSalt;
    uint8_t state = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Login::LS_DB_FIND_ACCOUNT_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU32(exists);
        w.WriteU32(resultCode);

        if (exists != 0)
        {
            w.WriteU64(accountId);
            w.WriteStringLp16(passwordHash);
            w.WriteStringLp16(passwordSalt);
            w.WriteU8(state);
        }
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        exists = r.ReadUInt32();
        resultCode = r.ReadUInt32();

        if (exists != 0)
        {
            accountId = r.ReadUInt64();
            passwordHash = r.ReadString();
            passwordSalt = r.ReadString();
            state = r.ReadUInt8();
        }
        else
        {
            accountId = 0;
            passwordHash.clear();
            passwordSalt.clear();
            state = 0;
        }

        return true;
    }
};