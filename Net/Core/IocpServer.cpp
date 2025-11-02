

#include "IocpServer.h"
#include <iostream>
#include <mswsock.h>

IocpServer::~IocpServer() {
    Stop(); // 안전 종료
}

bool IocpServer::Start(const char* ip, uint16_t port, int workerCount, SessionManager* mgr, INetListener* listener)
{
    if (!SocketUtils::InitWinsock()) { /*...*/ return false; }

    mgr_ = mgr;
    listener_ = listener;
    if (listener_) listener_->OnServerStarting();

    // 리슨 소켓
    listen_ = SocketUtils::CreateTcp();
    if (listen_ == INVALID_SOCKET) { /*...*/ return false; }
    SocketUtils::SetReuseAddr(listen_);

    // AcceptEx는 논블로킹 필수는 아님이지만, 일관성 위해 유지
    SocketUtils::SetNonBlocking(listen_);
    if (!SocketUtils::BindAndListen(listen_, ip, port, NetConfig::kListenBacklog)) { /*...*/ return false; }

    // IOCP 생성
    iocp_ = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!iocp_) { /*...*/ return false; }

    // 리슨 소켓을 IOCP에 연결: completionKey = this (서버 소유)
    if (!::CreateIoCompletionPort(reinterpret_cast<HANDLE>(listen_), iocp_,
        reinterpret_cast<ULONG_PTR>(this), 0)) {
        std::cerr << "[IocpServer] Associate listen socket to IOCP failed\n";
        SocketUtils::CloseSocket(listen_);
        return false;
    }

    // 확장 함수 로드
    if (!LoadExtensionFns()) {
        std::cerr << "[IocpServer] LoadExtensionFns failed\n";
        SocketUtils::CloseSocket(listen_);
        return false;
    }

    // 워커 시작 (기존과 동일)
    workerCount = (workerCount <= 0) ? 1 : workerCount;
    workers_.reserve(workerCount);
    for (int i = 0; i < workerCount; ++i) {
        auto* w = new IocpWorker(iocp_);
        w->Start();
        workers_.push_back(w);
    }

    // 서버 동작 + AcceptEx 사전등록
    running_ = true;
    PostAcceptBatch(prepostAccepts_);   // 예: 128

    std::cout << "[IocpServer] Started on " << ip << ":" << port
        << " (workers=" << workerCount << ", prepost=" << prepostAccepts_ << ")\n";

    if (listener_) listener_->OnServerStarted();
    return true;
}


void IocpServer::Stop() {
    if (!running_.exchange(false)) return;

    // 리슨 소켓 닫기 -> 대기 중 AcceptEx들은 오류완료(ABORTED)로 돌아옴
    SocketUtils::CloseSocket(listen_);

    for (auto* w : workers_) { w->Stop(); delete w; }
    workers_.clear();

    if (iocp_) {
        ::CloseHandle(iocp_);
        iocp_ = nullptr;
    }

    if (listener_) listener_->OnServerStopped();
    std::cout << "[IocpServer] Stopped\n";
}


//void IocpServer::AcceptLoop() {
//    // 논블로킹 리슨 소켓에서 루프로 accept 시도
//    while (running_) {
//        sockaddr_in caddr{};
//        int len = sizeof(caddr);
//
//        SOCKET cs = ::accept(listen_, reinterpret_cast<sockaddr*>(&caddr), &len);
//        if (cs == INVALID_SOCKET) {
//            std::this_thread::sleep_for(std::chrono::milliseconds(1));
//            continue;
//        }
//
//        // 신규 소켓 옵션(논블로킹/딜레이 제거)
//        SocketUtils::SetNonBlocking(cs);
//        SocketUtils::SetNoDelay(cs);
//
//        // 세션 생성/등록)
//        Session* s = mgr_->CreateSession(cs);
//        if (!s) {
//            // 풀 부족 등으로 실패 시 소켓 닫기
//            std::cerr << "[IocpServer] CreateSession failed (pool exhausted?)\n";
//            SocketUtils::CloseSocket(cs);
//            continue;
//        }
//
//        // 세션 소켓 IOCP에 연결
//        if (!RegisterSocketToIocp(cs, s)) {
//            std::cerr << "[IocpServer] RegisterSocketToIocp failed\n";
//            mgr_->DestroySession(s->id()); // 등록 실패 시 반납
//            SocketUtils::CloseSocket(cs);
//            continue;
//        }
//
//        // 세션 시작
//        s->Start();
//
//        //// 상위 앱에 알림
//        //if (listener_) listener_->OnSessionOpened(s);
//    }
//}

