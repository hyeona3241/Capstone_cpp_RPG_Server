#include "MSChatServerInternalHandler.h"

#include "MainServer.h"
#include "Session.h"
#include "PacketDef.h"
#include "PacketIds.h"

#include "CSPingPackets.h"

#include <Logger.h>
#include <cstdio>

MSChatServerInternalHandler::MSChatServerInternalHandler(MainServer* owner)
	:owner_(owner)
{
}

bool MSChatServerInternalHandler::Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    const uint16_t id = header.id;

    if (!InRange(id, 4000, 5000))
        return false;

    if (InRange(id, 4000, 4100))
    {
        return HandleHandshake(session, header, payload, length);
    }

    if (InRange(id, 4100, 4200))
    {
    }

    // 클라 범위 안인데 도메인 없음
    LOG_WARN("[ChatSrv] In ChatServer range but no domain matched. pktId=%u", (unsigned)id);
    return true;
}

bool MSChatServerInternalHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
	return (id >= begin) && (id < endExclusive);
}

bool MSChatServerInternalHandler::HandleHandshake(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return false;

    switch (header.id)
    {
    case PacketType::to_id(PacketType::Chat::CS_MS_PING_ACK): 
    {
        return HandlePingAck(session, payload, length);
    }

    default:
        std::printf("[WARN][ChatSrv:HS] Unhandled pktId=%u len=%zu\n",
            (unsigned)header.id, length);
        LOG_WARN("[ChatSrv:HS] Unhandled pktId=%u len=%zu", (unsigned)header.id, length);
        return true;
    }
}

bool MSChatServerInternalHandler::HandlePingAck(Session* session, const std::byte* payload, std::size_t length)
{
    CSPingAck ack;
    if (!ack.ParsePayload(payload, length))
    {
        std::printf("[WARN][ChatSrv] CS_MS_PING_ACK parse failed. len=%zu sessionId=%llu\n",
            length, (unsigned long long)session->GetId());
        LOG_WARN("[ChatSrv] CS_MS_PING_ACK parse failed. len=%zu sessionId=%llu",
            length, (unsigned long long)session->GetId());
        return true;
    }

    std::printf("[INFO][ChatSrv] CS_MS_PING_ACK recv. seq=%u serverTick=%llu\n",
        ack.seq, (unsigned long long)ack.serverTick);
    LOG_INFO("[ChatSrv] CS_MS_PING_ACK recv. seq=%u serverTick=%llu",
        ack.seq, (unsigned long long)ack.serverTick);

    if (owner_)
    {
        owner_->OnChatPingAck(ack.seq, ack.serverTick);
    }

    return true;
}
