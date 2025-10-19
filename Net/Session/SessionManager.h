#pragma once
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <winsock2.h>
#include <span>
#include "SessionPool.h"


// Ȱ�� ����(����� Ŭ���̾�Ʈ)�� ����
// ������ ���/����/�˻�/��ε�ĳ��Ʈ

class SessionManager
{
private:
    mutable std::mutex mtx_; // ���� ��� ��ȣ�� ���ؽ�
    std::unordered_map<uint64_t, Session*> sessions_; // Ȱ�� ���� ��� (ID -> Session)
    std::unordered_map<SOCKET, uint64_t> bySock_;  // �������� ���� ã��� (SOCKET -> ID)
    SessionPool pool_; // ���� ��ü ���� Ǯ
    std::atomic<uint64_t> created_{ 0 }; // ������ ���� ��
    std::atomic<uint64_t> destroyed_{ 0 }; // ������ ���� ��

public:
    explicit SessionManager(size_t maxSession);

    // ���� ����(�� Ŭ�� �������� �� ȣ��)
    Session* CreateSession(SOCKET sock);

    // ���� ����(Ŭ�� �������� �� ȣ��)
    void DestroySession(uint64_t id);

    // ���� �˻� (ID�� Session ��ü ������ã��)
    Session* Find(uint64_t id) const;

    // ���� �˻� (�������� Session ã��)
    Session* FindBySocket(SOCKET sock) const;

    // Ȱ�� ���� ��ü�� ��ȸ�ϸ� �־��� �Լ��� ����
    template<typename F>
    void ForEachActive(F&& fn);

    // ��ε�ĳ��Ʈ
    void Broadcast(std::span<const std::byte> payload);

    // ���� ����� ������ ����
    size_t ActiveCount() const;

    // ���ݱ��� ������ ���� �� (������)
    uint64_t TotalCreated() const { return created_.load(); }

    // ���ݱ��� ������ ���� �� (������)
    uint64_t TotalDestroyed() const { return destroyed_.load(); }

    // ���� SessionPool ����( ������)
    SessionPool& Pool() { return pool_; }
};

