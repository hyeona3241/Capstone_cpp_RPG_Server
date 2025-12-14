#include "DBServer.h"

#include <iostream>
#include <utility>

#include "Session.h" 
#include "IocpCommon.h"
#include "PacketDef.h"
#include <Logger.h>

DBServer::DBServer(const IocpConfig& iocpCfg, const DBConfig& dbCfg)
    : IocpServerBase(iocpCfg), dbCfg_(dbCfg)
{
}

DBServer::~DBServer()
{
    StopServer();
}

bool DBServer::InitializeDb()
{
    if (!dbManager_.Initialize(dbCfg_))
    {
        std::cout << "[DBServer] DBManager.Initialize failed\n";
        LOG_WARN("[DBServer] DBManager.Initialize failed");
        return false;
    }

    DBConnection* conn = dbManager_.GetConnection();
    authService_.SetConnection(conn);

    std::cout << "[DBServer] DB initialized\n";
    LOG_INFO("[DBServer] DB Initialized");
    return true;
}

void DBServer::FinalizeDb()
{
    dbManager_.Finalize();
    std::cout << "[DBServer] DB finalized\n";
    LOG_INFO("[DBServer] DB finalized");
}

bool DBServer::StartServer(uint16_t listenPort, int workerThreads)
{   
    // DB 준비
    if (!InitializeDb())
        return false;

    dbWorker_.Start();

    // IOCP 서버 시작
    if (!Start(listenPort, workerThreads))
    {
        std::cout << "[DBServer] IOCP Start failed\n";
        LOG_WARN("[DBServer] DB finalized");
        FinalizeDb();
        return false;
    }

    std::cout << "[DBServer] Listening on port " << listenPort << "\n";
    LOG_INFO("[DBServer] Listening on port " + std::to_string(listenPort));
    return true;
}

void DBServer::StopServer()
{
    Stop();
    dbWorker_.Stop();
    FinalizeDb();

    mainServerSession_.store(nullptr);
}

void DBServer::OnClientConnected(Session* session)
{
    // MainServer 단일 연결만 허용
    Session* expected = nullptr;
    if (!mainServerSession_.compare_exchange_strong(expected, session))
    {
        // 이미 메인 서버가 붙어있음면 추가 연결 차단
        std::cout << "[DBServer] Reject extra connection (only MainServer allowed)\n";
        LOG_WARN("[DBServer] Reject extra connection (only MainServer allowed)");
        session->Disconnect();
        return;
    }

    std::cout << "[DBServer] MainServer connected\n";
    LOG_INFO("[DBServer] MainServer connected");
}

void DBServer::OnClientDisconnected(Session* session)
{
    // mainServerSession_이 이 세션이면 해제
    Session* cur = mainServerSession_.load();
    if (cur == session)
    {
        mainServerSession_.store(nullptr);
        std::cout << "[DBServer] MainServer disconnected\n";
        LOG_INFO("[DBServer] MainServer disconnected");
    }
    else
    {
        std::cout << "[DBServer] A client disconnected (not main?)\n";
        LOG_WARN("[DBServer] A client disconnected (not main?)");
    }
}

void DBServer::OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    // 지금 단계에서는 “DBJob 큐잉” 전이니까 최소 로그만
    // 이후: header.packetId 기반으로 DB_LOGIN_REQ / DB_REGISTER_REQ 등을 분기해서 Job으로 큐잉
    if (mainServerSession_.load() != session)
    {
        // MainServer가 아닌 곳에서 온 패킷이면 무시/차단
        std::cout << "[DBServer] Packet from non-main session ignored\n";
        LOG_WARN("[DBServer] Packet from non-main session ignored");
        session->Disconnect();
        return;
    }

    packetHandler_.HandleFromMainServer(session, header, payload, length);
}