#pragma once
#include "IDomainHandler.h"

/// <summary>
/// 월드 진입 관련 패킷 처리. [월드 서버로 넘어가는 흐름 담당]
/// 
/// - 월드 입장 요청
/// - WorldServer 연결 허용 응답
/// - 월드 입장 상태 반영
/// </summary>

class GatewayWorldHandler :  public IDomainHandler
{
};

