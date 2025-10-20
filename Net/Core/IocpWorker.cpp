#include "IocpWorker.h"
#include "IocpCommon.h"
#include "Session/Session.h"

IocpWorker::~IocpWorker() {
    Stop();
}

void IocpWorker::Start() {
    // 이미 실행 중이면 재시작 x
    if (running_.exchange(true)) return;

    th_ = std::thread(&IocpWorker::Loop, this);
}

void IocpWorker::Stop() {
    // 실행 중이 아니면 무시
    if (!running_.exchange(false)) return;

    // 종료 신호
    PostQuitToIocp(iocp_);

    // 스레드 합류
    if (th_.joinable()) th_.join();
}

void IocpWorker::Loop() {
    while (running_) {
        DWORD bytes = 0; // 실제 완료된 바이트 수
        ULONG_PTR key = 0; 
        OVERLAPPED* base = nullptr; 

        // 완료 이벤트 대기
        BOOL ok = ::GetQueuedCompletionStatus(iocp_, &bytes, &key, &base, INFINITE);
        if (!running_) break; 

        // 종료 신호(null overlapped)면 다음 루프로
        if (base == nullptr) continue;

        // 확장 구조체로 변환
        OverlappedEx* ov = AsOv(base);

        auto* session = static_cast<Session*>(OvOwner(ov));
        if (session) {
            session->OnIoCompleted(ov, bytes, ok);
        }

        FreeOv(ov);
    }
}
