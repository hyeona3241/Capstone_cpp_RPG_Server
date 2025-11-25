#pragma once
#include <string>

#include "Pch.h"

// 소켓 초기화/옵션/바인드/리슨 기능

namespace SocketUtils {

	// 소켓 초기화
	bool InitWinsock();
	// TCP 소켓 생성
	SOCKET CreateTcp();
	// 논블로킹 모드로 전환
	bool SetNonBlocking(SOCKET s);
	// Nagle 비활성화
	bool SetNoDelay(SOCKET s);
	// 주소 재사용
	bool SetReuseAddr(SOCKET s);
	// LINGER 설정
	bool SetLinger(SOCKET s, bool on, uint16_t seconds);
	// IPv4 바인드/ 리슨
	bool BindAndListen(SOCKET s, const char* ip, uint16_t port, int backlog);
	// 소켓 종료
	void CloseSocket(SOCKET& s);
}