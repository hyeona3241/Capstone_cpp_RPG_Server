#include "IocpServerBase.h"
#include "Session.h"
#include "Buffer.h"

// AcceptEx 함수 포인터
LPFN_ACCEPTEX g_AcceptEx = nullptr;

bool LoadAcceptEx(SOCKET listenSock)
{
    if (g_AcceptEx)
        return true;

    GUID guidAcceptEx = WSAID_ACCEPTEX;
    DWORD bytes = 0;
    int result = WSAIoctl(
        listenSock,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &guidAcceptEx,
        sizeof(guidAcceptEx),
        &g_AcceptEx,
        sizeof(g_AcceptEx),
        &bytes,
        nullptr,
        nullptr
    );
    return (result == 0 && g_AcceptEx != nullptr);
}

IocpServerBase::IocpServerBase(const IocpConfig& cfg)
    : config_(cfg)
    , bufferPool_(cfg.bufferCount, cfg.bufferSize)
{
}

IocpServerBase::~IocpServerBase()
{
    Stop();
}

bool IocpServerBase::Start(uint16_t port, int workerThreads)
{
    if (running_)
        return false;

    if (!CreateListenSocket(port))
        return false;

    if (!CreateIocp())
        return false;

    if (!LoadAcceptEx(listenSock_))
        return false;

    running_ = true;

    PostInitialAccepts(1);
    CreateWorkerThreads(workerThreads);

    return true;
}

void IocpServerBase::Stop()
{
    if (!running_)
        return;

    running_ = false;

    for (std::size_t i = 0; i < workers_.size(); ++i)
    {
        ::PostQueuedCompletionStatus(iocpHandle_, 0, 0, nullptr);
    }

    DestroyWorkerThreads();

    if (listenSock_ != INVALID_SOCKET)
    {
        ::closesocket(listenSock_);
        listenSock_ = INVALID_SOCKET;
    }

    if (iocpHandle_)
    {
        ::CloseHandle(iocpHandle_);
        iocpHandle_ = nullptr;
    }
}

Session* IocpServerBase::ConnectTo(const char* ip, uint16_t port, SessionRole role)
{
    if (!running_ || !iocpHandle_)
        return nullptr;

    SOCKET s = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (s == INVALID_SOCKET)
        return nullptr;

    SOCKADDR_IN addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (::inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        ::closesocket(s);
        return nullptr;
    }

    if (::connect(s, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        ::closesocket(s);
        return nullptr;
    }

    Session* session = AcquireOutboundSession(s, role);
    if (!session)
    {
        ::closesocket(s);
        return nullptr;
    }

    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocpHandle_, 0, 0);
    if (h != iocpHandle_)
    {
        session->Disconnect();
        ReleaseSession(session);
        return nullptr;
    }

    session->PostRecv();
    OnClientConnected(session);

    return session;
}

bool IocpServerBase::CreateListenSocket(uint16_t port)
{
    listenSock_ = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (listenSock_ == INVALID_SOCKET)
        return false;

    SOCKADDR_IN addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (::bind(listenSock_, reinterpret_cast<SOCKADDR*>(&addr), sizeof(addr)) == SOCKET_ERROR)
        return false;

    if (::listen(listenSock_, SOMAXCONN) == SOCKET_ERROR)
        return false;

    return true;
}

bool IocpServerBase::CreateIocp()
{
    iocpHandle_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!iocpHandle_)
        return false;

    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(listenSock_), iocpHandle_, 0, 0);
    return (h == iocpHandle_);
}

void IocpServerBase::CreateWorkerThreads(int workerThreads)
{
    if (workerThreads <= 0)
        workerThreads = 1;

    workers_.reserve(workerThreads);
    for (int i = 0; i < workerThreads; ++i)
    {
        workers_.emplace_back([this]()
            {
                WorkerLoop();
            });
    }
}

void IocpServerBase::DestroyWorkerThreads()
{
    for (auto& th : workers_)
    {
        if (th.joinable())
            th.join();
    }
    workers_.clear();
}

