#include "AuthService.h"
#include "PasswordHasher.h"

AuthService::RegisterResult AuthService::Register(const std::string& loginId, const std::string& plainPassword, uint64_t& outAccountId)
{
    outAccountId = 0;

    // 중복 체크(Repository)
    if (repo_.IsLoginIdExists(loginId))
        return RegisterResult::DuplicateLoginId;

    // salt 생성 + hash 생성(Hasher)
    const std::string salt = PasswordHasher::GenerateSalt();
    const std::string hash = PasswordHasher::HashPassword(plainPassword, salt);

    // DB INSERT(Repository)
    const auto cr = repo_.CreateAccount(loginId, hash, salt, outAccountId);

    switch (cr)
    {
    case AccountRepository::CreateResult::Success:
        return RegisterResult::Success;
    case AccountRepository::CreateResult::DuplicateLoginId:
        return RegisterResult::DuplicateLoginId;
    default:
        return RegisterResult::DbError;
    }
}

AuthService::LoginLookupResult AuthService::Login(const std::string& loginId)
{
    LoginLookupResult r; // 기본값: 실패(네가 AuthService.h에서 기본 result 지정한 값)

    uint64_t accountId = 0;
    std::string hash;
    std::string salt;
    uint8_t status = 0;

    // DB 조회(인증용 최소 데이터)
    if (!repo_.FindByLoginId(loginId, accountId, hash, salt, status))
    {
        r.result = LoginResult::NoSuchId;
        return r;
    }

    // 상태 체크 (0: 정상, 1: 정지, 2: 삭제)
    if (status != 0)
    {
        r.result = LoginResult::Banned;

        if (status == 2) r.result = LoginResult::NoSuchId;
        return r;
    }

    r.result = LoginResult::Success;
    r.accountId = accountId;
    r.pwHash = std::move(hash);
    r.pwSalt = std::move(salt);
    r.status = status;
    return r;
}

bool AuthService::UpdateLastLogin(uint64_t accountId)
{
    return repo_.UpdateLastLogin(accountId);
}
