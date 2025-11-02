#pragma once
#include "INetListener.h"
#include "Core/Logger.h"

class DefaultNetListener final : public INetListener
{
public:
    void OnServerStarting() noexcept override { LOG_INFO("server starting"); }
    void OnServerStarted()  noexcept override { LOG_INFO("server started"); }
    void OnServerStopping() noexcept override { LOG_INFO("server stopping"); }
    void OnServerStopped()  noexcept override { LOG_INFO("server stopped"); }

    void OnSessionOpened(Session* s, const ListenerSessionInfo& info) noexcept override {
        LOG_INFO("session open uid={} {}:{} -> {}:{}",
            info.uid, info.remoteIp, info.remotePort, info.localIp, info.localPort);
        (void)s;
    }

    void OnSessionClosed(uint64_t uid) noexcept override {
        LOG_INFO("session close uid={}", uid);
    }

    void OnServerError(const ListenerErrorInfo& err) noexcept override {
        LOG_ERROR("server error where={}, sysErr={}", err.where, err.sysErr);
    }
};