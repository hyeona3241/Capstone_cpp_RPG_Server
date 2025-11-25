#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#include <iostream>
#include "Core/IocpServer.h"
#include "Session/SessionManager.h"
#include "App/DefaultNetListener.h"

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    // 나중에 상수 정의로 빼기 (NetConfig에 정의 되어있음)
    constexpr size_t kMaxSessions = 1024; 

    Util::BufferPool bufPool{ 8 * 1024, 256 };
    SessionManager sessionMgr{ kMaxSessions, bufPool};

    DefaultNetListener listener;
    IocpServer server;

    const char* ip = "0.0.0.0";
    const uint16_t port = 7777;
    const int workerCount = 4;

    LOG_INFO("server start");


    if (!server.Start(ip, port, workerCount, &sessionMgr, &listener))
    {
        std::cerr << "Server start failed\n";
        return 1;
    }

    std::cout << "[ServerTest] Running on " << ip << ":" << port << std::endl;
    std::cout << "Press ENTER to stop..." << std::endl;
    std::cin.get();

    server.Stop();
    WSACleanup();
    return 0;
}