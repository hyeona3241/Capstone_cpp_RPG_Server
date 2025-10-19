#pragma once
#include <winsock2.h>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>

// IOCP �����Ҷ� OverlappedEx�� ��ü
struct Session {
    SOCKET sock = INVALID_SOCKET;            // Ŭ���̾�Ʈ ����
    std::queue<std::vector<char>> sendQueue; // �۽� ��� ť
    std::mutex sendMtx;                      // �۽� ť ��ȣ�� ���ؽ�
    std::atomic<bool> sending{ false }; // ���� �۽� ������
    std::atomic<bool> closing{ false }; // ���� ó�� ������
    uint64_t id = 0;                    // ���� ���� ID
    std::atomic<bool> inUse{ false };   // Ǯ���� ��� ������

    // Ǯ���� ������ �� �⺻ ���·� �ǵ����� �� ���
    void ResetForReuse() {
        sock = INVALID_SOCKET;
        sending = false;
        closing = false;

        // ť ����
        while (!sendQueue.empty()) sendQueue.pop();
    }
};