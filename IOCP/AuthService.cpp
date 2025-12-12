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

AuthService::LoginResult AuthService::Login(const std::string& loginId, const std::string& plainPassword, AccountRecord& outAccount)
{
    // ID로 계정 조회
    if (!repo_.FindByLoginId(loginId, outAccount))
        return LoginResult::NoSuchId;

    // 상태 체크(정지/삭제 등)
    if (outAccount.status != 0)
        return LoginResult::Banned;

    // 비밀번호 검증: stored salt로 재해싱해서 stored hash와 비교
    if (!PasswordHasher::VerifyPassword(plainPassword, outAccount.pw_salt, outAccount.pw_hash))
        return LoginResult::WrongPassword;

    // 로그인 성공 처리: last_login_at 갱신
    // (실패해도 로그인 자체를 실패로 볼지 정책 선택 가능. 여기선 DB 에러면 실패 처리)
    if (!repo_.UpdateLastLogin(outAccount.account_id))
        return LoginResult::DbError;

    return LoginResult::Success;
}