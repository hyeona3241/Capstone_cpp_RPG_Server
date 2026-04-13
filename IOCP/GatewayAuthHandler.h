#pragma once
#include "IDomainHandler.h"

/// <summary>
/// 인증 관련 패킷 처리. [로그인 흐름 담당]
/// 
/// - 클라이언트 로그인 요청
/// - LoginServer 로그인 응답
/// - 인증 성공 / 실패 반영
/// </summary>

class GatewayAuthHandler : public IDomainHandler
{
};

