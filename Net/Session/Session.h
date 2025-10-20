#pragma once
#include <winsock2.h>
#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <span>
#include <cstddef>
#include "Config/NetConfig.h"
#include "Util/BufferPool.h"
#include "RecvRing.h"
#include "Core/IocpCommon.h"

// �� Ŭ���̾�Ʈ ������ ����ϴ� ����

class Session {
private:
    // ����
    SOCKET sock_ = INVALID_SOCKET; 
    // �ӽ� ���� ���� Ǯ
    Util::BufferPool& pool_;
    // ���� �ӽ� ����
    Util::BufferHandle rx_;
    // ���� ���� ������
    RecvRing rbuf_;

    // �۽� ť ��ȣ
    std::mutex sendMtx_;
    // �۽� ��� ť
    std::deque<std::vector<std::byte>> sendQ_;
    // ���� �۽� ��?
    std::atomic<bool> sending_{ false };
    // ���� ó�� ��?
    std::atomic<bool> closing_{ false };

    // ���� ID
    uint64_t id_{ 0 }; 

public:
    explicit Session(Util::BufferPool& pool)
        : pool_(pool), rbuf_(NetConfig::kRecvRingSize) {
    }

    // ���� ����
    void Start();

    // �񵿱� ����
    bool PostRecv();

    // �񵿱� �۽�
    bool Send(std::span<const std::byte> payload);

    // IOCP �Ϸ� �ݹ� (��Ŀ �����忡�� ���)
    void OnIoCompleted(OverlappedEx* ov, DWORD bytes, BOOL ok);

    // ����
    void Close();

    // �ĺ���/���� ����/��ȸ��
    void set_id(uint64_t v) { id_ = v; } 
    uint64_t id() const { return id_; }
    void set_socket(SOCKET s) { sock_ = s; }
    SOCKET socket() const { return sock_; }

private:
    // ���� �Ϸ� ó��
    void OnRecvCompleted(size_t bytes);

    // �۽� �Ϸ� ó��
    void OnSendCompleted(size_t bytes);

    // �����ۿ��� ��Ŷ���� ������ ��ŭ ������
    void ParseLoop();
};