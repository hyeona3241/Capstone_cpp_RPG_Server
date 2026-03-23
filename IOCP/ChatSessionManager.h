#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>
#include <chrono>

class ChatSession;

struct PendingAuth
{
    uint64_t accountId = 0;
    std::chrono::steady_clock::time_point expireAt;
};

class ChatSessionManager
{
public:
    ChatSessionManager() = default;
    ~ChatSessionManager();

    void Shutdown();

    void RegisterSession(ChatSession* session);
    void UnregisterSession(ChatSession* session);

    ChatSession* FindByChatSessionId(uint64_t chatSessionId);
    ChatSession* FindByAccountId(uint64_t accountId);

    // 인증 완료 후 유저 인덱스에 올리기(중복 접속 처리 포함)
    bool BindAccount(ChatSession* session, uint64_t accountId, const std::string& nickname);

    // 세션 끊길 때(NotifySessionDisconnect에서 호출) 정리용
    void OnSessionDisconnected(ChatSession* session);

    void RegisterPendingAuth(const std::string& token, uint64_t accountId,  uint32_t ttlSeconds);

    // 성공하면 pending에서 제거(1회용)
    bool ConsumePendingAuth(const std::string& token,  uint64_t& outAccountId);

    // 만료된 pending 정리 (타이머 스레드 없으면 패킷 처리 시 가끔 호출해도 됨)
    void CleanupExpiredPendingAuth();

private:
    // 내부: 중복 접속 킥이 필요할 때, 락 밖에서 Disconnect 하기 위해 포인터를 모아둠
    void CollectSessionsForDisconnect_NoLock(std::vector<ChatSession*>& out);

private:
    std::mutex mtx_;
    bool shuttingDown_ = false;

    // ChatServer 내부 세션 ID(Session::GetId()) -> ChatSession*
    std::unordered_map<uint64_t, ChatSession*> bySessionId_;

    // accountId -> ChatSession*
    std::unordered_map<uint64_t, ChatSession*> byAccountId_;

    // token -> PendingAuth
    std::unordered_map<std::string, PendingAuth> pendingAuth_;
};

