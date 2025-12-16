#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>

class ChatSession;

enum class JoinChannelResult : uint8_t
{
    Ok = 0,
    NotAuthed,
    AlreadyInChannel,
    ChannelFull,
    PasswordRequired,
    WrongPassword,
    Invalid,
    CloseChannel,
};

class ChatChannel
{
public:
    ChatChannel(uint32_t channelId, uint32_t maxUsers);

    uint32_t GetChannelId() const { return channelId_; }
    uint32_t GetMaxUsers()  const { return maxUsers_; }

    bool NeedPassword() const { return needPassword_; }
    void SetNeedPassword(bool v) { needPassword_ = v; }
    void SetPassword(const std::string& pw) { password_ = pw; } // 로직은 나중에

    uint32_t GetCurrentUsers() const; // members_.size()

public:
    JoinChannelResult TryJoin(ChatSession* session, const std::string& passwordOpt = "");

    bool TryLeave(ChatSession* session);

    bool HasMember(uint64_t accountId) const;

public:
    // 이미 만들어진 패킷 bytes를 채널 전체에 전송
    void BroadcastRaw(const void* data, size_t len);

    // 보내는 사람 제외 옵션
    void BroadcastRawExcept(ChatSession* exceptSession, const void* data, size_t len);

    // 한 사람에게 보내기
    bool SendToMember(uint64_t toAccountId, const void* data, size_t len);

    // 디버그/목록 요청용
    std::vector<uint64_t> GetMemberAccountIdsSnapshot() const;

public:
    bool IsClosed() const { return closed_; }

    void Close(const void* notifyData, size_t notifyLen);


private:
    bool IsFull_NoLock() const { return members_.size() >= maxUsers_; }

private:
    uint32_t channelId_{ 0 };
    uint32_t maxUsers_{ 0 };

    bool needPassword_{ false };
    std::string password_;

    // accountId -> ChatSession*
    std::unordered_map<uint64_t, ChatSession*> members_;

    mutable std::mutex mtx_;

    bool closed_{ false };
};

