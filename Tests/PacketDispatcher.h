#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>

class PacketDispatcher
{
public:
    using HandlerFn = std::function<void(uint16_t packetId, const std::byte* payload, size_t payloadLen)>;

    void Register(uint16_t packetId, HandlerFn fn)
    {
        handlers_[packetId] = std::move(fn);
    }

    void Dispatch(uint16_t packetId, const std::byte* payload, size_t payloadLen)
    {
        auto it = handlers_.find(packetId);
        if (it != handlers_.end())
        {
            it->second(packetId, payload, payloadLen);
            return;
        }
        // 등록 안 된 패킷은 무시(또는 로그)
    }

private:
    std::unordered_map<uint16_t, HandlerFn> handlers_;
};