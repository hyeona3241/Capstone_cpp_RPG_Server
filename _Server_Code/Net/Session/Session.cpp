#include "Session.h"
#include "Core/SocketUtils.h" 
#include "Protocol/PacketFramer.h" 

void Session::Start() {
    PostRecv();
}

// WSARecv
bool Session::PostRecv() {
    if (!rx_) rx_ = pool_.Acquire();    // 임시 수신 버퍼 임대
    rx_.used() = 0; // 사용량 초기화

    auto* ov = MakeOv(IoType::Recv, this);  // 수신용 OverlappedEx 생성
    if (!ov) return false;

    WSABUF wsa{};
    wsa.buf = reinterpret_cast<char*>(rx_.data());  // 버퍼 포인터 설정
    wsa.len = static_cast<ULONG>(rx_.capacity());   // 버퍼 크기 설정

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
    std::lock_guard lk(sendMtx_);   // 큐 접근 보호
    sendQ_.emplace_back(payload.begin(), payload.end());    // 페이로드 복사 저장

    if (sending_) return true;   // 이미 전송 중이면 끝

    sending_ = true;
    auto& front = sendQ_.front();

    auto* ov = MakeOv(IoType::Send, this);   // 송신용 OverlappedEx
    if (!ov) { sending_ = false; sendQ_.pop_front(); return false; }

    WSABUF wsa{};
    wsa.buf = reinterpret_cast<char*>(front.data());    // 보낼 데이터
    wsa.len = static_cast<ULONG>(front.size());

    DWORD bytes = 0;
    int rc = ::WSASend(sock_, &wsa, 1, &bytes, 0, ov, nullptr);
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        sending_ = false; sendQ_.pop_front();   // 즉시 실패 시 큐 제거
        FreeOv(ov);
        return false;
    }
    return true;
}

void Session::OnIoCompleted(OverlappedEx* ov, DWORD bytes, BOOL ok) {
    // 소켓 에러 / 취소 등
    if (!ok) {
        SocketUtils::CloseSocket(sock_);
        Close();
        return; 
    }

    switch (ov->type) {
    case IoType::Recv: OnRecvCompleted(bytes); break;  // 수신 완료 처리
    case IoType::Send: OnSendCompleted(bytes); break;  // 송신 완료 처리
    default: break;
    }
}

void Session::Close() {
    bool expected = false;

    if (!closing_.compare_exchange_strong(expected, true)) return; // 이미 종료 중이면 무시
    SocketUtils::CloseSocket(sock_); 
}

void Session::OnRecvCompleted(size_t bytes) {
    if (bytes == 0) {
        SocketUtils::CloseSocket(sock_);
        Close();
        return;
    }

    rx_.used() = bytes; // 실제 받은 크기 기록
    rbuf_.Write(rx_.cspan()); // 링버퍼에 누적
    rx_.reset(); // 임시 버퍼 반납(재사용 가능)
    ParseLoop(); // 패킷 파싱 시도
    PostRecv(); // 다음 수신 게시
}

void Session::OnSendCompleted(size_t /*bytes*/) {
    std::lock_guard lk(sendMtx_);   // 큐 보호
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

   /*PacketFramer 파싱 시도 함수 추가*/

}