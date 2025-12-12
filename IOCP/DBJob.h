#pragma once
#include <cstdint>

class AuthService;
class DBServer;


struct DbServices
{
    AuthService& auth;
};

class DBJob
{
public:
    explicit DBJob(std::uint64_t replySessionId)
        : replySessionId_(replySessionId)
    {
    }

    virtual ~DBJob() = default;

    // DB 전용 워커 스레드에서 호출됨
    virtual void Execute(DbServices& services, DBServer& server) = 0;

    // 결과를 돌려보낼 MainServer 세션 식별자
    std::uint64_t ReplySessionId() const { return replySessionId_; }

protected:
    std::uint64_t replySessionId_;
};