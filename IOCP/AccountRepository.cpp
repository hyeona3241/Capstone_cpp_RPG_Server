#include "AccountRepository.h"
#include "DBConnection.h"

#include "ThirdParty\mysql\include\mysql.h"
#include <iostream>
#include <sstream>

AccountRepository::AccountRepository(DBConnection* conn)
    : conn_(conn)
{
}

// 지금은 그냥 원본을 돌려주지만, 나중에 mysql_real_escape_string을 이용해서 안전하게 만들 예정
std::string AccountRepository::Escape(const std::string& value)
{
    // TODO: implement proper escaping using mysql_real_escape_string
    return value;
}

bool AccountRepository::IsLoginIdExists(const std::string& loginId)
{
    if (!conn_)
        return false;

    std::ostringstream oss;
    oss << "SELECT COUNT(*) FROM account WHERE login_id='" << Escape(loginId) << "';";

    MYSQL_RES* res = nullptr;
    if (!conn_->ExecuteQuery(oss.str(), &res))
        return false;

    bool exists = false;
    if (MYSQL_ROW row = mysql_fetch_row(res))
    {
        if (row[0])
        {
            long long count = std::atoll(row[0]);
            exists = (count > 0);
        }
    }

    mysql_free_result(res);
    return exists;
}

AccountRepository::CreateResult
AccountRepository::CreateAccount(const std::string& loginId, const std::string& pwHash, const std::string& pwSalt, uint64_t& outAccountId)
{
    outAccountId = 0;

    if (!conn_)
        return CreateResult::DbError;

    // 먼저 ID 중복 체크
    if (IsLoginIdExists(loginId))
        return CreateResult::DuplicateLoginId;

    std::ostringstream oss;
    oss << "INSERT INTO account (login_id, pw_hash, pw_salt) VALUES ('"  << Escape(loginId) << "', '"  << Escape(pwHash) << "', '"  << Escape(pwSalt) << "');";

    long long affected = 0;
    if (!conn_->ExecuteUpdate(oss.str(), affected))
        return CreateResult::DbError;

    if (affected <= 0)
        return CreateResult::DbError;

    // 마지막 INSERT된 auto_increment 값 가져오기
    MYSQL* handle = conn_->GetHandle();
    if (handle)
    {
        outAccountId = static_cast<uint64_t>(mysql_insert_id(handle));
    }

    return CreateResult::Success;
}

bool AccountRepository::FindByLoginId(const std::string& loginId, AccountRecord& outAccount)
{
    if (!conn_)
        return false;

    std::ostringstream oss;
    oss << "SELECT account_id, login_id, pw_hash, pw_salt, status, "
        "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s'), "
        "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s'), "
        "IFNULL(DATE_FORMAT(last_login_at, '%Y-%m-%d %H:%i:%s'), '') "
        "FROM account "
        "WHERE login_id='"
        << Escape(loginId) << "' "
        "LIMIT 1;";

    MYSQL_RES* res = nullptr;
    if (!conn_->ExecuteQuery(oss.str(), &res))
        return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    // 컬럼 순서에 맞춰서 파싱
    outAccount.account_id = row[0] ? std::strtoull(row[0], nullptr, 10) : 0;
    outAccount.login_id = row[1] ? row[1] : "";
    outAccount.pw_hash = row[2] ? row[2] : "";
    outAccount.pw_salt = row[3] ? row[3] : "";
    outAccount.status = row[4] ? static_cast<uint8_t>(std::atoi(row[4])) : 0;
    outAccount.created_at = row[5] ? row[5] : "";
    outAccount.updated_at = row[6] ? row[6] : "";
    outAccount.lastLogin_at = row[7] ? row[7] : "";

    mysql_free_result(res);
    return true;
}

bool AccountRepository::UpdateLastLogin(uint64_t accountId)
{
    if (!conn_)
        return false;

    std::ostringstream oss;
    oss << "UPDATE account SET last_login_at = NOW(), updated_at = NOW() " << "WHERE account_id = " << accountId << ";";

    long long affected = 0;
    if (!conn_->ExecuteUpdate(oss.str(), affected))
        return false;

    return affected > 0;
}

bool AccountRepository::UpdatePassword(uint64_t accountId, const std::string& newPwHash, const std::string& newPwSalt)
{
    if (!conn_)
        return false;

    std::ostringstream oss;
    oss << "UPDATE account SET pw_hash='" << Escape(newPwHash)  << "', pw_salt='" << Escape(newPwSalt)  << "', updated_at = NOW() "  << "WHERE account_id=" << accountId << ";";

    long long affected = 0;
    if (!conn_->ExecuteUpdate(oss.str(), affected))
        return false;

    return affected > 0;
}

bool AccountRepository::UpdateStatus(uint64_t accountId, uint8_t newStatus)
{
    if (!conn_)
        return false;

    std::ostringstream oss;
    oss << "UPDATE account SET status=" << static_cast<int>(newStatus) << ", updated_at = NOW() " << "WHERE account_id=" << accountId << ";";

    long long affected = 0;
    if (!conn_->ExecuteUpdate(oss.str(), affected))
        return false;

    return affected > 0;
}