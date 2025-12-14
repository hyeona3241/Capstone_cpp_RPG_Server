#pragma once

#include <string>
#include <cstdint>

#include "AccountRepository.h"

class AuthService
{
public:
    explicit AuthService(AccountRepository& repo)
        : repo_(repo)
    {
    }

    enum class RegisterResult
    {
        Success,
        DuplicateLoginId,
        DbError
    };

    enum class LoginResult
    {
        Success,
        NoSuchId,
        Banned,     // status != 0
        DbError
    };

    struct LoginLookupResult
    {
        LoginResult result = LoginResult::DbError;
        uint64_t accountId = 0;
        std::string pwHash;
        std::string pwSalt;
        uint8_t status = 0;
    };

    // 회원가입: plainPassword를 받아서 salt/hash 만들고 DB에 저장
    RegisterResult Register(const std::string& loginId, const std::string& plainPassword, uint64_t& outAccountId);

    // 로그인: loginId로 계정 조회만 수행 (비번 검증은 LoginServer가 수행)
    LoginLookupResult Login(const std::string& loginId);

    // 마지막 로그인 시간 업데이트
    bool UpdateLastLogin(uint64_t accountId);

    void SetConnection(DBConnection* conn) { repo_.SetConnection(conn); }


private:
    AccountRepository& repo_;
};

