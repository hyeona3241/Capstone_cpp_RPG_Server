#pragma once
#include "GatewayClientSession.h"
#include <unordered_map>

/// <summary>
/// Gateway에 접속한 클라이언트 세션들을 관리하는 클래스. [클라이언트 연결 목록 관리자]
/// 
/// - 세션 등록/삭제
/// - SessionId로 조회
/// - 연결 종료 처리
/// </summary>

class GatewaySessionManager
{
public:
    void AddSession(GatewayClientSession* session);
    void RemoveSession(GatewayClientSession* session);
    GatewayClientSession* FindBySessionId(uint64_t sessionId);
    size_t GetClientCount() const;

private:
    std::unordered_map<uint64_t, GatewayClientSession*> sessions_;
    mutable std::mutex lock_;
};