void IocpServerBase::PostInitialAccepts(int count)
{
    for (int i = 0; i < count; ++i)
    {
        PostAccept();
    }
}

void IocpServerBase::PostAccept()
{
    acceptSock_ = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (acceptSock_ == INVALID_SOCKET)
        return;

    acceptBuffer_ = bufferPool_.Acquire();
    if (!acceptBuffer_)
    {
        ::closesocket(acceptSock_);
        acceptSock_ = INVALID_SOCKET;
        return;
    }

    acceptOvl_.ResetOverlapped();
    acceptOvl_.Setup(IoType::Accept, nullptr, acceptBuffer_);

    acceptOvl_.wsaBuf.buf = acceptBuffer_->Data();
    acceptOvl_.wsaBuf.len = static_cast<ULONG>(acceptBuffer_->Capacity());

    DWORD bytesReceived = 0;

    BOOL ok = g_AcceptEx(
        listenSock_,
        acceptSock_,
        acceptOvl_.wsaBuf.buf,
        0,
        sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16,
        &bytesReceived,
        &acceptOvl_
    );

    if (!ok)
    {
        int err = ::WSAGetLastError();
        if (err != ERROR_IO_PENDING)
        {
            bufferPool_.Release(acceptBuffer_);
            acceptBuffer_ = nullptr;

            ::closesocket(acceptSock_);
            acceptSock_ = INVALID_SOCKET;
        }
    }
}

void IocpServerBase::OnAcceptCompleted(OverlappedEx* ovl, DWORD /*bytes*/)
{
    SOCKET clientSock = acceptSock_;

    PostAccept();

    Session* session = AcquireAcceptedSession(clientSock, SessionRole::Client);
    if (!session)
    {
        ::closesocket(clientSock);
        return;
    }

    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSock), iocpHandle_, 0, 0);
    if (h != iocpHandle_)
    {
        session->Disconnect();
        ReleaseSession(session);
        return;
    }

    if (ovl->buffer)
    {
        bufferPool_.Release(ovl->buffer);
        ovl->buffer = nullptr;
    }

    session->PostRecv();
    OnClientConnected(session);
}

void IocpServerBase::WorkerLoop()
{
    while (running_)
    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        LPOVERLAPPED overlapped = nullptr;

        BOOL ok = ::GetQueuedCompletionStatus(
            iocpHandle_,
            &bytes,
            &key,
            &overlapped,
            INFINITE
        );

        if (!running_)
            break;

        if (!ok && overlapped == nullptr)
            continue;

        if (!overlapped)
            continue;

        OverlappedEx* ovl = reinterpret_cast<OverlappedEx*>(overlapped);

        switch (ovl->type)
        {
        case IoType::Accept:
            OnAcceptCompleted(ovl, bytes);
            break;
        case IoType::Recv:
            HandleRecvCompleted(ovl, bytes);
            break;
        case IoType::Send:
            HandleSendCompleted(ovl, bytes);
            break;
        default:
            break;
        }
    }
}

void IocpServerBase::HandleRecvCompleted(OverlappedEx* ovl, DWORD bytes)
{
    Session* session = ovl->session;
    Buffer* buf = ovl->buffer;

    if (!session || !buf)
    {
        if (buf)
            bufferPool_.Release(buf);
        return;
    }

    session->OnRecvCompleted(buf, bytes);
}

void IocpServerBase::HandleSendCompleted(OverlappedEx* ovl, DWORD bytes)
{
    Session* session = ovl->session;
    Buffer* buf = ovl->buffer;

    if (!session || !buf)
    {
        if (buf)
            bufferPool_.Release(buf);
        return;
    }

    session->OnSendCompleted(buf, bytes);
}

void IocpServerBase::NotifySessionDisconnect(Session* s)
{
    if (!s)
        return;

    OnClientDisconnected(s);
    ReleaseSession(s);
}