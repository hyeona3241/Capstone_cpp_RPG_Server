#include "ClientApp.h"
#include <cstdio>
#include <iostream>
#include <string>

#include "PacketIds.h"
#include "CPingPackets.h"
#include "CLoginPackets.h"

static const char* ToLoginResultStr(uint32_t code)
{
    switch (static_cast<ELoginResult>(code))
    {
    case ELoginResult::OK:               return "OK";
    case ELoginResult::NO_SUCH_USER:     return "NO_SUCH_USER";
    case ELoginResult::INVALID_PASSWORD: return "INVALID_PASSWORD";
    case ELoginResult::BANNED:           return "BANNED";
    case ELoginResult::DB_ERROR:         return "DB_ERROR";
    case ELoginResult::INTERNAL_ERROR:   return "INTERNAL_ERROR";
    default:                             return "UNKNOWN";
    }
}

bool ClientApp::Start(const char* ip, uint16_t port)
{
    dispatcher_ = PacketDispatcher{};
    RegisterHandlers();

    client_.SetDispatcher(&dispatcher_);

    if (!client_.Connect(ip, port))
        return false;

    state_.store(State::Connected);

    // 입력 스레드 시작
    inputThread_ = std::thread([this]() { InputLoop(); });

    SendPing();
    return true;
}

void ClientApp::Stop()
{
    state_.store(State::Done);
    inputCv_.notify_all();

    client_.Close();

    if (inputThread_.joinable())
        inputThread_.join();
}

void ClientApp::RunUntilDone()
{
    while (client_.IsRunning() && state_.load() != State::Done)
    {
        Sleep(10);
    }
    Stop();
}

void ClientApp::RegisterHandlers()
{
    dispatcher_.Register(PacketType::to_id(PacketType::Client::C_PING_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnPingAck(payload, len);
        });

    dispatcher_.Register(PacketType::to_id(PacketType::Client::LOGIN_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnLoginAck(payload, len);
        });
}

void ClientApp::SendPing()
{
    CPingReq req;
    req.seq = 1;

    auto bytes = req.Build();
    client_.EnqueueSend(std::move(bytes));

    std::printf("[CLIENT] C_PING_REQ sent\n");

    state_.store(State::WaitingPingAck);
}

void ClientApp::OnPingAck(const std::byte* payload, size_t len)
{
    CPingAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] PING_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] C_PING_ACK seq=%u tick=%llu\n",
        ack.seq,
        static_cast<unsigned long long>(ack.serverTick));

    state_.store(State::NeedLoginInput);
    inputCv_.notify_one();
}

void ClientApp::OnLoginAck(const std::byte* payload, size_t len)
{
    LoginAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] LOGIN_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] LOGIN_ACK resultCode=%u (%s)\n",
        ack.resultCode, ToLoginResultStr(ack.resultCode));

    if (ack.success)
    {
        lastAccountId_ = ack.accountId;
        std::printf("[CLIENT] accountId=%llu\n",
            static_cast<unsigned long long>(ack.accountId));
    }

    state_.store(State::Done);
    inputCv_.notify_all();
}

void ClientApp::InputLoop()
{
    while (state_.load() != State::Done)
    {
        {
            std::unique_lock<std::mutex> lk(inputMtx_);
            inputCv_.wait(lk, [&]()
                {
                    return state_.load() == State::NeedLoginInput || state_.load() == State::Done;
                });

            if (state_.load() == State::Done)
                break;
        }

        LoginReq req;
        std::cout << "loginId: ";
        std::getline(std::cin, req.loginId);
        std::cout << "password: ";
        std::getline(std::cin, req.plainPw);

        auto bytes = req.Build();
        client_.EnqueueSend(std::move(bytes));

        std::printf("[CLIENT] LOGIN_REQ sent\n");
        state_.store(State::SentLogin);
    }
}