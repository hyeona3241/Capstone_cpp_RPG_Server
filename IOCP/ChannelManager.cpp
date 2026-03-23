#include "ChannelManager.h"
#include "ChatSession.h"

void ChannelManager::InitDefaultChannels()
{
    CreateChannel(kLobbyChannelId, /*maxUsers*/ 200, /*needPassword*/ false);
    CreateChannel(2, /*maxUsers*/ 200, /*needPassword*/ false);
}

bool ChannelManager::CreateChannel(uint32_t channelId, uint32_t maxUsers, bool needPassword, const std::string& password)
{
    if (channelId == 0 || maxUsers == 0)
        return false;

    std::lock_guard<std::mutex> lock(mtx_);

    if (channels_.find(channelId) != channels_.end())
        return false;

    auto ch = std::make_unique<ChatChannel>(channelId, maxUsers);
    ch->SetNeedPassword(needPassword);
    if (needPassword)
        ch->SetPassword(password);

    channels_[channelId] = std::move(ch);
    return true;
}

ChatChannel* ChannelManager::FindChannel(uint32_t channelId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = channels_.find(channelId);
    return (it != channels_.end()) ? it->second.get() : nullptr;
}

bool ChannelManager::CloseChannel(uint32_t channelId, const void* closeNotify, size_t closeNotifyLen)
{
    if (channelId == 0)
        return false;

    // 로비는 닫지 않음(정책)
    if (channelId == kLobbyChannelId)
        return false;

    std::unique_ptr<ChatChannel> target;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = channels_.find(channelId);
        if (it == channels_.end())
            return false;

        target = std::move(it->second);
        channels_.erase(it);
    }

    target->Close(closeNotify, closeNotifyLen);
    return true;
}

JoinChannelResult ChannelManager::Join(uint32_t channelId, ChatSession* session, const std::string& passwordOpt)
{
    if (!session)
        return JoinChannelResult::Invalid;

    if (!session->IsAuthed())
        return JoinChannelResult::NotAuthed;

    if (session->IsInChannel())
        return JoinChannelResult::AlreadyInChannel;


    ChatChannel* ch = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = channels_.find(channelId);
        if (it == channels_.end())
            return JoinChannelResult::Invalid;
        ch = it->second.get();
    }

    const JoinChannelResult jr = ch->TryJoin(session, passwordOpt);
    if (jr != JoinChannelResult::Ok)
        return jr;

    session->TryEnterChannel(channelId);
    return JoinChannelResult::Ok;
}

JoinChannelResult  ChannelManager::Leave(ChatSession* session)
{
    if (!session)
        return JoinChannelResult::Invalid;

    if (!session->IsAuthed())
        return JoinChannelResult::NotAuthed;

    if (!session->IsInChannel())
        return JoinChannelResult::Invalid;

    const uint32_t cid = session->GetChannelId();
    if (cid == 0)
        return JoinChannelResult::Invalid;

    ChatChannel* ch = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = channels_.find(cid);
        if (it == channels_.end())
        {
            return session->TryLeaveChannel() ? JoinChannelResult::Ok : JoinChannelResult::Invalid;
        }
        ch = it->second.get();
    }

    const bool ok = ch->TryLeave(session);
    return ok ? JoinChannelResult::Ok : JoinChannelResult::Invalid;

}

bool ChannelManager::LeaveIfInChannel(ChatSession* session)
{
    if (!session)
        return false;

    if (!session->IsInChannel())
        return false;

    const JoinChannelResult jr = Leave(session);
    return jr == JoinChannelResult::Ok;
}

std::vector<ChannelInfo> ChannelManager::GetChannelListSnapshot() const
{
    std::vector<ChannelInfo> list;

    std::lock_guard<std::mutex> lock(mtx_);
    list.reserve(channels_.size());

    for (auto& kv : channels_)
    {
        ChatChannel* ch = kv.second.get();
        if (!ch) continue;

        ChannelInfo info;
        info.channelId = ch->GetChannelId();
        info.maxUsers = ch->GetMaxUsers();
        info.needPassword = ch->NeedPassword();
        info.currentUsers = ch->GetCurrentUsers();

        list.push_back(info);
    }

    return list;
}
