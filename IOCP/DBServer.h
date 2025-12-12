#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "IocpServerBase.h"
#include "DBManager.h"

class Session;
struct PacketHeader;

class DBServer : public IocpServerBase
{
public:
    DBServer(const IocpConfig& iocpCfg, const DBConfig& dbCfg);
    ~DBServer() override;

    // DB 초기화 + IOCP 서버 Start
    bool StartServer(uint16_t listenPort, int workerThreads);

    // IOCP Stop + DB 종료
    void StopServer();

protected:
    void OnClientConnected(Session* session) override;
    void OnClientDisconnected(Session* session) override;

    // TODO: 여기에 패킷 디스패치 -> DBJob 큐잉 로직이 들어갈 예정
    void OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override;

private:
    bool InitializeDb();
    void FinalizeDb();

private:
    DBConfig dbCfg_;
    DBManager dbManager_;

    std::atomic<Session*> mainServerSession_{ nullptr };
};

