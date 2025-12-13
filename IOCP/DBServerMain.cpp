//#include "Pch.h"
//#include "DBServer.h"
//
//#include <csignal>
//#include <thread>
//#include <chrono>
//
//static bool g_running = true;
//
//static void SignalHandler(int sig)
//{
//    std::printf("\n[DB] Signal received (%d). Shutting down...\n", sig);
//    g_running = false;
//}
//
//int main()
//{
//    // WinSock
//    WSADATA wsa;
//    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
//    {
//        std::printf("[DB][ERROR] WSAStartup failed.\n");
//        return 1;
//    }
//
//    std::signal(SIGINT, SignalHandler);
//    std::signal(SIGTERM, SignalHandler);
//
//    // IOCP 공통 설정(기존 main.cpp에서 그대로) :contentReference[oaicite:2]{index=2}
//    IocpConfig iocpCfg;
//    iocpCfg.maxSessions = 2000;
//    iocpCfg.bufferCount = 4000;
//    iocpCfg.bufferSize = 4096;
//    iocpCfg.recvRingCapacity = 64 * 1024;
//
//    // DB 설정(기존 main.cpp에서 그대로) :contentReference[oaicite:3]{index=3}
//    DBConfig dbCfg;
//    dbCfg.host = "100.77.71.68";
//    dbCfg.port = 3306;
//    dbCfg.user = "gameuser";
//    dbCfg.password = "game";
//    dbCfg.schema = "game_db";
//    dbCfg.charset = "utf8mb4";
//
//    const uint16_t dbPort = 3600;
//    DBServer dbServer(iocpCfg, dbCfg);
//
//    if (!dbServer.StartServer(dbPort, 2))
//    {
//        std::printf("[DB][ERROR] Failed to start DBServer.\n");
//        WSACleanup();
//        return 1;
//    }
//
//    std::printf("[DB][INFO] DBServer started. Listening on port %u.\n", dbPort);
//    std::printf("[DB][INFO] Press Ctrl+C to stop.\n");
//
//    while (g_running)
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//    std::printf("[DB][INFO] Stopping...\n");
//    dbServer.StopServer();
//
//    WSACleanup();
//    std::printf("[DB][INFO] Stopped cleanly.\n");
//    return 0;
//}
