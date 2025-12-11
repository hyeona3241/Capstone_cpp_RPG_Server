#pragma once

#include <cstddef> 
#include <cstdint>

class MainServer;
class Session;
struct PacketHeader;

class MSPacketHandler
{
public:
    explicit MSPacketHandler(MainServer* owner);

    // 클라이언트 -> 메인 서버로 온 패킷
    void HandleFromClient(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // LoginServer -> 메인 서버로 온 내부 패킷
    void HandleFromLoginServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // DbServer -> 메인 서버로 온 내부 패킷
    void HandleFromDbServer(Session* session, const PacketHeader& header,  const std::byte* payload, std::size_t length);

private:
    // 클라이언트 패킷 범위 라우팅
    void HandleClientHandshake(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleClientLogin(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleClientLobby(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleClientWorld(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 내부 서버에서 올라온 패킷 처리용 (응답/알림 등)
    void HandleLoginServerInternal(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleDbServerInternal(Session* session, const PacketHeader& header, const std::byte* payload,  std::size_t length);

    // 범위 체크 헬퍼
    static bool InRange(std::uint32_t id,  std::uint32_t begin, std::uint32_t endExclusive);

private:
    MainServer* owner_{ nullptr };
};

