#include "Pch.h"
#include "MSClientInternalHandler.h"

#include "MainServer.h"
#include "Session.h"
#include "PacketDef.h"
#include "PacketIds.h"

#include "CLoginPackets.h"     // LoginReq/LoginAck
#include "LSLoginPackets.h"    // LSLoginReq
#include "CPingPackets.h"

#include "CRegisterPackets.h"
#include "DBRegisterPackets.h"
#include "CLogoutPackets.h"
#include "LSLogoutPackets.h"

#include <Logger.h>
#include <cstdio>

MSClientInternalHandler::MSClientInternalHandler(MainServer* owner)
	: owner_(owner)
{
}

bool MSClientInternalHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
    return (id >= begin) && (id < endExclusive);
}

bool MSClientInternalHandler::Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    const uint16_t id = header.id;

    if (!InRange(id, 1000, 2000)) 
        return false;

    if (InRange(id, 1000, 1100))
    {
        return HandleHandshake(session, header, payload, length);
    }
    if (InRange(id, 1100, 1200))
    {
        return HandleAuth(session, header, payload, length);
    }

    // 클라 범위 안인데 도메인 없음
    LOG_WARN("[Client] In client range but no domain matched. pktId=%u", (unsigned)id);
    return true;
}

bool MSClientInternalHandler::HandleHandshake(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return false;

    if (session->GetState() != SessionState::Handshake)
    {
        std::printf("[WARN][Handshake] Invalid state for handshake packet. "
            "sessionId=%llu, state=%d, pktId=%u\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(session->GetState()),
            static_cast<unsigned>(header.id));
        LOG_WARN(
            std::string("[Handshake] Invalid state for handshake packet. sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ", state=" +
            std::to_string(static_cast<int>(session->GetState())) +
            ", pktId=" +
            std::to_string(static_cast<unsigned>(header.id))
        );
        return true;
    }

    switch (header.id)
    {
    case PacketType::to_id(PacketType::Client::C_PING_REQ):
    {
        return HandlePingReq(session, payload, length);
    }

    default:
        std::printf("[Warn][Handshake] Unhandled handshake pktId=%u, len=%zu\n",
            static_cast<unsigned>(header.id), length);
        LOG_WARN("[Handshake] Unhandled auth pktId=%u len=%zu", (unsigned)header.id, length);
        return true;
    }
}

bool MSClientInternalHandler::HandlePingReq(Session* session, const std::byte* payload, std::size_t length)
{
    CPingReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][Handshake] C_PING_REQ parse failed (len=%zu, sessionId=%llu)\n",
            length, static_cast<unsigned long long>(session->GetId()));
        LOG_WARN(
            std::string("[Handshake] C_PING_REQ parse failed (len=") +
            std::to_string(length) +
            ", sessionId=" +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ")"
        );
        return true;
    }

    CPingAck ack;
    ack.seq = req.seq;
    ack.serverTick = 0; // 나중에 tick 붙이면 수정

    auto bytes = ack.Build();
    session->Send(bytes.data(), bytes.size());

    std::printf("[INFO][Handshake] C_PING_ACK sent. seq=%u (sessionId=%llu)\n",
        ack.seq, static_cast<unsigned long long>(session->GetId()));
    LOG_INFO(
        std::string("[Handshake] C_PING_ACK sent. seq=") +
        std::to_string(ack.seq) +
        " (sessionId=" +
        std::to_string(static_cast<unsigned long long>(session->GetId())) +
        ")"
    );
    return true;
}

