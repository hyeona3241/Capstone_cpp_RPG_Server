#include "Session.h"
#include "IocpServerBase.h"
#include "BufferPool.h"
#include "Buffer.h"

namespace
{
    constexpr std::size_t kRecvRingCapacity = 64 * 1024;
}

Session::Session()
    : recvRing_(kRecvRingCapacity)
{
    recvOvl_.Setup(IoType::Recv, this, nullptr);
    sendOvl_.Setup(IoType::Send, this, nullptr);
}

Session::~Session()
{
    Disconnect();
}

void Session::Init(SOCKET s, IocpServerBase* owner, SessionRole role, uint64_t id)
{
    sock_ = s;
    owner_ = owner;
    role_ = role;
    id_ = id;

    connected_ = true;
    state_ = (role == SessionRole::Client) ? SessionState::Handshake
        : SessionState::NoneClient;

    recvRing_.clear();
}

void Session::Disconnect()
{
    if (!connected_.exchange(false))
        return;

    closesocket(sock_);
    sock_ = INVALID_SOCKET;

    state_ = SessionState::Disconnected;

    // 상위 서버에게 알리기
    if (owner_)
        owner_->NotifySessionDisconnect(this);
}

void Session::ResetForReuse()
{
    sock_ = INVALID_SOCKET;

    connected_ = false;
    sending_ = false;
    closing_ = false;

    role_ = SessionRole::Unknown;
    state_ = SessionState::Disconnected;

    id_ = 0;

    recvRing_.clear();

    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        std::queue<SendJob> empty;
        std::swap(sendQueue_, empty);
    }

    recvOvl_.ResetAll();
    sendOvl_.ResetAll();
}

void Session::PostRecv()
{
    if (!connected_) return;

    Buffer* buf = owner_->GetBufferPool()->Acquire();
    if (!buf)
        return; // 버퍼 부족 -> 치명적 로그 가능

    recvOvl_.Setup(IoType::Recv, this, buf);

    DWORD flags = 0;
    DWORD bytes = 0;

    int ret = WSARecv(sock_, &recvOvl_.wsaBuf, 1, &bytes, &flags, &recvOvl_, nullptr);

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            Disconnect();
        }
    }
}

void Session::OnRecvCompleted(Buffer* buf, DWORD bytes)
{
    if (bytes == 0)
    {
        owner_->GetBufferPool()->Release(buf);
        Disconnect();
        return;
    }

    buf->AdvanceWrite(bytes);

    const void* src = static_cast<const void*>(buf->Data());
    if (!recvRing_.push(src, bytes))
    {
        // 링버퍼 공간 부족 등 에러 ( 세션 종료)
        owner_->GetBufferPool()->Release(buf);
        Disconnect();
        return;
    }

    ProcessPacketsFromRing();

    // 버퍼 반납
    owner_->GetBufferPool()->Release(buf);

    PostRecv();
}

void Session::ProcessPacketsFromRing()
{
    std::vector<std::byte> rawPacket;

    while (recvRing_.try_pop_packet(rawPacket))
    {
        if (rawPacket.size() < sizeof(PacketHeader))
        {
            Disconnect();
            return;
        }

        const auto* header = reinterpret_cast<const PacketHeader*>(rawPacket.data());
        const std::size_t totalLen = header->size;
        const std::size_t headerSize = sizeof(PacketHeader);

        if (totalLen < headerSize || totalLen != rawPacket.size())
        {
            Disconnect();
            return;
        }

        const std::byte* payload = rawPacket.data() + headerSize;
        const std::size_t payloadSz = totalLen - headerSize;

        if (owner_)
        {
            owner_->DispatchRawPacket(this, *header, payload, payloadSz);
        }
    }
}

void Session::Send(const void* data, size_t len)
{
    if (!connected_) return;

    std::lock_guard<std::mutex> lock(sendMutex_);

    sendQueue_.push({ std::vector<char>((char*)data, (char*)data + len) });

    // 이미 Send 중이면 반환
    if (sending_) return;

    sending_ = true;
    PostSend();
}

void Session::PostSend()
{
    if (sendQueue_.empty())
    {
        sending_ = false;
        return;
    }

    auto job = sendQueue_.front();
    sendQueue_.pop();

    Buffer* buf = owner_->GetBufferPool()->Acquire();
    if (!buf)
        return;

    memcpy(buf->Data(), job.data.data(), job.data.size());
    buf->AdvanceWrite(job.data.size());

    sendOvl_.Setup(IoType::Send, this, buf);

    DWORD bytes = 0;
    int ret = WSASend(sock_, &sendOvl_.wsaBuf, 1, &bytes, 0, &sendOvl_, nullptr);

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            Disconnect();
        }
    }
}

void Session::OnSendCompleted(Buffer* buf, DWORD)
{
    // 버퍼 반납
    owner_->GetBufferPool()->Release(buf);

    std::lock_guard<std::mutex> lock(sendMutex_);
    PostSend();
}
