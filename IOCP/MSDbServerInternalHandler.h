#pragma once
#include "IMSDomainHandler.h"

class MainServer;
class Session;

class DBFindAccountAck;
class DbRegisterAck;

class MSDbServerInternalHandler :  public IMSDomainHandler
{
public:
    explicit MSDbServerInternalHandler(MainServer* owner);

    bool Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override;

private:

    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

    void OnDbFindAccountAck(Session* session, const std::byte* payload, std::size_t length);

    void OnDbRegisterAck(Session* session, const std::byte* payload, std::size_t length);


private:
    void ForwardToLoginServer_FindAccountAck(const DBFindAccountAck& dbAck);

    void ForwardToClient_RegisterAck(const DbRegisterAck& dbAck);


private:
    MainServer* owner_{ nullptr };
};

