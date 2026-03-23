#include "ChatPacketHandler.h"

#include "Session.h"
#include "PacketDef.h" 
#include <cstdio>
#include <Logger.h>

#include "PacketIds.h"
#include "CSPingPackets.h"
#include "CCSAuthPackets.h"
#include "CChatChannelPackets.h"
#include "CChatMessagePackets.h"

#include "ChatSession.h"
#include "ChatSessionManager.h"

#include "ChatServer.h"

static uint64_t GetTickMs()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

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
    case PacketType::to_id(PacketType::Chat::CS_MS_PING_REQ):
        HandlePingReq(session, header, payload, length);
        break;

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
    case PacketType::to_id(PacketType::Chat::MS_CS_CHAT_ALLOW_TOKEN_REQ): // 4100
        HandleAllowTokenReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::Chat::CL_CS_CHAT_AUTH_REQ): // 4101
        HandleAuthReq(session, header, payload, length);
        break;

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
    case PacketType::to_id(PacketType::Chat::CL_CS_CHANNEL_LIST_REQ):
        HandleChannelListReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::Chat::CL_CS_CHANNEL_ENTER_REQ):
        HandleChannelEnterReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::Chat::CL_CS_CHANNEL_LEAVE_REQ):
        HandleChannelLeaveReq(session, header, payload, length);
        break;

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
    case PacketType::to_id(PacketType::Chat::CL_CS_CHAT_SEND_REQ):
        HandleChannelMsgReq(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][Chat:Msg] Unknown pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[Chat:Msg] Unknown pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}


void ChatPacketHandler::HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    CSPingReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][CHAT] PING_REQ parse failed (len=%zu)\n", length);
        LOG_WARN("[CHAT] PING_REQ parse failed (len=" + std::to_string(length) + ")");
        return;
    }

    std::printf("[DEBUG][CHAT] PING_REQ received seq=%u\n", req.seq);
    LOG_INFO("[CHAT] PING_REQ received seq=" + std::to_string(req.seq));

    session->SetRole(SessionRole::MainServer);

    CSPingAck ack;
    ack.seq = req.seq;
    ack.serverTick = GetTickMs();

    auto bytes = ack.Build();

    session->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][CHAT] PING_ACK sent seq=%u tick=%llu\n",
        ack.seq, static_cast<unsigned long long>(ack.serverTick));
    LOG_INFO("[CHAT] PING_ACK sent seq=" + std::to_string(ack.seq) + " tick="
        + std::to_string(static_cast<unsigned long long>(ack.serverTick)));
}

