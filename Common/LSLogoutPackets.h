#pragma once
#include <cstdint>

#include "IPacket.h"
#include "PacketIds.h"

// MainServer -> LoginServer
// accountId¸¸ Àü´Þ
class LSLogoutNotify final : public IPacket
{
public:
    uint64_t accountId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Login::LS_LOGOUT_NOTIFY);
    }

    void Write(Proto::BinaryWriter& w) const override
    {
        w.WriteU64(accountId);
    }

    bool Read(Proto::BinaryReader& r) override
    {
        accountId = r.ReadUInt64();
        return true;
    }
};
