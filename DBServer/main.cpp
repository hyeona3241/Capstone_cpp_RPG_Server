#include "Pch.h"
#include "DBServer.h"

#include <csignal>
#include <thread>
#include <chrono>
#include <Logger.h>

static bool g_running = true;

static void SignalHandler(int sig)
{
    std::printf("\n[DB] Signal received (%d). Shutting down...\n", sig);
    LOG_INFO("[DB] Signal received (%d). Shutting down...");
    g_running = false;
}

int main()
{
    Logger::Config logCfg;
    logCfg.app_name = "DBServer";
    logCfg.dir = "logs/main";
    logCfg.rotate_bytes = 10 * 1024 * 1024;
    logCfg.keep_files = 5;

    Logger::Init(logCfg);

    LOG_INFO("[System] DBServer process started");

    // WinSock
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        std::printf("[DB][ERROR] WSAStartup failed.\n");
        LOG_ERROR("[DB] WSAStartup failed.");
        return 1;
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // IOCP 공통 설정(기존 main.cpp에서 그대로) :contentReference[oaicite:2]{index=2}
    IocpConfig iocpCfg;
    iocpCfg.maxSessions = 2000;
    iocpCfg.bufferCount = 4000;
    iocpCfg.bufferSize = 4096;
    iocpCfg.recvRingCapacity = 64 * 1024;

    // DB 설정(기존 main.cpp에서 그대로) :contentReference[oaicite:3]{index=3}
    DBConfig dbCfg;
    dbCfg.host = "100.77.71.68";
    /*dbCfg.host = "127.0.0.1";*/
    dbCfg.port = 3306;
    dbCfg.user = "gameuser";
    dbCfg.password = "game";
    dbCfg.schema = "game_db";
    dbCfg.charset = "utf8mb4";

    const uint16_t dbPort = 3600;
    DBServer dbServer(iocpCfg, dbCfg);

    if (!dbServer.StartServer(dbPort, 2))
    {
        std::printf("[DB][ERROR] Failed to start DBServer.\n");
        LOG_ERROR("[DB] Failed to start DBServer.");
        WSACleanup();

        LOG_INFO("DBServer shutting down");
        Logger::Shutdown();
        return 1;
    }

    std::printf("[DB][INFO] DBServer started. Listening on port %u.\n", dbPort);
    LOG_INFO("[DB] DBServer started. Listening on port " + std::to_string(dbPort) + ".");

    std::printf("[DB][INFO] Press Ctrl+C to stop.\n");
    LOG_INFO("[DB] Press Ctrl+C to stop.");

    while (g_running)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::printf("[DB][INFO] Stopping...\n");
    LOG_INFO("[DB] Stopping...");
    dbServer.StopServer();

    WSACleanup();
    std::printf("[DB][INFO] Stopped cleanly.\n");
    LOG_INFO("[DB] Stopped cleanly.");


    LOG_INFO("DBServer shutting down");
    Logger::Shutdown();
    return 0;
}
