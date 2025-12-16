#include "Pch.h"
#include "MSPacketHandler.h"
#include "MainServer.h"
#include "Session.h"

#include "PacketDef.h"
#include "PacketIds.h"
#include "DbPingPackets.h"
#include "LSPingPackets.h"
#include "CPingPackets.h"

#include "CLoginPackets.h"
#include "LSLoginPackets.h"
#include "DBAuthPackets.h"
#include <Logger.h>

#include "MSClientInternalHandler.h"
#include "MSDbServerInternalHandler.h"
#include "MSChatServerInternalHandler.h"

#include "CCSAuthPackets.h"
#include "LoginServer.h"

MSPacketHandler::MSPacketHandler(MainServer* owner)
    : owner_(owner)
{
    clientHandler_ = std::make_unique<MSClientInternalHandler>(owner_);
    dbHandler_ = std::make_unique<MSDbServerInternalHandler>(owner_);
    chatHandler_ = std::make_unique<MSChatServerInternalHandler>(owner_);
}
MSPacketHandler::~MSPacketHandler() = default;

// 정적 범위 체크
bool MSPacketHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
    return (id >= begin) && (id < endExclusive);
}

//  클라이언트 -> 메인 서버 패킷
void MSPacketHandler::HandleFromClient(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    if (InRange(id, 1000, 2000))
    {
        if (!clientHandler_->Handle(session, header, payload, length))
        {
            std::printf("[WARN][MSPacketHandler] Unknown client packet id=%u (sessionId=%llu)\n", id, static_cast<unsigned long long>(session->GetId()));
            LOG_WARN(
                std::string("[MSPacketHandler] Unknown client packet id=") +
                std::to_string(id) +
                " (sessionId=" +
                std::to_string(static_cast<unsigned long long>(session->GetId())) +
                ")"
            );
            // 필요하면 여기서 프로토콜 에러로 세션을 끊을 수도 있음
            // 나중에 공격인지 아닌지 판단하고 로직 정하면 될듯
            // session->Disconnect();
        }
    }
}

//  LoginServer -> 메인 서버 내부 패킷
void MSPacketHandler::HandleFromLoginServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 예시: 2000 ~ 2999 범위를 LoginServer <-> MainServer 용도로 사용
    if (InRange(id, 2000, 3000))
    {
        HandleLoginServerInternal(session, header, payload, length);
    }
    else
    {
        std::printf("[WARN][MSPacketHandler] Unknown login-server packet id=%u\n", id);
        LOG_WARN(
            std::string("[MSPacketHandler] Unknown login-server packet id=") +
            std::to_string(id)
        );
    }
}


//  DbServer -> 메인 서버 내부 패킷
void MSPacketHandler::HandleFromDbServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 예시: 3000 ~ 3999 범위를 DbServer <-> MainServer 용도로 사용
    if (InRange(id, 3000, 4000))
    {
        if (!dbHandler_->Handle(session, header, payload, length))
        {
            std::printf("[WARN][MSPacketHandler] Unknown db-server packet id=%u\n",
                static_cast<unsigned>(header.id));
            LOG_WARN(std::string("[MSPacketHandler] Unknown db-server packet id=") +
                std::to_string(static_cast<unsigned>(header.id)));
        }
    }
}

void MSPacketHandler::HandleFromChatServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 예시: 4000 ~ 4999 범위를 ChatServer <-> MainServer 용도로 사용
    if (InRange(id, 4000, 5000))
    {
        if (!chatHandler_->Handle(session, header, payload, length))
        {
            std::printf("[WARN][MSPacketHandler] Unknown chat-server packet id=%u\n",
                static_cast<unsigned>(header.id));
            LOG_WARN(std::string("[MSPacketHandler] Unknown chat-server packet id=") +
                std::to_string(static_cast<unsigned>(header.id)));
        }
    }
}