bool IocpServer::RegisterSocketToIocp(SOCKET s, void* completionKeyOwner) {
    HANDLE h = ::CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), iocp_, reinterpret_cast<ULONG_PTR>(completionKeyOwner), 0);
    return h != nullptr;
}

bool IocpServer::LoadExtensionFns() {
    GUID g1 = WSAID_ACCEPTEX;
    GUID g2 = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD bytes = 0;

    // AcceptEx
    if (WSAIoctl(listen_, SIO_GET_EXTENSION_FUNCTION_POINTER, &g1, sizeof(g1),
        &pAcceptEx_, sizeof(pAcceptEx_), &bytes, nullptr, nullptr) == SOCKET_ERROR) {
        return false;
    }
    // GetAcceptExSockaddrs
    if (WSAIoctl(listen_, SIO_GET_EXTENSION_FUNCTION_POINTER, &g2, sizeof(g2),
        &pGetAcceptExSockaddrs_, sizeof(pGetAcceptExSockaddrs_), &bytes, nullptr, nullptr) == SOCKET_ERROR) {
        return false;
    }
    return pAcceptEx_ && pGetAcceptExSockaddrs_;
}


bool IocpServer::PostAcceptBatch(int n) {
    for (int i = 0; i < n; ++i)
        if (!PostOneAccept()) return false;
    return true;
}

bool IocpServer::PostOneAccept() {
    if (!running_) return false;

    auto* ctx = new AcceptCtx{};
    ctx->acceptSock = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (ctx->acceptSock == INVALID_SOCKET) { delete ctx; return false; }

    // 아직 세션을 만들지 않고, 완료 후에 만들고 싶다면 sess=nullptr 유지
    // (혹은 미리 풀에서 받아 놓는 방식으로도 가능)

    DWORD bytes = 0;
    BOOL ok = pAcceptEx_(listen_, ctx->acceptSock,
        ctx->addrBuf, 0,
        sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
        &bytes, &ctx->ov);

    if (!ok) {
        const int e = WSAGetLastError();
        if (e != WSA_IO_PENDING) {
            closesocket(ctx->acceptSock);
            delete ctx;
            return false;
        }
    }
    ++inflightAccepts_;
    return true;
}


void IocpServer::OnIocpAcceptComplete(AcceptCtx* ctx, DWORD /*bytes*/, int sysErr) noexcept {
    --inflightAccepts_;

    if (sysErr) {
        // 정상 종료/중단(WSA_OPERATION_ABORTED) 등은 조용히 정리
        closesocket(ctx->acceptSock);
        delete ctx;
        if (running_) PostOneAccept();  // steady-state 유지
        return;
    }

    // UPDATE_ACCEPT_CONTEXT (필수)
    if (setsockopt(ctx->acceptSock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        reinterpret_cast<char*>(&listen_), sizeof(listen_)) == SOCKET_ERROR) {
        closesocket(ctx->acceptSock);
        delete ctx;
        if (running_) PostOneAccept();
        return;
    }

    // 주소 파싱
    LPSOCKADDR local = nullptr;
    LPSOCKADDR remote = nullptr;
    INT localLen = 0;
    INT remoteLen = 0;

    pGetAcceptExSockaddrs_(ctx->addrBuf, 0,
        sizeof(SOCKADDR_STORAGE) + 16,
        sizeof(SOCKADDR_STORAGE) + 16,
        &local, &localLen, &remote, &remoteLen);

    // 3) 옵션 적용
    SocketUtils::SetNonBlocking(ctx->acceptSock);
    SocketUtils::SetNoDelay(ctx->acceptSock);

    // 4) 세션 생성 + IOCP 연결 + 시작
    Session* s = mgr_->CreateSession(ctx->acceptSock);
    if (!s) {
        SocketUtils::CloseSocket(ctx->acceptSock);
        delete ctx;
        if (running_) PostOneAccept();
        return;
    }

    if (!RegisterSocketToIocp(ctx->acceptSock, s)) {
        mgr_->DestroySession(s->id());
        SocketUtils::CloseSocket(ctx->acceptSock);
        delete ctx;
        if (running_) PostOneAccept();
        return;
    }

    s->Start(); // 내부에서 첫 WSARecv pre-post

    //// 상위 알림(선택)
    //if (listener_) {
    //    ListenerSessionInfo info{};
    //    info.uid = s->id();
    //    info.remoteIp = s->RemoteIpStr(); // 네 구현에 맞춰 제공
    //    info.remotePort = s->RemotePort();
    //    info.localIp = s->LocalIpStr();
    //    info.localPort = s->LocalPort();
    //    listener_->OnSessionOpened(s, info);
    //}

    delete ctx;

    // 다음 AcceptEx 보강
    if (running_) PostOneAccept();
}
