#include "ClientApp.h"
#include <cstdio>
#include <iostream>
#include <string>

#include "PacketIds.h"
#include "CPingPackets.h"
#include "CLoginPackets.h"
#include "CRegisterPackets.h"
#include "CLogoutPackets.h"

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
    case ELoginResult::ALREADY_LOGGED_IN: return "ALREADY_LOGGED_IN";
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
    SendLogoutReqIfNeeded();

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

    dispatcher_.Register(PacketType::to_id(PacketType::Client::REGISTER_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnRegisterAck(payload, len);
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

    state_.store(State::SelectAuthMenu);
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
        std::printf("[CLIENT] accountId=%llu\n", static_cast<unsigned long long>(ack.accountId));

        myLoginId_ = pendingLoginId_;
        pendingLoginId_.clear();


        state_.store(State::AuthedMenu);
        inputCv_.notify_one();
    }
    else
    {
        pendingLoginId_.clear();
        state_.store(State::SelectAuthMenu);
        inputCv_.notify_one();
    }

}

void ClientApp::OnRegisterAck(const std::byte* payload, size_t len)
{
    RegisterAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] REGISTER_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] REGISTER_ACK resultCode=%u (%s)\n",
        ack.resultCode, ToLoginResultStr(ack.resultCode));

    if (ack.success)
    {
        std::printf("Register success. Please login.\n");

    }
    else
    {
        std::printf("Register failed.\n");
        std::printf("[CLIENT] REGISTER_ACK resultCode=%u (%s)\n",
            ack.resultCode, ToLoginResultStr(ack.resultCode));
    }

    state_.store(State::SelectAuthMenu);
    inputCv_.notify_all();
}

void ClientApp::SendLogoutReqIfNeeded()
{
    if (lastAccountId_ == 0)
        return;

    LogoutReq req;
    req.accountId = lastAccountId_;

    auto bytes = req.Build();
    client_.EnqueueSend(std::move(bytes));

    std::printf("[CLIENT] LOGOUT_REQ sent (accountId=%llu)\n",
        static_cast<unsigned long long>(lastAccountId_));
}

void ClientApp::InputLoop()
{
    while (state_.load() != State::Done)
    {
        State s;
        {
            std::unique_lock<std::mutex> lk(inputMtx_);
            inputCv_.wait(lk, [&]()
                {
                    auto st = state_.load();
                    return st == State::SelectAuthMenu || st == State::NeedLoginInput
                        || st == State::NeedRegisterInput || st == State::AuthedMenu || st == State::Done;
                });

            s = state_.load();
            if (s == State::Done)
                break;
        }

        if (s == State::SelectAuthMenu)
        {
            std::string line;
            std::cout << "1) Login\n2) Register\n> ";
            std::getline(std::cin, line);

            if (line == "1")
            {
                state_.store(State::NeedLoginInput);
                inputCv_.notify_one();
            }
            else if (line == "2")
            {
                state_.store(State::NeedRegisterInput);
                inputCv_.notify_one();
            }
            else
            {
                std::printf("[CLIENT] invalid input. choose 1 or 2.\n");
                // 메뉴 상태 유지
                state_.store(State::SelectAuthMenu);
                inputCv_.notify_one();
            }

            continue;
        }

        if (s == State::NeedLoginInput)
        {
            LoginReq req;
            std::cout << "loginId: ";
            std::getline(std::cin, req.loginId);
            std::cout << "password: ";
            std::getline(std::cin, req.plainPw);

            pendingLoginId_ = req.loginId;

            auto bytes = req.Build();
            client_.EnqueueSend(std::move(bytes));

            std::printf("[CLIENT] LOGIN_REQ sent\n");
            state_.store(State::SentLogin);
            continue;
        }

        if (s == State::NeedRegisterInput)
        {
            RegisterReq req;
            std::cout << "new loginId: ";
            std::getline(std::cin, req.loginId);
            std::cout << "new password: ";
            std::getline(std::cin, req.plainPw);

            auto bytes = req.Build();
            client_.EnqueueSend(std::move(bytes));

            std::printf("[CLIENT] REGISTER_REQ sent\n");
            state_.store(State::SentRegister);
            continue;
        }

        if (s == State::AuthedMenu)
        {
            std::string line;
            std::cout << "\n[Authed] myId=" << myLoginId_ << " accountId=" << lastAccountId_ << "\n";
            std::cout << "0) Logout\n> ";
            std::getline(std::cin, line);

            if (line == "0")
            {
                SendLogoutReqIfNeeded();

                lastAccountId_ = 0;
                myLoginId_.clear();
                pendingLoginId_.clear();

                std::printf("[CLIENT] Logout. Back to auth menu.\n");
                state_.store(State::SelectAuthMenu);
                inputCv_.notify_one();
            }
            else
            {
                std::printf("[CLIENT] invalid input. choose 0.\n");
                state_.store(State::AuthedMenu);
                inputCv_.notify_one();
            }
            continue;
        }

    }
}
