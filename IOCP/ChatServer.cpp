#include "ChatServer.h"
#include "ChatSession.h"
#include <cstdio>
#include <atomic>
#include <Logger.h>

static std::atomic<uint64_t> g_chatSessionId{ 1 };

ChatServer::ChatServer(const IocpConfig& cfg)
    : IocpServerBase(cfg)
    , packetHandler_(this)
{
}

ChatServer::~ChatServer()
{
    Stop();
}

bool ChatServer::Start(uint16_t port, int workerThreads)
{
    if (!IocpServerBase::Start(port, workerThreads))
        return false;

    // 기본 채널(로비 등) 생성
    channelMgr_.InitDefaultChannels();

    std::printf("[INFO][CHAT] ChatServer started on port %u, workers=%d\n",
        static_cast<unsigned>(port), workerThreads);
    LOG_INFO(
        std::string("[CHAT] ChatServer started on port ") +
        std::to_string(static_cast<unsigned>(port)) +
        ", workers=" + std::to_string(workerThreads)
    );

    return true;
}

void ChatServer::Stop()
{
    // 1) 매니저 먼저 종료(세션 Disconnect 포함)
    sessionMgr_.Shutdown();

    // 2) IOCP 베이스 정지
    IocpServerBase::Stop();

    std::printf("[INFO][CHAT] ChatServer stopped.\n");
    LOG_INFO("[CHAT] ChatServer stopped.");
}

Session* ChatServer::CreateSessionForAccept(SOCKET clientSock, SessionRole role)
{
    auto* s = new ChatSession();
    const uint64_t id = g_chatSessionId.fetch_add(1, std::memory_order_relaxed);
    s->Init(clientSock, this, role, id);
    return s;
}

void ChatServer::OnClientConnected(Session* session)
{
    if (!session)
        return;

    std::printf("[INFO][CHAT][CONNECTION] Connected: sessionId=%llu role=%d state=%d\n",
        static_cast<unsigned long long>(session->GetId()),
        static_cast<int>(session->GetRole()),
        static_cast<int>(session->GetState()));

    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs)
    {
        std::printf("[WARN][CHAT] Not a ChatSession. Disconnect. sessionId=%llu\n",
            static_cast<unsigned long long>(session->GetId()));
        session->Disconnect();
        return;
    }

    sessionMgr_.RegisterSession(cs);
}


void ChatServer::OnClientDisconnected(Session* session)
{
    if (!session)
        return;

    std::printf("[INFO][CHAT][CONNECTION] Disconnected: sessionId=%llu role=%d state=%d\n",
        static_cast<unsigned long long>(session->GetId()),
        static_cast<int>(session->GetRole()),
        static_cast<int>(session->GetState()));

    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs)
        return;

    channelMgr_.LeaveIfInChannel(cs); 

    sessionMgr_.OnSessionDisconnected(cs);
}


void ChatServer::OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto role = session->GetRole();
    const auto state = session->GetState();

    std::printf("[DEBUG][CHAT][PACKET] Recv: sessionId=%llu, role=%d, state=%d, id=%u, len=%zu\n",
        static_cast<unsigned long long>(session->GetId()),
        static_cast<int>(role),
        static_cast<int>(state),
        static_cast<unsigned>(header.id),
        length);
    LOG_INFO(std::string("[CHAT][PACKET] Recv: sessionId=") +
        std::to_string(static_cast<unsigned long long>(session->GetId())) +
        ", role=" + std::to_string(static_cast<int>(role)) +
        ", state=" + std::to_string(static_cast<int>(state)) +
        ", id=" + std::to_string(static_cast<unsigned>(header.id)) +
        ", len=" + std::to_string(length));


    switch (session->GetRole())
    {
    case SessionRole::Client:
        packetHandler_.HandleFromClient(session, header, payload, length);
        break;
    case SessionRole::MainServer:
        packetHandler_.HandleFromMainServer(session, header, payload, length);
        break;
    default:
        std::printf(
            "[WARN][CHAT][PACKET] Unknown role packet ignored. sessionId=%llu role=%d\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(role)
        );
        break;
    }
}