void ChatPacketHandler::HandleAuthReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs)
    {
        std::printf("[WARN][CHAT][AUTH] HandleAuthReq called with non-ChatSession. pktId=%u\n",
            static_cast<unsigned>(header.id));
        LOG_WARN("[CHAT][AUTH] HandleAuthReq called with non-ChatSession.");
        return;
    }

    const uint64_t sessionId = cs->GetId();

    std::printf("[DEBUG][CHAT][AUTH] AUTH_REQ recv. sessionId=%llu len=%zu\n",
        static_cast<unsigned long long>(sessionId), length);
    LOG_INFO(std::string("[CHAT][AUTH] AUTH_REQ recv. sessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " len=" + std::to_string(length));

    // 이미 인증됨
    if (cs->IsAuthed())
    {
        std::printf("[WARN][CHAT][AUTH] AUTH_REQ ignored (already authed). sessionId=%llu\n",
            static_cast<unsigned long long>(sessionId));
        LOG_WARN(std::string("[CHAT][AUTH] AUTH_REQ ignored (already authed). sessionId=") +
            std::to_string(static_cast<unsigned long long>(sessionId)));

        CSCLChatAuthAck ack = CSCLChatAuthAck::MakeFail(EChatAuthResult::ALREADY_USED);
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    CLCSChatAuthReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][CHAT][AUTH] AUTH_REQ parse failed. sessionId=%llu len=%zu\n",
            static_cast<unsigned long long>(sessionId), length);
        LOG_WARN(std::string("[CHAT][AUTH] AUTH_REQ parse failed. sessionId=") +
            std::to_string(static_cast<unsigned long long>(sessionId)) +
            " len=" + std::to_string(length));
        return;
    }

    const std::string tokenPrefix = (req.token.size() >= 6) ? req.token.substr(0, 6) : req.token;

    std::printf("[DEBUG][CHAT][AUTH] AUTH_REQ parsed. sessionId=%llu loginId=%s tokenPrefix=%s\n",
        static_cast<unsigned long long>(sessionId),
        req.loginId.c_str(),
        tokenPrefix.c_str());
    LOG_INFO(std::string("[CHAT][AUTH] AUTH_REQ parsed. sessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " loginId=" + req.loginId +
        " tokenPrefix=" + tokenPrefix);

    if (req.loginId.empty())
    {
        std::printf("[WARN][CHAT][AUTH] AUTH_REQ invalid loginId(empty). sessionId=%llu\n",
            static_cast<unsigned long long>(sessionId));
        LOG_WARN(std::string("[CHAT][AUTH] AUTH_REQ invalid loginId(empty). sessionId=") +
            std::to_string(static_cast<unsigned long long>(sessionId)));

        CSCLChatAuthAck ack = CSCLChatAuthAck::MakeFail(EChatAuthResult::NOT_ALLOWED);
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    uint64_t accountId = 0;
    const bool tokenOk = owner_->GetSessionManager().ConsumePendingAuth(req.token, accountId);

    if (!tokenOk)
    {
        std::printf("[INFO][CHAT][AUTH] AUTH failed (invalid/expired token). sessionId=%llu tokenPrefix=%s\n",
            static_cast<unsigned long long>(sessionId),
            tokenPrefix.c_str());
        LOG_INFO(std::string("[CHAT][AUTH] AUTH failed (invalid/expired token). sessionId=") +
            std::to_string(static_cast<unsigned long long>(sessionId)) +
            " tokenPrefix=" + tokenPrefix);

        CSCLChatAuthAck ack = CSCLChatAuthAck::MakeFail(EChatAuthResult::INVALID_TOKEN);
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    std::printf("[DEBUG][CHAT][AUTH] Token consumed. sessionId=%llu aid=%llu tokenPrefix=%s\n",
        static_cast<unsigned long long>(sessionId),
        static_cast<unsigned long long>(accountId),
        tokenPrefix.c_str());
    LOG_INFO(std::string("[CHAT][AUTH] Token consumed. sessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " aid=" + std::to_string(static_cast<unsigned long long>(accountId)) +
        " tokenPrefix=" + tokenPrefix);

    const bool bindOk = owner_->GetSessionManager().BindAccount(cs, accountId, req.loginId);

    if (!bindOk)
    {
        std::printf("[WARN][CHAT][AUTH] BindAccount failed. sessionId=%llu aid=%llu loginId=%s\n",
            static_cast<unsigned long long>(sessionId),
            static_cast<unsigned long long>(accountId),
            req.loginId.c_str());
        LOG_WARN(std::string("[CHAT][AUTH] BindAccount failed. sessionId=") +
            std::to_string(static_cast<unsigned long long>(sessionId)) +
            " aid=" + std::to_string(static_cast<unsigned long long>(accountId)) +
            " loginId=" + req.loginId);
    }
    else
    {
        std::printf("[INFO][CHAT][AUTH] AUTH success. sessionId=%llu aid=%llu loginId=%s\n",
            static_cast<unsigned long long>(sessionId),
            static_cast<unsigned long long>(accountId),
            req.loginId.c_str());
        LOG_INFO(std::string("[CHAT][AUTH] AUTH success. sessionId=") +
            std::to_string(static_cast<unsigned long long>(sessionId)) +
            " aid=" + std::to_string(static_cast<unsigned long long>(accountId)) +
            " loginId=" + req.loginId);
    }

    CSCLChatAuthAck ack = bindOk ? CSCLChatAuthAck::MakeOk(accountId, req.loginId) : CSCLChatAuthAck::MakeFail(EChatAuthResult::INTERNAL_ERROR);

    auto bytes = ack.Build();
    cs->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][CHAT][AUTH] AUTH_ACK sent. sessionId=%llu success=%u resultCode=%u\n",
        static_cast<unsigned long long>(sessionId),
        ack.success,
        ack.resultCode);
    LOG_INFO(std::string("[CHAT][AUTH] AUTH_ACK sent. sessionId=") +
        std::to_string(static_cast<unsigned long long>(sessionId)) +
        " success=" + std::to_string(ack.success) +
        " resultCode=" + std::to_string(ack.resultCode));
}

void ChatPacketHandler::HandleChannelListReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    (void)header;
    (void)payload;
    (void)length;

    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs)
        return;

    const uint64_t sid = cs->GetId();

    if (!cs->IsAuthed())
    {
        std::printf("[WARN][CHAT][CHANNEL] LIST_REQ rejected (not authed). sessionId=%llu\n",
            (unsigned long long)sid);
        LOG_WARN("[CHAT][CHANNEL] LIST_REQ rejected (not authed). sessionId=" + std::to_string((unsigned long long)sid));

        // 실패 ACK
        CSCLChannelListAck ack = CSCLChannelListAck::MakeFail(/*code*/ 1);
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    const auto list = owner_->GetChannelManager().GetChannelListSnapshot();

    std::vector<ChatProtocol::ChannelInfo> out;
    out.reserve(list.size());

    for (const auto& c : list)
    {
        ChatProtocol::ChannelInfo info;
        info.channelId = c.channelId;
        info.userCount = static_cast<uint16_t>(c.currentUsers);
        info.maxUsers = static_cast<uint16_t>(c.maxUsers);
        info.needPassword = c.needPassword ? 1 : 0;

        // 기본 2채널이면 이름도 붙여주면 디버깅 편함
        if (c.channelId == 1) info.name = "Lobby";
        else if (c.channelId == 2) info.name = "Channel2";
        else info.name = "Channel" + std::to_string(c.channelId);

        out.push_back(std::move(info));
    }

    CSCLChannelListAck ack = CSCLChannelListAck::MakeOk(out);
    auto bytes = ack.Build();
    cs->Send(bytes.data(), bytes.size());

    std::printf("[INFO][CHAT][CHANNEL] LIST_ACK sent. sessionId=%llu count=%zu\n",
        (unsigned long long)sid, out.size());
    LOG_INFO("[CHAT][CHANNEL] LIST_ACK sent. sessionId=" + std::to_string((unsigned long long)sid) +
        " count=" + std::to_string(out.size()));
}

