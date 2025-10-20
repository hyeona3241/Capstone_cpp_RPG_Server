#include "Session.h"
#include "Core/SocketUtils.h" 
#include "Protocol/PacketFramer.h" 

void Session::Start() {
    PostRecv();
}

// WSARecv
bool Session::PostRecv() {
    if (!rx_) rx_ = pool_.Acquire();    // �ӽ� ���� ���� �Ӵ�
    rx_.used() = 0; // ��뷮 �ʱ�ȭ

    auto* ov = MakeOv(IoType::Recv, this);  // ���ſ� OverlappedEx ����
    if (!ov) return false;

    WSABUF wsa{};
    wsa.buf = reinterpret_cast<char*>(rx_.data());  // ���� ������ ����
    wsa.len = static_cast<ULONG>(rx_.capacity());   // ���� ũ�� ����

    DWORD flags = 0, bytes = 0;
    int rc = ::WSARecv(sock_, &wsa, 1, &bytes, &flags, ov, nullptr);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        FreeOv(ov);
        return false;
    }

    return true;
}

// WSASend
bool Session::Send(std::span<const std::byte> payload) {
    std::lock_guard lk(sendMtx_);   // ť ���� ��ȣ
    sendQ_.emplace_back(payload.begin(), payload.end());    // ���̷ε� ���� ����

    if (sending_) return true;   // �̹� ���� ���̸� ��

    sending_ = true;
    auto& front = sendQ_.front();

    auto* ov = MakeOv(IoType::Send, this);   // �۽ſ� OverlappedEx
    if (!ov) { sending_ = false; sendQ_.pop_front(); return false; }

    WSABUF wsa{};
    wsa.buf = reinterpret_cast<char*>(front.data());    // ���� ������
    wsa.len = static_cast<ULONG>(front.size());

    DWORD bytes = 0;
    int rc = ::WSASend(sock_, &wsa, 1, &bytes, 0, ov, nullptr);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        sending_ = false; sendQ_.pop_front();   // ��� ���� �� ť ����
        FreeOv(ov);
        return false;
    }
    return true;
}

void Session::OnIoCompleted(OverlappedEx* ov, DWORD bytes, BOOL ok) {
    // ���� ���� / ��� ��
    if (!ok) {
        SocketUtils::CloseSocket(sock_);
        Close();
        return; 
    }

    switch (ov->type) {
    case IoType::Recv: OnRecvCompleted(bytes); break;  // ���� �Ϸ� ó��
    case IoType::Send: OnSendCompleted(bytes); break;  // �۽� �Ϸ� ó��
    default: break;
    }
}

void Session::Close() {
    bool expected = false;

    if (!closing_.compare_exchange_strong(expected, true)) return; // �̹� ���� ���̸� ����
    SocketUtils::CloseSocket(sock_); 
}

void Session::OnRecvCompleted(size_t bytes) {
    if (bytes == 0) {
        SocketUtils::CloseSocket(sock_);
        Close();
        return;
    }

    rx_.used() = bytes; // ���� ���� ũ�� ���
    rbuf_.Write(rx_.cspan()); // �����ۿ� ����
    rx_.reset(); // �ӽ� ���� �ݳ�(���� ����)
    ParseLoop(); // ��Ŷ �Ľ� �õ�
    PostRecv(); // ���� ���� �Խ�
}

void Session::OnSendCompleted(size_t /*bytes*/) {
    std::lock_guard lk(sendMtx_);   // ť ��ȣ
    if (!sendQ_.empty()) sendQ_.pop_front();
    if (sendQ_.empty()) { 
        sending_ = false;
        return;
    }

    auto& front = sendQ_.front();
    auto* ov = MakeOv(IoType::Send, this);
    if (!ov) { sending_ = false; sendQ_.pop_front(); return; }

    WSABUF wsa{};
    wsa.buf = reinterpret_cast<char*>(front.data());
    wsa.len = static_cast<ULONG>(front.size());
    DWORD bytes = 0;

    int rc = ::WSASend(sock_, &wsa, 1, &bytes, 0, ov, nullptr);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        sending_ = false; sendQ_.pop_front();
        FreeOv(ov);
    }
}

void Session::ParseLoop() {

   /*PacketFramer �Ľ� �õ� �Լ� �߰�*/

}