bool MSClientInternalHandler::HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return false;

    if (header.id == PacketType::to_id(PacketType::Client::LOGOUT_REQ))
    {
        return HandleLogoutReq(session, payload, length);
    }

    if (session->GetState() != SessionState::Login && session->GetState() != SessionState::Handshake)
    {
        std::printf("[WARN][Login] Invalid state for login packet. sessionId=%llu state=%d pktId=%u\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(session->GetState()),
            static_cast<unsigned>(header.id));
        LOG_WARN(
            std::string("[Login] Invalid state for login packet. sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId())) +
            ", state=" + std::to_string(static_cast<int>(session->GetState())) +
            ", pktId=" + std::to_string(static_cast<unsigned>(header.id))
        );
        return true;
    }

    switch (header.id)
    {
    case PacketType::to_id(PacketType::Client::LOGIN_REQ):
    {
        if (session->GetState() == SessionState::Login)
        {
            LoginAck ack = LoginAck::MakeFail(ELoginResult::INTERNAL_ERROR);
            auto bytes = ack.Build();
            session->Send(bytes.data(), bytes.size());

            std::printf("[WARN][LOGIN] Duplicate LOGIN_REQ blocked. sessionId=%llu\n",
                static_cast<unsigned long long>(session->GetId()));
            LOG_WARN(
                std::string("[LOGIN] Duplicate LOGIN_REQ blocked. sessionId=") +
                std::to_string(static_cast<unsigned long long>(session->GetId()))
            );
            return true;
        }

        LoginReq clientReq;
        if (!clientReq.ParsePayload(payload, length))
        {
            std::printf("[WARN][LOGIN] LOGIN_REQ parse failed. sessionId=%llu\n",
                static_cast<unsigned long long>(session->GetId()));
            LOG_WARN(
                std::string("[LOGIN] LOGIN_REQ parse failed. sessionId=") +
                std::to_string(static_cast<unsigned long long>(session->GetId()))
            );
            return true;
        }

        session->SetState(SessionState::Login);

        ForwardLoginReqToLoginServer(session, clientReq);
        return true;
    }
    case PacketType::to_id(PacketType::Client::REGISTER_REQ):
    {
        RegisterReq clientReq;
        if (!clientReq.ParsePayload(payload, length))
        {
            std::printf("[WARN][REGISTER] REGISTER_REQ parse failed. sessionId=%llu\n",
                static_cast<unsigned long long>(session->GetId()));
            LOG_WARN(
                std::string("[REGISTER] REGISTER_REQ parse failed. sessionId=") +
                std::to_string(static_cast<unsigned long long>(session->GetId()))
            );
            return true;
        }

        ForwardRegisterReqToDbServer(session, clientReq);
        return true;
    }
    default:
        LOG_WARN("[Auth] Unhandled auth pktId=%u len=%zu", (unsigned)header.id, length);
        return true;
    }
}

void MSClientInternalHandler::ForwardLoginReqToLoginServer(Session* clientSession, const LoginReq& clientReq)
{
    Session* ls = owner_->GetLoginSession();
    if (!ls)
    {
        std::printf("[WARN][LOGIN] LoginServer not connected.\n");
        LOG_WARN("[LOGIN] LoginServer not connected.");
        clientSession->SetState(SessionState::Handshake);

        LoginAck ack = LoginAck::MakeFail(ELoginResult::INTERNAL_ERROR);
        auto bytes = ack.Build();
        clientSession->Send(bytes.data(), bytes.size());
        return;
    }

    LSLoginReq lsReq;
    lsReq.seq = owner_->NextLoginSeq();
    lsReq.clientSessionId = clientSession->GetId();
    lsReq.loginId = clientReq.loginId;
    lsReq.plainPw = clientReq.plainPw;

    auto bytes = lsReq.Build();
    ls->Send(bytes.data(), bytes.size());

    std::printf("[INFO][LOGIN] Forwarded LOGIN_REQ -> LS_LOGIN_REQ. seq=%u, clientSessionId=%llu\n",
        lsReq.seq, static_cast<unsigned long long>(lsReq.clientSessionId));
    LOG_INFO(
        std::string("[LOGIN] Forwarded LOGIN_REQ -> LS_LOGIN_REQ. seq=") +
        std::to_string(lsReq.seq) +
        ", clientSessionId=" +
        std::to_string(static_cast<unsigned long long>(lsReq.clientSessionId))
    );
}

