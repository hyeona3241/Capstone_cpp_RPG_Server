#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>

#include "TcpClient.h"
#include "PacketDispatcher.h"

class ClientApp
{
public:
    enum class State : uint32_t
    {
        Connected = 0,
        SentPing,
        WaitingPingAck,
        NeedLoginInput,
        SentLogin,
        Done,
    };

public:
    bool Start(const char* ip, uint16_t port);
    void Stop();

    void RunUntilDone();

private:
    void RegisterHandlers();
    void SendPing();

    void InputLoop();

    // handlers
    void OnPingAck(const std::byte* payload, size_t len);
    void OnLoginAck(const std::byte* payload, size_t len);

private:
    TcpClient client_;
    PacketDispatcher dispatcher_;

    std::atomic<State> state_{ State::Done };

    std::thread inputThread_;

    std::mutex inputMtx_;
    std::condition_variable inputCv_;

    uint64_t lastAccountId_ = 0;
};

