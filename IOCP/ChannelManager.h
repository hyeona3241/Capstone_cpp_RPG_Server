#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <cstdint>
#include <string>

#include "ChatChannel.h"

class ChatSession;

struct ChannelInfo
{
    uint32_t channelId = 0;
    uint32_t currentUsers = 0;
    uint32_t maxUsers = 0;
    bool needPassword = false;
};


class ChannelManager
{
public:
    ChannelManager() = default;
    ~ChannelManager() = default;

    // 서버 시작 시 기본 채널 생성
    void InitDefaultChannels();

    // 채널 생성/조회/삭제
    bool CreateChannel(uint32_t channelId, uint32_t maxUsers, bool needPassword, const std::string& password = "");
    ChatChannel* FindChannel(uint32_t channelId);

    // 채널 종료(닫기 알림 포함)
    bool CloseChannel(uint32_t channelId, const void* closeNotify, size_t closeNotifyLen);

    JoinChannelResult Join(uint32_t channelId, ChatSession* session, const std::string& passwordOpt = "");
    JoinChannelResult Leave(ChatSession* session);

    bool LeaveIfInChannel(ChatSession* session);

    // 채널 리스트
    std::vector<ChannelInfo> GetChannelListSnapshot() const;

private:
    // 로비 채널은 닫지 않는다
    static constexpr uint32_t kLobbyChannelId = 1;

private:
    mutable std::mutex mtx_;
    std::unordered_map<uint32_t, std::unique_ptr<ChatChannel>> channels_;
};

