#pragma once
#include "IocpServerBase.h"

/// <summary>
/// Gateway 서버 전체를 대표하는 메인 클래스. [Gateway 서버의 최상위 조립자]
/// 
/// - 서버 시작/종료
/// - 클라이언트 연결 수락
/// - 패킷 디스패처 초기화
/// - 세션 매니저 / 클라이언트 매니저 / 서버 연결 매니저 보유
/// - 로그인 / 월드 / 채팅 서버와의 전체 흐름 제어
/// </summary>

class GatewayServer :  public IocpServerBase
{
};

