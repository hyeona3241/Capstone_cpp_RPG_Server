#pragma once
#include "IDomainHandler.h"

/// <summary>
/// 공통 시스템 패킷 처리. [공통/예외 패킷 담당] 
/// 
/// - 에러 응답
/// - 핑 / 퐁
/// - 예외 처리
/// - 연결 종료 관련 패킷
/// </summary>

class GatewaySystemHandler :  public IDomainHandler
{
};

