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
    IoType type;   // IO ����
    void* owner;  // ���� ��ü(Session, Acceptor ��)

    OverlappedEx(IoType t, void* o) : type(t), owner(o) {
        ::ZeroMemory(static_cast<OVERLAPPED*>(this), sizeof(OVERLAPPED));
    }
};

// OverlappedEx ���� ��ȯ
OverlappedEx* MakeOv(IoType t, void* owner) noexcept;

// OverlappedEx ����
void FreeOv(OverlappedEx* ov) noexcept;

// OVERLAPPED�� OverlappedEx�� ��ȯ
inline OverlappedEx* AsOv(OVERLAPPED* ov) noexcept {
    return static_cast<OverlappedEx*>(ov);
}

// ������ ������
inline void* OvOwner(OverlappedEx* ov) noexcept { return ov ? ov->owner : nullptr; }

// ��Ŀ ���Ḧ ���� IOCP ť�� �ֱ�
bool PostQuitToIocp(HANDLE iocp) noexcept;

// ��ȿ�� �ڵ�����
inline bool IsValidHandle(HANDLE h) noexcept {
    return h && h != INVALID_HANDLE_VALUE;
}