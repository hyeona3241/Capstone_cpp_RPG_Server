#pragma once
#include "Pch.h"
#include <thread>
#include <atomic>

#include "IocpCommon.h"
#include "BufferPool.h"
#include "IIocpServer.h"

class Session;
class Buffer;
struct PacketHeader;
enum class SessionRole : uint8_t;

struct IocpConfig
{
    std::size_t maxSessions = 0;
    std::size_t bufferCount = 0;
    std::size_t bufferSize = 0;
    std::size_t recvRingCapacity = 0;
};

class IocpServerBase : public IIocpServer
{
public:
    explicit IocpServerBase(const IocpConfig& cfg);
    virtual ~IocpServerBase();

    bool Start(uint16_t port, int workerThreads);
    void Stop();

    BufferPool* GetBufferPool() override { return &bufferPool_; }
    HANDLE GetIocpHandle() const { return iocpHandle_; }

public:
    // 외부 서버(예: Login/World/Chat)로 outbound 연결을 생성할 때
    // 파생 서버가 적절한 세션 객체를 꺼내도록
    Session* ConnectTo(const char* ip, uint16_t port, SessionRole role);

protected:
    // Accept된 클라이언트 소켓에 대해 사용할 세션을 파생 서버가 제공
    // 예: GatewayServer는 GatewayClientSession 풀에서 꺼냄
    virtual Session* AcquireAcceptedSession(SOCKET s, SessionRole role) = 0;

    // 서버가 다른 서버로 outbound connect 할 때 사용할 세션을 파생 서버가 제공
    // 예: GatewayServer는 GatewayServerSession 풀에서 꺼냄
    virtual Session* AcquireOutboundSession(SOCKET s, SessionRole role) = 0;

    // 세션 반납 책임은 파생 서버가 가짐
    // 예: GatewayServer는 세션 kind를 보고 client/server pool 중 하나로 반환
    virtual void ReleaseSession(Session* session) = 0;

protected:
    virtual void OnClientConnected(Session* session) {}
    virtual void OnClientDisconnected(Session* session) {}
    virtual void OnRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) = 0;

public:
    void NotifySessionDisconnect(Session* s) override;
    void DispatchRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) override
    {
        OnRawPacket(session, header, payload, length);
    }

private:
    bool CreateListenSocket(uint16_t port);
    bool CreateIocp();
    void CreateWorkerThreads(int workerThreads);
    void DestroyWorkerThreads();

    void WorkerLoop();

    void PostInitialAccepts(int count);
    void PostAccept();

    void OnAcceptCompleted(OverlappedEx* ovl, DWORD bytes);
    void HandleRecvCompleted(OverlappedEx* ovl, DWORD bytes);
    void HandleSendCompleted(OverlappedEx* ovl, DWORD bytes);

private:
    IocpConfig config_;
    BufferPool bufferPool_;

    SOCKET listenSock_{ INVALID_SOCKET };
    HANDLE iocpHandle_{ nullptr };

    std::vector<std::thread> workers_;
    std::atomic<bool> running_{ false };

    SOCKET acceptSock_{ INVALID_SOCKET };
    OverlappedEx acceptOvl_;
    Buffer* acceptBuffer_{ nullptr };
};