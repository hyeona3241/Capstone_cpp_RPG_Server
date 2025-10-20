#include "IocpServer.h"
#include <iostream>

IocpServer::~IocpServer() {
    Stop(); // ���� ����
}

bool IocpServer::Start(const char* ip, uint16_t port, int workerCount, SessionManager* mgr, INetListener* listener)
{
    // Winsock �ʱ�ȭ
    if (!SocketUtils::InitWinsock()) {
        std::cerr << "[IocpServer] InitWinsock failed\n";
        return false;
    }

    mgr_ = mgr;
    //listener_ = listener;

    // ���� ���� ����
    listen_ = SocketUtils::CreateTcp();
    if (listen_ == INVALID_SOCKET) {
        std::cerr << "[IocpServer] CreateTcp failed\n";
        return false;
    }

    // ���� ���� �ɼ� ���� (����ε�/����ŷ)
    SocketUtils::SetReuseAddr(listen_);
    SocketUtils::SetNonBlocking(listen_);

    // ���ε� / ����
    if (!SocketUtils::BindAndListen(listen_, ip, port, NetConfig::kListenBacklog)) {
        std::cerr << "[IocpServer] BindAndListen failed\n";
        SocketUtils::CloseSocket(listen_);
        return false;
    }

    // IOCP ����
    iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!iocp_) {
        std::cerr << "[IocpServer] CreateIoCompletionPort (create) failed\n";
        SocketUtils::CloseSocket(listen_);
        return false;
    }

    // ��Ŀ ������ ����
    workerCount = (workerCount <= 0) ? 1 : workerCount;
    workers_.reserve(workerCount);
    for (int i = 0; i < workerCount; ++i) {
        auto* w = new IocpWorker(iocp_); // IOCP �ڵ� ����
        w->Start();
        workers_.push_back(w);
    }

    // ���� ���� ����
    running_ = true;

    // accept ���� ������ ����
    acceptThread_ = std::thread(&IocpServer::AcceptLoop, this);

    std::cout << "[IocpServer] Started on " << ip << ":" << port
        << " (workers=" << workerCount << ")\n";
    return true;
}

void IocpServer::Stop() {
    if (!running_.exchange(false)) return; // �̹� ������ ����

    // ���� ���� �ݱ�
    SocketUtils::CloseSocket(listen_);

    // accept ������ �շ�
    if (acceptThread_.joinable()) acceptThread_.join();

    // ��Ŀ ���� �� ����
    for (auto* w : workers_) { w->Stop(); delete w; }
    workers_.clear();

    // IOCP �ڵ� �ݱ�
    if (iocp_) {
        ::CloseHandle(iocp_);
        iocp_ = nullptr;
    }

    std::cout << "[IocpServer] Stopped\n";
}

void IocpServer::AcceptLoop() {
    // ����ŷ ���� ���Ͽ��� ������ accept �õ�
    while (running_) {
        sockaddr_in caddr{};
        int len = sizeof(caddr);

        SOCKET cs = ::accept(listen_, reinterpret_cast<sockaddr*>(&caddr), &len);
        if (cs == INVALID_SOCKET) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // �ű� ���� �ɼ�(����ŷ/������ ����)
        SocketUtils::SetNonBlocking(cs);
        SocketUtils::SetNoDelay(cs);

        // ���� ����/���)
        Session* s = mgr_->CreateSession(cs);
        if (!s) {
            // Ǯ ���� ������ ���� �� ���� �ݱ�
            std::cerr << "[IocpServer] CreateSession failed (pool exhausted?)\n";
            SocketUtils::CloseSocket(cs);
            continue;
        }

        // ���� ���� IOCP�� ����
        if (!RegisterSocketToIocp(cs, s)) {
            std::cerr << "[IocpServer] RegisterSocketToIocp failed\n";
            mgr_->DestroySession(s->id()); // ��� ���� �� �ݳ�
            SocketUtils::CloseSocket(cs);
            continue;
        }

        // ���� ����
        s->Start();

        //// ���� �ۿ� �˸�
        //if (listener_) listener_->OnSessionOpened(s);
    }
}

bool IocpServer::RegisterSocketToIocp(SOCKET s, void* completionKeyOwner) {
    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocp_, reinterpret_cast<ULONG_PTR>(completionKeyOwner), 0);
    return h != nullptr;
}