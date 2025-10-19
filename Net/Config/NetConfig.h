#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include "Core/Const.h" 

namespace NetConfig {

    // 1) ���ε� ����
    inline constexpr std::string_view kBindIp = "0.0.0.0";
    inline constexpr uint16_t kBindPort = Const::kMainServerPort;

    // 2) IO/������
    inline constexpr int kIocpWorkers = Const::kIoThreadsDefault;
    inline constexpr int kListenBacklog = 128; 

    // 3) ���� ��å (���Ǵ� ����/1ȸ Receive ����)
    inline constexpr std::size_t kRecvRingSize = Const::kRecvBufferSize;
    inline constexpr std::size_t kRecvBlockSize = 4096;

    // 4) ����/�뷮 ����(������ Common �� ����)
    inline constexpr int         kMaxSessions = Const::kMaxClientsPerWorld; 
    inline constexpr std::size_t kMaxPacketSize = Const::kMaxPacketSize;
}