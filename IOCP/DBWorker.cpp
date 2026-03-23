#include "DBWorker.h"
#include "DBJobQueue.h"
#include "DBJob.h"
#include "AuthService.h"
#include "DBServer.h"

#include <iostream>
#include <Logger.h>

DBWorker::DBWorker(DBJobQueue& queue, DbServices& services, DBServer& server)
    : queue_(queue), services_(services), server_(server)
{
}

DBWorker::~DBWorker()
{
    Stop();
}

void DBWorker::Start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
        return;

    thread_ = std::thread(&DBWorker::RunLoop, this);
    std::cout << "[DBWorker] Started\n";
    LOG_INFO("[DBWorker] Started");
}

void DBWorker::Stop()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false))
        return; 

    // 큐를 깨워서 PopBlocking()이 빠져나오도록 함
    queue_.Stop();

    if (thread_.joinable())
        thread_.join();

    std::cout << "[DBWorker] Stopped\n";
    LOG_INFO("[DBWorker] Stopped");
}

void DBWorker::RunLoop()
{
    while (running_.load())
    {
        auto job = queue_.PopBlocking();
        if (!job)
        {
            // Stop() 호출로 큐가 종료되면 nullptr 반환
            break;
        }

        try
        {
            job->Execute(services_, server_);
        }
        catch (const std::exception& e)
        {
            std::cout << "[DBWorker] Job exception: " << e.what() << "\n";
            LOG_INFO(std::string("[DBWorker] Job exception: ") + e.what());
        }
        catch (...)
        {
            std::cout << "[DBWorker] Job unknown exception\n";
            LOG_INFO("[DBWorker] Job unknown exception");
        }
    }
}