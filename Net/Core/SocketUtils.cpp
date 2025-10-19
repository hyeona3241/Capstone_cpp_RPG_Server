#include "SocketUtils.h"
#include <iostream>

namespace SocketUtils {

    bool InitWinsock() {
        WSADATA wsa{};
        const int ret = WSAStartup(MAKEWORD(2, 2), &wsa);

        if (ret != 0) {
            // �ʱ�ȭ ����
            std::cerr << "[SocketUtils] WSAStartup failed: " << ret << std::endl;
            return false;
        }

        return true;
    }

    SOCKET CreateTcp() {
        SOCKET s = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

        if (s == INVALID_SOCKET) {
            // ���� ���� ����
            std::cerr << "[SocketUtils] WSASocket failed: " << WSAGetLastError() << std::endl;
        }
        return s;
    }

    bool SetNonBlocking(SOCKET s) {
        u_long on = 1;
        const int rc = ioctlsocket(s, FIONBIO, &on);

        if (rc != 0) {
            // ����ŷ ��� ���� ����
            std::cerr << "[SocketUtils] ioctlsocket(FIONBIO) failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    bool SetNoDelay(SOCKET s) {
        BOOL yes = TRUE;
        const int rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&yes), sizeof(yes));

        if (rc != 0) {
            // Nagle �˰��� ���� ����
            std::cerr << "[SocketUtils] setsockopt(TCP_NODELAY) failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    bool SetReuseAddr(SOCKET s) {
        BOOL yes = TRUE;
        const int rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

        if (rc != 0) {
            // �ּ� ���� �ɼ� ���� ����
            std::cerr << "[SocketUtils] setsockopt(SO_REUSEADDR) failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    bool SetLinger(SOCKET s, bool on, uint16_t seconds) {
        linger lg{};
        lg.l_onoff = on ? 1 : 0;
        lg.l_linger = static_cast<u_short>(seconds);

        const int rc = setsockopt(s, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char*>(&lg), sizeof(lg));

        if (rc != 0) {
            // ���� ���� ��� ���� ����
            std::cerr << "[SocketUtils] setsockopt(SO_LINGER) failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    bool BindAndListen(SOCKET s, const char* ip, uint16_t port, int backlog) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
            // IP ��ȯ ����
            std::cerr << "[SocketUtils] inet_pton failed for ip: " << (ip ? ip : "(null)") << std::endl;
            return false;
        }

        if (bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            // ���ε� ����
            std::cerr << "[SocketUtils] bind failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        if (listen(s, backlog) != 0) {
            // ���� ����
            std::cerr << "[SocketUtils] listen failed: " << WSAGetLastError() << std::endl;
            return false;
        }

        return true;
    }

    void CloseSocket(SOCKET& s) {
        if (s != INVALID_SOCKET) {
            ::shutdown(s, SD_BOTH);  // �������� ���� ����
            ::closesocket(s);
            s = INVALID_SOCKET;
        }
    }

} 
