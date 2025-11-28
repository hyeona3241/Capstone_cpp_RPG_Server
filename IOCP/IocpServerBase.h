#pragma once

class Packet;

class IocpServerBase
{
public:
    virtual ~IocpServerBase() = default;

    virtual BufferPool* GetBufferPool() = 0;

    virtual void OnDisconnect(Session* session) = 0;
    virtual void OnPacket(Session* session, const Packet& pkt) = 0;
};

