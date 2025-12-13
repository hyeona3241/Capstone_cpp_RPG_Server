#include "Pch.h"
#include "MainServer.h"

#include <csignal>
#include <thread>
#include <chrono>

static bool g_running = true;

static void SignalHandler(int sig)
{
    std::printf("\n[MAIN] Signal received (%d). Shutting down...\n", sig);
    g_running = false;
}

int main()
{
    // WinSock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::printf("[MAIN][ERROR] WSAStartup failed.\n");
        return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // IOCP 공통 설정(기존 main.cpp에서 그대로) :contentReference[oaicite:5]{index=5}
    IocpConfig iocpCfg;
    iocpCfg.maxSessions = 2000;
    iocpCfg.bufferCount = 4000;
    iocpCfg.bufferSize = 4096;
    iocpCfg.recvRingCapacity = 64 * 1024;

    const uint16_t mainPort = 3500;
    MainServer mainServer(iocpCfg);

    if (!mainServer.Start(mainPort, 4))
    {
        std::printf("[MAIN][ERROR] Failed to start MainServer.\n");
        WSACleanup();
        return 1;
    }

    std::printf("[MAIN][INFO] MainServer started. Listening on port %u.\n", mainPort);

    // Main이 모든 연결 담당(기존 main.cpp 로직을 여기로) :contentReference[oaicite:6]{index=6}
    const uint16_t dbPort = 3600;
    const uint16_t loginPort = 3700;

    // DB 연결
    if (!mainServer.ConnectToDbServer("127.0.0.1", dbPort))
    {
        std::printf("[MAIN][ERROR] Failed to connect to DBServer (%u)\n", dbPort);
        mainServer.Stop();
        WSACleanup();
        return 1;
    }
    std::printf("[MAIN][INFO] Connected to DBServer.\n");

    // Login 연결
    if (!mainServer.ConnectToLoginServer("127.0.0.1", loginPort))
    {
        std::printf("[MAIN][ERROR] Failed to connect to LoginServer (%u)\n", loginPort);
        mainServer.Stop();
        WSACleanup();
        return 1;
    }
    std::printf("[MAIN][INFO] Connected to LoginServer.\n");

    std::printf("[MAIN][INFO] Press Ctrl+C to stop.\n");

    while (g_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::printf("[MAIN][INFO] Stopping...\n");
    mainServer.Stop();

    WSACleanup();
    std::printf("[MAIN][INFO] Stopped cleanly.\n");
    return 0;
}
