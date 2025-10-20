#pragma once
#include <windows.h>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>
#include "Config/NetConfig.h" 
#include "IocpWorker.h" 
#include "SocketUtils.h" 
#include "Session/SessionManager.h" 
#include "App/INetListener.h" 

// 나중에 AcceptEx로 교체
class IocpServer
{
private:
    // IOCP 핸들
    HANDLE iocp_ = INVALID_HANDLE_VALUE;
    // 리슨 소켓
    SOCKET listen_ = INVALID_SOCKET;
    // 워커 스레드들
    std::vector<IocpWorker*> workers_; 
    // accept 전용 스레드
    std::thread acceptThread_;
    // 서버 동작 중인가
    std::atomic<bool> running_{ false };     

    // 세션 매니저(등록/해제)
    SessionManager* mgr_ = nullptr; 
    //// 상위 앱 콜백
    //INetListener* listener_ = nullptr;

public:
    IocpServer() = default; 
    ~IocpServer(); 

    // 서버 시작
    bool Start(const char* ip, uint16_t port, int workerCount, SessionManager* mgr, INetListener* listener);

    // 서버 정지
    void Stop();

private:
    // accept 전용 루프
    void AcceptLoop();

    // 소켓을 IOCP에 연결
    bool RegisterSocketToIocp(SOCKET s, void* completionKeyOwner);

};

