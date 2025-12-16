#pragma once
#include <vector>
#include <stack>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include "Session.h"

class IocpServerBase;

class SessionPool
{
public:
    using Factory = std::function<std::unique_ptr<Session>()>;

    SessionPool(std::size_t maxSessions, Factory factory)
        : sessions_(maxSessions), factory_(std::move(factory))
    {
        for (std::size_t i = 0; i < maxSessions; ++i)
        {
            sessions_[i] = factory_();
            freeIndices_.push(i);
        }
    }

    // 세션 발급
    Session* Acquire(SOCKET s, IocpServerBase* owner, SessionRole role)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (freeIndices_.empty())
            return nullptr;

        std::size_t index = freeIndices_.top();
        freeIndices_.pop();

        Session* session = sessions_[index].get();

        uint64_t id = nextId_.fetch_add(1, std::memory_order_relaxed);
        session->Init(s, owner, role, id);

        return session;
    }

    // 세션 반환
    void Release(Session* session)
    {
        if (!session) return;

        session->Disconnect();
        session->ResetForReuse();

        std::lock_guard<std::mutex> lock(mutex_);

        // 포인터가 어느 인덱스인지 찾아서 반환 (O(N)이라도 maxSessions 크지 않으면 OK)
        for (std::size_t i = 0; i < sessions_.size(); ++i)
        {
            if (sessions_[i].get() == session)
            {
                freeIndices_.push(i);
                break;
            }
        }
    }

    std::size_t Capacity() const { return sessions_.size(); }

    std::size_t FreeCount() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return freeIndices_.size();
    }

    Session* GetById(uint64_t id)
    {
        if (id == 0)
            return nullptr;

        std::lock_guard<std::mutex> lock(mutex_);

        for (auto& uptr : sessions_)
        {
            Session* s = uptr.get();
            if (!s)
                continue;

            if (s->GetId() == id)
                return s;
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
    std::vector<std::unique_ptr<Session>> sessions_;
    std::stack<std::size_t> freeIndices_;
    mutable std::mutex mutex_;
    std::atomic<uint64_t> nextId_{ 1 };
    Factory factory_;
};

