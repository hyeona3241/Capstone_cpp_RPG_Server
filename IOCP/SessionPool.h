#pragma once
#include <vector>
#include <stack>
#include <mutex>
#include <atomic>
#include "Session.h"

class SessionPool
{
public:
    explicit SessionPool(std::size_t maxSessions)
        : sessions_(maxSessions)
    {
        for (std::size_t i = 0; i < maxSessions; ++i)
        {
            freeIndices_.push(i);
        }
    }

    // 세션 발급
    Session* Acquire(SOCKET s, IocpServerBase* owner, SessionRole role)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (freeIndices_.empty())
        {
            return nullptr;
        }

        std::size_t index = freeIndices_.top();
        freeIndices_.pop();

        Session& session = sessions_[index];

        // 고유 ID 발급
        uint64_t id = nextId_.fetch_add(1, std::memory_order_relaxed);

        session.Init(s, owner, role, id);

        return &session;
    }

    // 세션 반환
    void Release(Session* session)
    {
        if (!session)
            return;

        session->Disconnect();
        session->ResetForReuse();

        std::lock_guard<std::mutex> lock(mutex_);

        std::size_t index = static_cast<std::size_t>(session - sessions_.data());
        if (index >= sessions_.size())
            return;

        freeIndices_.push(index);
    }

    Session* GetById(uint64_t id)
    {
        if (id == 0)
            return nullptr;

        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& s : sessions_)
        {
            if (s.GetId() == id)
                return &s;
        }
        return nullptr;
    }

    std::size_t Capacity() const
    {
        return sessions_.size();
    }

    std::size_t FreeCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return freeIndices_.size();
    }

private:
    std::vector<Session> sessions_;
    std::stack<std::size_t> freeIndices_;
    mutable std::mutex mutex_;
    std::atomic<uint64_t> nextId_{ 1 };
};

