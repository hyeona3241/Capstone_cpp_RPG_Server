#pragma once
#include "Pch.h"
#include <atomic>
#include <queue>
#include <mutex>
#include "IocpCommon.h"
#include "RecvRing.h"

enum class SessionRole : uint8_t
{
    Unknown,
    Client,        // 실제 게임 클라이언트
    LoginServer,   // 내부 로그인 서버와의 연결
    DbServer,      // 내부 DB 서버와의 연결

    // 뒤에 계속해서 추가하기
};

enum class SessionState : uint8_t
{
    Disconnected,
    Handshake,     // 버전체크, 기본 프로토콜

    NoneClient,    // 클라이언트가 아닌 연결

    Login,         // 로그인/인증 진행 중
    Lobby,         // 로그인 완료 (로비/대기)
    World,         // 인게임 접속 중

    // 뒤에 계속 추가하기
};

class IocpServerBase;   // owner 서버
class Buffer;
class BufferPool;

class Session
{

public:
    Session();
    ~Session();

    void Init(SOCKET s, IocpServerBase* owner, SessionRole role, uint64_t id);

    void Disconnect();

    void ResetForReuse();

    void PostRecv();

    void OnRecvCompleted(Buffer* buf, DWORD bytes);

    void ProcessPacketsFromRing();

    void Send(const void* data, size_t len);
    void OnSendCompleted(Buffer* buf, DWORD bytes);

    void SetState(SessionState newState)
    {
        state_ = newState;
    }

    SessionState GetState() const { return state_; }
    SessionRole GetRole()  const { return role_; }
    SOCKET GetSocket() const { return sock_; }
    uint64_t GetId() const { return id_; }

private:
    void PostSend();

private:
    SOCKET sock_{ INVALID_SOCKET };
    IocpServerBase* owner_{ nullptr };

    std::atomic<bool> connected_{ false };
    std::atomic<bool> sending_{ false };
    std::atomic<bool> closing_{ false };

    SessionRole  role_{ SessionRole::Unknown };
    SessionState state_{ SessionState::Disconnected };

    OverlappedEx recvOvl_;
    RecvRing recvRing_;

    OverlappedEx sendOvl_;
    std::mutex   sendMutex_;

    struct SendJob
    {
        std::vector<char> data;
        //uint16_t packetId;
    };

    std::queue<SendJob> sendQueue_;

    uint64_t id_{ 0 };    // 세션 고유 아이디
    
};