//void MSPacketHandler::HandleClientLobby(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
//{
//    if (session->GetState() != SessionState::Lobby)
//    {
//        std::printf("[WARN][Lobby] Packet in non-lobby state. "
//            "sessionId=%llu, state=%d, pktId=%u\n",
//            static_cast<unsigned long long>(session->GetId()),
//            static_cast<int>(session->GetState()),
//            static_cast<unsigned>(header.id));
//        LOG_WARN(
//            std::string("[Lobby] Packet in non-lobby state. sessionId=") +
//            std::to_string(static_cast<unsigned long long>(session->GetId())) +
//            ", state=" +
//            std::to_string(static_cast<int>(session->GetState())) +
//            ", pktId=" +
//            std::to_string(static_cast<unsigned>(header.id))
//        );
//        return;
//    }
//
//    std::printf("[DEBUG][Lobby] HandleClientLobby: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
//    LOG_INFO(
//        std::string("[Lobby] HandleClientLobby: pktId=") +
//        std::to_string(static_cast<unsigned>(header.id)) +
//        ", len=" +
//        std::to_string(length)
//    );
//}
//
//void MSPacketHandler::HandleClientWorld(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
//{
//    if (session->GetState() != SessionState::World)
//    {
//        std::printf("[WARN][World] Packet in non-world state. "
//            "sessionId=%llu, state=%d, pktId=%u\n",
//            static_cast<unsigned long long>(session->GetId()),
//            static_cast<int>(session->GetState()),
//            static_cast<unsigned>(header.id));
//        LOG_WARN(
//            std::string("[World] Packet in non-world state. sessionId=") +
//            std::to_string(static_cast<unsigned long long>(session->GetId())) +
//            ", state=" +
//            std::to_string(static_cast<int>(session->GetState())) +
//            ", pktId=" +
//            std::to_string(static_cast<unsigned>(header.id))
//        );
//        return;
//    }
//
//    std::printf("[DEBUG][World] HandleClientWorld: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
//    LOG_INFO(
//        std::string("[World] HandleClientWorld: pktId=") +
//        std::to_string(static_cast<unsigned>(header.id)) +
//        ", len=" +
//        std::to_string(length)
//    );
//
//}



void MSPacketHandler::HandleLoginServerInternal(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    case PacketType::to_id(PacketType::Login::LS_PING_ACK):
    {
        if (session != owner_->GetLoginSession())
        {
            std::printf("[WARN][LoginSrv] LS_PING_ACK from non-login session\n");
            LOG_WARN("[LoginSrv] LS_PING_ACK from non-login session");
            return;
        }

        LSPingAck ack;
        if (!ack.ParsePayload(payload, length))
        {
            std::printf("[WARN][LoginSrv] LS_PING_ACK parse failed (len=%zu)\n", length);
            LOG_WARN(
                std::string("[LoginSrv] LS_PING_ACK parse failed (len=") +
                std::to_string(length) +
                ")"
            );
            return;
        }

        std::printf("[INFO][LoginSrv] LS_PING_ACK seq=%u, serverTick=%llu (from sessionId=%llu)\n",
            ack.seq,
            static_cast<unsigned long long>(ack.serverTick),
            static_cast<unsigned long long>(session->GetId()));
        LOG_INFO(
            std::string("[LoginSrv] LS_PING_ACK seq=") +
            std::to_string(ack.seq) +
            ", serverTick=" +
            std::to_string(static_cast<unsigned long long>(ack.serverTick)) +
            " (from sessionId=" +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ")"
        );

        return;
    }

    case PacketType::to_id(PacketType::Login::LS_DB_FIND_ACCOUNT_REQ):
    {
        OnLsDbFindAccountReq(session, payload, length);
        return;
    }

    case PacketType::to_id(PacketType::Login::LS_LOGIN_ACK):
    {
        OnLsLoginAck(session, payload, length);
        return;
    }

    default:
        std::printf("[DEBUG][LoginSrv] HandleLoginServerInternal: pktId=%u, len=%zu\n",
            static_cast<unsigned>(header.id), length);
        LOG_INFO(
            std::string("[LoginSrv] HandleLoginServerInternal: pktId=") +
            std::to_string(static_cast<unsigned>(header.id)) +
            ", len=" +
            std::to_string(length)
        );
        return;
    }
}

