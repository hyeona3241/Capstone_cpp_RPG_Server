#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <string>

#include "TcpClient.h"
#include "PacketDispatcher.h"

#include <unordered_map>

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

        WaitingChatConnectInfo,
        ChatConnecting,
        ChatAuthed,

        ChannelListPending,
        NeedChannelEnterInput,
        ChannelEnterPending,
        InChannelMenu, 
        InChannelChat,
        ChannelLeavePending,

        Done,
    };

    struct CachedChannelInfo
    {
        uint32_t channelId = 0;
        uint16_t userCount = 0;
        uint16_t maxUsers = 0;
        uint8_t needPassword = 0;
        std::string name;
    };

public:
    bool Start(const char* ip, uint16_t port);
    void Stop();

    void RunUntilDone();

private:
    void TryStartChatConnect();

    void RegisterHandlers();
    void RegisterChatHandlers();

    void SendPing();

    void InputLoop();

    void OnPingAck(const std::byte* payload, size_t len);
    void OnLoginAck(const std::byte* payload, size_t len);
    void OnRegisterAck(const std::byte* payload, size_t len);
    void OnChatConnectInfoAck(const std::byte* payload, size_t len);
    void OnChatAuthAck(const std::byte* payload, size_t len);

    void SendLogoutReqIfNeeded();

    void ResetChatLinkState();

    void OnChannelListAck(const std::byte* payload, size_t len);
    void SendChannelListReq();

    void SendChannelEnterReq(uint32_t channelId);
    void OnChannelEnterAck(const std::byte* payload, size_t len);

    void SendChannelLeaveReq();
    void OnChannelLeaveAck(const std::byte* payload, size_t len);

private:
    TcpClient client_;
    PacketDispatcher dispatcher_;

    TcpClient chatClient_;
    PacketDispatcher chatDispatcher_;

    std::atomic<State> state_{ State::Done };

    std::thread inputThread_;

    std::mutex inputMtx_;
    std::condition_variable inputCv_;

    uint64_t lastAccountId_ = 0;

    std::string myLoginId_;
    std::string pendingLoginId_;

    std::string chatIp_;
    uint16_t chatPort_ = 0;
    std::string chatToken_;

    // 채팅 접속 정보 도착 여부
    bool hasChatConnectInfo_ = false;

    // 채팅 연결/인증 중복 시작 방지
    bool chatConnectStarted_ = false;
    std::mutex chatStartMu_;


    std::unordered_map<uint32_t, CachedChannelInfo> channelCache_;

    uint32_t currentChannelId_ = 0;
    std::string currentChannelName_;

    std::atomic<bool> chatAuthed_{ false };
};

