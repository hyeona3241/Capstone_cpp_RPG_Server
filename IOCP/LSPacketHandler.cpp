#include "LSPacketHandler.h"
#include "LoginServer.h"
#include "Session.h"
#include "PacketDef.h"

#include <cstdio>

#include "LSLoginPackets.h"
#include "PasswordHasher.h"
#include <Logger.h>
#include "LSLogoutPackets.h"

static uint64_t GetTickMs()
{
    using namespace std::chrono;
    return static_cast<uint64_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

LSPacketHandler::LSPacketHandler(LoginServer* owner)
    : owner_(owner)
{
}

bool LSPacketHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
    return (id >= begin) && (id < endExclusive);
}

void LSPacketHandler::HandleFromMainServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    if (InRange(id, 2000, 2100))
    {
        HandleSystem(session, header, payload, length);
    }
    else if (InRange(id, 2100, 2200))
    {
        HandleAuth(session, header, payload, length);
    }
    else if (InRange(id, 2200, 2300))
    {
        HandleDbResult(session, header, payload, length);
    }
    else
    {
        std::printf("[WARN][LS] Unknown packet id=%u\n", id);
        LOG_WARN(std::string("[LS] Unknown packet id=") + std::to_string(id));

    }
}

void LSPacketHandler::HandleSystem(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    case PacketType::to_id(PacketType::Login::LS_PING_REQ):
        HandlePingReq(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][LS:System] Unknown pktId=%u\n",
            static_cast<unsigned>(header.id));
        LOG_WARN(std::string("[LS:System] Unknown pktId=") 
            + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void LSPacketHandler::HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    LSPingReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][LS] PING_REQ parse failed\n");
        LOG_WARN("[LS] PING_REQ parse failed");
        return;
    }

    LSPingAck ack;
    ack.seq = req.seq;
    ack.serverTick =  GetTickMs();

    auto bytes = ack.Build();
    session->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][LS] PING_ACK seq=%u\n", ack.seq);
    LOG_INFO(std::string("[LS] PING_ACK seq=") + std::to_string(ack.seq));
}

void LSPacketHandler::HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    case PacketType::to_id(PacketType::Login::LS_LOGIN_REQ):
        HandleLoginReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::Login::LS_DB_FIND_ACCOUNT_ACK):
        HandleDbFindAccountAck(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::Login::LS_LOGOUT_NOTIFY):

        HandleLogoutNoti(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][LS:Auth] Unknown pktId=%u\n",
            static_cast<unsigned>(header.id));
        LOG_WARN(std::string("[LS:Auth] Unknown pktId=") 
            + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void LSPacketHandler::HandleLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    LSLoginReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][LS] LS_LOGIN_REQ parse failed. len=%zu\n", length);
        LOG_WARN(std::string("[LS] LS_LOGIN_REQ parse failed. len=") + std::to_string(length));
        return;
    }

    // pending 등록 (seq로 매칭)
    PendingLogin p;
    p.seq = req.seq;
    p.clientSessionId = req.clientSessionId;
    p.loginId = req.loginId;
    p.plainPw = req.plainPw;
    p.createdTickMs = GetTickMs();

    owner_->AddPendingLogin(std::move(p));

    // Main으로 DB 조회 요청
    LSDbFindAccountReq dbReq;
    dbReq.seq = req.seq;
    dbReq.loginId = req.loginId;

    auto bytes = dbReq.Build();
    if (!owner_->SendToMain(bytes))
    {
        owner_->RemovePendingLogin(req.seq);
        std::printf("[WARN][LS] SendToMain failed. seq=%u\n", req.seq);
        LOG_WARN(std::string("[LS] SendToMain failed. seq=") + std::to_string(req.seq));
        return;
    }

    std::printf("[DEBUG][LS] Forward LS_DB_FIND_ACCOUNT_REQ seq=%u loginId=%s\n",
        req.seq, req.loginId.c_str());
    LOG_INFO(std::string("[LS] Forward LS_DB_FIND_ACCOUNT_REQ seq=") 
        + std::to_string(req.seq) + " loginId=" + req.loginId);
}

void LSPacketHandler::HandleRegisterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    std::printf("[DEBUG][LS] REGISTER_REQ len=%zu\n", length);

    // 1) payload 파싱
    // 2) PasswordHasher로 salt/hash 생성
    // 3) MainServer로 "DB 계정 생성 요청"
}

void LSPacketHandler::HandleDbResult(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    

    default:
        std::printf("[WARN][LS:DB] Unknown pktId=%u\n",
            static_cast<unsigned>(header.id));
        LOG_WARN(std::string("[LS:DB] Unknown pktId=") + std::to_string(static_cast<unsigned>(header.id)));
        break;
    }
}

