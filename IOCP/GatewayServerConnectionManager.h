#pragma once
#include "GatewayServerSession.h"

/// <summary>
/// Login/World/Chat 서버와의 내부 연결을 관리하는 클래스. [백엔드 서버 연결 관리자]
/// 
/// - 각 서버 연결 생성/유지
/// - 서버 타입별 세션 보관
/// - 특정 서버로 패킷 전송
/// - 재연결 처리
/// </summary>

class GatewayServerConnectionManager
{
public:
    void RegisterLoginServer(GatewayServerSession* session);
    void RegisterWorldServer(GatewayServerSession* session);
    void RegisterChatServer(GatewayServerSession* session);

    void UnregisterServer(GatewayServerSession* session);

    GatewayServerSession* GetLoginServer();
    GatewayServerSession* GetWorldServer();
    GatewayServerSession* GetChatServer();

    size_t GetConnectedLoginCount() const;
    size_t GetConnectedWorldCount() const;
    size_t GetConnectedChatCount() const;

private:
    GatewayServerSession* loginServer_{ nullptr };
    GatewayServerSession* worldServer_{ nullptr };
    GatewayServerSession* chatServer_{ nullptr };
    mutable std::mutex lock_;
};

