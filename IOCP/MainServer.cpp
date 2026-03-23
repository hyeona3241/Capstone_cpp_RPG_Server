#include "Pch.h"
#include "MainServer.h"
#include "Session.h"
#include <chrono>

#include "PacketIds.h"
#include "DbPingPackets.h"
#include "LSPingPackets.h"
#include <Logger.h>
#include "CSPingPackets.h"
#include "LSLogoutPackets.h"
#include "CCSAuthPackets.h"

#include <random>
#include <sstream>
#include <iomanip>


MainServer::MainServer(const IocpConfig& cfg)
    : IocpServerBase(cfg), packetHandler_(this)
{
}

MainServer::~MainServer()
{
    Stop();
}

bool MainServer::Start(uint16_t port, int workerThreads)
{
    if (!IocpServerBase::Start(port, workerThreads))
        return false;

    // 통계 스레드 시작
    StartStatsThread();

    std::printf("[INFO][MAIN] MainServer started on port %u, workers=%d\n", static_cast<unsigned>(port), workerThreads);
    LOG_INFO(
        std::string("[MAIN] MainServer started on port ") +
        std::to_string(static_cast<unsigned>(port)) +
        ", workers=" +
        std::to_string(workerThreads)
    );

    return true;
}

void MainServer::Stop()
{
    // 통계 스레드 먼저 종료
    StopStatsThread();

    // 베이스 서버 정지 (워커 스레드/소켓/IOCP 정리)
    IocpServerBase::Stop();

    std::printf("[INFO][MAIN] MainServer stopped.\n");
    LOG_INFO("[MAIN] MainServer stopped.");
}

