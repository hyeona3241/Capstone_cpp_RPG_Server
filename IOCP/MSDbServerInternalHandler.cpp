#include "Pch.h"
#include "MSDbServerInternalHandler.h"

#include "MainServer.h"
#include "Session.h"
#include "PacketDef.h"
#include "PacketIds.h"
#include <Logger.h>

#include "DbPingPackets.h"
#include "DBAuthPackets.h"
#include "CRegisterPackets.h"
#include "DBRegisterPackets.h"
#include "LSLoginPackets.h"

MSDbServerInternalHandler::MSDbServerInternalHandler(MainServer* owner)
	: owner_(owner)
{
}

bool MSDbServerInternalHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
	return (id >= begin) && (id < endExclusive);
}

bool MSDbServerInternalHandler::Handle(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
	const uint16_t id = header.id;

	if (!InRange(id, 3000, 4000))
		return false;

    switch (header.id)
    {
    case PacketType::to_id(PacketType::DB::DB_PING_ACK):
    {
        if (session != owner_->GetDbSession())
        {
            std::printf("[WARN][DbSrv] DB_PING_ACK from non-db session\n");
            LOG_WARN("[DbSrv] DB_PING_ACK from non-db session");
            return true;
        }

        DBPingAck ack;
        if (!ack.ParsePayload(payload, length))
        {
            std::printf("[WARN][DbSrv] DB_PING_ACK parse failed (len=%zu)\n", length);
            LOG_WARN(std::string("[DbSrv] DB_PING_ACK parse failed (len=") + std::to_string(length) + ")");
            return true;
        }

        std::printf("[INFO][DbSrv] DB_PING_ACK seq=%u tick=%llu\n",
            ack.seq, static_cast<unsigned long long>(ack.serverTick));
        LOG_INFO(std::string("[DbSrv] DB_PING_ACK seq=") + std::to_string(ack.seq));
        return true;
    }

    case PacketType::to_id(PacketType::DB::DB_FIND_ACCOUNT_ACK):
    {
        OnDbFindAccountAck(session, payload, length);
        return true;
    }

    case PacketType::to_id(PacketType::DB::DB_REGISTER_ACK):
    {
        OnDbRegisterAck(session, payload, length);
        return true;
    }

    default:
        std::printf("[WARN][DbSrv] Unhandled db packet id=%u len=%zu\n",
            static_cast<unsigned>(header.id), length);
        LOG_WARN(std::string("[DbSrv] Unhandled db packet id=") +
            std::to_string(static_cast<unsigned>(header.id)));
        return true;
    }
}


void MSDbServerInternalHandler::OnDbFindAccountAck(Session* session, const std::byte* payload, std::size_t length)
{
    if (session != owner_->GetDbSession())
    {
        LOG_WARN("[DbSrv] DB_FIND_ACCOUNT_ACK from non-db session");
        return;
    }

    DBFindAccountAck dbAck;
    if (!dbAck.ParsePayload(payload, length))
    {
        LOG_WARN("[DbSrv] DB_FIND_ACCOUNT_ACK parse failed");
        return;
    }

    ForwardToLoginServer_FindAccountAck(dbAck);
}

void MSDbServerInternalHandler::OnDbRegisterAck(Session* session, const std::byte* payload, std::size_t length)
{
    if (session != owner_->GetDbSession())
    {
        std::printf("[WARN][DbSrv] DB_REGISTER_ACK from non-db session\n");
        LOG_WARN("[DbSrv] DB_REGISTER_ACK from non-db session");
        return;
    }

    DbRegisterAck dbAck;
    if (!dbAck.ParsePayload(payload, length))
    {
        std::printf("[WARN][DbSrv] DB_REGISTER_ACK parse failed (len=%zu)\n", length);
        LOG_WARN(std::string("[DbSrv] DB_REGISTER_ACK parse failed (len=") + std::to_string(length) + ")");
        return;
    }

    ForwardToClient_RegisterAck(dbAck);
}

void MSDbServerInternalHandler::ForwardToLoginServer_FindAccountAck(const DBFindAccountAck& dbAck)
{
    Session* login = owner_->GetLoginSession();
    if (!login)
    {
        LOG_WARN(std::string("[MS] LoginServer not connected. cannot forward DB_FIND_ACCOUNT_ACK. seq=") +
            std::to_string(dbAck.seq));
        return;
    }

    LSDbFindAccountAck lsAck;
    lsAck.seq = dbAck.seq;
    lsAck.exists = dbAck.exists;

    if (dbAck.exists != 0)
    {
        lsAck.accountId = dbAck.accountId;
        lsAck.passwordHash = dbAck.passwordHash;
        lsAck.passwordSalt = dbAck.passwordSalt;
        lsAck.state = dbAck.status;
        lsAck.resultCode = static_cast<uint32_t>(ELoginResult::OK);
    }
    else
    {
        lsAck.resultCode = static_cast<uint32_t>(ELoginResult::NO_SUCH_USER);
    }

    auto bytes = lsAck.Build();
    login->Send(bytes.data(), bytes.size());

    LOG_INFO(std::string("[MS] Forward DB_FIND_ACCOUNT_ACK -> LS_DB_FIND_ACCOUNT_ACK. seq=") +
        std::to_string(lsAck.seq) +
        " exists=" + std::to_string(lsAck.exists));
}

void MSDbServerInternalHandler::ForwardToClient_RegisterAck(const DbRegisterAck& dbAck)
{
    Session* client = owner_->FindClientSession(dbAck.clientSessionId);
    if (!client)
    {
        std::printf("[WARN][MS] Client session not found. clientSessionId=%llu seq=%u\n",
            static_cast<unsigned long long>(dbAck.clientSessionId), dbAck.seq);
        LOG_WARN(std::string("[MS] Client session not found. clientSessionId=") +
            std::to_string(static_cast<unsigned long long>(dbAck.clientSessionId)));
        return;
    }

    if (dbAck.success != 0)
    {
        RegisterAck ack = RegisterAck::MakeOk(dbAck.accountId);
        auto bytes = ack.Build();
        client->Send(bytes.data(), bytes.size());

        std::printf("[INFO][MS] REGISTER success. clientSessionId=%llu accountId=%llu\n",
            static_cast<unsigned long long>(dbAck.clientSessionId),
            static_cast<unsigned long long>(dbAck.accountId));
        LOG_INFO("[MS] REGISTER success.");
    }
    else
    {
        RegisterAck ack;
        ack.success = 0;
        ack.resultCode = dbAck.resultCode;
        auto bytes = ack.Build();
        client->Send(bytes.data(), bytes.size());

        std::printf("[INFO][MS] REGISTER failed. clientSessionId=%llu result=%u\n",
            static_cast<unsigned long long>(dbAck.clientSessionId),
            static_cast<unsigned>(dbAck.resultCode));
        LOG_INFO("[MS] REGISTER failed.");
    }
}

