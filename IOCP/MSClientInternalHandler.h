#pragma once
#include "IMSDomainHandler.h"

class MainServer;
class LoginReq;
class RegisterReq;

class MSClientInternalHandler :  public IMSDomainHandler
{
public:
    explicit MSClientInternalHandler(MainServer* owner);

    bool Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override;

private:

    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

    bool HandleHandshake(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    bool HandlePingReq(Session* session, const std::byte* payload, std::size_t length);

private:

    bool HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void ForwardLoginReqToLoginServer(Session* clientSession, const LoginReq& clientReq);

    void ForwardRegisterReqToDbServer(Session* clientSession, const RegisterReq& clientReq);

    bool HandleLogoutReq(Session* session, const std::byte* payload, std::size_t length);

    void ForwardLogoutNotifyToLoginServer(Session* clientSession, uint64_t accountId);

private:
    MainServer* owner_{ nullptr };
};

