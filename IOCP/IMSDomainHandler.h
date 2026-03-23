#pragma once
#include <cstddef>
#include <cstdint>

class Session;
struct PacketHeader;

class IMSDomainHandler
{
public:
    virtual ~IMSDomainHandler() = default;

    // 처리했으면 true, 내 담당이 아니면 false
    virtual bool Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length) = 0;
};

