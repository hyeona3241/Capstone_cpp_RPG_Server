#pragma once
#include "IMSDomainHandler.h"

class MainServer;
class Session;
struct PacketHeader;

class MSChatServerInternalHandler : public IMSDomainHandler
{
public:
    explicit MSChatServerInternalHandler(MainServer* owner);

    bool Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override;

private:

    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

    bool HandleHandshake(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // CS_MS_PING_ACK Ã³¸®
    bool HandlePingAck(Session* session, const std::byte* payload, std::size_t length);


private:
    MainServer* owner_{ nullptr };
};

