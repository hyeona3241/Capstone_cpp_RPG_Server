#include "LoginServer.h"

#include <iostream>
#include "Session.h"
#include "PacketDef.h"
#include <Logger.h>

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
        LOG_ERROR("[LoginServer] IOCP Start failed");
        return false;
    }

    std::cout << "[LoginServer] Listening on port " << listenPort << "\n";
    LOG_INFO(std::string("[LoginServer] Listening on port ") + std::to_string(listenPort));
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
        LOG_WARN("[LoginServer] Reject extra connection (only MainServer allowed)");
        session->Disconnect();
        return;
    }

    std::cout << "[LoginServer] MainServer connected\n";
    LOG_INFO("[LoginServer] MainServer connected");
}

void LoginServer::OnClientDisconnected(Session* session)
{
    Session* cur = mainServerSession_.load();
    if (cur == session)
    {
        mainServerSession_.store(nullptr);
        std::cout << "[LoginServer] MainServer disconnected\n";
        LOG_WARN("[LoginServer] MainServer disconnected");
    }
    else
    {
        std::cout << "[LoginServer] A client disconnected (not main?)\n";
        LOG_WARN("[LoginServer] A client disconnected (not main?)");
    }
}

void LoginServer::OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (mainServerSession_.load() != session)
    {
        std::cout << "[LoginServer] Packet from non-main session ignored\n";
        LOG_WARN("[LoginServer] Packet from non-main session ignored");
        session->Disconnect();
        return;
    }

    // 패킷 분기/인증 로직은 핸들러로 위임
    packetHandler_.HandleFromMainServer(session, header, payload, length);
}

void LoginServer::AddPendingLogin(PendingLogin p)
{
    std::scoped_lock lk(pendingMu_);
    pendingLogins_[p.seq] = std::move(p);
}

bool LoginServer::TryGetPendingLogin(uint32_t seq, PendingLogin& out)
{
    std::scoped_lock lk(pendingMu_);
    auto it = pendingLogins_.find(seq);
    if (it == pendingLogins_.end()) return false;
    out = it->second;
    return true;
}

void LoginServer::RemovePendingLogin(uint32_t seq)
{
    std::scoped_lock lk(pendingMu_);
    pendingLogins_.erase(seq);

}

bool LoginServer::SendToMain(const std::vector<std::byte>& bytes)
{
    Session* ms = mainServerSession_.load();
    if (!ms) return false;

    ms->Send(bytes.data(), bytes.size());
    return true;
}

bool LoginServer::TryMarkLoggedIn(uint64_t accountId, uint64_t clientSessionId)
{
    std::scoped_lock lk(onlineMu_);

    // 이미 같은 accountId가 로그인 중이면 실패
    if (onlineByAccount_.find(accountId) != onlineByAccount_.end())
        return false;

    onlineByAccount_[accountId] = clientSessionId;
    onlineByClient_[clientSessionId] = accountId;
    return true;
}

void LoginServer::UnmarkLoggedInByAccount(uint64_t accountId)
{
    std::scoped_lock lk(onlineMu_);
    auto it = onlineByAccount_.find(accountId);
    if (it == onlineByAccount_.end()) return;

    const uint64_t clientSessionId = it->second;
    onlineByAccount_.erase(it);
    onlineByClient_.erase(clientSessionId);
}

void LoginServer::UnmarkLoggedInByClientSession(uint64_t clientSessionId)
{
    std::scoped_lock lk(onlineMu_);
    auto it = onlineByClient_.find(clientSessionId);
    if (it == onlineByClient_.end()) return;

    const uint64_t accountId = it->second;
    onlineByClient_.erase(it);
    onlineByAccount_.erase(accountId);
}


