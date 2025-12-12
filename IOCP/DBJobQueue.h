#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>

class DBJob;

class DBJobQueue
{
public:
    using JobPtr = std::unique_ptr<DBJob>;

    DBJobQueue() = default;
    ~DBJobQueue() = default;

    DBJobQueue(const DBJobQueue&) = delete;
    DBJobQueue& operator=(const DBJobQueue&) = delete;

    // 잡을 넣음 (IOCP 워커에서 호출)
    void Push(JobPtr job);

    // 잡을 꺼냄 (DBWorker에서 호출)
    JobPtr PopBlocking();

    // 서버 종료 시 호출
    void Stop();

    // 통계/디버깅용
    std::size_t Size() const;

private:
    mutable std::mutex m_;

    std::condition_variable cv_;
    std::queue<JobPtr> q_;

    bool stopping_ = false;
};

