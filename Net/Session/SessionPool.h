#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include "Session.h"

class SessionPool
{
private:
    // ��ü Session ������ ���
    std::vector<Session*> sessions_; 
    // ���� �� �ִ� Session ť
    std::queue<Session*> freeList_; 
    // freeList/sessions ���� ��ȣ
    mutable std::mutex mtx_;
    // ���� ID �߱ޱ�(1���� ����)
    std::atomic<uint64_t> idGen_{ 1 }; 

public:
    explicit SessionPool(size_t maxSession);
    ~SessionPool();

    // ��� ������ Session ��ȯ (������ nullptr)
    Session* Allocate();

    // ����� ��ģ Session�� �ݳ�
    void Release(Session* s);

    // ���� ��� ���� ���ǵ��� �ݳ�
    std::vector<Session*> GetActiveSessions();

    // ��ü ���� �� ��ȯ
    size_t Capacity() const noexcept { return sessions_.size(); }

    // ���� �ִ� �� ���� �˷���
    size_t FreeCount() const;
};

