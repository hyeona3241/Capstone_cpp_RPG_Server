#include "IocpCommon.h"
#include "Buffer.h"  
#include "Session.h"

OverlappedEx::OverlappedEx()
{
    ResetOverlapped();
    type = IoType::Recv;
    session = nullptr;
    buffer = nullptr;
    wsaBuf = {};
}

void OverlappedEx::ResetOverlapped()
{
    OVERLAPPED* ov = static_cast<OVERLAPPED*>(this);
    memset(ov, 0, sizeof(OVERLAPPED));
}

void OverlappedEx::Setup(IoType t, Session* s, Buffer* b)
{
    ResetOverlapped();

    type = t;
    session = s;
    buffer = b;

    if (!buffer)
    {
        wsaBuf.buf = nullptr;
        wsaBuf.len = 0;
        return;
    }

    switch (t)
    {
    case IoType::Recv:
        wsaBuf.buf = buffer->WritePtr();
        wsaBuf.len = (ULONG)buffer->WritableSize();
        break;

    case IoType::Send:
        wsaBuf.buf = buffer->Data();
        wsaBuf.len = (ULONG)buffer->Size();
        break;

    case IoType::Accept:
        wsaBuf.buf = buffer->Data();
        wsaBuf.len = (ULONG)buffer->Capacity();
        break;

    default:
        wsaBuf.buf = nullptr;
        wsaBuf.len = 0;
        break;
    }
}

void OverlappedEx::ResetAll()
{
    ResetOverlapped();

    type = IoType::Recv;
    session = nullptr;
    buffer = nullptr;
    wsaBuf.buf = nullptr;
    wsaBuf.len = 0;
}