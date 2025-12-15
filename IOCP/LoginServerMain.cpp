//#include "Pch.h"
//#include "LoginServer.h"
//
//#include <csignal>
//#include <thread>
//#include <chrono>
//#include <Logger.h>
//
//static bool g_running = true;
//
//static void SignalHandler(int sig)
//{
//    std::printf("\n[LS] Signal received (%d). Shutting down...\n", sig);
//    LOG_INFO(std::string("[LS] Signal received (") + std::to_string(sig) + "). Shutting down...");
//    g_running = false;
//}
//
//int main()
//{
//    Logger::Config logCfg;
//    logCfg.app_name = "LoginServer";
//    logCfg.dir = "logs/main";
//    logCfg.rotate_bytes = 10 * 1024 * 1024;
//    logCfg.keep_files = 5;
//
//    Logger::Init(logCfg);
//
//    LOG_INFO("[System] LoginServer process started");
//
//
//    // WinSock
//    WSADATA wsa;
//    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
//    {
//        std::printf("[LS][ERROR] WSAStartup failed.\n");
//        LOG_ERROR("[LS] WSAStartup failed.");
//
//        LOG_INFO("LoginServer shutting down");
//        Logger::Shutdown();
//        return 1;
//    }
//
//    std::signal(SIGINT, SignalHandler);
//    std::signal(SIGTERM, SignalHandler);
//
//    // IOCP 공통 설정(기존 main.cpp에서 그대로) :contentReference[oaicite:4]{index=4}
//    IocpConfig iocpCfg;
//    iocpCfg.maxSessions = 2000;
//    iocpCfg.bufferCount = 4000;
//    iocpCfg.bufferSize = 4096;
//    iocpCfg.recvRingCapacity = 64 * 1024;
//
//    const uint16_t loginPort = 3700;
//    LoginServer loginServer(iocpCfg);
//
//    if (!loginServer.StartServer(loginPort, 2))
//    {
//        std::printf("[LS][ERROR] Failed to start LoginServer.\n");
//        LOG_ERROR("[LS] Failed to start LoginServer.");
//        WSACleanup();
//
//        LOG_INFO("LoginServer shutting down");
//        Logger::Shutdown();
//        return 1;
//    }
//
//    std::printf("[LS][INFO] LoginServer started. Listening on port %u.\n", loginPort);
//    LOG_INFO(std::string("[LS] LoginServer started. Listening on port ") + std::to_string(loginPort));
//    std::printf("[LS][INFO] Press Ctrl+C to stop.\n");
//
//    while (g_running)
//        std::this_thread::sleep_for(std::chrono::milliseconds(100));
//
//    std::printf("[LS][INFO] Stopping...\n");
//    LOG_INFO("[LS] Stopping...");
//    loginServer.StopServer();
//
//    WSACleanup();
//    std::printf("[LS][INFO] Stopped cleanly.\n");
//    LOG_INFO("[LS] Stopped cleanly.");
//
//    LOG_INFO("LoginServer shutting down");
//    Logger::Shutdown();
//    return 0;
//}
