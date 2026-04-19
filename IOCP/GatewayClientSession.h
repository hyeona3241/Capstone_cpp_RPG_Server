#pragma once
#include "Session.h"

// 구현하기 전에 Session과 SessionPool 범용성 있게 바꾸기!!

/// <summary>
/// 클라이언트와 실제 네트워크 연결을 담당하는 세션 객체. [클라 1명의 실제 연결 객체]
/// 
/// - 클라이언트 소켓 송수신
/// - 패킷 수신 시 디스패처로 전달
/// - 연결 종료 통지
/// </summary>
/// 
/// 해야 하는 일
/// - 최초 접속 상태 보관
/// - 핸드셰이크 완료 여부 보관
/// - 현재 다음으로 어디로 보내야 하는지 판단에 필요한 상태 보관
/// - 로그인 후 식별 가능한 최소 정보 보관
/// - redirect 발급 여부 보관
/// - 접속 시간 / 마지막 활동 시간 같은 디버깅용 정보 보관
/// 
///하지 말아야 하는 일
/// - 로그인 검증 직접 처리
/// - 월드 상태 직접 보관
/// - 채팅 상태 직접 보관
/// - 게임 로직 처리


/// - 이 값은 월드 내부의 세부 상태(캐릭터 생성/선택/인게임)를 표현하지 않는다.
/// - 그런 세부 단계는 WorldServer가 관리해야 하므로 Gateway에서는 크게만 구분한다.

enum class GatewayClientPhase : uint8_t
{
    WaitingHandshake = 0,   // 최초 접속 직후, 핸드셰이크/버전 확인 대기
    WaitingLogin,           // 로그인 서버로 보내야 하거나 로그인 완료를 기다리는 단계
    WaitingWorld,           // 월드 서버로 보내야 하는 단계
    InWorldFlow,            // 월드 흐름 안에 있는 상태 (로비/캐릭터 선택/인게임 포함)
    Closed                  // 세션 종료 상태
};

/// - 이 값은 "현재 접속 중인 서버"를 의미하지 않는다.
/// - 현재 접속 여부는 connectionFlags_ 로 관리한다.
/// - 주로 디버깅/로그/최근 전환 대상 추적용으로 사용한다.

enum class GatewayRedirectTarget : uint8_t
{
    None = 0,
    Login,
    World,
    Chat
};

/// 클라이언트가 현재 연결 중인 서버들을 비트 플래그로 표현한다.
/// 
/// - WorldServer만 연결: WorldServer
/// - World + Chat 동시 연결: WorldServer | ChatServer

enum class GatewayConnectionFlag : uint8_t
{
    None = 0,
    LoginServer = 1 << 0,
    WorldServer = 1 << 1,
    ChatServer = 1 << 2
};

/// enum class 비트 플래그 연산용 헬퍼 함수
inline GatewayConnectionFlag operator|(GatewayConnectionFlag lhs, GatewayConnectionFlag rhs)
{
    return static_cast<GatewayConnectionFlag>(
        static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
        );
}

inline GatewayConnectionFlag operator&(GatewayConnectionFlag lhs, GatewayConnectionFlag rhs)
{
    return static_cast<GatewayConnectionFlag>(
        static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
        );
}

inline GatewayConnectionFlag operator~(GatewayConnectionFlag value)
{
    return static_cast<GatewayConnectionFlag>(~static_cast<uint8_t>(value));
}



class GatewayClientSession : public Session
{
public:
    GatewayClientSession() = default;
    ~GatewayClientSession() override = default;

public:
    GatewayClientPhase GetPhase() const { return phase_; }
    void SetPhase(GatewayClientPhase phase) { phase_ = phase; }

    // 로비/캐릭터 생성/캐릭터 선택/인게임은 모두 WorldServer 책임이므로 Gateway에서는 InWorldFlow 하나로 크게 관리한다.
    bool IsInWorldFlow() const { return phase_ == GatewayClientPhase::InWorldFlow; }

