//#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#pragma comment(lib, "Ws2_32.lib")
//
//#include <iostream>
//#include <vector>
//
//#include "PacketDef.h"
//#include "PacketIds.h"
//
//#include "CPingPackets.h"
//#include "CLoginPackets.h"
//
//using std::byte;
//
//// --------------------------------------------------
//// TCP util
//// --------------------------------------------------
//static bool SendAll(SOCKET s, const byte* data, int len)
//{
//    int sent = 0;
//    while (sent < len)
//    {
//        int n = send(s,
//            reinterpret_cast<const char*>(data + sent),
//            len - sent,
//            0);
//        if (n <= 0)
//            return false;
//        sent += n;
//    }
//    return true;
//}
//
//static const char* ToLoginResultStr(uint32_t code)
//{
//    switch (static_cast<ELoginResult>(code))
//    {
//    case ELoginResult::OK:               return "OK";
//    case ELoginResult::NO_SUCH_USER:     return "NO_SUCH_USER";
//    case ELoginResult::INVALID_PASSWORD: return "INVALID_PASSWORD";
//    case ELoginResult::BANNED:           return "BANNED";
//    case ELoginResult::DB_ERROR:         return "DB_ERROR";
//    case ELoginResult::INTERNAL_ERROR:   return "INTERNAL_ERROR";
//    default:                             return "UNKNOWN";
//    }
//}
//
//// --------------------------------------------------
//// main
//// --------------------------------------------------
//int main()
//{
//    WSADATA wsa{};
//    WSAStartup(MAKEWORD(2, 2), &wsa);
//
//    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//
//    sockaddr_in addr{};
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(3500);            // MainServer 포트
//    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
//
//    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
//    {
//        std::printf("[ERROR] connect failed\n");
//        return 1;
//    }
//
//    std::printf("[CLIENT] Connected to MainServer\n");
//
//    // --------------------------------------------------
//    // 1. PING_REQ
//    // --------------------------------------------------
//    {
//        CPingReq req;
//        req.seq = 1;
//
//        auto bytes = req.Build();
//        SendAll(sock, bytes.data(), static_cast<int>(bytes.size()));
//
//        std::printf("[CLIENT] C_PING_REQ sent\n");
//    }
//
//    std::vector<byte> recvBuf;
//    recvBuf.reserve(8192);
//
//    char temp[4096];
//
//    bool pingDone = false;
//    bool loginSent = false;
//    bool done = false;
//
//    while (!done)
//    {
//        int n = recv(sock, temp, sizeof(temp), 0);
//        if (n <= 0)
//            break;
//
//        recvBuf.insert(
//            recvBuf.end(),
//            reinterpret_cast<byte*>(temp),
//            reinterpret_cast<byte*>(temp + n)
//        );
//
//        // --------------------------------------------------
//        // 프레이밍 (서버 핸들러와 동일)
//        // --------------------------------------------------
//        while (recvBuf.size() >= Proto::kHeaderSize)
//        {
//            const auto* header =
//                reinterpret_cast<const Proto::PacketHeader*>(recvBuf.data());
//
//            if (!Proto::IsValidPacketSize(header->size))
//            {
//                std::printf("[CLIENT][ERROR] invalid packet size=%u\n", header->size);
//                return 1;
//            }
//
//            if (recvBuf.size() < header->size)
//                break;
//
//            const byte* payload = recvBuf.data() + Proto::kHeaderSize;
//            const size_t payloadLen = header->size - Proto::kHeaderSize;
//
//            // --------------------------------------------------
//            // 패킷 분기 (DBPacketHandler 스타일)
//            // --------------------------------------------------
//            switch (header->id)
//            {
//            case PacketType::to_id(PacketType::Client::C_PING_ACK):
//            {
//                CPingAck ack;
//                if (!ack.ParsePayload(payload, payloadLen))
//                {
//                    std::printf("[CLIENT][WARN] PING_ACK parse failed\n");
//                    break;
//                }
//
//                std::printf("[CLIENT] C_PING_ACK seq=%u tick=%llu\n",
//                    ack.seq,
//                    static_cast<unsigned long long>(ack.serverTick));
//
//                pingDone = true;
//                break;
//            }
//
//            case PacketType::to_id(PacketType::Client::LOGIN_ACK):
//            {
//                LoginAck ack;
//                if (!ack.ParsePayload(payload, payloadLen))
//                {
//                    std::printf("[CLIENT][WARN] LOGIN_ACK parse failed\n");
//                    break;
//                }
//
//                std::printf(" resultCode : %u (%s)\n", ack.resultCode, ToLoginResultStr(ack.resultCode));
//                if (ack.success)
//                {
//                    std::printf(" accountId  : %llu\n", (unsigned long long)ack.accountId);
//                }
//
//                break;
//            }
//
//            default:
//                std::printf("[CLIENT][WARN] Unknown packet id=%u\n", header->id);
//                break;
//            }
//
//            // consume packet
//            recvBuf.erase(
//                recvBuf.begin(),
//                recvBuf.begin() + header->size
//            );
//        }
//
//        // --------------------------------------------------
//        // 2. LOGIN_REQ (PING 완료 후 1회)
//        // --------------------------------------------------
//        if (pingDone && !loginSent)
//        {
//            LoginReq req;
//            std::cout << "loginId: ";
//            std::getline(std::cin, req.loginId);
//            std::cout << "password: ";
//            std::getline(std::cin, req.plainPw);
//
//            auto bytes = req.Build();
//            SendAll(sock, bytes.data(), static_cast<int>(bytes.size()));
//
//            std::printf("[CLIENT] LOGIN_REQ sent\n");
//            loginSent = true;
//        }
//    }
//
//    closesocket(sock);
//    WSACleanup();
//
//    std::printf("[CLIENT] Exit\n");
//    return 0;
//}
