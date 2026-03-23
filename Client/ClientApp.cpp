#include "ClientApp.h"
#include <cstdio>
#include <iostream>
#include <string>

#include "PacketIds.h"
#include "CPingPackets.h"
#include "CLoginPackets.h"
#include "CRegisterPackets.h"
#include "CLogoutPackets.h"
#include "CCSAuthPackets.h"
#include "CChatChannelPackets.h"
#include "CChatMessagePackets.h"

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

    chatDispatcher_ = PacketDispatcher{};
    RegisterChatHandlers();

    client_.SetDispatcher(&dispatcher_);
    chatClient_.SetDispatcher(&chatDispatcher_);

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

    chatClient_.Close();
    ResetChatLinkState();

    client_.Close();

    if (inputThread_.joinable())
        inputThread_.join();
}

void ClientApp::RunUntilDone()
{
    while ((client_.IsRunning() || chatClient_.IsRunning()) && state_.load() != State::Done)
    {
        Sleep(10);
    }
    Stop();
}

void ClientApp::TryStartChatConnect()
{
    std::string ip, token, myId;
    uint16_t port = 0;

    {
        std::lock_guard<std::mutex> lk(chatStartMu_);

        if (chatConnectStarted_) return;
        if (!hasChatConnectInfo_) return;
        if (myLoginId_.empty()) return;

        chatConnectStarted_ = true;

        ip = chatIp_;
        port = chatPort_;
        token = chatToken_;
        myId = myLoginId_;
    }

    std::printf("[CLIENT] TryStartChatConnect. ip=%s port=%u myId=%s tokenPrefix=%s\n",
        ip.c_str(),
        (unsigned)port,
        myId.c_str(),
        token.substr(0, 6).c_str());

    state_.store(State::ChatConnecting);

    if (!chatClient_.Connect(ip.c_str(), port))
    {
        std::printf("[CLIENT][ERROR] Chat connect failed ip=%s port=%u\n",
            ip.c_str(), (unsigned)port);

        state_.store(State::AuthedMenu);
        inputCv_.notify_one();
        return;
    }

    std::printf("[CLIENT] Chat connected. sending CL_CS_CHAT_AUTH_REQ...\n");

    CLCSChatAuthReq req;
    req.token = token;
    req.loginId = myId;

    auto bytes = req.Build();
    chatClient_.EnqueueSend(std::move(bytes));

    std::printf("[CLIENT] CL_CS_CHAT_AUTH_REQ sent loginId=%s tokenPrefix=%s\n",
        myId.c_str(),
        token.substr(0, 6).c_str());
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

    dispatcher_.Register(PacketType::to_id(PacketType::Client::MS_CL_CHAT_CONNECT_INFO_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnChatConnectInfoAck(payload, len);
        });
}

