#pragma once

#include <string>

class PasswordHasher
{
public:
    // 16바이트 랜덤 솔트 생성
    static std::string GenerateSalt();

    // 솔트 + 비밀번호를 합쳐 SHA256 hex 생성
    static std::string HashPassword(const std::string& password, const std::string& salt);

    // 로그인 시 비밀번호 검사 도우미
    static bool VerifyPassword(const std::string& inputPassword, const std::string& salt, const std::string& storedHash);
};

