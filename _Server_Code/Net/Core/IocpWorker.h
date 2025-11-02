#pragma once
#include <thread>
#include <atomic>

#include "Pch.h"

class IocpWorker
{
private:
    // IOCP 핸들
    HANDLE iocp_ = INVALID_HANDLE_VALUE; 
    // 루프 동작 중인지
    std::atomic<bool> running_{ false };  
    // 워커 스레드 객체
    std::thread th_;     

public:
    explicit IocpWorker(HANDLE iocp) : iocp_(iocp) {}
    ~IocpWorker();

    // 워커 스레드 시작(이미 시작되어 있으면 무시)
    void Start();

    // 워커 스레드 종료 요청(큐에 종료 신호를 넣고 조인)
    void Stop();

private:
    // GQCS 루프: 완료 이벤트를 기다렸다가 소유자(Session 등)로 전달
    void Loop();

};

