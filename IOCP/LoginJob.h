#pragma once
#include "DBJob.h"

#include <string>
#include <utility>

class DBServer;
struct DbServices;

class LoginJob : public DBJob
{
public:
    LoginJob(std::uint64_t replySessionId, std::string loginId, std::string plainPassword)
        : DBJob(replySessionId), loginId_(std::move(loginId)), plainPassword_(std::move(plainPassword))
    {
    }

    void Execute(DbServices& services, DBServer& server) override;

private:
    std::string loginId_;
    std::string plainPassword_;
};

