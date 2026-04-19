#pragma once
#include <cstdint>
#include <atomic>

class GatewayStateMonitor
{
public:
    void OnPacketRecv();
    void OnPacketSend();

    uint64_t ConsumeRecvTick();
    uint64_t ConsumeSendTick();

private:
    std::atomic<uint64_t> packetsRecvTick_{ 0 };
    std::atomic<uint64_t> packetsSentTick_{ 0 };
    std::atomic<uint64_t> packetsRecvTotal_{ 0 };
    std::atomic<uint64_t> packetsSentTotal_{ 0 };
};

