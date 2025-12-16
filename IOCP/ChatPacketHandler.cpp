#include "ChatPacketHandler.h"

#include "Session.h"
#include "PacketDef.h" 
#include <cstdio>
#include <Logger.h>

ChatPacketHandler::ChatPacketHandler(ChatServer* owner)
    : owner_(owner)
{
}

bool ChatPacketHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
    return (id >= begin) && (id < endExclusive);
}

void ChatPacketHandler::HandleFromClient(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 4000~5999: Chat
    if (!InRange(id, 4000, 5000))
    {
        std::printf("[WARN][Chat] Out of chat range pktId=%u\n", id);
        LOG_WARN("[Chat] Out of chat range pktId=" + std::to_string(id));
        return;
    }

    if (InRange(id, 4000, 4100))
        HandleSystem(session, header, payload, length);
    else if (InRange(id, 4100, 4200))
        HandleAuth(session, header, payload, length);
    else if (InRange(id, 4200, 4300))
        HandleChannel(session, header, payload, length);
    else if (InRange(id, 4300, 4400))
        HandleMessage(session, header, payload, length);
    else
    {
        std::printf("[WARN][Chat] Unknown packet id=%u\n", id);
        LOG_WARN("[Chat] Unknown packet id=" + std::to_string(id));
    }
}

void ChatPacketHandler::HandleFromMainServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 내부 패킷도 일단 같은 라우팅을 쓰되, 나중에 범위를 따로 잡아도 됨.
    // 예: 2500~2599 내부컨트롤 이런 식으로.
    if (!InRange(id, 4000, 5000))
    {
        std::printf("[WARN][Chat] (FromMain) Out of chat range pktId=%u\n", id);
        LOG_WARN("[Chat] (FromMain) Out of chat range pktId=" + std::to_string(id));
        return;
    }

    if (InRange(id, 4000, 4100))
        HandleSystem(session, header, payload, length);
    else if (InRange(id, 4100, 4200))
        HandleAuth(session, header, payload, length);
    else if (InRange(id, 4200, 4300))
        HandleChannel(session, header, payload, length);
    else if (InRange(id, 4300, 4400))
        HandleMessage(session, header, payload, length);
    else
    {
        std::printf("[WARN][Chat] (FromMain) Unknown packet id=%u\n", id);
        LOG_WARN("[Chat] (FromMain) Unknown packet id=" + std::to_string(id));
    }
}

void ChatPacketHandler::HandleSystem(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
        // TODO: PacketIds.h에 정의되면 여기 case 추가
        // case PacketType::to_id(PacketType::Chat::CHAT_PING_REQ):
        //     HandlePingReq(session, header, payload, length);
        //     break;

    default:
        std::printf("[WARN][Chat:System] Unknown pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[Chat:System] Unknown pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void ChatPacketHandler::HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
        // TODO: 예) CHAT_AUTH_REQ
        // case PacketType::to_id(PacketType::Chat::CHAT_AUTH_REQ):
        //     HandleAuthReq(session, header, payload, length);
        //     break;

    default:
        std::printf("[WARN][Chat:Auth] Unknown pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[Chat:Auth] Unknown pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void ChatPacketHandler::HandleChannel(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
        // TODO: 채널 리스트/입장/퇴장/닫기 패킷들
        // case PacketType::to_id(PacketType::Chat::CHAT_CHANNEL_LIST_REQ):
        //     HandleChannelListReq(session, header, payload, length);
        //     break;
        // case PacketType::to_id(PacketType::Chat::CHAT_CHANNEL_ENTER_REQ):
        //     HandleChannelEnterReq(session, header, payload, length);
        //     break;

    default:
        std::printf("[WARN][Chat:Channel] Unknown pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[Chat:Channel] Unknown pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void ChatPacketHandler::HandleMessage(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
        // TODO: 예) CHAT_CHANNEL_MESSAGE_REQ
        // case PacketType::to_id(PacketType::Chat::CHAT_CHANNEL_MESSAGE_REQ):
        //     HandleChannelMsgReq(session, header, payload, length);
        //     break;

    default:
        std::printf("[WARN][Chat:Msg] Unknown pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[Chat:Msg] Unknown pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}


void ChatPacketHandler::HandlePingReq(Session*, const PacketHeader&, const std::byte*, std::size_t)
{
    // TODO
}

void ChatPacketHandler::HandleAuthReq(Session*, const PacketHeader&, const std::byte*, std::size_t)
{
    // TODO
}

void ChatPacketHandler::HandleChannelListReq(Session*, const PacketHeader&, const std::byte*, std::size_t)
{
    // TODO
}

void ChatPacketHandler::HandleChannelEnterReq(Session*, const PacketHeader&, const std::byte*, std::size_t)
{
    // TODO
}

void ChatPacketHandler::HandleChannelLeaveReq(Session*, const PacketHeader&, const std::byte*, std::size_t)
{
    // TODO
}

void ChatPacketHandler::HandleChannelMsgReq(Session*, const PacketHeader&, const std::byte*, std::size_t)
{
    // TODO
}