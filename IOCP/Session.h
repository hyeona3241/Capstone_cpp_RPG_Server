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
    Client,
    MainServer,
    LoginServer,
    DbServer,
    ChatServer,
    WorldServer,
};

class IIocpServer;
class Buffer;
class BufferPool;

class Session
{
public:
    Session();
    virtual ~Session();

    virtual void Init(SOCKET s, IIocpServer* owner, SessionRole role, uint64_t id);
    virtual void ResetForReuse();

    void Disconnect();
    void PostRecv();
    void OnRecvCompleted(Buffer* buf, DWORD bytes);
    void ProcessPacketsFromRing();

    void Send(const void* data, size_t len);
    void OnSendCompleted(Buffer* buf, DWORD bytes);

    SOCKET GetSocket() const { return sock_; }
    uint64_t GetId() const { return id_; }
    SessionRole GetRole() const { return role_; }

    bool IsConnected() const { return connected_; }

protected:
    virtual void OnInit() {}
    virtual void OnReset() {}

private:
    void PostSend();

protected:
    SOCKET sock_{ INVALID_SOCKET }; 
    IIocpServer* owner_{ nullptr };

    std::atomic<bool> connected_{ false };
    std::atomic<bool> sending_{ false };
    std::atomic<bool> closing_{ false };

    SessionRole role_{ SessionRole::Unknown };

    OverlappedEx recvOvl_;
    RecvRing recvRing_;

    OverlappedEx sendOvl_;
    std::mutex sendMutex_;

    struct SendJob
    {
        std::vector<char> data;
    };

    std::queue<SendJob> sendQueue_;
    uint64_t id_{ 0 };
};