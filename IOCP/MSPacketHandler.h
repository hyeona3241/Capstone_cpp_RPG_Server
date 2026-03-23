#pragma once

#include <cstddef> 
#include <cstdint>
#include "ELoginResult.h"

class MainServer;
class Session;
struct PacketHeader;

class LoginReq;
class MSClientInternalHandler;
class MSDbServerInternalHandler;
class MSChatServerInternalHandler;

class MSPacketHandler
{
public:
    explicit MSPacketHandler(MainServer* owner);
    ~MSPacketHandler();

    // 클라이언트 -> 메인 서버로 온 패킷
    void HandleFromClient(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // LoginServer -> 메인 서버로 온 내부 패킷
    void HandleFromLoginServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // DbServer -> 메인 서버로 온 내부 패킷
    void HandleFromDbServer(Session* session, const PacketHeader& header,  const std::byte* payload, std::size_t length);
    
    // ChatSErver -> 메인 서버로 온 내부 패킷
    void HandleFromChatServer(Session* session, const PacketHeader& header,  const std::byte* payload, std::size_t length);

private:

    /*void HandleClientLobby(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleClientWorld(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);*/

    // 내부 서버에서 올라온 패킷 처리용 (응답/알림 등)
    void HandleLoginServerInternal(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleDbServerInternal(Session* session, const PacketHeader& header, const std::byte* payload,  std::size_t length);


private:
    // 범위 체크 헬퍼
    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

    void FailLoginBySeq(uint32_t seq, ELoginResult result);

    void OnLsDbFindAccountReq(Session* session, const std::byte* payload, std::size_t length);

    void OnDbFindAccountAck(Session* session, const std::byte* payload, std::size_t length);

    void OnLsLoginAck(Session* session, const std::byte* payload, std::size_t length);

private:
    MainServer* owner_{ nullptr };

    std::unique_ptr<MSClientInternalHandler> clientHandler_;
    std::unique_ptr<MSDbServerInternalHandler> dbHandler_;
    std::unique_ptr<MSChatServerInternalHandler> chatHandler_;
};

