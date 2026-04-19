#pragma once
#include "Pch.h"
#include "IocpServerBase.h"
#include "GatewayPacketHandler.h"
#include "GatewaySessionManager.h"
#include "GatewayServerConnectionManager.h"
#include "GatewayTokenManager.h"
#include "GatewayStateMonitor.h"

/// <summary>
/// Gateway 서버 전체를 대표하는 메인 클래스. [Gateway 서버의 최상위 조립자]
/// 
/// - 서버 시작/종료
/// - 클라이언트 연결 수락
/// - 패킷 디스패처 초기화
/// - 세션 매니저 / 클라이언트 매니저 / 서버 연결 매니저 보유
/// - 로그인 / 월드 / 채팅 서버와의 전체 흐름 제어
/// </summary>
/// 
/// Gateway는 최초 접속과 서버 전환만 담당
/// 실제 기능 서버는 클라이언트와 직접 통신
/// Gateway는 상태를 오래 들고 있지 않음
/// 각 서버는 자기 도메인 패킷만 처리
/// 서버 간에는 필요한 최소 정보만 전달


class GatewayServer :  public IocpServerBase<GatewaySession>
{

};

