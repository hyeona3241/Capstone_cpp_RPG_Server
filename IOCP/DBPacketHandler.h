#pragma once

#include <cstddef>
#include <cstdint>

class DBServer;
class Session;
struct PacketHeader;

class DBPacketHandler
{
public:
    explicit DBPacketHandler(DBServer* owner);

    // MainServer -> DBServer 로 온 내부 패킷
    void HandleFromMainServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

private:
    // 3000 ~ 3099 : 시스템/헬스체크
    void HandleSystem(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 3100 ~ 3199 : Auth(회원가입/로그인) 요청
    void HandleAuth(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    // 실제 핸들러(스텁)
    void HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleRegisterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

private:
    DBServer* owner_{ nullptr };

};

