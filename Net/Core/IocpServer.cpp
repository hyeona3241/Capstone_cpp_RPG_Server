#include "IocpServer.h"
#include <iostream>

IocpServer::~IocpServer() {
    Stop(); // 안전 종료
}

bool IocpServer::Start(const char* ip, uint16_t port, int workerCount, SessionManager* mgr, INetListener* listener)
{
    // Winsock 초기화
    if (!SocketUtils::InitWinsock()) {
        std::cerr << "[IocpServer] InitWinsock failed\n";
        return false;
    }

    mgr_ = mgr;
    //listener_ = listener;

    // 리슨 소켓 생성
    listen_ = SocketUtils::CreateTcp();
    if (listen_ == INVALID_SOCKET) {
        std::cerr << "[IocpServer] CreateTcp failed\n";
        return false;
    }

    // 리슨 소켓 옵션 설정 (재바인드/논블로킹)
    SocketUtils::SetReuseAddr(listen_);
    SocketUtils::SetNonBlocking(listen_);

    // 바인드 / 리슨
    if (!SocketUtils::BindAndListen(listen_, ip, port, NetConfig::kListenBacklog)) {
        std::cerr << "[IocpServer] BindAndListen failed\n";
        SocketUtils::CloseSocket(listen_);
        return false;
    }

    // IOCP 생성
    iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!iocp_) {
        std::cerr << "[IocpServer] CreateIoCompletionPort (create) failed\n";
        SocketUtils::CloseSocket(listen_);
        return false;
    }

    // 워커 스레드 시작
    workerCount = (workerCount <= 0) ? 1 : workerCount;
    workers_.reserve(workerCount);
    for (int i = 0; i < workerCount; ++i) {
        auto* w = new IocpWorker(iocp_); // IOCP 핸들 공유
        w->Start();
        workers_.push_back(w);
    }

    // 서버 동작 시작
    running_ = true;

    // accept 전용 스레드 시작
    acceptThread_ = std::thread(&IocpServer::AcceptLoop, this);

    std::cout << "[IocpServer] Started on " << ip << ":" << port
        << " (workers=" << workerCount << ")\n";
    return true;
}

void IocpServer::Stop() {
    if (!running_.exchange(false)) return; // 이미 정지면 무시

    // 리슨 소켓 닫기
    SocketUtils::CloseSocket(listen_);

    // accept 스레드 합류
    if (acceptThread_.joinable()) acceptThread_.join();

    // 워커 정지 및 해제
    for (auto* w : workers_) { w->Stop(); delete w; }
    workers_.clear();

    // IOCP 핸들 닫기
    if (iocp_) {
        ::CloseHandle(iocp_);
        iocp_ = nullptr;
    }

    std::cout << "[IocpServer] Stopped\n";
}

void IocpServer::AcceptLoop() {
    // 논블로킹 리슨 소켓에서 루프로 accept 시도
    while (running_) {
        sockaddr_in caddr{};
        int len = sizeof(caddr);

        SOCKET cs = ::accept(listen_, reinterpret_cast<sockaddr*>(&caddr), &len);
        if (cs == INVALID_SOCKET) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        // 신규 소켓 옵션(논블로킹/딜레이 제거)
        SocketUtils::SetNonBlocking(cs);
        SocketUtils::SetNoDelay(cs);

        // 세션 생성/등록)
        Session* s = mgr_->CreateSession(cs);
        if (!s) {
            // 풀 부족 등으로 실패 시 소켓 닫기
            std::cerr << "[IocpServer] CreateSession failed (pool exhausted?)\n";
            SocketUtils::CloseSocket(cs);
            continue;
        }

        // 세션 소켓 IOCP에 연결
        if (!RegisterSocketToIocp(cs, s)) {
            std::cerr << "[IocpServer] RegisterSocketToIocp failed\n";
            mgr_->DestroySession(s->id()); // 등록 실패 시 반납
            SocketUtils::CloseSocket(cs);
            continue;
        }

        // 세션 시작
        s->Start();

        //// 상위 앱에 알림
        //if (listener_) listener_->OnSessionOpened(s);
    }
}

bool IocpServer::RegisterSocketToIocp(SOCKET s, void* completionKeyOwner) {
    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocp_, reinterpret_cast<ULONG_PTR>(completionKeyOwner), 0);
    return h != nullptr;
}