#include "DBJobQueue.h"
#include "DBJob.h"

void DBJobQueue::Push(JobPtr job)
{
    if (!job)
        return;

    {
        std::lock_guard<std::mutex> lock(m_);
        if (stopping_)
        {
            // 서버 종료 중이면 새 작업은 버림(또는 로그)
            return;
        }
        q_.push(std::move(job));
    }

    cv_.notify_one(); // 대기 중인 워커 1개 깨움
}

DBJobQueue::JobPtr DBJobQueue::PopBlocking()
{
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock, [this] { return stopping_ || !q_.empty(); });

    if (stopping_)
    {
        return nullptr;
    }

    JobPtr job = std::move(q_.front());
    q_.pop();
    return job;
}

void DBJobQueue::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_);
        stopping_ = true;

        // 큐에 남아있는 작업 정리
        while (!q_.empty())
            q_.pop();
    }

    cv_.notify_all(); // 대기 중인 워커 전체 깨움
}

std::size_t DBJobQueue::Size() const
{
    std::lock_guard<std::mutex> lock(m_);
    return q_.size();
}