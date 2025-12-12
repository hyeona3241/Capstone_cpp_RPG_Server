#pragma once

#include <atomic>
#include <thread>

class DBJobQueue;
class DBServer;

struct DbServices;

class DBWorker
{
public:
    DBWorker(DBJobQueue& queue, DbServices& services, DBServer& server);
    ~DBWorker();

    DBWorker(const DBWorker&) = delete;
    DBWorker& operator=(const DBWorker&) = delete;

    void Start();

    void Stop();

    bool IsRunning() const { return running_.load(); }

private:
    void RunLoop();

private:
    DBJobQueue& queue_;
    DbServices& services_;
    DBServer& server_;

    std::atomic<bool> running_{ false };
    // 나중에 보고 벡터로 바꿔서 워커 개수 늘릴수도
    std::thread thread_;
};

