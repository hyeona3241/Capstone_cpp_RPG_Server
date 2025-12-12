#include "Pch.h"
#include "DBPacketHandler.h"
#include "DBServer.h"
#include "Session.h"
#include "Common/Protocol/PacketDef.h"
#include "LoginJob.h"

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
    // 3200 ~ 3299 : (옵션) DBServer -> MainServer 응답(ACK)은 DBServer가 "보내는" 것이므로
    //               여기선 보통 받을 일이 없음(프로토콜 실수 검증용으로만 체크 가능)
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
    }
}

void DBPacketHandler::HandleSystem(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
        // 예: DB_PING_REQ = 3000
    case 3000:
        HandlePingReq(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][DBSystem] Unknown system pktId=%u\n", static_cast<unsigned>(header.id));
        break;
    }
}

void DBPacketHandler::HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
        // 예: DB_REGISTER_REQ = 3100
    case 3100:
        HandleRegisterReq(session, header, payload, length);
        break;

        // 예: DB_LOGIN_REQ = 3101
    case 3101:
        HandleLoginReq(session, header, payload, length);
        break;

    default:
        std::printf("[WARN][DBAuth] Unknown auth pktId=%u\n", static_cast<unsigned>(header.id));
        break;
    }
}

void DBPacketHandler::HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    std::printf("[DEBUG][DB] PING_REQ received\n");

    // TODO: PING_ACK 전송 (예: 3001)
    // owner_->SendPingAck(*session);
}

void DBPacketHandler::HandleRegisterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    std::printf("[DEBUG][DB] REGISTER_REQ len=%zu\n", length);

    // TODO: payload parse -> loginId, plainPassword
    // TODO(중요): 여기서 DB 쿼리를 바로 하지 말고 "DBJob"로 넘길 예정

    // AuthService 매핑(명시):
    // AuthService::Register(loginId, plainPassword, outAccountId)
    // 결과 -> DB_REGISTER_ACK(3200)로 응답

    (void)session;
    (void)payload;
}

void DBPacketHandler::HandleLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    std::printf("[DEBUG][DB] LOGIN_REQ len=%zu\n", length);

    // TODO: payload 파싱해서 loginId, plainPassword 추출
    // std::string loginId = ...
    // std::string plainPw = ...

    // replySessionId는 보통 “요청을 보낸 MainServer 세션”의 id
    const uint64_t replySessionId = session->GetId();

    // TODO: 실제 값 파싱 후 사용
    std::string loginId = "TODO";
    std::string plainPw = "TODO";

    owner_->EnqueueJob(std::make_unique<LoginJob>(replySessionId, std::move(loginId), std::move(plainPw)));
}