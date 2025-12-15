#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#include <cstdio>
#include "ClientApp.h"

int main()
{
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::printf("[CLIENT][ERROR] WSAStartup failed\n");
        return 1;
    }

    ClientApp app;
    if (!app.Start("127.0.0.1", 3500))
    {
        std::printf("[CLIENT][ERROR] Start failed\n");
        WSACleanup();
        return 1;
    }

    std::printf("[CLIENT] Connected. Waiting...\n");
    app.RunUntilDone();

    WSACleanup();
    std::printf("[CLIENT] Exit\n");
    return 0;
}
