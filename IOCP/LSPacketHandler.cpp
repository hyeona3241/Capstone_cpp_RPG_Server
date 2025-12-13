#include "LSPacketHandler.h"
#include "LoginServer.h"
#include "Session.h"
#include "PacketDef.h"

#include <cstdio>

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
        break;
    }
}

void LSPacketHandler::HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    LSPingReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][LS] PING_REQ parse failed\n");
        return;
    }

    LSPingAck ack;
    ack.seq = req.seq;
    ack.serverTick =  GetTickMs();

    auto bytes = ack.Build();
    session->Send(bytes.data(), bytes.size());

    std::printf("[DEBUG][LS] PING_ACK seq=%u\n", ack.seq);
}

void LSPacketHandler::HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    /*case PacketType::to_id(PacketType::Login::LS_LOGIN_REQ):
        HandleLoginReq(session, header, payload, length);
        break;

    case PacketType::to_id(PacketType::Login::LS_REGISTER_REQ):
        HandleRegisterReq(session, header, payload, length);
        break;*/

    default:
        std::printf("[WARN][LS:Auth] Unknown pktId=%u\n",
            static_cast<unsigned>(header.id));
        break;
    }
}

void LSPacketHandler::HandleLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    std::printf("[DEBUG][LS] LOGIN_REQ len=%zu\n", length);

    // 1) payload 파싱 (id / pw)
    // 2) pending(seq) 등록
    // 3) MainServer로 "DB 로그인 조회 요청" 전송
    //    → owner_->SendToMain(...)
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
   /* case PacketType::to_id(PacketType::Login::LS_DB_LOGIN_RESULT):
        HandleDbLoginResult(session, header, payload, length);
        break;*/

    default:
        std::printf("[WARN][LS:DB] Unknown pktId=%u\n",
            static_cast<unsigned>(header.id));
        break;
    }
}

void LSPacketHandler::HandleDbLoginResult(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    // 1) pending(seq) 찾기
    // 2) PasswordHasher.Verify()
    // 3) 성공/실패 판단
    // 4) MainServer에 LOGIN_AUTH_ACK 전송
}