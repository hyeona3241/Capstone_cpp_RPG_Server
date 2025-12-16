#pragma once
#include "Pch.h"
#include "IocpServerBase.h"
#include "MSPacketHandler.h"
#include <unordered_map>

class MainServer final : public IocpServerBase
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
    bool ConnectToLoginServer(const char* ip, uint16_t port);
    bool ConnectToChatServer(const char* ip, uint16_t port);
    
    Session* GetDbSession() { return dbSession_; }
    Session* GetLoginSession() { return loginSession_; }
    Session* GetChatSession() { return chatSession_; }

    void SendDbPing();
    void SendLoginPing();
    void SendChatPing();

    uint32_t NextLoginSeq();
    uint32_t NextRegisterSeq();

    Session* FindClientSession(uint64_t sessionId);

    void OnChatPingAck(uint32_t seq, uint64_t serverTick);

    bool TryMarkLoggedIn(uint64_t accountId, uint64_t sessionId);
    void UnmarkLoggedInBySession(uint64_t sessionId);
    bool TryGetAccountIdBySession(uint64_t sessionId, uint64_t& outAccountId);

    void NotifyLoginServerLogout(uint64_t accountId);

    std::string GenerateChatToken();

    void HandlePostLoginForChat(Session* client, uint64_t accountId);

private:
    MSPacketHandler packetHandler_;

    // OnClientConnected와 OnClientDisconnected에서만 통계 변수 +,-하기
    std::atomic<uint32_t> currentClientCount_{ 0 };
    std::atomic<uint64_t> totalAccepted_{ 0 };
    std::atomic<uint64_t> totalDisconnected_{ 0 };

    std::atomic<uint32_t> currentInternalServerCount_{ 0 };
    std::atomic<uint64_t> totalInternalServerConnected_{ 0 };
    std::atomic<uint64_t> totalInternalServerDisconnected_{ 0 };

    // 서버 추가되면 여기서도 추가
    std::atomic<uint32_t> currentDbServerConnected_{ 0 };
    std::atomic<uint32_t> currentLoginServerConnected_{ 0 };
    std::atomic<uint32_t> currentChatServerConnected_{ 0 };

    std::atomic<uint64_t> packetsRecvTotal_{ 0 };
    std::atomic<uint64_t> packetsSentTotal_{ 0 };   // 나중에 Send 경로에서 증가시킬 예정

    std::atomic<uint64_t> packetsRecvTick_{ 0 };    // 1초당 수신 패킷 수
    std::atomic<uint64_t> packetsSentTick_{ 0 };    // 1초당 송신 패킷 수

    std::atomic<uint64_t> lastChatPingAckMs_{ 0 };
    std::atomic<uint32_t> lastChatPingAckSeq_{ 0 };

    // 통계 출력용 쓰레드
    std::thread statsThread_;
    std::atomic<bool> statsRunning_{ false };

private:
    std::atomic<uint32_t> dbPingSeq_{ 0 };
    Session* dbSession_ = nullptr;

    std::atomic<uint32_t> loginPingSeq_{ 0 };
    Session* loginSession_ = nullptr;

    std::atomic<uint32_t> chatPingSeq_{ 0 };
    Session* chatSession_ = nullptr;

    std::atomic<uint32_t> loginSeq_{ 0 };
    std::atomic<uint32_t> registerSeq_{ 0 }; 

    std::unordered_map<uint64_t, Session*> clientSessions_;
    std::mutex clientSessionsLock_;

private:
    std::mutex onlineMu_;
    std::unordered_map<uint64_t, uint64_t> onlineByAccount_; // accountId -> sessionId
    std::unordered_map<uint64_t, uint64_t> onlineBySession_; // sessionId -> accountId

    std::mutex pendingLoginMu_;
    std::unordered_map<uint64_t, std::string> pendingLoginIdBySession_; // sessionId -> loginId
};

