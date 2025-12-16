#include "TcpClient.h"
#include <cstdio>

#include "PacketDef.h"
#include "PacketIds.h"
#include "PacketDispatcher.h"

TcpClient::TcpClient()
{
    recvBuf_.reserve(8192);
}

TcpClient::~TcpClient()
{
    Close();
}

bool TcpClient::Connect(const char* ip, uint16_t port)
{
    if (running_.load())
        return false;

    sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_ == INVALID_SOCKET)
    {
        std::printf("[CLIENT][ERROR] socket() failed\n");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        std::printf("[CLIENT][ERROR] connect failed\n");
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
        return false;
    }

    running_.store(true);

    recvThread_ = std::thread([this]() { RecvLoop(); });
    sendThread_ = std::thread([this]() { SendLoop(); });

    return true;
}

void TcpClient::Close()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false))
    {
        return;
    }

    // send thread 깨우기
    sendCv_.notify_all();

    if (sock_ != INVALID_SOCKET)
    {
        shutdown(sock_, SD_BOTH);
        closesocket(sock_);
        sock_ = INVALID_SOCKET;
    }

    if (recvThread_.joinable()) recvThread_.join();
    if (sendThread_.joinable()) sendThread_.join();

    // 큐 비우기
    {
        std::lock_guard<std::mutex> lk(sendMtx_);
        while (!sendQ_.empty()) sendQ_.pop();
    }
    recvBuf_.clear();
}

void TcpClient::EnqueueSend(std::vector<std::byte>&& bytes)
{
    if (!running_.load())
        return;

    {
        std::lock_guard<std::mutex> lk(sendMtx_);
        sendQ_.push(std::move(bytes));
    }
    sendCv_.notify_one();
}

void TcpClient::SendLoop()
{
    while (running_.load())
    {
        std::vector<std::byte> msg;

        {
            std::unique_lock<std::mutex> lk(sendMtx_);
            sendCv_.wait(lk, [&]() { return !running_.load() || !sendQ_.empty(); });

            if (!running_.load())
                break;

            msg = std::move(sendQ_.front());
            sendQ_.pop();
        }

        // send all
        size_t sent = 0;
        const size_t len = msg.size();
        while (sent < len)
        {
            int n = send(
                sock_,
                reinterpret_cast<const char*>(msg.data() + sent),
                static_cast<int>(len - sent),
                0
            );
            if (n <= 0)
            {
                std::printf("[CLIENT][ERROR] send failed\n");
                running_.store(false);
                break;
            }
            sent += static_cast<size_t>(n);
        }
    }
}

void TcpClient::RecvLoop()
{
    char temp[4096];

    while (running_.load())
    {
        int n = recv(sock_, temp, sizeof(temp), 0);
        if (n <= 0)
        {
            std::printf("[CLIENT] recv closed\n");
            running_.store(false);
            break;
        }

        recvBuf_.insert(
            recvBuf_.end(),
            reinterpret_cast<std::byte*>(temp),
            reinterpret_cast<std::byte*>(temp + n)
        );

        // 프레이밍: 네 기존 코드 그대로 :contentReference[oaicite:1]{index=1}
        while (recvBuf_.size() >= Proto::kHeaderSize)
        {
            const auto* header =
                reinterpret_cast<const Proto::PacketHeader*>(recvBuf_.data());

            if (!Proto::IsValidPacketSize(header->size))
            {
                std::printf("[CLIENT][ERROR] invalid packet size=%u\n", header->size);
                running_.store(false);
                break;
            }

            if (recvBuf_.size() < header->size)
                break;

            const std::byte* payload = recvBuf_.data() + Proto::kHeaderSize;
            const size_t payloadLen = header->size - Proto::kHeaderSize;

            if (dispatcher_)
                dispatcher_->Dispatch(header->id, payload, payloadLen);
            else
                std::printf("[CLIENT][WARN] dispatcher not set\n");

            // consume packet
            recvBuf_.erase(recvBuf_.begin(), recvBuf_.begin() + header->size);
        }
    }

    // send loop도 깨워서 같이 종료
    sendCv_.notify_all();
}