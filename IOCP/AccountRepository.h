#pragma once

#include <string>
#include <cstdint>

class DBConnection;

// account 테이블 속성들
struct AccountRecord
{
    uint64_t account_id = 0;
    std::string login_id;
    std::string pw_hash;
    std::string pw_salt;
    uint8_t status = 0;   // 0: 정상, 1: 정지, 2: 삭제
    std::string created_at;
    std::string updated_at;
    std::string lastLogin_at;
};

class AccountRepository
{
public:
    explicit AccountRepository(DBConnection* conn);

    // ID 중복 여부 확인
    bool IsLoginIdExists(const std::string& loginId);

    // 계정 생성 결과
    enum class CreateResult
    {
        Success,
        DuplicateLoginId,
        DbError
    };

    // 회원가입: 이미 해시/솔트가 계산된 상태라고 가정
    // (실제 해시/솔트 생성은 다른 레이어에서 담당)
    CreateResult CreateAccount(const std::string& loginId, const std::string& pwHash, const std::string& pwSalt, uint64_t& outAccountId);

    // 로그인 시: login_id로 계정 정보 조회
    bool FindByLoginId(const std::string& loginId, uint64_t& outAccountId, std::string& outPwHash, std::string& outPwSalt, uint8_t& outStatus);

    // 로그인 성공 후: 마지막 로그인 시간 갱신
    bool UpdateLastLogin(uint64_t accountId);

    // 비밀번호 변경 - 해시/솔트는 이미 계산되어 들어온다고 가정
    bool UpdatePassword(uint64_t accountId, const std::string& newPwHash, const std::string& newPwSalt);

    // 상태 변경(정상/정지/삭제 등)
    bool UpdateStatus(uint64_t accountId, uint8_t newStatus);

    void SetConnection(DBConnection* conn) { conn_ = conn; }


private:
    DBConnection* conn_; // 생명주기는 DBManager가 관리

    // TODO: 나중에 mysql_real_escape_string을 이용해
    // 문자열 이스케이프를 제대로 구현할 예정
    std::string Escape(const std::string& value);
};

