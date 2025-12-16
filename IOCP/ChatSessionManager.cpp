#include "ChatSessionManager.h"
#include "ChatSession.h" 
#include <algorithm>

ChatSessionManager::~ChatSessionManager()
{
    Shutdown();
}

void ChatSessionManager::Shutdown()
{
    std::vector<ChatSession*> toDisconnect;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (shuttingDown_)
            return;

        shuttingDown_ = true;

        // 현재 등록된 세션들 모으기 
        CollectSessionsForDisconnect_NoLock(toDisconnect);

        bySessionId_.clear();
        byAccountId_.clear();
        pendingAuth_.clear();
    }

    for (ChatSession* s : toDisconnect)
    {
        if (s)
        {
            s->MarkClosing();
            s->Disconnect();
        }
    }
}

void ChatSessionManager::RegisterSession(ChatSession* session)
{
    if (!session) return;

    std::lock_guard<std::mutex> lock(mtx_);
    if (shuttingDown_) return;

    const uint64_t sid = session->GetId();
    bySessionId_[sid] = session;
}

void ChatSessionManager::UnregisterSession(ChatSession* session)
{
    if (!session) return;

    std::lock_guard<std::mutex> lock(mtx_);

    const uint64_t sid = session->GetId();
    auto it = bySessionId_.find(sid);
    if (it != bySessionId_.end() && it->second == session)
        bySessionId_.erase(it);

    // accountId 맵에서도 제거
    const uint64_t aid = session->GetAccountId();
    if (aid != 0)
    {
        auto it2 = byAccountId_.find(aid);
        if (it2 != byAccountId_.end() && it2->second == session)
            byAccountId_.erase(it2);
    }
}

ChatSession* ChatSessionManager::FindByChatSessionId(uint64_t chatSessionId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = bySessionId_.find(chatSessionId);
    return (it != bySessionId_.end()) ? it->second : nullptr;
}

ChatSession* ChatSessionManager::FindByAccountId(uint64_t accountId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = byAccountId_.find(accountId);
    return (it != byAccountId_.end()) ? it->second : nullptr;
}

bool ChatSessionManager::BindAccount(ChatSession* session, uint64_t accountId, const std::string& nickname)
{
    if (!session) return false;
    if (accountId == 0 || nickname.empty()) return false;

    ChatSession* oldSessionToKick = nullptr;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (shuttingDown_) return false;

        // 중복 접속 처리: 같은 accountId가 이미 있으면 기존 세션 킥
        auto it = byAccountId_.find(accountId);
        if (it != byAccountId_.end() && it->second != session)
        {
            oldSessionToKick = it->second;
        }

        byAccountId_[accountId] = session;
    }

    if (oldSessionToKick)
    {
        oldSessionToKick->MarkClosing();
        oldSessionToKick->Disconnect();
    }

    return session->TryAuth(accountId, nickname);
}

void ChatSessionManager::OnSessionDisconnected(ChatSession* session)
{
    // 서버 owner->NotifySessionDisconnect(...)에서 호출된다고 가정
    // 여기서는 맵 제거만 담당. 채널 Leave 같은 건 여기서 하거나,
    // ChatServer 쪽에서 ChannelManager를 같이 호출해도 됨.
    UnregisterSession(session);
}

void ChatSessionManager::RegisterPendingAuth(const std::string& token, uint64_t accountId, const std::string& nickname, uint32_t ttlSeconds)
{
    if (token.empty()) return;
    if (accountId == 0) return;
    if (nickname.empty()) return;

    const auto now = std::chrono::steady_clock::now();
    PendingAuth pa;
    pa.accountId = accountId;
    pa.nickname = nickname;
    pa.expireAt = now + std::chrono::seconds(ttlSeconds);

    std::lock_guard<std::mutex> lock(mtx_);
    if (shuttingDown_) return;

    pendingAuth_[token] = std::move(pa);
}

bool ChatSessionManager::ConsumePendingAuth(const std::string& token, uint64_t& outAccountId, std::string& outNickname)
{
    outAccountId = 0;
    outNickname.clear();

    if (token.empty()) return false;

    const auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mtx_);
    if (shuttingDown_) return false;

    auto it = pendingAuth_.find(token);
    if (it == pendingAuth_.end())
        return false;

    // 만료 체크
    if (it->second.expireAt < now)
    {
        pendingAuth_.erase(it);
        return false;
    }

    outAccountId = it->second.accountId;
    outNickname = it->second.nickname;

    // 1회용 토큰 소모
    pendingAuth_.erase(it);
    return true;
}

void ChatSessionManager::CleanupExpiredPendingAuth()
{
    const auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(mtx_);
    if (shuttingDown_) return;

    for (auto it = pendingAuth_.begin(); it != pendingAuth_.end(); )
    {
        if (it->second.expireAt < now)
            it = pendingAuth_.erase(it);
        else
            ++it;
    }
}

void ChatSessionManager::CollectSessionsForDisconnect_NoLock(std::vector<ChatSession*>& out)
{
    out.reserve(bySessionId_.size());
    for (auto& kv : bySessionId_)
    {
        if (kv.second)
            out.push_back(kv.second);
    }

    // 혹시 중복 포인터가 들어갈 수 있는 상황 방어
    std::sort(out.begin(), out.end());
    out.erase(std::unique(out.begin(), out.end()), out.end());
}