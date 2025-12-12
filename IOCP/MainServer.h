#pragma once
#include "Pch.h"
#include "IocpServerBase.h"
#include "MSPacketHandler.h"

class MainServer : public IocpServerBase
{
public:
    explicit MainServer(const IocpConfig& cfg);
    ~MainServer();

    // IocpServerBase::Start/Stop을 감싸서 통계 스레드까지 같이 관리
    bool Start(uint16_t port, int workerThreads);
    void Stop();

protected:
    // 새 세션이 생성되었을 때 (클라/내부 서버 공통)
    void OnClientConnected(Session* session) override;

    // 세션이 완전히 끊기기 직전
    void OnClientDisconnected(Session* session) override;

    // RecvRing에서 완성된 패킷이 올라오는 진입점
    void OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override;

private:
    void StartStatsThread();
    void StopStatsThread();
    void StatsLoop();


public:
    bool ConnectToDbServer(const char* ip, uint16_t port);

private:
    MSPacketHandler packetHandler_;

    std::atomic<uint32_t> currentClientCount_{ 0 };
    std::atomic<uint64_t> totalAccepted_{ 0 };
    std::atomic<uint64_t> totalDisconnected_{ 0 };

    std::atomic<uint64_t> packetsRecvTotal_{ 0 };
    std::atomic<uint64_t> packetsSentTotal_{ 0 };   // 나중에 Send 경로에서 증가시킬 예정

    std::atomic<uint64_t> packetsRecvTick_{ 0 };    // 1초당 수신 패킷 수
    std::atomic<uint64_t> packetsSentTick_{ 0 };    // 1초당 송신 패킷 수

    // 통계 출력용 쓰레드
    std::thread statsThread_;
    std::atomic<bool> statsRunning_{ false };

    Session* dbSession_ = nullptr;
};

