#include "Pch.h"
#include "MainServer.h"
#include "Session.h"
#include <chrono>

#include "PacketIds.h"
#include "DbPingPackets.h"
#include "LSPingPackets.h"


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

    return true;
}

void MainServer::Stop()
{
    // 통계 스레드 먼저 종료
    StopStatsThread();

    // 베이스 서버 정지 (워커 스레드/소켓/IOCP 정리)
    IocpServerBase::Stop();

    std::printf("[INFO][MAIN] MainServer stopped.\n");
}

void MainServer::OnClientConnected(Session* session)
{
    if (!session)
        return;

    const SessionRole role = session->GetRole();

    if (role == SessionRole::Client)
    {
        session->SetState(SessionState::Handshake);
        currentClientCount_.fetch_add(1, std::memory_order_relaxed);
        totalAccepted_.fetch_add(1, std::memory_order_relaxed);

        std::printf("[INFO][CONNECTION] Client connected: sessionId=%llu\n",
            static_cast<unsigned long long>(session->GetId()));
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

        std::printf("[INFO][CONNECTION] Internal server connected: sessionId=%llu, role=%d\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(role));

    }
}

void MainServer::OnClientDisconnected(Session* session)
{
    if (!session)
        return;

    // 간단하게 Role/State 정도만 찍어주자.
    const SessionRole  role = session->GetRole();
    const SessionState state = session->GetState();

    // 통계 업데이트 (외부 클라 기준)
    if (role == SessionRole::Client)
    {
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
    }

    if (role == SessionRole::DbServer && dbSession_ == session)
        dbSession_ = nullptr;

    if (role == SessionRole::LoginServer && loginSession_ == session)
        loginSession_ = nullptr;

    std::printf("[INFO][CONNECTION] Client disconnected: sessionId=%llu, role=%d, state=%d\n",
        static_cast<unsigned long long>(session->GetId()),
        static_cast<int>(role),
        static_cast<int>(state));
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
    
    default:
    {
        std::printf("[WARN][PACKET] Unknown session role: sessionId=%llu, role=%d\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(role));
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

        const auto recvTick = packetsRecvTick_.exchange(0, std::memory_order_relaxed);
        const auto sendTick = packetsSentTick_.exchange(0, std::memory_order_relaxed);

        std::printf("[STATS][MAIN] clients=%u, internal=%u (db=%u, login=%u), recv/s=%llu, send/s=%llu\n",
            static_cast<unsigned>(clients),
            static_cast<unsigned>(internal),
            static_cast<unsigned>(dbCnt),
            static_cast<unsigned>(loginCnt),
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

void MainServer::SendDbPing()
{
    if (!dbSession_)
    {
        std::printf("[WARN][MAIN] SendDbPing called but dbSession_ is null\n");
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
}

void MainServer::SendLoginPing()
{
    if (!loginSession_)
    {
        std::printf("[WARN][MAIN] SendLoginPing called but loginSession_ is null\n");
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
}
