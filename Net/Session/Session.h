#pragma once
#include <winsock2.h>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>

// IOCP 조립할때 OverlappedEx로 교체
struct Session {
    SOCKET sock = INVALID_SOCKET;            // 클라이언트 소켓
    std::queue<std::vector<char>> sendQueue; // 송신 대기 큐
    std::mutex sendMtx;                      // 송신 큐 보호용 뮤텍스
    std::atomic<bool> sending{ false }; // 현재 송신 중인지
    std::atomic<bool> closing{ false }; // 종료 처리 중인지
    uint64_t id = 0;                    // 세션 고유 ID
    std::atomic<bool> inUse{ false };   // 풀에서 사용 중인지

    // 풀에서 재사용할 때 기본 상태로 되돌려줄 때 사용
    void ResetForReuse() {
        sock = INVALID_SOCKET;
        sending = false;
        closing = false;

        // 큐 비우기
        while (!sendQueue.empty()) sendQueue.pop();
    }
};