void ChatPacketHandler::HandleChannelEnterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    (void)header;

    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs) return;

    const uint64_t sid = cs->GetId();

    CLCSChannelEnterReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][CHAT][CHANNEL] ENTER_REQ parse failed. sessionId=%llu\n",
            (unsigned long long)sid);
        LOG_WARN("[CHAT][CHANNEL] ENTER_REQ parse failed. sessionId=" + std::to_string((unsigned long long)sid));
        return;
    }

    const JoinChannelResult jr = owner_->GetChannelManager().Join(req.channelId, cs /*, passwordOpt*/);

    if (jr != JoinChannelResult::Ok)
    {
        uint16_t code = CER_INVALID_CHANNEL;
        switch (jr)
        {
        case JoinChannelResult::NotAuthed:        code = CER_NOT_AUTHED; break;
        case JoinChannelResult::AlreadyInChannel: code = CER_ALREADY_IN; break;
        case JoinChannelResult::ChannelFull:      code = CER_CHANNEL_FULL; break;
        case JoinChannelResult::PasswordRequired: code = CER_PASSWORD_REQUIRED; break;
        case JoinChannelResult::WrongPassword:    code = CER_WRONG_PASSWORD; break;
        case JoinChannelResult::Invalid:          code = CER_INVALID_CHANNEL; break;
        case JoinChannelResult::CloseChannel:     code = CER_CHANNEL_CLOSED; break;
        default:                                  code = CER_INVALID_CHANNEL; break;
        }

        std::printf("[INFO][CHAT][CHANNEL] ENTER failed. sessionId=%llu cid=%u jr=%d\n",
            (unsigned long long)sid, req.channelId, (int)jr);
        LOG_INFO("[CHAT][CHANNEL] ENTER failed. sessionId=" + std::to_string((unsigned long long)sid) +
            " cid=" + std::to_string(req.channelId) + " jr=" + std::to_string((int)jr));

        CSCLChannelEnterAck ack = CSCLChannelEnterAck::MakeFail(code);
        ack.channelId = req.channelId;
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    std::printf("[INFO][CHAT][CHANNEL] ENTER success. sessionId=%llu cid=%u\n",
        (unsigned long long)sid, req.channelId);
    LOG_INFO("[CHAT][CHANNEL] ENTER success. sessionId=" + std::to_string((unsigned long long)sid) +
        " cid=" + std::to_string(req.channelId));

    CSCLChannelEnterAck ack = CSCLChannelEnterAck::MakeOk(req.channelId);
    auto bytes = ack.Build();
    cs->Send(bytes.data(), bytes.size());
}