void MSClientInternalHandler::ForwardRegisterReqToDbServer(Session* clientSession, const RegisterReq& clientReq)
{
    Session* db = owner_->GetDbSession();
    if (!db)
    {
        std::printf("[WARN][REGISTER] DBServer not connected.\n");
        LOG_WARN("[REGISTER] DBServer not connected.");

        RegisterAck ack = RegisterAck::MakeFail(ERegisterResult::DB_ERROR);
        auto bytes = ack.Build();
        clientSession->Send(bytes.data(), bytes.size());
        return;
    }

    DbRegisterReq dbReq;
    dbReq.seq = owner_->NextRegisterSeq();
    dbReq.clientSessionId = clientSession->GetId();
    dbReq.loginId = clientReq.loginId;
    dbReq.plainPw = clientReq.plainPw;

    auto bytes = dbReq.Build();
    db->Send(bytes.data(), bytes.size());

    std::printf("[INFO][REGISTER] Forward REGISTER_REQ -> DB_REGISTER_REQ. seq=%u clientSessionId=%llu loginId=%s\n",
        dbReq.seq,
        static_cast<unsigned long long>(dbReq.clientSessionId),
        dbReq.loginId.c_str());
    LOG_INFO(
        std::string("[REGISTER] Forward REGISTER_REQ -> DB_REGISTER_REQ. seq=") +
        std::to_string(dbReq.seq) +
        " clientSessionId=" +
        std::to_string(static_cast<unsigned long long>(dbReq.clientSessionId)) +
        " loginId=" +
        dbReq.loginId
    );
}

bool MSClientInternalHandler::HandleLogoutReq(Session* session, const std::byte* payload, std::size_t length)
{
    LogoutReq req;
    if (!req.ParsePayload(payload, length))
    {
        std::printf("[WARN][LOGOUT] LOGOUT_REQ parse failed. sessionId=%llu\n",
            static_cast<unsigned long long>(session->GetId()));
        LOG_WARN(
            std::string("[LOGOUT] LOGOUT_REQ parse failed. sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId()))
        );
        return true;
    }

    // 0이면 무시 (클라 버그/로그인 전)
    if (req.accountId == 0)
    {
        std::printf("[WARN][LOGOUT] accountId=0 ignored. sessionId=%llu\n",
            static_cast<unsigned long long>(session->GetId()));
        LOG_WARN(
            std::string("[LOGOUT] accountId=0 ignored. sessionId=") +
            std::to_string(static_cast<unsigned long long>(session->GetId()))
        );
        return true;
    }

    ForwardLogoutNotifyToLoginServer(session, req.accountId);

    session->SetState(SessionState::Handshake);

    return true;
}

void MSClientInternalHandler::ForwardLogoutNotifyToLoginServer(Session* clientSession, uint64_t accountId)
{
    Session* ls = owner_->GetLoginSession();
    if (!ls)
    {
        std::printf("[WARN][LOGOUT] LoginServer not connected. skip notify. accountId=%llu\n",
            static_cast<unsigned long long>(accountId));
        LOG_WARN(
            std::string("[LOGOUT] LoginServer not connected. skip notify. accountId=") +
            std::to_string(static_cast<unsigned long long>(accountId))
        );
        return;
    }

    LSLogoutNotify notify;
    notify.accountId = accountId;

    auto bytes = notify.Build();
    ls->Send(bytes.data(), bytes.size());

    std::printf("[INFO][LOGOUT] Forward LOGOUT_REQ -> LS_LOGOUT_NOTIFY. accountId=%llu sessionId=%llu\n",
        static_cast<unsigned long long>(accountId),
        static_cast<unsigned long long>(clientSession->GetId()));
    LOG_INFO(
        std::string("[LOGOUT] Forward LOGOUT_REQ -> LS_LOGOUT_NOTIFY. accountId=") +
        std::to_string(static_cast<unsigned long long>(accountId)) +
        " sessionId=" +
        std::to_string(static_cast<unsigned long long>(clientSession->GetId()))
    );
}
