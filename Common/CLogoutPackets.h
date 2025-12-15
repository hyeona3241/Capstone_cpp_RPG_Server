#pragma once
#include <cstdint>

#include "IPacket.h"
#include "PacketIds.h"

// Client -> MainServer
// accountId¸¸ Àü´Þ
class LogoutReq final : public IPacket
{
public:
    uint64_t accountId = 0;

    uint16_t PacketId() const override
    {
        return PacketType::to_id(PacketType::Client::LOGOUT_REQ);
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
