#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>

#include "IocpServerBase.h"
#include "DBManager.h"

#include "DBJobQueue.h"
#include "DBWorker.h"
#include "DbJob.h"

#include "AccountRepository.h"
#include "AuthService.h"

#include "DBPacketHandler.h"

class Session;
struct PacketHeader;

class DBServer final : public IocpServerBase
{
public:
    DBServer(const IocpConfig& iocpCfg, const DBConfig& dbCfg);
    ~DBServer() override;

    // DB 초기화 + IOCP 서버 Start
    bool StartServer(uint16_t listenPort, int workerThreads);

    // IOCP Stop + DB 종료
    void StopServer();

    void EnqueueJob(std::unique_ptr<DBJob> job) { jobQueue_.Push(std::move(job)); }

    // (나중에 ACK 전송 API를 여기 추가하면 LoginJob에서 사용 가능)
    // void SendToMainServer(uint64_t replySessionId, const Packet& pkt);

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

    AccountRepository accountRepo_{ dbManager_.GetConnection() };
    AuthService authService_{ accountRepo_ };
    DbServices services_{ authService_ };

    DBJobQueue jobQueue_;
    DBWorker dbWorker_{ jobQueue_, services_, *this };

    DBPacketHandler packetHandler_{ this };
};

