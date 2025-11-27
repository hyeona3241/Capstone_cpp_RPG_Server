#pragma once

enum class SessionRole : uint8_t
{
    Unknown,
    Client,        // 실제 게임 클라이언트
    LoginServer,   // 내부 로그인 서버와의 연결
    DbServer,      // 내부 DB 서버와의 연결

    // 뒤에 계속해서 추가하기
};

enum class SessionState : uint8_t
{
    Disconnected,
    Handshake,     // 버전체크, 기본 프로토콜
    Auth,          // 로그인/인증 진행 중
    Authed,        // 로그인 완료 (로비/대기)
    InGame,        // 인게임 접속 중

    // 뒤에 계속 추가하기
};

class Session
{
};