void MSPacketHandler::HandleDbServerInternal(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    switch (header.id)
    {
    case PacketType::to_id(PacketType::DB::DB_PING_ACK):
    {
        if (session != owner_->GetDbSession())
        {
            std::printf("[WARN][DbSrv] DB_PING_ACK from non-db session\n");
            LOG_WARN("[DbSrv] DB_PING_ACK from non-db session");
            return;
        }

        DBPingAck ack;
        if (!ack.ParsePayload(payload, length))
        {
            std::printf("[WARN][DbSrv] DB_PING_ACK parse failed (len=%zu)\n", length);
            LOG_WARN(
                std::string("[DbSrv] DB_PING_ACK parse failed (len=") +
                std::to_string(length) +
                ")"
            );
            return;
        }

        std::printf("[INFO][DbSrv] DB_PING_ACK seq=%u, serverTick=%llu (from sessionId=%llu)\n",
            ack.seq,
            static_cast<unsigned long long>(ack.serverTick),
            static_cast<unsigned long long>(session->GetId()));
        LOG_INFO(
            std::string("[DbSrv] DB_PING_ACK seq=") +
            std::to_string(ack.seq) +
            ", serverTick=" +
            std::to_string(static_cast<unsigned long long>(ack.serverTick)) +
            " (from sessionId=" +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ")"
        );

        return;
    }

    case PacketType::to_id(PacketType::DB::DB_FIND_ACCOUNT_ACK):
    {
        OnDbFindAccountAck(session, payload, length);
        return;
    }

    default:
        std::printf("[DEBUG][DbSrv] HandleDbServerInternal: pktId=%u, len=%zu\n",
            static_cast<unsigned>(header.id), length);
        LOG_INFO(
            std::string("[DbSrv] HandleDbServerInternal: pktId=") +
            std::to_string(static_cast<unsigned>(header.id)) +
            ", len=" +
            std::to_string(length)
        );
        return;
    }
}

void MSPacketHandler::FailLoginBySeq(uint32_t seq, ELoginResult result)
{
    Session* login = owner_->GetLoginSession();
    if (!login)
    {
        std::printf("[WARN][MS] FailLoginBySeq: LoginServer not connected. seq=%u result=%u\n",
            seq, static_cast<unsigned>(result));
        LOG_WARN(
            std::string("[MS] FailLoginBySeq: LoginServer not connected. seq=") +
            std::to_string(seq) +
            " result=" +
            std::to_string(static_cast<unsigned>(result))
        );
        return;
    }

    LSDbFindAccountAck ack;
    ack.seq = seq;

    ack.exists = false;
    ack.resultCode = static_cast<uint32_t>(result);

    auto bytes = ack.Build();
    login->Send(bytes.data(), bytes.size());

    std::printf("[INFO][MS] Sent LS_DB_FIND_ACCOUNT_ACK fail. seq=%u result=%u\n",
        seq, static_cast<unsigned>(result));
    LOG_INFO(
        std::string("[MS] Sent LS_DB_FIND_ACCOUNT_ACK fail. seq=") +
        std::to_string(seq) +
        " result=" +
        std::to_string(static_cast<unsigned>(result))
    );
}

