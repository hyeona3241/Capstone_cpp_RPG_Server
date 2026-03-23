#pragma once
#include <cstddef>
#include <cstdint>

class ChatServer;
class Session;
struct PacketHeader;

enum ChannelEnterResultCode : uint16_t
{
    CER_OK = 0,
    CER_NOT_AUTHED = 1,
    CER_ALREADY_IN = 2,
    CER_CHANNEL_FULL = 3,
    CER_PASSWORD_REQUIRED = 4,
    CER_WRONG_PASSWORD = 5,
    CER_INVALID_CHANNEL = 6,
    CER_CHANNEL_CLOSED = 7,
    CER_NOT_IN_CHANNEL = 8,
};

class ChatPacketHandler
{
public:
    explicit ChatPacketHandler(ChatServer* owner);

    // Client -> ChatServer 로 온 패킷
    void HandleFromClient(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // MainServer -> ChatServer 로 온 내부 패킷
    void HandleFromMainServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

private:
    // 4000 ~ 4099 : 시스템/헬스체크
    void HandleSystem(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 4100 ~ 4199 : 인증/세션 바인딩
    void HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 4200 ~ 4299 : 채널(목록/입장/퇴장/닫기)
    void HandleChannel(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 4300 ~ 4399 : 채팅 메시지(채널 메시지/귓속말 등)
    void HandleMessage(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 실제 핸들러(스텁 자리)
    void HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleAuthReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);
    void HandleChannelListReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);
    void HandleChannelEnterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);
    void HandleChannelLeaveReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);
    void HandleChannelMsgReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);
    void HandleAllowTokenReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);


    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

private:
    ChatServer* owner_{ nullptr };
};

