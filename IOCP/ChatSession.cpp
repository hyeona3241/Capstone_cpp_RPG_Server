#include "ChatSession.h"

bool ChatSession::TryAuth(uint64_t accountId, const std::string& nickname)
{
    if (IsClosing())
        return false;

    // 이미 인증된 상태에서 재인증 시도 방지
    if (chatState_ == ChatSessionState::Authed)
        return false;

    if (accountId == 0)
        return false;

    if (nickname.empty())
        return false;

    accountId_ = accountId;
    nickname_ = nickname;

    chatState_ = ChatSessionState::Authed;
    return true;
}

bool ChatSession::TryEnterChannel(uint32_t channelId)
{
    if (IsClosing())
        return false;

    if (!IsAuthed())
        return false;

    if (channelId == 0)
        return false;

    // 이미 채널에 있으면 중복 입장 방지
    if (channelId_ != 0)
        return false;

    channelId_ = channelId;
    return true;
}

bool ChatSession::TryLeaveChannel()
{
    if (IsClosing())
        return false;

    if (!IsAuthed())
        return false;

    // 채널에 없는데 퇴장 요청 방지
    if (channelId_ == 0)
        return false;

    channelId_ = 0;
    return true;
}

void ChatSession::MarkClosing()
{
    chatState_ = ChatSessionState::Closing;
}

void ChatSession::ResetChatState()
{
    chatState_ = ChatSessionState::Connected;
    accountId_ = 0;
    nickname_.clear();
    channelId_ = 0;
}