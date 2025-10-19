#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

// ���� �ʱ�ȭ/�ɼ�/���ε�/���� ���

namespace SocketUtils {

	// ���� �ʱ�ȭ
	bool InitWinsock();
	// TCP ���� ����
	SOCKET CreateTcp();
	// ����ŷ ���� ��ȯ
	bool SetNonBlocking(SOCKET s);
	// Nagle ��Ȱ��ȭ
	bool SetNoDelay(SOCKET s);
	// �ּ� ����
	bool SetReuseAddr(SOCKET s);
	// LINGER ����
	bool SetLinger(SOCKET s, bool on, uint16_t seconds);
	// IPv4 ���ε�/ ����
	bool BindAndListen(SOCKET s, const char* ip, uint16_t port, int backlog);
	// ���� ����
	void CloseSocket(SOCKET& s);
}