void ClientApp::RegisterChatHandlers()
{
    chatDispatcher_.Register(PacketType::to_id(PacketType::Chat::CS_CL_CHAT_AUTH_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnChatAuthAck(payload, len);
        });

    chatDispatcher_.Register(PacketType::to_id(PacketType::Chat::CS_CL_CHANNEL_LIST_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnChannelListAck(payload, len);
        });

    chatDispatcher_.Register(PacketType::to_id(PacketType::Chat::CS_CL_CHANNEL_ENTER_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnChannelEnterAck(payload, len);
        });

    chatDispatcher_.Register(PacketType::to_id(PacketType::Chat::CS_CL_CHANNEL_LEAVE_ACK),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            OnChannelLeaveAck(payload, len);
        });

    chatDispatcher_.Register(PacketType::to_id(PacketType::Chat::CS_CL_CHAT_BROADCAST_NFY),
        [this](uint16_t, const std::byte* payload, size_t len)
        {
            CSCLChatBroadcastNfy nfy;
            if (!nfy.ParsePayload(payload, len))
            {
                std::printf("[CLIENT][WARN] CHAT_BROADCAST parse failed\n");
                return;
            }
            
            std::printf("[Channel %u] %s: %s\n",
                nfy.channelId,
                nfy.senderLoginId.c_str(),
                nfy.message.c_str());
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

        state_.store(State::WaitingChatConnectInfo);

        /*state_.store(State::AuthedMenu);
        inputCv_.notify_one();*/

        TryStartChatConnect();
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

void ClientApp::ResetChatLinkState()
{
    std::lock_guard<std::mutex> lk(chatStartMu_);
    hasChatConnectInfo_ = false;
    chatConnectStarted_ = false;

    chatIp_.clear();
    chatPort_ = 0;
    chatToken_.clear();

    chatAuthed_.store(false);
}

void ClientApp::OnChannelListAck(const std::byte* payload, size_t len)
{

    CSCLChannelListAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] CS_CL_CHANNEL_LIST_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] CHANNEL_LIST_ACK success=%u resultCode=%u count=%zu\n",
        ack.success, ack.resultCode, ack.channels.size());

    if (!ack.success)
        return;

    channelCache_.clear();
    for (const auto& c : ack.channels)
    {
        CachedChannelInfo info;
        info.channelId = c.channelId;
        info.userCount = c.userCount;
        info.maxUsers = c.maxUsers;
        info.needPassword = c.needPassword;
        info.name = c.name;
        channelCache_[c.channelId] = std::move(info);
    }

    for (const auto& c : ack.channels)
    {
        std::printf(" - channelId=%u users=%u/%u pw=%u name=%s\n",
            c.channelId,
            (unsigned)c.userCount,
            (unsigned)c.maxUsers,
            (unsigned)c.needPassword,
            c.name.c_str());
    }

    state_.store(State::AuthedMenu);
    inputCv_.notify_one();
}

void ClientApp::SendChannelListReq()
{
    if (!chatAuthed_.load())
    {
        std::printf("[CLIENT][WARN] Cannot request channel list. chat not authed.\n");
        return;
    }

    CLCSChannelListReq req;
    auto bytes = req.Build();
    chatClient_.EnqueueSend(std::move(bytes));

    std::printf("[CLIENT] CL_CS_CHANNEL_LIST_REQ sent\n");
}

void ClientApp::SendChannelEnterReq(uint32_t channelId)
{
    if (!chatAuthed_.load())
    {
        std::printf("[CLIENT][WARN] Cannot enter channel. chat is not authed yet.\n");
        return;
    }

    CLCSChannelEnterReq req;
    req.channelId = channelId;

    auto bytes = req.Build();
    chatClient_.EnqueueSend(std::move(bytes));

    std::printf("[CLIENT] CL_CS_CHANNEL_ENTER_REQ sent. channelId=%u\n", channelId);
}

void ClientApp::OnChannelEnterAck(const std::byte* payload, size_t len)
{
    CSCLChannelEnterAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] CS_CL_CHANNEL_ENTER_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] CHANNEL_ENTER_ACK success=%u resultCode=%u channelId=%u\n",
        ack.success, ack.resultCode, ack.channelId);

    if (!ack.success)
    {
        // 실패하면 다시 메뉴로
        state_.store(State::AuthedMenu);
        inputCv_.notify_one();
        return;
    }

    currentChannelId_ = ack.channelId;

    // 채널 이름/정보 출력(리스트 캐시가 있으면 더 예쁘게)
    auto it = channelCache_.find(currentChannelId_);
    if (it != channelCache_.end())
    {
        const auto& c = it->second;
        currentChannelName_ = c.name;

        std::printf("\n[CLIENT] Entered Channel!\n");
        std::printf("  name=%s\n", c.name.c_str());
        std::printf("  channelId=%u\n", c.channelId);
        std::printf("  users=%u/%u\n", (unsigned)c.userCount, (unsigned)c.maxUsers);
        std::printf("  password=%u\n\n", (unsigned)c.needPassword);
    }
    else
    {
        currentChannelName_ = "Unknown";
        std::printf("\n[CLIENT] Entered Channel! channelId=%u\n\n", currentChannelId_);
    }

    state_.store(State::InChannelChat);
    inputCv_.notify_one();
}

