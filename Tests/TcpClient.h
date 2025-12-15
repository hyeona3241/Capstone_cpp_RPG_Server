#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Pch.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class PacketDispatcher;

class TcpClient
{
public:
    TcpClient();
    ~TcpClient();

    bool Connect(const char* ip, uint16_t port);
    void Close();

    void EnqueueSend(std::vector<std::byte>&& bytes);

    // 수신 스레드에서 dispatcher로 던짐
    void SetDispatcher(PacketDispatcher* dispatcher) { dispatcher_ = dispatcher; }

    bool IsRunning() const { return running_.load(); }

private:
    void RecvLoop();
    void SendLoop();

private:
    SOCKET sock_ = INVALID_SOCKET;

    std::atomic<bool> running_{ false };

    std::thread recvThread_;
    std::thread sendThread_;

    // 수신 버퍼
    std::vector<std::byte> recvBuf_;

    // 송신 큐
    std::mutex sendMtx_;
    std::condition_variable sendCv_;
    std::queue<std::vector<std::byte>> sendQ_;

    PacketDispatcher* dispatcher_ = nullptr;
};

