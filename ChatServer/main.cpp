// ChatMain.cpp
#include "Pch.h"
#include "ChatServer.h"

#include <csignal>
#include <thread>
#include <chrono>
#include <cstdio>
#include <Logger.h>

static bool g_running = true;

static void SignalHandler(int sig)
{
    std::printf("\n[CHAT] Signal received (%d). Shutting down...\n", sig);
    LOG_INFO(std::string("[CHAT] Signal received (") + std::to_string(sig) + "). Shutting down...");
    g_running = false;
}

int main()
{
    // Logger
    Logger::Config logCfg;
    logCfg.app_name = "ChatServer";
    logCfg.dir = "logs/chat";
    logCfg.rotate_bytes = 10 * 1024 * 1024;
    logCfg.keep_files = 5;
    Logger::Init(logCfg);

    LOG_INFO("[System] ChatServer process started");

    // WinSock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::printf("[CHAT][ERROR] WSAStartup failed.\n");
        LOG_ERROR("[CHAT] WSAStartup failed.");
        return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // IOCP 공통 설정 (MainServer main.cpp와 동일 패턴) :contentReference[oaicite:2]{index=2}
    IocpConfig iocpCfg;
    iocpCfg.maxSessions = 2000;
    iocpCfg.bufferCount = 4000;
    iocpCfg.bufferSize = 4096;
    iocpCfg.recvRingCapacity = 64 * 1024;

    // ChatServer
    const uint16_t chatPort = 3800;   // 네가 원하는 포트로 조정
    const int workerThreads = 4;

    ChatServer chatServer(iocpCfg);

    if (!chatServer.Start(chatPort, workerThreads))
    {
        std::printf("[CHAT][ERROR] Failed to start ChatServer.\n");
        LOG_ERROR("[CHAT] Failed to start ChatServer.");
        WSACleanup();

        LOG_INFO("ChatServer shutting down");
        Logger::Shutdown();
        return 1;
    }

    std::printf("[CHAT][INFO] ChatServer started. Listening on port %u.\n", chatPort);
    LOG_INFO(std::string("[CHAT] ChatServer started. Listening on port ") + std::to_string(chatPort));
    std::printf("[CHAT][INFO] Press Ctrl+C to stop.\n");

    while (g_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::printf("[CHAT][INFO] Stopping...\n");
    LOG_INFO("[CHAT] Stopping...");
    chatServer.Stop();

    WSACleanup();
    std::printf("[CHAT][INFO] Stopped cleanly.\n");
    LOG_INFO("[CHAT] Stopped cleanly.");

    LOG_INFO("ChatServer shutting down");
    Logger::Shutdown();
    return 0;
}
