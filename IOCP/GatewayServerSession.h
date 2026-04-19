#pragma once
#include "Session.h"

/// <summary>
/// Gateway와 Login/World/Chat 서버 사이의 내부 서버 연결 세션. [다른 서버랑 붙는 내부 연결 객체]
/// 
/// - 백엔드 서버와 송수신
/// - 서버 간 요청 / 응답 전달
/// - 서버 타입 보유(Login / World / Chat)
/// </summary>
/// 
/// 서버 연결이 중간에 끊기면 어떻게 처리할지 예외 처리 중요


enum class GatewayBackendType : uint8_t
{
    Unknown,
    Login,
    World,
    Chat
};

enum class GatewayBackendState : uint8_t
{
    Connecting,
    Connected,
    Alive,
    Dead
};


class GatewayServerSession : public Session
{
public:
    GatewayBackendType GetBackendType() const { return backendType_; }
    void SetBackendType(GatewayBackendType type) { backendType_ = type; }

    GatewayBackendState GetBackendState() const { return backendState_; }
    void SetBackendState(GatewayBackendState state) { backendState_ = state; }

    uint32_t GetServerId() const { return serverId_; }
    void SetServerId(uint32_t id) { serverId_ = id; }

protected:
    void OnInit() override
    {
        backendType_ = GatewayBackendType::Unknown;
        backendState_ = GatewayBackendState::Connecting;
        serverId_ = 0;
    }

    void OnReset() override
    {
        backendType_ = GatewayBackendType::Unknown;
        backendState_ = GatewayBackendState::Dead;
        serverId_ = 0;
    }

private:
    GatewayBackendType backendType_{ GatewayBackendType::Unknown };
    GatewayBackendState backendState_{ GatewayBackendState::Connecting };
    uint32_t serverId_{ 0 };
};

