#pragma once

#include "GatewaySession.h"
#include <string>
#include <cstdint>

/// <summary>
/// Gateway 기준 클라이언트의 큰 진행 단계를 나타낸다.
/// 
/// 주의:
/// - 로비, 캐릭터 생성, 캐릭터 선택, 인게임 세부 상태는 WorldServer가 관리한다.
/// - Gateway는 로그인 전/월드 진입 전/월드 흐름 안 정도만 크게 구분한다.
/// </summary>
enum class GatewayClientPhase : uint8_t
{
    WaitingHandshake = 0,   // 최초 접속 후 핸드셰이크/버전 확인 대기
    WaitingLogin,           // 로그인 서버로 보내야 하거나 로그인 완료를 기다리는 단계
    WaitingWorld,           // 월드 서버로 보내야 하는 단계
    InWorldFlow,            // 월드 서버 흐름 안에 있는 상태
    DebugTest,              // 패킷 송수신/로그/세션 테스트용 임시 상태
    Closed                  // 세션 종료 상태
};

/// <summary>
/// 최근 또는 현재 발급한 Redirect 대상 서버.
/// 현재 연결 상태가 아니라, 최근 어디로 보냈는지 추적하기 위한 값이다.
/// </summary>
enum class GatewayRedirectTarget : uint8_t
{
    None = 0,
    Login,
    World,
    Chat,
    Test
};

/// <summary>
/// 클라이언트가 현재 연결 중인 서버들을 비트 플래그로 표현한다.
/// WorldServer와 ChatServer처럼 동시에 연결될 수 있는 상태를 표현하기 위해 사용한다.
/// </summary>
enum class GatewayConnectionFlag : uint8_t
{
    None        = 0,
    LoginServer = 1 << 0,
    WorldServer = 1 << 1,
    ChatServer  = 1 << 2,
    TestServer  = 1 << 3
};

class GatewayClientSession : public GatewaySession
{
public:
    GatewayClientSession() = default;
    ~GatewayClientSession() override = default;

public:
    // -------------------------------------------------------------------------
    // Phase
    // -------------------------------------------------------------------------

    GatewayClientPhase GetPhase() const
    {
        return phase_;
    }

    void SetPhase(GatewayClientPhase phase)
    {
        phase_ = phase;
    }

    bool IsInWorldFlow() const
    {
        return phase_ == GatewayClientPhase::InWorldFlow;
    }

    bool IsDebugTest() const
    {
        return phase_ == GatewayClientPhase::DebugTest;
    }

    bool IsClosed() const
    {
        return phase_ == GatewayClientPhase::Closed;
    }

    // -------------------------------------------------------------------------
    // Connection Flags
    // -------------------------------------------------------------------------

    uint8_t GetConnectionFlagsRaw() const
    {
        return connectionFlags_;
    }

    bool HasConnection(GatewayConnectionFlag flag) const
    {
        const uint8_t mask = static_cast<uint8_t>(flag);
        return (connectionFlags_ & mask) != 0;
    }

    void AddConnection(GatewayConnectionFlag flag)
    {
        connectionFlags_ |= static_cast<uint8_t>(flag);
    }

    void RemoveConnection(GatewayConnectionFlag flag)
    {
        connectionFlags_ &= ~static_cast<uint8_t>(flag);
    }

    void ClearConnections()
    {
        connectionFlags_ = static_cast<uint8_t>(GatewayConnectionFlag::None);
    }

    bool IsConnectedToLogin() const
    {
        return HasConnection(GatewayConnectionFlag::LoginServer);
    }

    bool IsConnectedToWorld() const
    {
        return HasConnection(GatewayConnectionFlag::WorldServer);
    }

    bool IsConnectedToChat() const
    {
        return HasConnection(GatewayConnectionFlag::ChatServer);
    }

    bool IsConnectedToTestServer() const
    {
        return HasConnection(GatewayConnectionFlag::TestServer);
    }

    // -------------------------------------------------------------------------
    // Redirect
    // -------------------------------------------------------------------------

    GatewayRedirectTarget GetLastRedirectTarget() const
    {
        return lastRedirectTarget_;
    }

    void SetLastRedirectTarget(GatewayRedirectTarget target)
    {
        lastRedirectTarget_ = target;
    }

    const std::string& GetLastRedirectToken() const
    {
        return lastRedirectToken_;
    }

    void SetLastRedirectToken(const std::string& token)
    {
        lastRedirectToken_ = token;
    }

    void ClearRedirectInfo()
    {
        lastRedirectTarget_ = GatewayRedirectTarget::None;
        lastRedirectToken_.clear();
    }

    // -------------------------------------------------------------------------
    // Protocol / Account
    // -------------------------------------------------------------------------

    uint32_t GetProtocolVersion() const
    {
        return protocolVersion_;
    }

    void SetProtocolVersion(uint32_t version)
    {
        protocolVersion_ = version;
    }

    int64_t GetAccountId() const
    {
        return accountId_;
    }

    void SetAccountId(int64_t accountId)
    {
        accountId_ = accountId;
    }

    bool HasAccountId() const
    {
        return accountId_ > 0;
    }

    void ClearAccountInfo()
    {
        accountId_ = 0;
    }

protected:
    /// <summary>
    /// 세션이 새로 할당될 때 GatewayClientSession 전용 상태를 초기화한다.
    /// </summary>
    void OnInit() override
    {
        GatewaySession::OnInit();
        SetGatewaySessionKind(GatewaySessionKind::Client);

        phase_ = GatewayClientPhase::WaitingHandshake;

        connectionFlags_ = static_cast<uint8_t>(GatewayConnectionFlag::None);

        lastRedirectTarget_ = GatewayRedirectTarget::None;
        lastRedirectToken_.clear();

        protocolVersion_ = 0;
        accountId_ = 0;
    }

    /// <summary>
    /// 세션이 풀로 반환될 때 이전 클라이언트 상태가 남지 않도록 정리한다.
    /// </summary>
    void OnReset() override
    {
        GatewaySession::OnReset();

        phase_ = GatewayClientPhase::Closed;

        connectionFlags_ = static_cast<uint8_t>(GatewayConnectionFlag::None);

        lastRedirectTarget_ = GatewayRedirectTarget::None;
        lastRedirectToken_.clear();

        protocolVersion_ = 0;
        accountId_ = 0;
    }

private:
    /// <summary>
    /// Gateway 기준 클라이언트 진행 단계.
    /// </summary>
    GatewayClientPhase phase_{ GatewayClientPhase::WaitingHandshake };

    /// <summary>
    /// 현재 클라이언트가 연결 중인 서버들을 나타내는 비트 플래그.
    /// </summary>
    uint8_t connectionFlags_{ static_cast<uint8_t>(GatewayConnectionFlag::None) };

    /// <summary>
    /// 최근 Redirect 대상 서버.
    /// </summary>
    GatewayRedirectTarget lastRedirectTarget_{ GatewayRedirectTarget::None };

    /// <summary>
    /// 최근 발급한 Redirect 토큰.
    /// </summary>
    std::string lastRedirectToken_;

    /// <summary>
    /// 클라이언트 프로토콜 버전.
    /// </summary>
    uint32_t protocolVersion_{ 0 };

    /// <summary>
    /// 로그인 이후 식별 가능한 계정 ID.
    /// 로그인 전에는 0.
    /// </summary>
    int64_t accountId_{ 0 };
};