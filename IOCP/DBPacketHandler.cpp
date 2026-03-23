#include "Pch.h"
#include "DBPacketHandler.h"
#include "DBServer.h"
#include "Session.h"
#include "PacketDef.h"

#include "DBAuthPackets.h"
#include "AuthService.h"
#include <Logger.h>

#include "DBRegisterPackets.h"

static uint64_t GetTickMs()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

DBPacketHandler::DBPacketHandler(DBServer* owner)
    : owner_(owner)
{
}

bool DBPacketHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
    return (id >= begin) && (id < endExclusive);
}

void DBPacketHandler::HandleFromMainServer(Session* session,
    const PacketHeader& header,
    const std::byte* payload,
    std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 3000 ~ 3099 : 시스템/헬스체크
    // 3100 ~ 3199 : Auth(회원가입/로그인)
    if (InRange(id, 3000, 3100))
    {
        HandleSystem(session, header, payload, length);
    }
    else if (InRange(id, 3100, 3200))
    {
        HandleAuth(session, header, payload, length);
    }
    else
    {
        std::printf("[WARN][DBPacketHandler] Unknown packet id=%u (sessionId=%llu)\n", id, static_cast<unsigned long long>(session->GetId()));
        LOG_WARN("[DBPacketHandler] Unknown packet id=" + std::to_string(id) 
            + "(sessionId = " + std::to_string(static_cast<unsigned long long>(session->GetId())) + ")");
    }
}

void DBPacketHandler::HandleSystem(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    case PacketType::to_id(PacketType::DB::DB_PING_REQ):
        HandlePingReq(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][DBSystem] Unknown system pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[DBSystem] Unknown system pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void DBPacketHandler::HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    case PacketType::to_id(PacketType::DB::DB_FIND_ACCOUNT_REQ):
        HandleLoginReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::DB::DB_UPDATE_LASTLOGIN_REQ):
        HandleUpdateLastLoginReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::DB::DB_REGISTER_REQ):
        HandleRegisterReq(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][DBAuth] Unknown auth pktId=%u\n", static_cast<unsigned>(header.id));
        LOG_WARN("[DBAuth] Unknown auth pktId=" + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void DBPacketHandler::HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    DBPingReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][DB] PING_REQ parse failed (len=%zu)\n", length);
        LOG_WARN("[DB] PING_REQ parse failed (len=" + std::to_string(length) + ")");
        return;
    }

    std::printf("[DEBUG][DB] PING_REQ received seq=%u\n", req.seq);
    LOG_INFO("[DB] PING_REQ received seq=" + std::to_string(req.seq));

    DBPingAck ack;
    ack.seq = req.seq;
    ack.serverTick = GetTickMs();

    auto bytes = ack.Build();

    session->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][DB] PING_ACK sent seq=%u tick=%llu\n",
        ack.seq, static_cast<unsigned long long>(ack.serverTick));
    LOG_INFO("[DB] PING_ACK sent seq=" + std::to_string(ack.seq) + " tick=" 
        + std::to_string(static_cast<unsigned long long>(ack.serverTick)));
}

void DBPacketHandler::HandleLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    DBFindAccountReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][DB] DB_FIND_ACCOUNT_REQ parse failed (len=%zu)\n", length);
        LOG_WARN("[DB] DB_FIND_ACCOUNT_REQ parse failed (len=" + std::to_string(length) + ")");
        return;
    }
    std::printf("[DB] FindAccount loginId='%s' (len=%zu)\n", req.loginId.c_str(), req.loginId.size());
    LOG_INFO("[DB] FindAccount loginId='" + req.loginId + "' (len=" + std::to_string(req.loginId.size()) +")");

    auto lookup = owner_->GetAuthService().Login(req.loginId);

    DBFindAccountAck ack;
    ack.seq = req.seq;

    if (lookup.result == AuthService::LoginResult::Success)
    {
        ack.exists = 1;
        ack.accountId = lookup.accountId;
        ack.passwordHash = std::move(lookup.pwHash);
        ack.passwordSalt = std::move(lookup.pwSalt);
        ack.status = lookup.status;
    }
    else
    {
        ack.exists = 0;
    }

    auto bytes = ack.Build();
    session->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][DB] DB_FIND_ACCOUNT_ACK sent seq=%u exists=%u\n", ack.seq, ack.exists);
    LOG_INFO("[DB] DB_FIND_ACCOUNT_ACK sent seq=" + std::to_string(ack.seq) + " exists=" + std::to_string(ack.exists));
}

void DBPacketHandler::HandleUpdateLastLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    DBUpdateLastLoginReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][DB] DB_UPDATE_LASTLOGIN_REQ parse failed (len=%zu)\n", length);
        LOG_WARN("[DB] DB_UPDATE_LASTLOGIN_REQ parse failed (len=" + std::to_string(length) + ")");
        return;
    }

    const bool ok = owner_->GetAuthService().UpdateLastLogin(req.accountId);

    if (!ok)
    {
        std::printf("[WARN][DB] UpdateLastLogin failed. accountId=%llu\n",
            static_cast<unsigned long long>(req.accountId));
        LOG_WARN("[DB] UpdateLastLogin failed. accountId=" + std::to_string(static_cast<unsigned long long>(req.accountId)));
        return;
    }

    std::printf("[DEBUG][DB] UpdateLastLogin success. accountId=%llu\n",
        static_cast<unsigned long long>(req.accountId));
    LOG_INFO("[DB] UpdateLastLogin success. accountId=" + std::to_string(static_cast<unsigned long long>(req.accountId)));
}

void DBPacketHandler::HandleRegisterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    DbRegisterReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][DB] DB_REGISTER_REQ parse failed (len=%zu)\n", length);
        LOG_WARN("[DB] DB_REGISTER_REQ parse failed (len=" + std::to_string(length) + ")");
        return;
    }

    uint64_t newAccountId = 0;
    auto rr = owner_->GetAuthService().Register(req.loginId, req.plainPw, newAccountId);

    DbRegisterAck ack;
    switch (rr)
    {
    case AuthService::RegisterResult::Success:
        ack = DbRegisterAck::MakeOk(req.seq, req.clientSessionId, newAccountId);
        break;
    case AuthService::RegisterResult::DuplicateLoginId:
        ack = DbRegisterAck::MakeFail(req.seq, req.clientSessionId, ERegisterResult::DUPLICATE_LOGIN_ID);
        break;
    default:
        ack = DbRegisterAck::MakeFail(req.seq, req.clientSessionId, ERegisterResult::INTERNAL_ERROR);
        break;
    }

    auto bytes = ack.Build();
    session->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][DB] DB_REGISTER_ACK sent seq=%u success=%u result=%u\n",
        ack.seq, ack.success, ack.resultCode);
    LOG_INFO("[DB] DB_REGISTER_ACK sent seq=" + std::to_string(ack.seq));
}
