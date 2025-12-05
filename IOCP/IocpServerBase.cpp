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
    : config_(cfg), sessionPool_(cfg.maxSessions), bufferPool_(cfg.bufferCount, cfg.bufferSize)
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

    // 초기 Accept 한 개만
    PostInitialAccepts(1);

    // 워커 스레드 생성
    CreateWorkerThreads(workerThreads);

    return true;
}

void IocpServerBase::Stop()
{
    if (!running_)
        return;

    running_ = false;

    // 워커 스레드를 깨우기 위해 더미 완료 패킷 전송
    for (std::size_t i = 0; i < workers_.size(); ++i)
    {
        ::PostQueuedCompletionStatus(
            iocpHandle_,
            0,
            0,
            nullptr
        );
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

    // 리슨 소켓을 IOCP에 연결 (completionKey는 사용 안 할 예정이라 0)
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
    // Accept용 새 소켓 생성
    acceptSock_ = ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (acceptSock_ == INVALID_SOCKET)
        return;

    // 주소 정보 저장용 버퍼 하나 빌리기
    acceptBuffer_ = bufferPool_.Acquire();
    if (!acceptBuffer_)
    {
        ::closesocket(acceptSock_);
        acceptSock_ = INVALID_SOCKET;
        return;
    }

    // OverlappedEx 세팅
    acceptOvl_.ResetOverlapped();
    acceptOvl_.Setup(IoType::Accept, nullptr, acceptBuffer_);

    acceptOvl_.wsaBuf.buf = acceptBuffer_->Data();
    acceptOvl_.wsaBuf.len = static_cast<ULONG>(acceptBuffer_->Capacity());

    DWORD bytesReceived = 0;

    BOOL ok = g_AcceptEx(
        listenSock_,
        acceptSock_,
        acceptOvl_.wsaBuf.buf,
        0, // 실제 데이터는 바로 Recv에서 받을 거라 0으로 두고
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
            // AcceptEx 실패 → 버퍼/소켓 정리
            bufferPool_.Release(acceptBuffer_);
            acceptBuffer_ = nullptr;

            ::closesocket(acceptSock_);
            acceptSock_ = INVALID_SOCKET;
        }
    }
}

void IocpServerBase::OnAcceptCompleted(OverlappedEx* ovl, DWORD /*bytes*/)
{
    // 새 세션 소켓
    SOCKET clientSock = acceptSock_;

    // 다음 Accept 예약 (끊기지 않도록)
    PostAccept();

    // 세션 풀에서 하나 꺼내기
    Session* session = sessionPool_.Acquire(clientSock, this, SessionRole::Client);
    if (!session)
    {
        ::closesocket(clientSock);
        return;
    }

    // 새 소켓을 IOCP에 연결
    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(clientSock), iocpHandle_, 0, 0);
    if (h != iocpHandle_)
    {
        session->Disconnect();
        ReleaseSession(session);
        return;
    }

    // Accept용 버퍼는 더 이상 사용 안 하므로 반납
    if (ovl->buffer)
    {
        bufferPool_.Release(ovl->buffer);
        ovl->buffer = nullptr;
    }

    // 첫 Recv 예약
    session->PostRecv();

    // 파생 서버에 알림
    OnClientConnected(session);
}


void IocpServerBase::WorkerLoop()
{
    while (running_)
    {
        DWORD     bytes = 0;
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
        {
            // IOCP 자체 에러 or 깨어남 (Stop에서 보낸 패킷 등)
            continue;
        }

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
        case IoType::Connect:
        case IoType::Quit:
        default:
            // 필요에 따라 추가
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

    if (bytes == 0)
    {
        // Session::OnRecvCompleted 내부에서 Disconnect()를 호출했다고 가정
        // 여기서 세션 풀 반납까지 처리해도 됨
        ReleaseSession(session);
    }
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
    // OnSendCompleted 내부에서 버퍼를 반납할 수도 있고,
    // 여기서 Release(buf) 할 수도 있음. 설계에 따라 조정.
}


void IocpServerBase::ReleaseSession(Session* session)
{
    if (!session)
        return;

    // 파생 서버에게 끊김 알림
    OnClientDisconnected(session);

    // 세션은 여기서 풀에 반환
    sessionPool_.Release(session);
}