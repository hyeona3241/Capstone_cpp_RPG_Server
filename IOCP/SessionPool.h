#pragma once
#include <vector>
#include <stack>
#include <mutex>
#include <atomic>
#include <memory>
#include <type_traits>

#include "Session.h"

class IocpServerBase;

template <typename TSession>
class SessionPool
{
    static_assert(std::is_base_of_v<Session, TSession>, "TSession must derive from Session");

public:
    explicit SessionPool(std::size_t maxSessions)
        : sessions_(maxSessions)
    {
        for (std::size_t i = 0; i < maxSessions; ++i)
        {
            sessions_[i] = std::make_unique<TSession>();
            freeIndices_.push(i);
        }
    }

    TSession* Acquire(SOCKET s, IocpServerBase* owner, SessionRole role)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (freeIndices_.empty())
            return nullptr;

        const std::size_t index = freeIndices_.top();
        freeIndices_.pop();

        TSession* session = sessions_[index].get();

        const uint64_t id = nextId_.fetch_add(1, std::memory_order_relaxed);
        session->Init(s, owner, role, id);

        return session;
    }

    void Release(TSession* session)
    {
        if (!session)
            return;

        session->Disconnect();
        session->ResetForReuse();

        std::lock_guard<std::mutex> lock(mutex_);

        for (std::size_t i = 0; i < sessions_.size(); ++i)
        {
            if (sessions_[i].get() == session)
            {
                freeIndices_.push(i);
                break;
            }
        }
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

    TSession* GetById(uint64_t id)
    {
        if (id == 0)
            return nullptr;

        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& uptr : sessions_)
        {
            TSession* s = uptr.get();
            if (s && s->GetId() == id)
                return s;
        }

        return nullptr;
    }

private:
    std::vector<std::unique_ptr<TSession>> sessions_;
    std::stack<std::size_t> freeIndices_;
    mutable std::mutex mutex_;
    std::atomic<uint64_t> nextId_{ 1 };
};