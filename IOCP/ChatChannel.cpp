#include "ChatChannel.h"
#include "ChatSession.h"

ChatChannel::ChatChannel(uint32_t channelId, uint32_t maxUsers)
    : channelId_(channelId), maxUsers_(maxUsers)
{
}

uint32_t ChatChannel::GetCurrentUsers() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return static_cast<uint32_t>(members_.size());
}

bool ChatChannel::HasMember(uint64_t accountId) const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return members_.find(accountId) != members_.end();
}

JoinChannelResult ChatChannel::TryJoin(ChatSession* session, const std::string& passwordOpt)
{
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (closed_)
            return JoinChannelResult::CloseChannel;
    }

    if (!session)
        return JoinChannelResult::Invalid;

    // 세션이 인증 안 된 상태면 거절
    if (!session->IsAuthed())
        return JoinChannelResult::NotAuthed;

    // 세션이 이미 다른 채널에 들어가 있는지
    if (session->IsInChannel())
        return JoinChannelResult::AlreadyInChannel;

    // (나중에 만들기) 비번 채널 로직. 지금은 저장만 해두고 체크는 옵션으로 둠.
    if (needPassword_)
    {
        if (password_.empty())
            return JoinChannelResult::PasswordRequired; // 설정이 이상한 상태

        if (passwordOpt.empty())
            return JoinChannelResult::PasswordRequired;

        if (passwordOpt != password_)
            return JoinChannelResult::WrongPassword;
    }

    const uint64_t aid = session->GetAccountId();
    if (aid == 0)
        return JoinChannelResult::Invalid;

    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (IsFull_NoLock())
            return JoinChannelResult::ChannelFull;

        if (members_.find(aid) != members_.end())
            return JoinChannelResult::AlreadyInChannel; // 이미 목록에 있음

        // 채널 멤버 등록
        members_[aid] = session;
    }

    // 세션 상태 변경은 채널 밖에서
    if (!session->TryEnterChannel(channelId_))
    {
        // 세션 상태가 바뀌는 데 실패했다면 롤백
        std::lock_guard<std::mutex> lock(mtx_);
        members_.erase(aid);
        return JoinChannelResult::Invalid;
    }

    return JoinChannelResult::Ok;
}

bool ChatChannel::TryLeave(ChatSession* session)
{
    if (!session)
        return false;

    if (!session->IsAuthed())
        return false;

    const uint64_t aid = session->GetAccountId();
    if (aid == 0)
        return false;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = members_.find(aid);
        if (it == members_.end())
            return false;

        // 채널 멤버 제거
        members_.erase(it);
    }

    // 세션 상태 변경
    return session->TryLeaveChannel();
}

void ChatChannel::BroadcastRaw(const void* data, size_t len)
{
    if (!data || len == 0) return;

    std::vector<ChatSession*> snapshot;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (closed_) return;

        snapshot.reserve(members_.size());
        for (auto& kv : members_)
        {
            if (kv.second)
                snapshot.push_back(kv.second);
        }
    }

    for (ChatSession* s : snapshot)
    {
        s->Send(data, len);
    }
}

void ChatChannel::BroadcastRawExcept(ChatSession* exceptSession, const void* data, size_t len)
{
    if (!data || len == 0) return;

    std::vector<ChatSession*> snapshot;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        snapshot.reserve(members_.size());
        for (auto& kv : members_)
        {
            if (kv.second && kv.second != exceptSession)
                snapshot.push_back(kv.second);
        }
    }

    for (ChatSession* s : snapshot)
    {
        s->Send(data, len);
    }
}

bool ChatChannel::SendToMember(uint64_t toAccountId, const void* data, size_t len)
{
    if (toAccountId == 0 || !data || len == 0)
        return false;

    ChatSession* target = nullptr;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = members_.find(toAccountId);
        if (it == members_.end())
            return false;
        target = it->second;
    }

    if (!target || target->IsClosing())
        return false;

    target->Send(data, len);
    return true;
}

std::vector<uint64_t> ChatChannel::GetMemberAccountIdsSnapshot() const
{
    std::vector<uint64_t> ids;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        ids.reserve(members_.size());
        for (auto& kv : members_)
            ids.push_back(kv.first);
    }
    return ids;
}

void ChatChannel::Close(const void* notifyData, size_t notifyLen)
{
    std::vector<ChatSession*> snapshot;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (closed_)
            return;

        closed_ = true;

        snapshot.reserve(members_.size());
        for (auto& kv : members_)
        {
            if (kv.second)
                snapshot.push_back(kv.second);
        }

        members_.clear();
    }

    // 알림 브로드캐스트 (락 밖)
    if (notifyData && notifyLen > 0)
    {
        for (ChatSession* s : snapshot)
        {
            if (s && !s->IsClosing())
                s->Send(notifyData, notifyLen);
        }
    }

    for (ChatSession* s : snapshot)
    {
        if (s && !s->IsClosing())
        {
            s->TryLeaveChannel();
        }
    }
}
