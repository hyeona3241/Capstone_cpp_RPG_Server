#pragma once
#include "ThirdParty\mysql\include\mysql.h"
#include <string>
#include <mutex>

class DBConnection
{
public:
    DBConnection();
    ~DBConnection();

    // DB 연결 시도
    bool Connect(const std::string& host, unsigned int port, const std::string& user, const std::string& password,
        const std::string& schema, const std::string& charset = "utf8mb4");

    void Disconnect();

    // raw MYSQL 핸들 반환
    MYSQL* GetHandle() const { return conn_; }

    // 단일 커넥션에서 다중 스레드가 접근할 경우 사용
    std::mutex& GetMutex() { return mutex_; }

    // 쿼리 실행 헬퍼 (SELECT 전용)
    bool ExecuteQuery(const std::string& query, MYSQL_RES** outResult);

    // INSERT / UPDATE / DELETE 같은 Non-Query
    bool ExecuteUpdate(const std::string& query, long long& affectedRows);

private:
    MYSQL* conn_ = nullptr;
    std::mutex mutex_;
};