void ClientApp::SendChannelLeaveReq()
{
    if (!chatAuthed_.load())
    {
        std::printf("[CLIENT][WARN] Cannot leave channel. chat not authed.\n");
        return;
    }

    if (currentChannelId_ == 0)
    {
        std::printf("[CLIENT][WARN] Cannot leave channel. not in channel.\n");
        return;
    }

    CLCSChannelLeaveReq req;
    auto bytes = req.Build();
    chatClient_.EnqueueSend(std::move(bytes));

    std::printf("[CLIENT] CL_CS_CHANNEL_LEAVE_REQ sent. cid=%u\n", currentChannelId_);
}

void ClientApp::OnChannelLeaveAck(const std::byte* payload, size_t len)
{
    CSCLChannelLeaveAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] CS_CL_CHANNEL_LEAVE_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] CHANNEL_LEAVE_ACK success=%u resultCode=%u channelId=%u\n",
        ack.success, ack.resultCode, ack.channelId);

    if (!ack.success)
    {
        state_.store(State::InChannelChat);
        inputCv_.notify_one();
        return;
    }

    currentChannelId_ = 0;
    currentChannelName_.clear();

    state_.store(State::AuthedMenu);
    inputCv_.notify_one();
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
                    return st == State::SelectAuthMenu
                        || st == State::NeedLoginInput
                        || st == State::NeedRegisterInput
                        || st == State::AuthedMenu
                        || st == State::NeedChannelEnterInput
                        || st == State::ChannelListPending
                        || st == State::ChannelEnterPending
                        || st == State::InChannelMenu
                        || st == State::InChannelChat
                        || st == State::ChannelLeavePending
                        || st == State::Done;
                });

            s = state_.load();
            if (s == State::Done)
                break;
        }

        /* =========================
           1. 인증 선택 메뉴
           ========================= */
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
                state_.store(State::SelectAuthMenu);
                inputCv_.notify_one();
            }
            continue;
        }

        /* =========================
           2. 로그인 입력
           ========================= */
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

        /* =========================
           3. 회원가입 입력
           ========================= */
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

        /* =========================
           4. 로그인 이후 메뉴
           ========================= */
        if (s == State::AuthedMenu)
        {
            std::string line;
            std::cout << "\n[Authed] myId=" << myLoginId_
                << " accountId=" << lastAccountId_ << "\n";
            std::cout << "1) Channel List\n2) Enter Channel\n0) Logout\n> ";
            std::getline(std::cin, line);

            if (line == "1")
            {
                SendChannelListReq();
                state_.store(State::ChannelListPending);
                continue;
            }
            else if (line == "2")
            {
                state_.store(State::NeedChannelEnterInput);
                inputCv_.notify_one();
                continue;
            }
            else if (line == "0")
            {
                SendLogoutReqIfNeeded();

                lastAccountId_ = 0;
                myLoginId_.clear();
                pendingLoginId_.clear();

                chatClient_.Close();
                ResetChatLinkState();

                std::printf("[CLIENT] Logout. Back to auth menu.\n");
                state_.store(State::SelectAuthMenu);
                inputCv_.notify_one();
                continue;
            }
            else
            {
                std::printf("[CLIENT] invalid input.\n");
                state_.store(State::AuthedMenu);
                inputCv_.notify_one();
                continue;
            }
        }

        /* =========================
           5. 채널 ID 입력
           ========================= */
        if (s == State::NeedChannelEnterInput)
        {
            std::string line;
            std::cout << "Enter channelId: ";
            std::getline(std::cin, line);

            uint32_t cid = 0;
            try { cid = static_cast<uint32_t>(std::stoul(line)); }
            catch (...) { cid = 0; }

            if (cid == 0)
            {
                std::printf("[CLIENT] invalid channelId.\n");
                state_.store(State::AuthedMenu);
                inputCv_.notify_one();
                continue;
            }

            SendChannelEnterReq(cid);
            state_.store(State::ChannelEnterPending);
            continue;
        }

        /* =========================
           6. 채널 목록 응답 대기
           ========================= */
        if (s == State::ChannelListPending)
        {
            std::unique_lock<std::mutex> lk(inputMtx_);
            inputCv_.wait(lk, [this]()
                {
                    auto st = state_.load();
                    return st != State::ChannelListPending && st != State::Done;
                });
            continue;
        }

        /* =========================
           7. 채널 입장 응답 대기
           ========================= */
        if (s == State::ChannelEnterPending)
        {
            std::unique_lock<std::mutex> lk(inputMtx_);
            inputCv_.wait(lk, [this]()
                {
                    auto st = state_.load();
                    return st != State::ChannelEnterPending && st != State::Done;
                });
            continue;
        }

        /* =========================
           8. 채널 내부 메뉴
           ========================= */
        if (s == State::InChannelMenu)
        {
            std::string line;
            std::cout << "\n[Channel] id=" << currentChannelId_
                << " name=" << currentChannelName_ << "\n";
            std::cout << "0) Leave (TODO)\n> ";
            std::getline(std::cin, line);

            if (line == "0")
            {
                std::printf("[CLIENT] Leave channel not implemented yet.\n");
                state_.store(State::InChannelMenu);
                inputCv_.notify_one();
            }
            continue;
        }

        /* =========================
           9. 채팅
           ========================= */
        if (s == State::InChannelChat)
        {
            std::cout << "\n[Channel " << currentChannelId_ << " | " << currentChannelName_ << "]\n";
            std::cout << "Type message and press Enter. (0 to exit)\n";

            while (state_.load() == State::InChannelChat)
            {
                std::string msg;
                std::cout << "> ";
                std::getline(std::cin, msg);

                // 종료 (나중에 leave 패킷으로 바꾸면 됨)
                if (msg == "0")
                {
                    std::printf("[CLIENT] Leaving channel...\n");
                    SendChannelLeaveReq();
                    state_.store(State::ChannelLeavePending);
                    break;
                }

                if (msg.empty())
                    continue;

                // 메시지 전송
                CLCSChatSendReq req;
                req.channelId = currentChannelId_;
                req.message = msg;

                auto bytes = req.Build();
                chatClient_.EnqueueSend(std::move(bytes));
            }

            continue;
        }

        if (s == State::ChannelLeavePending)
        {
            std::unique_lock<std::mutex> lk(inputMtx_);
            inputCv_.wait(lk, [this]()
                {
                    auto st = state_.load();
                    return st != State::ChannelLeavePending && st != State::Done;
                });
            continue;
        }
    }
}

