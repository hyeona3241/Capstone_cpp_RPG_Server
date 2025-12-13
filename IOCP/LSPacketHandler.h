#pragma once

#include <cstddef>
#include <cstdint>

#include "PacketIds.h"
#include "LSPingPackets.h"
#include <chrono>

class LoginServer;
class Session;
struct PacketHeader;

class LSPacketHandler
{
public:
    explicit LSPacketHandler(LoginServer* owner);

    // MainServer -> LoginServer
    void HandleFromMainServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

private:
    // 2000 ~ 2099 : 시스템
    void HandleSystem(Session* session,
        const PacketHeader& header,
        const std::byte* payload,
        std::size_t length);

    // 2100 ~ 2199 : Auth 요청 (Login / Register)
    void HandleAuth(Session* session,
        const PacketHeader& header,
        const std::byte* payload,
        std::size_t length);

    // 2200 ~ 2299 : DB 결과(Main을 통해 전달됨)
    void HandleDbResult(Session* session,
        const PacketHeader& header,
        const std::byte* payload,
        std::size_t length);

    void HandlePingReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleLoginReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleRegisterReq(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    void HandleDbLoginResult(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length);

    static bool InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive);

private:
    LoginServer* owner_{ nullptr };
};