void MSPacketHandler::OnLsDbFindAccountReq(Session* session, const std::byte* payload, std::size_t length)
{
    if (session != owner_->GetLoginSession())
    {
        std::printf("[WARN][LoginSrv] LS_DB_FIND_ACCOUNT_REQ from non-login session\n");
        LOG_WARN("[LoginSrv] LS_DB_FIND_ACCOUNT_REQ from non-login session");
        return;
    }

    LSDbFindAccountReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][LoginSrv] LS_DB_FIND_ACCOUNT_REQ parse failed (len=%zu)\n", length);
        LOG_WARN(
            std::string("[LoginSrv] LS_DB_FIND_ACCOUNT_REQ parse failed (len=") +
            std::to_string(length) +
            ")"
        );
        return;
    }

    Session* db = owner_->GetDbSession();
    if (!db)
    {
        std::printf("[WARN][LoginSrv] DBServer not connected. seq=%u\n", req.seq);
        LOG_WARN(
            std::string("[LoginSrv] DBServer not connected. seq=") +
            std::to_string(req.seq)
        );

        FailLoginBySeq(req.seq, ELoginResult::DB_ERROR);
        return;
    }

    // Main -> DB : DB_FIND_ACCOUNT_REQ
    DBFindAccountReq dbReq;
    dbReq.seq = req.seq;
    dbReq.loginId = req.loginId;

    auto bytes = dbReq.Build();
    db->Send(bytes.data(), bytes.size());

    std::printf("[INFO][MS] Forward LS_DB_FIND_ACCOUNT_REQ -> DB_FIND_ACCOUNT_REQ. seq=%u, loginId=%s\n",
        dbReq.seq, dbReq.loginId.c_str());
    LOG_INFO(
        std::string("[MS] Forward LS_DB_FIND_ACCOUNT_REQ -> DB_FIND_ACCOUNT_REQ. seq=") +
        std::to_string(dbReq.seq) +
        ", loginId=" +
        dbReq.loginId
    );

    return;
}

void MSPacketHandler::OnDbFindAccountAck(Session* session, const std::byte* payload, std::size_t length)
{
    if (session != owner_->GetDbSession())
    {
        std::printf("[WARN][DbSrv] DB_FIND_ACCOUNT_ACK from non-db session\n");
        LOG_WARN("[DbSrv] DB_FIND_ACCOUNT_ACK from non-db session");
        return;
    }

    DBFindAccountAck dbAck;
    if (!dbAck.ParsePayload(payload, length))
    {
        std::printf("[WARN][DbSrv] DB_FIND_ACCOUNT_ACK parse failed (len=%zu)\n", length);
        LOG_WARN(
            std::string("[DbSrv] DB_FIND_ACCOUNT_ACK parse failed (len=") +
            std::to_string(length) +
            ")"
        );
        return;
    }

    Session* login = owner_->GetLoginSession();
    if (!login)
    {
        std::printf("[WARN][MS] LoginServer not connected. cannot forward DB_FIND_ACCOUNT_ACK. seq=%u\n", dbAck.seq);
        LOG_WARN(
            std::string("[MS] LoginServer not connected. cannot forward DB_FIND_ACCOUNT_ACK. seq=") +
            std::to_string(dbAck.seq)
        );
        return;
    }

    // Main -> Login : LS_DB_FIND_ACCOUNT_ACK
    LSDbFindAccountAck lsAck;
    lsAck.seq = dbAck.seq;
    lsAck.exists = dbAck.exists;

    if (dbAck.exists != 0)
    {
        lsAck.accountId = dbAck.accountId;
        lsAck.passwordHash = std::move(dbAck.passwordHash);
        lsAck.passwordSalt = std::move(dbAck.passwordSalt);
        lsAck.state = dbAck.status;

        lsAck.resultCode = static_cast<uint32_t>(ELoginResult::OK);
    }
    else
    {
        lsAck.resultCode = static_cast<uint32_t>(ELoginResult::NO_SUCH_USER);
    }

    auto bytes = lsAck.Build();
    login->Send(bytes.data(), bytes.size());

    std::printf("[INFO][MS] Forward DB_FIND_ACCOUNT_ACK -> LS_DB_FIND_ACCOUNT_ACK. seq=%u exists=%u\n",
        lsAck.seq, lsAck.exists);
    LOG_INFO(
        std::string("[MS] Forward DB_FIND_ACCOUNT_ACK -> LS_DB_FIND_ACCOUNT_ACK. seq=") +
        std::to_string(lsAck.seq) +
        " exists=" +
        std::to_string(lsAck.exists)
    );
    return;
}