void ChatPacketHandler::HandleChannelLeaveReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    (void)header;
    (void)payload;
    (void)length;

    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs)
        return;

    const uint64_t sid = cs->GetId();

    std::printf("[DEBUG][CHAT][CHANNEL] LEAVE_REQ recv. sessionId=%llu\n",
        (unsigned long long)sid);
    LOG_INFO("[CHAT][CHANNEL] LEAVE_REQ recv. sessionId=" +
        std::to_string((unsigned long long)sid));

    if (!cs->IsAuthed())
    {
        std::printf("[WARN][CHAT][CHANNEL] LEAVE_REQ rejected (not authed). sessionId=%llu\n",
            (unsigned long long)sid);
        LOG_WARN("[CHAT][CHANNEL] LEAVE_REQ rejected (not authed). sessionId=" +
            std::to_string((unsigned long long)sid));

        CSCLChannelLeaveAck ack = CSCLChannelLeaveAck::MakeFail(CER_NOT_AUTHED);
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    if (!cs->IsInChannel())
    {
        std::printf("[WARN][CHAT][CHANNEL] LEAVE_REQ rejected (not in channel). sessionId=%llu\n",
            (unsigned long long)sid);
        LOG_WARN("[CHAT][CHANNEL] LEAVE_REQ rejected (not in channel). sessionId=" +
            std::to_string((unsigned long long)sid));

        CSCLChannelLeaveAck ack = CSCLChannelLeaveAck::MakeFail(CER_NOT_IN_CHANNEL);
        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    const uint32_t cid = cs->GetChannelId();

    const JoinChannelResult jr = owner_->GetChannelManager().Leave(cs);

    if (jr != JoinChannelResult::Ok)
    {
        std::printf("[WARN][CHAT][CHANNEL] LEAVE failed. sessionId=%llu cid=%u jr=%d\n",
            (unsigned long long)sid, cid, (int)jr);
        LOG_WARN("[CHAT][CHANNEL] LEAVE failed. sessionId=" +
            std::to_string((unsigned long long)sid) +
            " cid=" + std::to_string(cid) +
            " jr=" + std::to_string((int)jr));

        CSCLChannelLeaveAck ack =
            CSCLChannelLeaveAck::MakeFail(static_cast<uint16_t>(jr));
        ack.channelId = cid;

        auto bytes = ack.Build();
        cs->Send(bytes.data(), bytes.size());
        return;
    }

    std::printf("[INFO][CHAT][CHANNEL] LEAVE success. sessionId=%llu cid=%u\n",
        (unsigned long long)sid, cid);
    LOG_INFO("[CHAT][CHANNEL] LEAVE success. sessionId=" +
        std::to_string((unsigned long long)sid) +
        " cid=" + std::to_string(cid));

    CSCLChannelLeaveAck ack =
        CSCLChannelLeaveAck::MakeOk(cid);

    auto bytes = ack.Build();
    cs->Send(bytes.data(), bytes.size());
}

void ChatPacketHandler::HandleChannelMsgReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    (void)header;

    auto* cs = dynamic_cast<ChatSession*>(session);
    if (!cs) return;

    const uint64_t sid = cs->GetId();

    CLCSChatSendReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][CHAT][MSG] SEND_REQ parse failed. sessionId=%llu len=%zu\n",
            (unsigned long long)sid, length);
        LOG_WARN("[CHAT][MSG] SEND_REQ parse failed. sessionId=" + std::to_string((unsigned long long)sid));
        return;
    }

    // 인증/채널 상태 체크
    if (!cs->IsAuthed() || !cs->IsInChannel())
    {
        std::printf("[WARN][CHAT][MSG] SEND rejected (not ready). sessionId=%llu authed=%d inChannel=%d\n",
            (unsigned long long)sid, cs->IsAuthed() ? 1 : 0, cs->IsInChannel() ? 1 : 0);
        LOG_WARN("[CHAT][MSG] SEND rejected (not ready). sessionId=" + std::to_string((unsigned long long)sid));
        return;
    }

    const uint32_t myCid = cs->GetChannelId();
    if (req.channelId != myCid)
    {
        std::printf("[WARN][CHAT][MSG] SEND rejected (cid mismatch). sessionId=%llu reqCid=%u myCid=%u\n",
            (unsigned long long)sid, req.channelId, myCid);
        LOG_WARN("[CHAT][MSG] SEND rejected (cid mismatch). sessionId=" + std::to_string((unsigned long long)sid) +
            " reqCid=" + std::to_string(req.channelId) + " myCid=" + std::to_string(myCid));
        return;
    }

    // 채널 찾기
    ChatChannel* ch = owner_->GetChannelManager().FindChannel(myCid);
    if (!ch || ch->IsClosed())
    {
        std::printf("[WARN][CHAT][MSG] SEND rejected (channel missing/closed). sessionId=%llu cid=%u\n",
            (unsigned long long)sid, myCid);
        LOG_WARN("[CHAT][MSG] SEND rejected (channel missing/closed). sessionId=" + std::to_string((unsigned long long)sid) +
            " cid=" + std::to_string(myCid));
        return;
    }

    if (req.message.empty())
        return;

    // 브로드캐스트 패킷 만들고 전송
    CSCLChatBroadcastNfy nfy;
    nfy.channelId = myCid;
    nfy.senderLoginId = cs->GetNickname();
    nfy.message = req.message;

    auto bytes = nfy.Build();

    /*ch->BroadcastRaw(bytes.data(), bytes.size());*/
    ch->BroadcastRawExcept(cs, bytes.data(), bytes.size());

    std::printf("[DEBUG][CHAT][MSG] Broadcast. cid=%u from=%s len=%zu\n",
        myCid, nfy.senderLoginId.c_str(), req.message.size());
    LOG_INFO("[CHAT][MSG] Broadcast. cid=" + std::to_string(myCid) +
        " from=" + nfy.senderLoginId +
        " len=" + std::to_string(req.message.size()));
}

