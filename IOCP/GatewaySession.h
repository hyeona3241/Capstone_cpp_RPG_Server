#pragma once

#include "Session.h"
#include <chrono>
#include <cstdint>

/// <summary>
/// Gateway에 연결된 세션의 종류
/// 
/// - Client        : 외부 클라이언트가 Gateway에 접속한 세션
/// - BackendServer : Login/World/Chat 같은 내부 서버와의 연결 세션
/// </summary>
enum class GatewaySessionKind : uint8_t
{
    Client = 0,
    BackendServer
};

/// <summary>
/// Gateway에 붙는 모든 세션의 공통 부모 클래스.
///
/// 역할:
/// - 세션이 클라이언트용인지 / 내부 서버용인지 구분
/// - 접속 시각, 마지막 활동 시각 같은 공통 정보 보관
/// - GatewayServer에서 세션 반납 시 어떤 풀로 돌려보낼지 판단 기준 제공
///
/// 주의:
/// - accountId, phase 같은 클라이언트 전용 정보는 넣지 않는다.
/// - backendType, heartbeat 같은 서버 전용 정보도 넣지 않는다.
/// - 그런 정보는 각각 GatewayClientSession / GatewayServerSession이 가진다.
/// </summary>
/// 
class GatewaySession : public Session
{
public:
    GatewaySession() = default;
    ~GatewaySession() override = default;

public:
    // 세션 종류 관련

    GatewaySessionKind GetGatewaySessionKind() const { return kind_; }
    void SetGatewaySessionKind(GatewaySessionKind kind) { kind_ = kind; }
    bool IsClientSession() const { return kind_ == GatewaySessionKind::Client; }
    bool IsBackendServerSession() const { return kind_ == GatewaySessionKind::BackendServer; }

public:
    // 시간 관련
    
    // 최초 연결 시각을 반환
    std::chrono::steady_clock::time_point GetConnectedAt() const { return connectedAt_; }
    // 마지막 활동 시각을 반환
    std::chrono::steady_clock::time_point GetLastActivityAt() const { return lastActivityAt_; }
    // 마지막 활동 시각을 현재 시각으로 갱신
    void Touch() { lastActivityAt_ = std::chrono::steady_clock::now(); }
    // 연결 후 경과 시간(ms)을 반환
    uint64_t GetConnectedDurationMs() const
    {
        if (connectedAt_ == std::chrono::steady_clock::time_point{})
            return 0;

        const auto now = std::chrono::steady_clock::now();
        return static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(now - connectedAt_).count()
            );
    }

    // 마지막 활동 이후 유휴 시간(ms)을 반환
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
    // 세션이 새로 초기화될 때 공통 Gateway 상태를 세팅
    void OnInit() override
    {
        kind_ = GatewaySessionKind::Client;

        connectedAt_ = std::chrono::steady_clock::now();
        lastActivityAt_ = connectedAt_;
    }

    // 세션이 풀로 반환될 때 공통 Gateway 상태를 초기화
    void OnReset() override
    {
        kind_ = GatewaySessionKind::Client;

        connectedAt_ = std::chrono::steady_clock::time_point{};
        lastActivityAt_ = std::chrono::steady_clock::time_point{};
    }

private:
    // 이 세션이 클라이언트용인지, 내부 서버용인지
    GatewaySessionKind kind_{ GatewaySessionKind::Client };

    // 최초 연결 시각
    std::chrono::steady_clock::time_point connectedAt_{};

    // 마지막 활동 시각
    std::chrono::steady_clock::time_point lastActivityAt_{};
};