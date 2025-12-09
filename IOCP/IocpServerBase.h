#pragma once
#include "Pch.h"
#include <thread>
#include <atomic>

#include "IocpCommon.h"
#include "SessionPool.h"
#include "BufferPool.h"

class Session;
class Buffer;
struct PacketHeader;

class Packet;

struct IocpConfig
{
    std::size_t maxSessions = 0;    // 세션 풀 크기
    std::size_t bufferCount = 0;    // 버퍼 풀 개수
    std::size_t bufferSize = 0;     // 각 버퍼 크기
    std::size_t recvRingCapacity = 0;   // 세션당 리시브 링 용량
};


class IocpServerBase
{
public:
    explicit IocpServerBase(const IocpConfig& cfg);
    virtual ~IocpServerBase();

    bool Start(uint16_t port, int workerThreads);
    void Stop();

    BufferPool* GetBufferPool() { return &bufferPool_; }
    SessionPool* GetSessionPool() { return &sessionPool_; }
    HANDLE GetIocpHandle() const { return iocpHandle_; }

protected:
    // 클라나 서버 연결 됐을 때 호출
    virtual void OnClientConnected(Session* session) {}

    // 세션이 완전히 끊기기 직전에 호출
    virtual void OnClientDisconnected(Session* session) {}

    // RecvRing이 완성한 패킷이 여기로 들어옴
    virtual void OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) = 0;

    // 세션 정리용 헬퍼
    void ReleaseSession(Session* session);


public:
    void NotifySessionDisconnect(Session* s);

    void DispatchRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
    {
        OnRawPacket(session, header, payload, length);
    }

private:

    bool CreateListenSocket(uint16_t port);
    bool CreateIocp();
    void CreateWorkerThreads(int workerThreads);
    void DestroyWorkerThreads();

    void WorkerLoop(); // GetQueuedCompletionStatus 루프

    void PostInitialAccepts(int count);
    void PostAccept();

    void OnAcceptCompleted(OverlappedEx* ovl, DWORD bytes);
    void HandleRecvCompleted(OverlappedEx* ovl, DWORD bytes);
    void HandleSendCompleted(OverlappedEx* ovl, DWORD bytes);

private:
    IocpConfig config_;

    SessionPool sessionPool_;
    BufferPool bufferPool_;

    SOCKET listenSock_{ INVALID_SOCKET };
    HANDLE iocpHandle_{ nullptr };

    std::vector<std::thread> workers_;
    std::atomic<bool> running_{ false };

    // Accept용 컨텍스트
    SOCKET acceptSock_{ INVALID_SOCKET };
    OverlappedEx acceptOvl_;
    Buffer* acceptBuffer_{ nullptr };
};