void ChatPacketHandler::HandleAllowTokenReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (session->GetRole() != SessionRole::MainServer)
    {
        std::printf("[WARN][CHAT][AUTH] AllowToken from non-main. sessionId=%llu role=%d\n",
            (unsigned long long)session->GetId(), (int)session->GetRole());
        LOG_WARN("[CHAT][AUTH] AllowToken from non-main.");
        return;
    }

    MSCSChatAllowTokenReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][CHAT][AUTH] ALLOW_TOKEN_REQ parse failed len=%zu\n", length);
        LOG_WARN("[CHAT][AUTH] ALLOW_TOKEN_REQ parse failed len=" + std::to_string(length));
        return;
    }

    owner_->GetSessionManager().RegisterPendingAuth( req.token, req.accountId, req.ttlSec);

    std::printf(
        "[INFO][CHAT][AUTH] ALLOW_TOKEN_REQ token=%s aid=%llu ttl=%u\n",
        req.token.c_str(),
        (unsigned long long)req.accountId,
        req.ttlSec
    );
    LOG_INFO(
        std::string("[CHAT][AUTH] ALLOW_TOKEN_REQ token=") +
        req.token +
        " aid=" + std::to_string((unsigned long long)req.accountId) +
        " ttl=" + std::to_string(req.ttlSec)
    );
}
