#pragma once
#include "IocpServerBase.h"
#include "ChatPacketHandler.h"
#include "ChatSessionManager.h"
#include "ChannelManager.h"

class ChatServer : public IocpServerBase
{
public:
    explicit ChatServer(const IocpConfig& cfg);
    ~ChatServer();

    bool Start(uint16_t port, int workerThreads);
    void Stop();

    ChatSessionManager& GetSessionManager() { return sessionMgr_; }
    ChannelManager& GetChannelManager() { return channelMgr_; }

protected:
    Session* CreateSessionForAccept(SOCKET clientSock, SessionRole role) override;

    void OnClientConnected(Session* session) override;
    void OnClientDisconnected(Session* session) override;

    void OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override;

private:
    ChatPacketHandler packetHandler_;
    ChatSessionManager sessionMgr_;
    ChannelManager channelMgr_;
};

