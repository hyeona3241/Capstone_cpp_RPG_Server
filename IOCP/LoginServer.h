#pragma once
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "IocpServerBase.h"
#include "LSPacketHandler.h"

class Session;
struct PacketHeader;

struct PendingLogin
{
    uint32_t seq = 0;
    uint64_t clientSessionId = 0;
    std::string loginId;
    std::string plainPw;
    uint64_t createdTickMs = 0;
};

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

public:
    void AddPendingLogin(PendingLogin p);
    bool TryGetPendingLogin(uint32_t seq, PendingLogin& out);
    void RemovePendingLogin(uint32_t seq);

    bool SendToMain(const std::vector<std::byte>& bytes);

private:
    std::atomic<Session*> mainServerSession_{ nullptr };
    LSPacketHandler packetHandler_{ this };

    std::mutex pendingMu_;
    std::unordered_map<uint32_t, PendingLogin> pendingLogins_;
};

