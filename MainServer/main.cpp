#include "Pch.h"
#include "MainServer.h"

#include <csignal>
#include <thread>
#include <chrono>
#include <Logger.h>

static bool g_running = true;

static void SignalHandler(int sig)
{
    std::printf("\n[MAIN] Signal received (%d). Shutting down...\n", sig);
    LOG_INFO(std::string("[MAIN] Signal received (") + std::to_string(sig) + "). Shutting down...");
    g_running = false;
}

int main()
{
    Logger::Config logCfg;
    logCfg.app_name = "MainServer";
    logCfg.dir = "logs/main";
    logCfg.rotate_bytes = 10 * 1024 * 1024;
    logCfg.keep_files = 5;

    Logger::Init(logCfg);

    LOG_INFO("[System] MainServer process started");

    // WinSock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::printf("[MAIN][ERROR] WSAStartup failed.\n");
        LOG_ERROR("[MAIN] WSAStartup failed.");
        return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

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
        LOG_ERROR("[MAIN] Failed to start MainServer.");
        WSACleanup();
        return 1;
    }

    std::printf("[MAIN][INFO] MainServer started. Listening on port %u.\n", mainPort);
    LOG_INFO(std::string("[MAIN] MainServer started. Listening on port ") + std::to_string(mainPort));

    // Main이 모든 연결 담당(기존 main.cpp 로직을 여기로) :contentReference[oaicite:6]{index=6}
    const uint16_t dbPort = 3600;
    const uint16_t loginPort = 3700;
    const uint16_t chatPort = 3800;

    // DB 연결
    if (!mainServer.ConnectToDbServer("127.0.0.1", dbPort))
    {
        std::printf("[MAIN][ERROR] Failed to connect to DBServer (%u)\n", dbPort);
        LOG_ERROR(std::string("[MAIN] Failed to connect to DBServer (") + std::to_string(dbPort) + ")");
        mainServer.Stop();
        WSACleanup();

        LOG_INFO("MainServer shutting down");
        Logger::Shutdown();
        return 1;
    }
    std::printf("[MAIN][INFO] Connected to DBServer.\n");
    LOG_INFO("[MAIN] Connected to DBServer.");

    // Login 연결
    if (!mainServer.ConnectToLoginServer("127.0.0.1", loginPort))
    {
        std::printf("[MAIN][ERROR] Failed to connect to LoginServer (%u)\n", loginPort);
        LOG_ERROR(std::string("[MAIN] Failed to connect to LoginServer (") + std::to_string(loginPort) + ")");
        mainServer.Stop();
        WSACleanup();

        LOG_INFO("MainServer shutting down");
        Logger::Shutdown();
        return 1;
    }
    std::printf("[MAIN][INFO] Connected to LoginServer.\n");
    LOG_INFO("[MAIN] Connected to LoginServer.");

    if (!mainServer.ConnectToChatServer("127.0.0.1", chatPort))
    {
        std::printf("[MAIN][ERROR] Failed to connect to ChatServer (%u)\n", chatPort);
        LOG_ERROR(std::string("[MAIN] Failed to connect to ChatServer (") + std::to_string(chatPort) + ")");
        mainServer.Stop();
        WSACleanup();

        LOG_INFO("MainServer shutting down");
        Logger::Shutdown();
        return 1;
    }
    std::printf("[MAIN][INFO] Connected to ChatServer.\n");
    LOG_INFO("[MAIN] Connected to ChatServer.");

    std::printf("[MAIN][INFO] Press Ctrl+C to stop.\n");

    while (g_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::printf("[MAIN][INFO] Stopping...\n");
    LOG_INFO("[MAIN] Stopping...");
    mainServer.Stop();

    WSACleanup();
    std::printf("[MAIN][INFO] Stopped cleanly.\n");
    LOG_INFO("[MAIN] Stopped cleanly.");

    LOG_INFO("MainServer shutting down");
    Logger::Shutdown();
    return 0;
}
