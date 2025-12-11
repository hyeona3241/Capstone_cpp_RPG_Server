#include "DBConnection.h"
#include <iostream>

DBConnection::DBConnection()
{
    conn_ = mysql_init(nullptr);
    if (conn_ == nullptr)
    {
        std::cerr << "[DbConnection] mysql_init() failed\n";
    }
}

DBConnection::~DBConnection()
{
    Disconnect();
}

bool DBConnection::Connect(const std::string& host, unsigned int port, const std::string& user, const std::string& password,
    const std::string& schema, const std::string& charset)
{
    if (conn_ == nullptr)
        return false;

    // charset 설정
    if (mysql_options(conn_, MYSQL_SET_CHARSET_NAME, charset.c_str()))
    {
        std::cerr << "[DbConnection] Failed to set charset: " << mysql_error(conn_) << "\n";
        return false;
    }

    // 실제 연결 시도
    if (!mysql_real_connect(
        conn_,
        host.c_str(),
        user.c_str(),
        password.c_str(),
        schema.c_str(),
        port,
        nullptr,
        0))
    {
        std::cerr << "[DbConnection] mysql_real_connect failed: " << mysql_error(conn_) << "\n";
        return false;
    }

    std::cout << "[DbConnection] Connected to MySQL: " << host << ":" << port << "\n";

    return true;
}

void DBConnection::Disconnect()
{
    if (conn_ != nullptr)
    {
        mysql_close(conn_);
        conn_ = nullptr;
    }
}

bool DBConnection::ExecuteQuery(const std::string& query, MYSQL_RES** outResult)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (mysql_query(conn_, query.c_str()))
    {
        std::cerr << "[DbConnection] Query failed: " << mysql_error(conn_) << "\n";
        return false;
    }

    *outResult = mysql_store_result(conn_);
    if (*outResult == nullptr)
    {
        std::cerr << "[DbConnection] Failed to fetch result: " << mysql_error(conn_) << "\n";
        return false;
    }

    return true;
}

bool DBConnection::ExecuteUpdate(const std::string& query, long long& affectedRows)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (mysql_query(conn_, query.c_str()))
    {
        std::cerr << "[DbConnection] Update failed: " << mysql_error(conn_) << "\n";
        return false;
    }

    affectedRows = mysql_affected_rows(conn_);
    return true;
}