void MSPacketHandler::OnLsLoginAck(Session* session, const std::byte* payload, std::size_t length)
{
    if (session != owner_->GetLoginSession())
    {
        std::printf("[WARN][LoginSrv] LS_LOGIN_ACK from non-login session\n");
        LOG_WARN("[LoginSrv] LS_LOGIN_ACK from non-login session");
        return;
    }

    LSLoginAck lsAck;
    if (!lsAck.ParsePayload(payload, length))
    {
        std::printf("[WARN][LoginSrv] LS_LOGIN_ACK parse failed (len=%zu)\n", length);
        LOG_WARN(
            std::string("[LoginSrv] LS_LOGIN_ACK parse failed (len=") +
            std::to_string(length) +
            ")"
        );
        return;
    }

    // 1) client session 찾기
    Session* client = owner_->FindClientSession(lsAck.clientSessionId);
    if (!client)
    {
        std::printf("[WARN][MS] Client session not found. clientSessionId=%llu seq=%u\n",
            static_cast<unsigned long long>(lsAck.clientSessionId), lsAck.seq);
        LOG_WARN(
            std::string("[MS] Client session not found. clientSessionId=") +
            std::to_string(static_cast<unsigned long long>(lsAck.clientSessionId)) +
            " seq=" +
            std::to_string(lsAck.seq)
        );
        return;
    }

    //Client에게 LOGIN_ACK 전송
    if (lsAck.success != 0)
    {
        owner_->HandlePostLoginForChat(client, lsAck.accountId);

        // 성공
        auto ack = LoginAck::MakeSuccess(lsAck.accountId);
        auto bytes = ack.Build();
        client->Send(bytes.data(), bytes.size());

        client->SetState(SessionState::Lobby);

        // DB에 last_login 갱신 요청
        Session* db = owner_->GetDbSession();
        if (!db)
        {
            std::printf("[WARN][MS] DBServer not connected. skip DB_UPDATE_LASTLOGIN_REQ. accountId=%llu\n",
                static_cast<unsigned long long>(lsAck.accountId));
            LOG_WARN(
                std::string("[MS] DBServer not connected. skip DB_UPDATE_LASTLOGIN_REQ. accountId=") +
                std::to_string(static_cast<unsigned long long>(lsAck.accountId))
            );
            return;
        }

        DBUpdateLastLoginReq up;
        up.seq = owner_->NextLoginSeq();
        up.accountId = lsAck.accountId;

        auto upBytes = up.Build();
        db->Send(upBytes.data(), upBytes.size());

        std::printf("[INFO][MS] LOGIN success. Sent LOGIN_ACK and DB_UPDATE_LASTLOGIN_REQ. clientSessionId=%llu accountId=%llu\n",
            static_cast<unsigned long long>(lsAck.clientSessionId),
            static_cast<unsigned long long>(lsAck.accountId));
        LOG_INFO(
            std::string("[MS] LOGIN success. Sent LOGIN_ACK and DB_UPDATE_LASTLOGIN_REQ. clientSessionId=") +
            std::to_string(static_cast<unsigned long long>(lsAck.clientSessionId)) +
            " accountId=" +
            std::to_string(static_cast<unsigned long long>(lsAck.accountId))
        );
    }
    else
    {
        // 실패
        auto reason = static_cast<ELoginResult>(lsAck.resultCode);
        auto ack = LoginAck::MakeFail(reason);
        auto bytes = ack.Build();
        client->Send(bytes.data(), bytes.size());

        // 실패면 로그인 상태 원복
        client->SetState(SessionState::Handshake);

        std::printf("[INFO][MS] LOGIN failed. Sent LOGIN_ACK fail. clientSessionId=%llu result=%u\n",
            static_cast<unsigned long long>(lsAck.clientSessionId),
            static_cast<unsigned>(lsAck.resultCode));
        LOG_INFO(
            std::string("[MS] LOGIN failed. Sent LOGIN_ACK fail. clientSessionId=") +
            std::to_string(static_cast<unsigned long long>(lsAck.clientSessionId)) +
            " result=" +
            std::to_string(static_cast<unsigned>(lsAck.resultCode))
        );

    }
}
