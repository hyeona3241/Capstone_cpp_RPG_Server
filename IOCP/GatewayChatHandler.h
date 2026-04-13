#pragma once
#include "IDomainHandler.h"

/// <summary>
/// 채팅 서버 연결 관련 패킷 처리. [채팅 서버 연결 흐름 담당]
/// 
/// - 채팅 연결 요청
/// - ChatServer 응답 처리
/// - 채팅 가능 상태 반영
/// </summary>

class GatewayChatHandler : public IDomainHandler
{
};

