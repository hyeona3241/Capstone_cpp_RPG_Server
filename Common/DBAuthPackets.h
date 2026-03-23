#pragma once
#include <cstdint>
#include <string>

#include "IPacket.h"
#include "PacketIds.h"

// 로그인 관련 패킷 정의

// MainServer -> DBServer
// loginId로 계정 조회
class DBFindAccountReq final : public IPacket
{
public:
    uint32_t seq = 0;

    std::string loginId;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_FIND_ACCOUNT_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteStringLp16(loginId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        loginId = r.ReadString();
        return true;
    }
};


// DBServer -> MainServer
// 계정 조회 결과 응답
class DBFindAccountAck final : public IPacket
{
public:
    uint32_t seq = 0;

    uint32_t exists = 0; // 0=false, 1=true (DB에 존재안함 / 존재함)

    // exists==1일 때만 유효
    uint64_t accountId = 0;
    std::string passwordHash;
    std::string passwordSalt;
    uint8_t status = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_FIND_ACCOUNT_ACK);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);

        w.WriteU32(exists);

        if (exists != 0)
        {
            w.WriteU64(accountId);
            w.WriteStringLp16(passwordHash);
            w.WriteStringLp16(passwordSalt);
            w.WriteU8(status);
        }
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();

        exists = r.ReadUInt32();

        if (exists != 0)
        {
            accountId = r.ReadUInt64();
            passwordHash = r.ReadString();
            passwordSalt = r.ReadString();
            status = r.ReadUInt8();
        }
        else
        {
            accountId = 0;
            passwordHash.clear();
            passwordSalt.clear();
            status = 0;
        }

        return true;
    }
};


// MainServer -> DBServer
// accountID를 전달
class DBUpdateLastLoginReq final : public IPacket
{
public:
    uint32_t seq = 0;

    uint64_t accountId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::DB::DB_UPDATE_LASTLOGIN_REQ);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU32(seq);
        w.WriteU64(accountId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        seq = r.ReadUInt32();
        accountId = r.ReadUInt64();
        return true;
    }
};


// DBServer -> MainServer
// AccountId랑 성공 여부 전달
//class DBUpdateLastLoginAck final : public IPacket
//{
//public:
//    uint32_t seq = 0;
//    uint64_t accountId = 0;
//
//    uint32_t success = 0; // 0=false, 1=true
//
//    uint16_t PacketId() const override
//    {
//        return PacketType::to_id(PacketType::DB::DB_UPDATE_LASTLOGIN_ACK);
//    }
//
//    void Write(Proto::BinaryWriter& w) const override
//    {
//        w.WriteU32(seq);
//        w.WriteU64(accountId);
//        w.WriteU32(success);
//    }
//
//    bool Read(Proto::BinaryReader& r) override
//    {
//        seq = r.ReadUInt32();
//        accountId = r.ReadUInt64();
//        success = r.ReadUInt32();
//        return true;
//    }
//
//    static DBUpdateLastLoginAck Make(bool ok, uint32_t seq, uint64_t accountId)
//    {
//        DBUpdateLastLoginAck a;
//        a.seq = seq;
//        a.accountId = accountId;
//        a.success = ok ? 1u : 0u;
//        return a;
//    }
//};