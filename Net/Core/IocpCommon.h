#pragma once
#include <windows.h> 
#include <cstdint> 
#include <utility>

enum class IoType : uint8_t {
    Recv, 
    Send, 
    Accept,
    Connect,
    Quit 
};

struct OverlappedEx : OVERLAPPED {
    IoType type;   // IO 종류
    void* owner;  // 소유 객체(Session, Acceptor 등)

    OverlappedEx(IoType t, void* o) : type(t), owner(o) {
        ::ZeroMemory(static_cast<OVERLAPPED*>(this), sizeof(OVERLAPPED));
    }
};

// OverlappedEx 만들어서 반환
OverlappedEx* MakeOv(IoType t, void* owner) noexcept;

// OverlappedEx 해제
void FreeOv(OverlappedEx* ov) noexcept;

// OVERLAPPED를 OverlappedEx로 변환
inline OverlappedEx* AsOv(OVERLAPPED* ov) noexcept {
    return static_cast<OverlappedEx*>(ov);
}

// 소유자 포인터
inline void* OvOwner(OverlappedEx* ov) noexcept { return ov ? ov->owner : nullptr; }

// 워커 종료를 위해 IOCP 큐에 넣기
bool PostQuitToIocp(HANDLE iocp) noexcept;

// 유효한 핸들인지
inline bool IsValidHandle(HANDLE h) noexcept {
    return h && h != INVALID_HANDLE_VALUE;
}