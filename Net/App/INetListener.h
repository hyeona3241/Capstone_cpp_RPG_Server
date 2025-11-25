#pragma once
#include <string_view>
#include <cstdint>

class Session;

struct ListenerSessionInfo {
    uint64_t     uid;           // 선택: 세션 고유 ID
    const char* remoteIp;      // "x.x.x.x" or "[::1]"
    uint16_t     remotePort;
    const char* localIp;       // 리슨 바인드 IP
    uint16_t     localPort;
};

struct ListenerErrorInfo {
    int          sysErr;        // WSAGetLastError() 등
    const char* where;         // "AcceptEx", "WSARecv", "OnClose" 등
};

class INetListener
{
public:
    virtual ~INetListener() = default;

    // 서버 라이프사이클 이벤트
    virtual void OnServerStarting() noexcept {}
    virtual void OnServerStarted()  noexcept {}
    virtual void OnServerStopping() noexcept {}
    virtual void OnServerStopped()  noexcept {}

    // 세션 라이프사이클
    virtual void OnSessionOpened(Session* s, const ListenerSessionInfo& info) noexcept = 0;
    virtual void OnSessionClosed(uint64_t uid) noexcept = 0;

    // 예외/오류 보고(정책 결정은 상위로 위임)
    virtual void OnServerError(const ListenerErrorInfo& err) noexcept = 0;
};