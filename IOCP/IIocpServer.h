#pragma once
#include <cstddef>
#include <cstdint>

class Session;
class Buffer;
class BufferPool;
struct PacketHeader;

class IIocpServer
{
public:
    virtual ~IIocpServer() = default;

    virtual BufferPool* GetBufferPool() = 0;
    virtual void NotifySessionDisconnect(Session* s) = 0;
    virtual void DispatchRawPacket(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) = 0;
};