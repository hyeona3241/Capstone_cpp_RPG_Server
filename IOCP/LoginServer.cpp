#include "LoginServer.h"

#include <iostream>
#include "Session.h"
#include "PacketDef.h"

LoginServer::LoginServer(const IocpConfig& cfg)
    : IocpServerBase(cfg)
{
}

LoginServer::~LoginServer()
{
    StopServer();
}

bool LoginServer::StartServer(uint16_t listenPort, int workerThreads)
{
    if (!Start(listenPort, workerThreads))
    {
        std::cout << "[LoginServer] IOCP Start failed\n";
        return false;
    }

    std::cout << "[LoginServer] Listening on port " << listenPort << "\n";
    return true;
}

void LoginServer::StopServer()
{
    Stop();
    mainServerSession_.store(nullptr);
}

void LoginServer::OnClientConnected(Session* session)
{
    // MainServer 단일 연결만 허용 (DBServer와 동일 정책) :contentReference[oaicite:2]{index=2}
    Session* expected = nullptr;
    if (!mainServerSession_.compare_exchange_strong(expected, session))
    {
        std::cout << "[LoginServer] Reject extra connection (only MainServer allowed)\n";
        session->Disconnect();
        return;
    }

    std::cout << "[LoginServer] MainServer connected\n";
}

void LoginServer::OnClientDisconnected(Session* session)
{
    Session* cur = mainServerSession_.load();
    if (cur == session)
    {
        mainServerSession_.store(nullptr);
        std::cout << "[LoginServer] MainServer disconnected\n";
    }
    else
    {
        std::cout << "[LoginServer] A client disconnected (not main?)\n";
    }
}

void LoginServer::OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (mainServerSession_.load() != session)
    {
        std::cout << "[LoginServer] Packet from non-main session ignored\n";
        session->Disconnect();
        return;
    }

    // 패킷 분기/인증 로직은 핸들러로 위임
    packetHandler_.HandleFromMainServer(session, header, payload, length);
}