#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <string>

#include "TcpClient.h"
#include "PacketDispatcher.h"

class ClientApp
{
public:
    enum class State
    {
        Connected,
        WaitingPingAck,
        SelectAuthMenu,
        NeedLoginInput,
        NeedRegisterInput,
        SentLogin,
        SentRegister,

        AuthedMenu,

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

    void OnPingAck(const std::byte* payload, size_t len);
    void OnLoginAck(const std::byte* payload, size_t len);
    void OnRegisterAck(const std::byte* payload, size_t len);

    void SendLogoutReqIfNeeded();

private:
    TcpClient client_;
    PacketDispatcher dispatcher_;

    std::atomic<State> state_{ State::Done };

    std::thread inputThread_;

    std::mutex inputMtx_;
    std::condition_variable inputCv_;

    uint64_t lastAccountId_ = 0;

    std::string myLoginId_;
    std::string pendingLoginId_;
};

