#include "IocpWorker.h"
#include "IocpCommon.h"
#include "Session/Session.h"

IocpWorker::~IocpWorker() {
    Stop();
}

void IocpWorker::Start() {
    // �̹� ���� ���̸� ����� x
    if (running_.exchange(true)) return;

    th_ = std::thread(&IocpWorker::Loop, this);
}

void IocpWorker::Stop() {
    // ���� ���� �ƴϸ� ����
    if (!running_.exchange(false)) return;

    // ���� ��ȣ
    PostQuitToIocp(iocp_);

    // ������ �շ�
    if (th_.joinable()) th_.join();
}

void IocpWorker::Loop() {
    while (running_) {
        DWORD bytes = 0; // ���� �Ϸ�� ����Ʈ ��
        ULONG_PTR key = 0; 
        OVERLAPPED* base = nullptr; 

        // �Ϸ� �̺�Ʈ ���
        BOOL ok = ::GetQueuedCompletionStatus(iocp_, &bytes, &key, &base, INFINITE);
        if (!running_) break; 

        // ���� ��ȣ(null overlapped)�� ���� ������
        if (base == nullptr) continue;

        // Ȯ�� ����ü�� ��ȯ
        OverlappedEx* ov = AsOv(base);

        auto* session = static_cast<Session*>(OvOwner(ov));
        if (session) {
            session->OnIoCompleted(ov, bytes, ok);
        }

        FreeOv(ov);
    }
}
