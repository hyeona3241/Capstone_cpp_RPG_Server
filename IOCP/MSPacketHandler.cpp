#include "Pch.h"
#include "MSPacketHandler.h"
#include "MainServer.h"
#include "Session.h"
#include "Common/Protocol/PacketDef.h"

MSPacketHandler::MSPacketHandler(MainServer* owner)
    : owner_(owner)
{
}

// 정적 범위 체크
bool MSPacketHandler::InRange(std::uint32_t id, std::uint32_t begin, std::uint32_t endExclusive)
{
    return (id >= begin) && (id < endExclusive);
}

//  클라이언트 -> 메인 서버 패킷
void MSPacketHandler::HandleFromClient(Session* session,
    const PacketHeader& header,
    const std::byte* payload,
    std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 1000 ~ 1099 : 핸드셰이크 / 버전체크 / 기본 설정
    // 1100 ~ 1199 : 로그인 / 계정 관련
    // 1200 ~ 1299 : 로비 / 캐릭터 선택
    // 1300 ~ 1399 : 월드 / 인게임 관련
    //
    if (InRange(id, 1000, 1100))
    {
        HandleClientHandshake(session, header, payload, length);
    }
    else if (InRange(id, 1100, 1200))
    {
        HandleClientLogin(session, header, payload, length);
    }
    else if (InRange(id, 1200, 1300))
    {
        HandleClientLobby(session, header, payload, length);
    }
    else if (InRange(id, 1300, 1400))
    {
        HandleClientWorld(session, header, payload, length);
    }
    else
    {
        std::printf("[WARN][MSPacketHandler] Unknown client packet id=%u (sessionId=%llu)\n", id, static_cast<unsigned long long>(session->GetId()));
        // 필요하면 여기서 프로토콜 에러로 세션을 끊을 수도 있음
        // 나중에 공격인지 아닌지 판단하고 로직 정하면 될듯
        // session->Disconnect();
    }
}

//  LoginServer -> 메인 서버 내부 패킷
void MSPacketHandler::HandleFromLoginServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 예시: 2000 ~ 2099 범위를 LoginServer <-> MainServer 용도로 사용
    if (InRange(id, 2000, 2100))
    {
        HandleLoginServerInternal(session, header, payload, length);
    }
    else
    {
        std::printf("[WARN][MSPacketHandler] Unknown login-server packet id=%u\n", id);
    }
}


//  DbServer -> 메인 서버 내부 패킷
void MSPacketHandler::HandleFromDbServer(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (!session)
        return;

    const auto id = static_cast<std::uint32_t>(header.id);

    // 예시: 3000 ~ 3099 범위를 DbServer <-> MainServer 용도로 사용
    if (InRange(id, 3000, 3100))
    {
        HandleDbServerInternal(session, header, payload, length);
    }
    else
    {
        std::printf("[WARN][MSPacketHandler] Unknown db-server packet id=%u\n", id);
    }
}


//  클라이언트 핸들러들 (세부 로직은 나중에 구현)
void MSPacketHandler::HandleClientHandshake(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    // 상태 검사 예시 (SessionState enum은 네 프로젝트 정의에 맞게 수정)
    if (session->GetState() != SessionState::Handshake)
    {
        std::printf("[WARN][Handshake] Invalid state for handshake packet. "
            "sessionId=%llu, state=%d, pktId=%u\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(session->GetState()),
            static_cast<unsigned>(header.id));
        return;
    }

    // TODO: 여기서 header.id에 따라 버전체크 / 기본설정 패킷 처리
    // switch (header.id) { case PKT_C2S_VERSION_CHECK: ... }

    std::printf("[DEBUG][Handshake] HandleClientHandshake: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
}

void MSPacketHandler::HandleClientLogin(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    // 예: 로그인 패킷은 보통 Handshake 이후 상태에서만 허용
    if (session->GetState() != SessionState::Login && session->GetState() != SessionState::Handshake)
    {
        std::printf("[WARN][Login] Invalid state for login packet. "
            "sessionId=%llu, state=%d, pktId=%u\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(session->GetState()),
            static_cast<unsigned>(header.id));
        return;
    }

    // TODO: header.id에 따라 로그인 요청, 회원가입, 토큰 로그인 등 분기
    std::printf("[DEBUG][Login] HandleClientLogin: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
}

void MSPacketHandler::HandleClientLobby(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (session->GetState() != SessionState::Lobby)
    {
        std::printf("[WARN][Lobby] Packet in non-lobby state. "
            "sessionId=%llu, state=%d, pktId=%u\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(session->GetState()),
            static_cast<unsigned>(header.id));
        return;
    }

    // TODO: 로비 메뉴(캐릭터 목록, 생성/삭제, 매칭 준비 등) 처리
    std::printf("[DEBUG][Lobby] HandleClientLobby: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
}

void MSPacketHandler::HandleClientWorld(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    if (session->GetState() != SessionState::World)
    {
        std::printf("[WARN][World] Packet in non-world state. "
            "sessionId=%llu, state=%d, pktId=%u\n",
            static_cast<unsigned long long>(session->GetId()),
            static_cast<int>(session->GetState()),
            static_cast<unsigned>(header.id));
        return;
    }

    // TODO: 인게임 이동/공격/스킬/채팅 등 처리 or WorldServer로 포워딩
    std::printf("[DEBUG][World] HandleClientWorld: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
}



void MSPacketHandler::HandleLoginServerInternal(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    // LoginServer -> MainServer 로그인 결과 / 토큰 검증 결과 등 처리
    std::printf("[DEBUG][LoginSrv] HandleLoginServerInternal: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);

    // 예: 로그인 성공 -> 해당 클라 세션 상태를 Lobby로 변경, ACK 전송 등
}

void MSPacketHandler::HandleDbServerInternal(Session* session, const PacketHeader& header, const std::byte* payload, std::size_t length)
{
    // TODO: DbServer -> MainServer 쿼리 결과 처리 (캐릭터 리스트, 인벤토리 등)
    std::printf("[DEBUG][DbSrv] HandleDbServerInternal: pktId=%u, len=%zu\n", static_cast<unsigned>(header.id), length);
}