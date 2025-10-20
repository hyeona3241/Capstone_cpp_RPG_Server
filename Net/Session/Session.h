#pragma once
#include <winsock2.h>
#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <span>
#include <cstddef>
#include "Config/NetConfig.h"
#include "Util/BufferPool.h"
#include "RecvRing.h"
#include "Core/IocpCommon.h"

// 한 클라이언트 연결을 담당하는 세션

class Session {
private:
    // 소켓
    SOCKET sock_ = INVALID_SOCKET; 
    // 임시 수신 버퍼 풀
    Util::BufferPool& pool_;
    // 수신 임시 버퍼
    Util::BufferHandle rx_;
    // 누적 수신 링버퍼
    RecvRing rbuf_;

    // 송신 큐 보호
    std::mutex sendMtx_;
    // 송신 대기 큐
    std::deque<std::vector<std::byte>> sendQ_;
    // 현재 송신 중?
    std::atomic<bool> sending_{ false };
    // 종료 처리 중?
    std::atomic<bool> closing_{ false };

    // 세션 ID
    uint64_t id_{ 0 }; 

public:
    explicit Session(Util::BufferPool& pool)
        : pool_(pool), rbuf_(NetConfig::kRecvRingSize) {
    }

    // 수신 시작
    void Start();

    // 비동기 수신
    bool PostRecv();

    // 비동기 송신
    bool Send(std::span<const std::byte> payload);

    // IOCP 완료 콜백 (워커 스레드에서 사용)
    void OnIoCompleted(OverlappedEx* ov, DWORD bytes, BOOL ok);

    // 종료
    void Close();

    // 식별자/소켓 설정/조회자
    void set_id(uint64_t v) { id_ = v; } 
    uint64_t id() const { return id_; }
    void set_socket(SOCKET s) { sock_ = s; }
    SOCKET socket() const { return sock_; }

private:
    // 수신 완료 처리
    void OnRecvCompleted(size_t bytes);

    // 송신 완료 처리
    void OnSendCompleted(size_t bytes);

    // 링버퍼에서 패킷들을 가능한 만큼 꺼내기
    void ParseLoop();
};