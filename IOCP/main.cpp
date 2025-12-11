#include "Pch.h"
#include "MainServer.h"
#include "IocpServerBase.h"

#include <iostream>
#include <csignal>

static bool g_running = true;

void SignalHandler(int sig)
{
    std::printf("\n[INFO] Signal received (%d). Shutting down server...\n", sig);
    g_running = false;
}

int main()
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::printf("[ERROR] WSAStartup failed.\n");
        return 1;
    }

    // Ctrl + C 로 종료할 수 있게 설정
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // IOCP 설정
    IocpConfig cfg;
    cfg.maxSessions = 2000;   // 원하는 세션 풀 크기
    cfg.bufferCount = 4000;   // 사용할 버퍼 풀 개수
    cfg.bufferSize = 4096;   // 각 버퍼 하나의 크기
    cfg.recvRingCapacity = 64 * 1024; // 세션당 리시브 링 버퍼(64KB 예시)

    // 메인 서버 생성
    MainServer server(cfg);

    const uint16_t port = 3500;
    const int workerThreads = 4;

    if (!server.Start(port, workerThreads))
    {
        std::printf("[ERROR] Failed to start MainServer.\n");
        return 1;
    }

    std::printf("[INFO] MainServer started. Listening on port %u.\n", port);
    std::printf("[INFO] Press Ctrl+C to stop.\n");

    // 서버가 꺼질 때까지 대기
    while (g_running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 종료
    server.Stop();

    std::printf("[INFO] Server stopped cleanly.\n");
    return 0;
}
