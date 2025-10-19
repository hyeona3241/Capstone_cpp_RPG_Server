#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include "Core/Const.h" 

namespace NetConfig {

    // 1) 바인딩 정보
    inline constexpr std::string_view kBindIp = "0.0.0.0";
    inline constexpr uint16_t kBindPort = Const::kMainServerPort;

    // 2) IO/스레딩
    inline constexpr int kIocpWorkers = Const::kIoThreadsDefault;
    inline constexpr int kListenBacklog = 128; 

    // 3) 버퍼 정책 (세션당 누적/1회 Receive 버퍼)
    inline constexpr std::size_t kRecvRingSize = Const::kRecvBufferSize;
    inline constexpr std::size_t kRecvBlockSize = 4096;

    // 4) 세션/용량 상한(가급적 Common 값 재사용)
    inline constexpr int         kMaxSessions = Const::kMaxClientsPerWorld; 
    inline constexpr std::size_t kMaxPacketSize = Const::kMaxPacketSize;
}