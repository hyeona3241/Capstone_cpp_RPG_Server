#pragma once
#include "Session.h"

#include <string>
#include <cstdint>

enum class ChatSessionState : uint8_t
{
    Connected,   // TCP 연결만 됨 (인증 전)
    Authed,      // 인증 완료
    Closing      // 끊기는 중/정리 중
};

class ChatSession : public Session
{
public:
    ChatSession() = default;
    ~ChatSession() override = default;

    ChatSessionState GetChatState() const { return chatState_; }
    bool IsAuthed() const { return chatState_ == ChatSessionState::Authed; }
    bool IsClosing() const { return chatState_ == ChatSessionState::Closing; }
    bool IsInChannel() const { return channelId_ != 0; }

    uint64_t GetAccountId() const { return accountId_; }
    const std::string& GetNickname() const { return nickname_; }
    uint32_t GetChannelId() const { return channelId_; }


    // 인증 성공 처리
    bool TryAuth(uint64_t accountId, const std::string& nickname);

    // 채널 입장/퇴장
    bool TryEnterChannel(uint32_t channelId);
    bool TryLeaveChannel();

    // 끊김 처리
    void MarkClosing();

    void ResetChatState();

private:
    ChatSessionState chatState_{ ChatSessionState::Connected };

    uint64_t    accountId_{ 0 };
    std::string nickname_;
    uint32_t    channelId_{ 0 }; // 0이면 미입장
};

