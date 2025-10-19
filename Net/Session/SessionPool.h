#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include "Session.h"

class SessionPool
{
private:
    // 전체 Session 포인터 목록
    std::vector<Session*> sessions_; 
    // 빌릴 수 있는 Session 큐
    std::queue<Session*> freeList_; 
    // freeList/sessions 접근 보호
    mutable std::mutex mtx_;
    // 세션 ID 발급기(1부터 증가)
    std::atomic<uint64_t> idGen_{ 1 }; 

public:
    explicit SessionPool(size_t maxSession);
    ~SessionPool();

    // 사용 가능한 Session 반환 (없으면 nullptr)
    Session* Allocate();

    // 사용을 마친 Session을 반납
    void Release(Session* s);

    // 현재 사용 중인 세션들을 반납
    std::vector<Session*> GetActiveSessions();

    // 전체 세션 수 반환
    size_t Capacity() const noexcept { return sessions_.size(); }

    // 남아 있는 빈 슬롯 알려줌
    size_t FreeCount() const;
};

