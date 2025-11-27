#pragma once
#include "Pch.h"

enum class IoType : uint8_t {
    Recv,
    Send,
    Accept,
    Connect,
    Quit
};

class Session;
class Buffer;

//오버랩드ex에서 버퍼 만들고 그 클래스를 인자로 가지고 있게 만들기 위해서 버퍼 먼저 만들기
struct OverlappedEx : public OVERLAPPED
{
    IoType   type{ IoType::Recv };
    Session* session{ nullptr }; 
    Buffer*  buffer{ nullptr }; // BufferPool에서 빌린 거
    WSABUF   wsaBuf{}; // Winsock에 넘길 버퍼 어댑터

    OverlappedEx();
    void Setup(IoType t, Session* s, Buffer* b);
    void ResetOverlapped();
};