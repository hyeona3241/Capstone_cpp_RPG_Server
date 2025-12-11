#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>

// 링커 설정이 귀찮으면 이 두 줄로 강제 링크
#pragma comment(lib, "Ws2_32.lib")

int main()
{
    // 1. WinSock 초기화
    WSADATA wsaData;
    int wsaRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaRet != 0)
    {
        std::cout << "[ERROR] WSAStartup 실패. code=" << wsaRet << "\n";
        return 1;
    }

    // 2. 소켓 생성 (TCP)
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        std::cout << "[ERROR] socket 생성 실패. WSAGetLastError=" << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // 3. 서버 주소 설정
    //    메인 서버가 127.0.0.1:3500 에서 돌아간다고 가정
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(3500);               // 포트 맞춰서 수정 가능
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 4. 서버에 연결
    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "[ERROR] 서버 연결 실패. WSAGetLastError=" << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "[INFO] 서버에 연결 성공! (127.0.0.1:3500)\n";
    std::cout << "[INFO] 문자열을 입력하면 서버로 전송합니다. 'q' 입력 시 종료.\n";

    // 5. 송신/수신 루프
    char recvBuf[4096];

    while (true)
    {
        std::string msg;
        std::cout << "> ";
        if (!std::getline(std::cin, msg))
            break;

        if (msg == "q" || msg == "Q")
            break;

        // 빈 문자열은 그냥 스킵
        if (msg.empty())
            continue;

        // 서버로 메시지 전송
        int sent = send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
        if (sent == SOCKET_ERROR)
        {
            std::cout << "[ERROR] send 실패. WSAGetLastError=" << WSAGetLastError() << "\n";
            break;
        }

        // 서버에서 에코 응답을 받는다고 가정
        int received = recv(sock, recvBuf, sizeof(recvBuf) - 1, 0);
        if (received == 0)
        {
            std::cout << "[INFO] 서버에서 연결을 종료했습니다.\n";
            break;
        }
        else if (received == SOCKET_ERROR)
        {
            std::cout << "[ERROR] recv 실패. WSAGetLastError=" << WSAGetLastError() << "\n";
            break;
        }

        recvBuf[received] = '\0';
        std::cout << "[ECHO] " << recvBuf << "\n";
    }

    // 6. 정리
    closesocket(sock);
    WSACleanup();

    std::cout << "[INFO] 클라이언트 종료.\n";
    return 0;
}