void LSPacketHandler::HandleDbFindAccountAck(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    LSDbFindAccountAck dbAck;
    if (!dbAck.ParsePayload(payload, length))
    {
        std::printf("[WARN][LS] LS_DB_FIND_ACCOUNT_ACK parse failed. len=%zu\n", length);
        LOG_WARN(std::string("[LS] LS_DB_FIND_ACCOUNT_ACK parse failed. len=") + std::to_string(length));
        return;
    }

    PendingLogin pending;
    if (!owner_->TryGetPendingLogin(dbAck.seq, pending))
    {
        std::printf("[WARN][LS] No pending login for seq=%u\n", dbAck.seq);
        LOG_WARN(std::string("[LS] No pending login for seq=") + std::to_string(dbAck.seq));
        return;
    }

    ELoginResult result = static_cast<ELoginResult>(dbAck.resultCode);
    bool success = false;
    uint64_t accountId = 0;

    if (result != ELoginResult::OK)
    {
        success = false;
    }
    else
    {
        // OK인데 exists가 0이면: 계정 없음
        if (dbAck.exists == 0)
        {
            success = false;
            result = ELoginResult::NO_SUCH_USER;
        }
        else
        {
            // exists==1이면 state 확인
            // state: 0 정상 / 1 정지 / 2 삭제
            if (dbAck.state != 0)
            {
                success = false;
                result = ELoginResult::BANNED;
            }
            else
            {
                result = VerifyLoginPassword(pending, dbAck);

                if (result == ELoginResult::OK)
                {
                    const uint64_t aid = dbAck.accountId;
                    const uint64_t csid = pending.clientSessionId;

                    if (!owner_->TryMarkLoggedIn(aid, csid))
                    {
                        success = false;
                        accountId = 0;
                        result = ELoginResult::ALREADY_LOGGED_IN;
                    }
                    else
                    {
                        success = true;
                        accountId = aid;
                    }
                }
                else
                {
                    success = false;
                    accountId = 0;
                }
            }
        }
    }

    owner_->RemovePendingLogin(dbAck.seq);

    LSLoginAck out;
    out.seq = dbAck.seq;
    out.clientSessionId = pending.clientSessionId;
    out.success = success ? 1 : 0;
    out.resultCode = static_cast<uint32_t>(result);
    out.accountId = accountId;

    auto bytes = out.Build();
    if (!owner_->SendToMain(bytes))
    {
        std::printf("[WARN][LS] SendToMain failed (LS_LOGIN_ACK). seq=%u\n", out.seq);
        LOG_WARN(std::string("[LS] SendToMain failed (LS_LOGIN_ACK). seq=") +
            std::to_string(out.seq));
        return;
    }

    std::printf("[INFO][LS] Sent LS_LOGIN_ACK seq=%u clientSessionId=%llu success=%u result=%u\n",
        out.seq,
        static_cast<unsigned long long>(out.clientSessionId),
        static_cast<unsigned>(out.success),
        static_cast<unsigned>(out.resultCode));

    LOG_INFO(
        std::string("[LS] Sent LS_LOGIN_ACK seq=") +
        std::to_string(out.seq) +
        " clientSessionId=" +
        std::to_string(static_cast<unsigned long long>(out.clientSessionId)) +
        " success=" +
        std::to_string(static_cast<unsigned>(out.success)) +
        " result=" +
        std::to_string(static_cast<unsigned>(out.resultCode))
    );

}

void LSPacketHandler::HandleLogoutNoti(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    LSLogoutNotify notify;
    if (!notify.ParsePayload(payload, length))
    {
        std::printf("[WARN][LS] LS_LOGOUT_NOTIFY parse failed. len=%zu\n", length);
        LOG_WARN(std::string("[LS] LS_LOGOUT_NOTIFY parse failed. len=") + std::to_string(length));
        return;
    }

    owner_->UnmarkLoggedInByAccount(notify.accountId);

    std::printf("[LOGIN] LOGOUT_NOTIFY accountId=%llu removed from online list\n",
        static_cast<unsigned long long>(notify.accountId));
    LOG_INFO(std::string("[LOGIN] LOGOUT_NOTIFY accountId=") + std::to_string(notify.accountId) + std::string("removed from online list"));
    
}

ELoginResult LSPacketHandler::VerifyLoginPassword(const PendingLogin& pending, const LSDbFindAccountAck& dbAck) const
{
    if (dbAck.passwordSalt.empty() || dbAck.passwordHash.empty())
        return ELoginResult::INTERNAL_ERROR;

    const bool ok = PasswordHasher::VerifyPassword(
        pending.plainPw,
        dbAck.passwordSalt,
        dbAck.passwordHash);

    return ok ? ELoginResult::OK : ELoginResult::INVALID_PASSWORD;
}
