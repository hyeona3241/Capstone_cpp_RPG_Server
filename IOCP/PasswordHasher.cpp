#include "PasswordHasher.h"
#include "SHA256.h"
#include <random>

std::string PasswordHasher::GenerateSalt()
{
    std::string salt;
    salt.resize(16);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint16_t> dist(0, 255);

    for (int i = 0; i < 16; i++)
        salt[i] = static_cast<unsigned char>(dist(gen));

    return salt;
}

std::string PasswordHasher::HashPassword(const std::string& password, const std::string& salt)
{
    SHA256 sha;
    return sha.CalculateHex(salt + password);
}

bool PasswordHasher::VerifyPassword(const std::string& inputPassword, const std::string& salt, const std::string& storedHash)
{
    std::string newHash = HashPassword(inputPassword, salt);
    return (newHash == storedHash);
}