#include "DBManager.h"
#include <iostream>

int main()
{
    DBConfig cfg;
    cfg.host = "127.0.0.1";

    DBManager dbManager;
    if (!dbManager.Initialize(cfg))
    {
        std::cerr << "DB Init failed\n";
        return 1;
    }

    DBConnection* conn = dbManager.GetConnection();
    MYSQL_RES* res = nullptr;
    std::string query = "SELECT account_id, login_id FROM account;";

    if (conn->ExecuteQuery(query, &res))
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            std::cout << "account_id=" << row[0]
                << ", login_id=" << row[1] << "\n";
        }
        mysql_free_result(res);
    }

    // SELECT 테스트
    if (conn->ExecuteQuery("SELECT account_id, login_id FROM account;", &res))
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)))
        {
            std::cout << "account_id=" << row[0]
                << ", login_id=" << row[1] << "\n";
        }
        mysql_free_result(res);
    }

    // INSERT 테스트
    std::string loginId = "testUser";
    std::string pwHash = "abcdef123456";  // 실제로는 SHA256
    std::string salt = "randomSalt";

    query = "INSERT INTO account (login_id, pw_hash, pw_salt) VALUES ('" +
        loginId + "', '" + pwHash + "', '" + salt + "');";

    long long affected = 0;
    if (conn->ExecuteUpdate(query, affected))
    {
        std::cout << "INSERT OK, affected rows = " << affected << "\n";
    }
    else
    {
        std::cout << "INSERT FAILED\n";
    }

    dbManager.Finalize();
    return 0;
}