    // 세션이 종료 상태인지 확인
    bool IsClosed() const { return phase_ == GatewayClientPhase::Closed; }

public:
    uint8_t GetConnectionFlagsRaw() const { return connectionFlags_; }
    // 특정 서버 연결 플래그가 켜져 있는지 확인
    bool HasConnection(GatewayConnectionFlag flag) const
    {
        const uint8_t mask = static_cast<uint8_t>(flag);
        return (connectionFlags_ & mask) != 0;
    }
    // 특정 서버 연결 플래그를 추가
    void AddConnection(GatewayConnectionFlag flag)
    {
        connectionFlags_ |= static_cast<uint8_t>(flag);
    }
    /// 특정 서버 연결 플래그를 제거
    void RemoveConnection(GatewayConnectionFlag flag)
    {
        connectionFlags_ &= ~static_cast<uint8_t>(flag);
    }
    /// 현재 연결 플래그를 전부 제거
    void ClearConnections()
    {
        connectionFlags_ = static_cast<uint8_t>(GatewayConnectionFlag::None);
    }
    /// 현재 월드 서버에 연결 중인지 확인
    bool IsConnectedToWorld() const
    {
        return HasConnection(GatewayConnectionFlag::WorldServer);
    }
    /// 현재 채팅 서버에 연결 중인지 확인
    bool IsConnectedToChat() const
    {
        return HasConnection(GatewayConnectionFlag::ChatServer);
    }
    /// 현재 로그인 서버에 연결 중인지 확인
    bool IsConnectedToLogin() const
    {
        return HasConnection(GatewayConnectionFlag::LoginServer);
    }

public:
    // Redirect 관련
    GatewayRedirectTarget GetLastRedirectTarget() const { return lastRedirectTarget_; }
    void SetLastRedirectTarget(GatewayRedirectTarget target) { lastRedirectTarget_ = target; }
    const std::string& GetLastRedirectToken() const { return lastRedirectToken_; }
    void SetLastRedirectToken(const std::string& token) { lastRedirectToken_ = token; }
    void ClearRedirectInfo()
    {
        lastRedirectTarget_ = GatewayRedirectTarget::None;
        lastRedirectToken_.clear();
    }

public:
    // 프로토콜 / 계정 정보 관련
    uint32_t GetProtocolVersion() const { return protocolVersion_; }
    void SetProtocolVersion(uint32_t version) { protocolVersion_ = version; }

    int64_t GetAccountId() const { return accountId_; }
    void SetAccountId(int64_t accountId) { accountId_ = accountId; }
    bool HasAccountId() const { return accountId_ > 0; }
    void ClearAccountInfo() { accountId_ = 0; }


public:
    // 시간 관련
    std::chrono::steady_clock::time_point GetConnectedAt() const { return connectedAt_; }
    std::chrono::steady_clock::time_point GetLastActivityAt() const { return lastActivityAt_; }
    void Touch() { lastActivityAt_ = std::chrono::steady_clock::now(); }

    // 연결 후 경과 시간(ms)을 계산
    uint64_t GetConnectedDurationMs() const
    {
        if (connectedAt_ == std::chrono::steady_clock::time_point{})
            return 0;

        const auto now = std::chrono::steady_clock::now();
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - connectedAt_).count()
            );
    }

    /// 마지막 활동 이후 경과 시간(ms)을 계산
    uint64_t GetIdleDurationMs() const
    {
        if (lastActivityAt_ == std::chrono::steady_clock::time_point{})
            return 0;

        const auto now = std::chrono::steady_clock::now();
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastActivityAt_).count()
            );
    }

protected:
    // 세션이 새로 할당될 때 호출되는 초기화
    void OnInit() override
    {
        phase_ = GatewayClientPhase::WaitingHandshake;

        connectionFlags_ = static_cast<uint8_t>(GatewayConnectionFlag::None);

        lastRedirectTarget_ = GatewayRedirectTarget::None;
        lastRedirectToken_.clear();

        protocolVersion_ = 0;
        accountId_ = 0;

        connectedAt_ = std::chrono::steady_clock::now();
        lastActivityAt_ = connectedAt_;
    }

    // 세션이 새로 할당될 때 호출되는 초기화
    void OnReset() override
    {
        phase_ = GatewayClientPhase::Closed;

        connectionFlags_ = static_cast<uint8_t>(GatewayConnectionFlag::None);

        lastRedirectTarget_ = GatewayRedirectTarget::None;
        lastRedirectToken_.clear();

        protocolVersion_ = 0;
        accountId_ = 0;

        connectedAt_ = std::chrono::steady_clock::time_point{};
        lastActivityAt_ = std::chrono::steady_clock::time_point{};
    }

private:
    // Gateway 기준 현재 진행 단계
    GatewayClientPhase phase_{ GatewayClientPhase::WaitingHandshake };

    // 현재 어떤 서버들과 연결 중인지 나타내는 비트 플래그.
    uint8_t connectionFlags_{ static_cast<uint8_t>(GatewayConnectionFlag::None) };

    // 최근 Redirect 대상 서버.
    GatewayRedirectTarget lastRedirectTarget_{ GatewayRedirectTarget::None };

    // 최근 발급한 Redirect 토큰 문자열
    std::string lastRedirectToken_;

    // 클라이언트가 핸드셰이크에서 보낸 프로토콜 버전
    uint32_t protocolVersion_{ 0 };

    // 현재 세션과 연관된 계정 ID
    int64_t accountId_{ 0 };

    // 세션 최초 연결 시각
    std::chrono::steady_clock::time_point connectedAt_{};

    // 마지막 패킷 수신/활동 시각
    std::chrono::steady_clock::time_point lastActivityAt_{};
};