void MainServer::OnClientConnected(Session* session)
{
    if (!session)
        return;

    const SessionRole role = session->GetRole();

    if (role == SessionRole::Client)
    {
        session->SetState(SessionState::Handshake);
        {
            std::lock_guard<std::mutex> lock(clientSessionsLock_);
            clientSessions_[session->GetId()] = session;
        }
        currentClientCount_.fetch_add(1, std::memory_order_relaxed);
        totalAccepted_.fetch_add(1, std::memory_order_relaxed);

        std::printf("[INFO][CONNECTION] Client connected: sessionId=%llu\n",
            static_cast<unsigned long long>(session->GetId()));
        LOG_INFO(
            std::string("[CONNECTION] Client connected: sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId()))
        );
    }
    else
    {
        session->SetState(SessionState::NoneClient);

        currentInternalServerCount_.fetch_add(1, std::memory_order_relaxed);
        totalInternalServerConnected_.fetch_add(1, std::memory_order_relaxed);

        if (role == SessionRole::DbServer)
            currentDbServerConnected_.fetch_add(1, std::memory_order_relaxed);
        else if (role == SessionRole::LoginServer)
            currentLoginServerConnected_.fetch_add(1, std::memory_order_relaxed);
        else if (role == SessionRole::ChatServer)
            currentChatServerConnected_.fetch_add(1, std::memory_order_relaxed);

        std::printf("[INFO][CONNECTION] Internal server connected: sessionId=%llu, role=%d\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(role));
        LOG_INFO(
            std::string("[CONNECTION] Internal server connected: sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ", role=" +
            std::to_string(static_cast<int>(role))
        );

    }
}

void MainServer::OnClientDisconnected(Session* session)
{
    if (!session)
        return;

    const SessionRole  role = session->GetRole();
    const SessionState state = session->GetState();

    // 통계 업데이트
    if (role == SessionRole::Client)
    {
        uint64_t accountId = 0;
        if (TryGetAccountIdBySession(session->GetId(), accountId))
        {
            UnmarkLoggedInBySession(session->GetId());

            NotifyLoginServerLogout(accountId);
        }

        {
            std::lock_guard<std::mutex> lock(clientSessionsLock_);
            clientSessions_.erase(session->GetId());
        }
        currentClientCount_.fetch_sub(1, std::memory_order_relaxed);
        totalDisconnected_.fetch_add(1, std::memory_order_relaxed);
    }
    else
    {
        currentInternalServerCount_.fetch_sub(1, std::memory_order_relaxed);
        totalInternalServerDisconnected_.fetch_add(1, std::memory_order_relaxed);

        if (role == SessionRole::DbServer)
            currentDbServerConnected_.fetch_sub(1, std::memory_order_relaxed);
        else if (role == SessionRole::LoginServer)
            currentLoginServerConnected_.fetch_sub(1, std::memory_order_relaxed);
        else if (role == SessionRole::ChatServer)
            currentChatServerConnected_.fetch_sub(1, std::memory_order_relaxed);
    }

    if (role == SessionRole::DbServer && dbSession_ == session)
        dbSession_ = nullptr;

    if (role == SessionRole::LoginServer && loginSession_ == session)
        loginSession_ = nullptr;

    if (role == SessionRole::ChatServer && chatSession_ == session)
        chatSession_ = nullptr;

    std::printf("[INFO][CONNECTION] Client disconnected: sessionId=%llu, role=%d, state=%d\n",
        static_cast<unsigned long long>(session->GetId()),
        static_cast<int>(role),
        static_cast<int>(state));
    LOG_INFO(
        std::string("[CONNECTION] Client disconnected: sessionId=") +
        std::to_string(static_cast<unsigned long long>(session->GetId())) +
        ", role=" +
        std::to_string(static_cast<int>(role)) +
        ", state=" +
        std::to_string(static_cast<int>(state))
    );
}


void MainServer::OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    // 통계 증가
    packetsRecvTotal_.fetch_add(1, std::memory_order_relaxed);
    packetsRecvTick_.fetch_add(1, std::memory_order_relaxed);

    const SessionRole  role = session->GetRole();
    const SessionState state = session->GetState();

    // 디버그용 로그
    std::printf("[DEBUG][PACKET] Recv: sessionId=%llu, role=%d, state=%d, id=%u, len=%zu\n",
        static_cast<unsigned long long>(session->GetId()),
        static_cast<int>(role),
        static_cast<int>(state),
        static_cast<unsigned>(header.id),
        length);
    LOG_INFO(
        std::string("[PACKET] Recv: sessionId=") +
        std::to_string(static_cast<unsigned long long>(session->GetId())) +
        ", role=" +
        std::to_string(static_cast<int>(role)) +
        ", state=" +
        std::to_string(static_cast<int>(state)) +
        ", id=" +
        std::to_string(static_cast<unsigned>(header.id)) +
        ", len=" +
        std::to_string(length)
    );

    switch (role)
    {
    case SessionRole::Client:
        packetHandler_.HandleFromClient(session, header, payload, length);
        break;
    case SessionRole::LoginServer:
        packetHandler_.HandleFromLoginServer(session, header, payload, length);
        break;
    case SessionRole::DbServer:
        packetHandler_.HandleFromDbServer(session, header, payload, length);
        break;
    case SessionRole::ChatServer:
        packetHandler_.HandleFromChatServer(session, header, payload, length);
        break;
    
    default:
    {
        std::printf("[WARN][PACKET] Unknown session role: sessionId=%llu, role=%d\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(role));
        LOG_WARN(
            std::string("[PACKET] Unknown session role: sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ", role=" +
            std::to_string(static_cast<int>(role))
        );
        break;
    }
    }
}


void MainServer::StartStatsThread()
{
    bool expected = false;
    if (!statsRunning_.compare_exchange_strong(expected, true))
        return; // 이미 돌고 있으면 무시

    statsThread_ = std::thread([this]()
        {
            StatsLoop();
        });
}

void MainServer::StopStatsThread()
{
    bool expected = true;
    if (!statsRunning_.compare_exchange_strong(expected, false))
        return; // 이미 꺼져있으면 무시

    if (statsThread_.joinable())
        statsThread_.join();
}

void MainServer::StatsLoop()
{
    using namespace std::chrono_literals;

    while (statsRunning_)
    {
        std::this_thread::sleep_for(1s);

        const auto clients = currentClientCount_.load(std::memory_order_relaxed);
        const auto internal = currentInternalServerCount_.load(std::memory_order_relaxed);

        const auto dbCnt = currentDbServerConnected_.load(std::memory_order_relaxed);
        const auto loginCnt = currentLoginServerConnected_.load(std::memory_order_relaxed);
        const auto chatCnt = currentChatServerConnected_.load(std::memory_order_relaxed);

        const auto recvTick = packetsRecvTick_.exchange(0, std::memory_order_relaxed);
        const auto sendTick = packetsSentTick_.exchange(0, std::memory_order_relaxed);

        std::printf("[STATS][MAIN] clients=%u, internal=%u (db=%u, login=%u, chat-%u), recv/s=%llu, send/s=%llu\n",
            static_cast<unsigned>(clients),
            static_cast<unsigned>(internal),
            static_cast<unsigned>(dbCnt),
            static_cast<unsigned>(loginCnt),
            static_cast<unsigned>(chatCnt),
            static_cast<unsigned long long>(recvTick),
            static_cast<unsigned long long>(sendTick));
    }
}

bool MainServer::ConnectToDbServer(const char* ip, uint16_t port)
{
    dbSession_ = ConnectTo(ip, port, SessionRole::DbServer);

    const bool ok = (dbSession_ != nullptr);

    if (ok)
    {
        // 연결 직후 1회 PING / 나중에 하트비트 체크하면 될 듯
        SendDbPing();
    }

    return ok;
}

bool MainServer::ConnectToLoginServer(const char* ip, uint16_t port)
{
    loginSession_ = ConnectTo(ip, port, SessionRole::LoginServer);

    const bool ok = (loginSession_ != nullptr);

    if (ok)
    {
        SendLoginPing();
    }

    return ok;
}

bool MainServer::ConnectToChatServer(const char* ip, uint16_t port)
{
    chatSession_ = ConnectTo(ip, port, SessionRole::ChatServer);

    const bool ok = (chatSession_ != nullptr);
    if (ok)
    {
        SendChatPing();
    }
    return ok;
}

void MainServer::SendDbPing()
{
    if (!dbSession_)
    {
        std::printf("[WARN][MAIN] SendDbPing called but dbSession_ is null\n");
        LOG_WARN("[MAIN] SendDbPing called but dbSession_ is null");
        return;
    }

    DBPingReq req;
    req.seq = dbPingSeq_.fetch_add(1, std::memory_order_relaxed) + 1;

    auto bytes = req.Build();

    dbSession_->Send(bytes.data(), bytes.size());

    packetsSentTotal_.fetch_add(1, std::memory_order_relaxed);
    packetsSentTick_.fetch_add(1, std::memory_order_relaxed);

    std::printf("[DEBUG][MAIN] Sent DB_PING_REQ seq=%u (to dbSessionId=%llu)\n", 
        req.seq, static_cast<unsigned long long>(dbSession_->GetId()));
    LOG_INFO(
        std::string("[MAIN] Sent DB_PING_REQ seq=") +
        std::to_string(req.seq) +
        " (to dbSessionId=" +
        std::to_string(static_cast<unsigned long long>(dbSession_->GetId())) +
        ")"
    );
}

void MainServer::SendLoginPing()
{
    if (!loginSession_)
    {
        std::printf("[WARN][MAIN] SendLoginPing called but loginSession_ is null\n");
        LOG_WARN("[MAIN] SendLoginPing called but loginSession_ is null");
        return;
    }

    LSPingReq req;
    req.seq = loginPingSeq_.fetch_add(1, std::memory_order_relaxed) + 1;

    auto bytes = req.Build();

    loginSession_->Send(bytes.data(), bytes.size());

    packetsSentTotal_.fetch_add(1, std::memory_order_relaxed);
    packetsSentTick_.fetch_add(1, std::memory_order_relaxed);

    std::printf("[DEBUG][MAIN] Sent LS_PING_REQ seq=%u (to loginSessionId=%llu)\n",
        req.seq, static_cast<unsigned long long>(loginSession_->GetId()));
    LOG_INFO(
        std::string("[MAIN] Sent LS_PING_REQ seq=") +
        std::to_string(req.seq) +
        " (to loginSessionId=" +
        std::to_string(static_cast<unsigned long long>(loginSession_->GetId())) +
        ")"
    );

}

void MainServer::SendChatPing()
{
    if (!chatSession_)
    {
        std::printf("[WARN][MAIN] SendChatPing called but chatSession_ is null\n");
        LOG_WARN("[MAIN] SendChatPing called but chatSession_ is null");
        return;
    }

    CSPingReq req;
    req.seq = chatPingSeq_.fetch_add(1, std::memory_order_relaxed) + 1;

    auto bytes = req.Build();

    chatSession_->Send(bytes.data(), bytes.size());

    packetsSentTotal_.fetch_add(1, std::memory_order_relaxed);
    packetsSentTick_.fetch_add(1, std::memory_order_relaxed);

    std::printf("[DEBUG][MAIN] Sent CS_MS_PING_REQ seq=%u (to chatSessionId=%llu)\n",
        req.seq, static_cast<unsigned long long>(chatSession_->GetId()));
    LOG_INFO(
        std::string("[MAIN] Sent CS_MS_PING_REQ seq=") +
        std::to_string(req.seq) +
        " (to chatSessionId=" +
        std::to_string(static_cast<unsigned long long>(chatSession_->GetId())) +
        ")"
    );
}

uint32_t MainServer::NextLoginSeq()
{
    // fetch_add는 이전 값을 반환하니까 +1 해서 1부터 쓰도록
    return loginSeq_.fetch_add(1, std::memory_order_relaxed) + 1;
}

uint32_t MainServer::NextRegisterSeq()
{
    return registerSeq_.fetch_add(1, std::memory_order_relaxed) + 1;
}

Session* MainServer::FindClientSession(uint64_t sessionId)
{
    std::lock_guard<std::mutex> lock(clientSessionsLock_);
    auto it = clientSessions_.find(sessionId);
    return (it != clientSessions_.end()) ? it->second : nullptr;
}

void MainServer::OnChatPingAck(uint32_t seq, uint64_t serverTick)
{
    (void)serverTick;

    const uint64_t nowMs =
        (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

    lastChatPingAckMs_.store(nowMs, std::memory_order_relaxed);
    lastChatPingAckSeq_.store(seq, std::memory_order_relaxed);

    std::printf("[INFO][MAIN] Chat ping ack updated. seq=%u\n", seq);
}

bool MainServer::TryMarkLoggedIn(uint64_t accountId, uint64_t sessionId)
{
    std::scoped_lock lk(onlineMu_);

    if (onlineByAccount_.find(accountId) != onlineByAccount_.end())
        return false;

    onlineByAccount_[accountId] = sessionId;
    onlineBySession_[sessionId] = accountId;
    return true;
}

void MainServer::UnmarkLoggedInBySession(uint64_t sessionId)
{
    std::scoped_lock lk(onlineMu_);

    auto it = onlineBySession_.find(sessionId);
    if (it == onlineBySession_.end())
        return;

    const uint64_t accountId = it->second;
    onlineBySession_.erase(it);
    onlineByAccount_.erase(accountId);
}

bool MainServer::TryGetAccountIdBySession(uint64_t sessionId, uint64_t& outAccountId)
{
    std::scoped_lock lk(onlineMu_);
    auto it = onlineBySession_.find(sessionId);
    if (it == onlineBySession_.end())
        return false;
    outAccountId = it->second;
    return true;
}

void MainServer::NotifyLoginServerLogout(uint64_t accountId)
{
    Session* ls = GetLoginSession();
    if (!ls)
        return;

    LSLogoutNotify noti;
    noti.accountId = accountId;

    auto bytes = noti.Build();
    ls->Send(bytes.data(), bytes.size());
}

std::string MainServer::GenerateChatToken()
{
    static std::atomic<uint64_t> ctr{ 0 };

    const uint64_t now =
        (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

    uint64_t c = ctr.fetch_add(1, std::memory_order_relaxed);

    std::random_device rd;
    uint64_t r = (uint64_t(rd()) << 32) ^ uint64_t(rd());

    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << now
        << std::setw(16) << std::setfill('0') << r
        << std::setw(16) << std::setfill('0') << c;

    return oss.str();
}

void MainServer::HandlePostLoginForChat(Session* client, uint64_t accountId)
{
    if (!client)
    {
        std::printf("[WARN][CHAT] HandlePostLoginForChat called with null client\n");
        LOG_WARN("[CHAT] HandlePostLoginForChat called with null client");
        return;
    }

    const uint64_t sessionId = client->GetId();

    std::printf(
        "[INFO][CHAT] PostLogin start. clientSessionId=%llu accountId=%llu\n",
        static_cast<unsigned long long>(sessionId),
        static_cast<unsigned long long>(accountId)
    );
    LOG_INFO(
        std::string("[CHAT] PostLogin start. clientSessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " accountId=" +
        std::to_string(static_cast<unsigned long long>(accountId))
    );

    // MainServer 로그인 세션 등록
    const bool marked = TryMarkLoggedIn(accountId, sessionId);
    if (!marked)
    {
        std::printf(
            "[WARN][CHAT] Account already marked as logged in. accountId=%llu sessionId=%llu\n",
            static_cast<unsigned long long>(accountId),
            static_cast<unsigned long long>(sessionId)
        );
        LOG_WARN(
            std::string("[CHAT] Account already marked as logged in. accountId=") +
            std::to_string(static_cast<unsigned long long>(accountId)) +
            " sessionId=" +
            std::to_string(static_cast<unsigned long long>(sessionId))
        );
    }

    // 채팅 토큰 생성
    std::string chatToken = GenerateChatToken();

    std::printf(
        "[DEBUG][CHAT] Generated chat token. token=%s accountId=%llu\n",
        chatToken.c_str(),
        static_cast<unsigned long long>(accountId)
    );
    LOG_INFO(
        std::string("[CHAT] Generated chat token. token=") +
        chatToken +
        " accountId=" +
        std::to_string(static_cast<unsigned long long>(accountId))
    );

    // Main -> ChatServer : 토큰 허용
    if (Session* chat = GetChatSession())
    {
        MSCSChatAllowTokenReq allow;
        allow.token = chatToken;
        allow.accountId = accountId;
        allow.ttlSec = 30;

        auto bytes = allow.Build();
        chat->Send(bytes.data(), bytes.size());

        std::printf(
            "[INFO][CHAT] Sent MS_CS_CHAT_ALLOW_TOKEN_REQ. chatSessionId=%llu accountId=%llu ttl=%u\n",
            static_cast<unsigned long long>(chat->GetId()),
            static_cast<unsigned long long>(accountId),
            allow.ttlSec
        );
        LOG_INFO(
            std::string("[CHAT] Sent MS_CS_CHAT_ALLOW_TOKEN_REQ. chatSessionId=") +
            std::to_string(static_cast<unsigned long long>(chat->GetId())) +
            " accountId=" +
            std::to_string(static_cast<unsigned long long>(accountId)) +
            " ttl=" +
            std::to_string(allow.ttlSec)
        );
    }
    else
    {
        std::printf(
            "[WARN][CHAT] ChatServer not connected. cannot send token allow. accountId=%llu\n",
            static_cast<unsigned long long>(accountId)
        );
        LOG_WARN(
            std::string("[CHAT] ChatServer not connected. cannot send token allow. accountId=") +
            std::to_string(static_cast<unsigned long long>(accountId))
        );
    }

    // Main -> Client : 채팅 접속 정보
    ChatConnectInfoAck chatInfo;
    chatInfo.chatIp = "127.0.0.1";
    chatInfo.chatPort = 3800;
    chatInfo.token = chatToken;

    auto chatBytes = chatInfo.Build();
    client->Send(chatBytes.data(), chatBytes.size());

    std::printf(
        "[INFO][CHAT] Sent MS_CL_CHAT_CONNECT_INFO_ACK. clientSessionId=%llu ip=%s port=%u\n",
        static_cast<unsigned long long>(sessionId),
        chatInfo.chatIp.c_str(),
        chatInfo.chatPort
    );
    LOG_INFO(
        std::string("[CHAT] Sent MS_CL_CHAT_CONNECT_INFO_ACK. clientSessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " ip=" +
        chatInfo.chatIp +
        " port=" +
        std::to_string(chatInfo.chatPort)
    );

    std::printf(
        "[INFO][CHAT] PostLogin finished. clientSessionId=%llu accountId=%llu\n",
        static_cast<unsigned long long>(sessionId),
        static_cast<unsigned long long>(accountId)
    );
    LOG_INFO(
        std::string("[CHAT] PostLogin finished. clientSessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " accountId=" +
        std::to_string(static_cast<unsigned long long>(accountId))
    );
}