void ClientApp::OnChatConnectInfoAck(const std::byte* payload, size_t len)
{
    ChatConnectInfoAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] MS_CL_CHAT_CONNECT_INFO_ACK parse failed\n");
        return;
    }

    chatIp_ = ack.chatIp;
    chatPort_ = static_cast<uint16_t>(ack.chatPort);
    chatToken_ = ack.token;
    hasChatConnectInfo_ = true;

    std::printf("[CLIENT] CHAT_CONNECT_INFO ip=%s port=%u tokenPrefix=%s\n",
        chatIp_.c_str(),
        (unsigned)chatPort_,
        chatToken_.substr(0, 6).c_str());

    TryStartChatConnect();
}

void ClientApp::OnChatAuthAck(const std::byte* payload, size_t len)
{
    CSCLChatAuthAck ack;
    if (!ack.ParsePayload(payload, len))
    {
        std::printf("[CLIENT][WARN] CS_CL_CHAT_AUTH_ACK parse failed\n");
        return;
    }

    std::printf("[CLIENT] CHAT_AUTH_ACK success=%u resultCode=%u\n",
        ack.success, ack.resultCode);

    if (ack.success)
    {
        std::printf("[CLIENT] Chat authed. aid=%llu nickname=%s\n",
            (unsigned long long)ack.accountId,
            ack.nickname.c_str());

        chatAuthed_.store(true);

        state_.store(State::AuthedMenu);
        inputCv_.notify_one();
    }
    else
    {
        std::printf("[CLIENT][WARN] Chat auth failed. resultCode=%u\n", ack.resultCode);
        chatAuthed_.store(false);

        // 실패하면 채팅 연결 닫고, AuthedMenu로 보내서 로그아웃은 가능하게
        chatClient_.Close();
        ResetChatLinkState();

        state_.store(State::AuthedMenu);
        inputCv_.notify_one();
    }
}
