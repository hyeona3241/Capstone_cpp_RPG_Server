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
        WrongPassword,
        Banned,     // status != 0
        DbError
    };

    // 회원가입: plainPassword를 받아서 salt/hash 만들고 DB에 저장
    RegisterResult Register(const std::string& loginId, const std::string& plainPassword, uint64_t& outAccountId);

    // 로그인: loginId로 계정 조회 -> salt로 재해싱 비교 -> 성공 시 last_login_at 갱신
    LoginResult Login(const std::string& loginId, const std::string& plainPassword, AccountRecord& outAccount);

private:
    AccountRepository& repo_;
};

