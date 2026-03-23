#pragma once
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "RingBuffer.h"

#pragma pack(push, 1)
struct PacketHeader
{
    uint16_t size;
    uint16_t id;
};
#pragma pack(pop)

class RecvRing
{
public:
    explicit RecvRing(std::size_t ring_capacity, std::size_t max_packet_size = 4096)
        : ring_(ring_capacity), max_packet_size_(max_packet_size)
    {
    }

    void clear()
    {
        ring_.clear();
        error_ = false;
    }

    std::size_t buffered_size() const noexcept
    {
        return ring_.size();
    }

    bool has_error() const noexcept { return error_; }

    // 바이트 받기
    bool push(const void* data, std::size_t len)
    {
        if (!data || len == 0 || error_)
            return false;

        const auto* src = reinterpret_cast<const std::byte*>(data);

        if (!ring_.can_write(len))
        {
            error_ = true;
            return false;
        }

        ring_.write(src, len);
        return true;
    }

    // 패킷 하나 꺼냄
    bool try_pop_packet(std::vector<std::byte>& outPacket)
    {
        if (error_)
            return false;

        if (!ring_.can_read(sizeof(PacketHeader)))
            return false;

        PacketHeader header{};
        std::memset(&header, 0, sizeof(header));

        ring_.peek(reinterpret_cast<std::byte*>(&header), sizeof(header));

        const std::size_t total_len = header.size;

        if (total_len < sizeof(PacketHeader) || total_len > max_packet_size_)
        {
            // 말이 안 되는 길이 프로토콜 에러 처리
            error_ = true;
            return false;
        }

        if (!ring_.can_read(total_len))
            return false;

        outPacket.resize(total_len);
        ring_.read(outPacket.data(), total_len);

        return true;
    }


private:
    RingBuffer ring_;
    std::size_t max_packet_size_{ 0 };
    bool error_{ false };
};

