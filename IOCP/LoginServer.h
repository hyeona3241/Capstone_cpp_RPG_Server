#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "IocpServerBase.h"
#include "LSPacketHandler.h"

class Session;
struct PacketHeader;

class LoginServer final : public IocpServerBase
{
public:
    explicit LoginServer(const IocpConfig& cfg);
    ~LoginServer() override;

    bool StartServer(uint16_t listenPort, int workerThreads);
    void StopServer();

protected:
    void OnClientConnected(Session* session) override;
    void OnClientDisconnected(Session* session) override;
    void OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload,
        std::size_t length) override;

private:
    std::atomic<Session*> mainServerSession_{ nullptr };

    LSPacketHandler packetHandler_{ this };
};

