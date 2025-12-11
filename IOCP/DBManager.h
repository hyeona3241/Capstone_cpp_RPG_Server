#pragma once
#include <memory>
#include <string>
#include "DBConnection.h"

struct DBConfig
{
    std::string host;
    unsigned int port = 3306;
    std::string user = "gameuser";
    std::string password = "game";
    std::string schema = "game_db"; // 사용할 DB 이름 
    std::string charset = "utf8mb4";
};

class DBManager
{
public:
    DBManager() = default;
    ~DBManager();

    // DB 설정을 받아서 내부에서 DBConnection을 생성하고 Connect()까지 수행
    bool Initialize(const DBConfig& config);

    // 서버 종료 시 호출 (연결 해제)
    void Finalize();

    // 현재 설정된 DBConfig 조회용
    const DBConfig& GetConfig() const { return config_; }

    // DBConnection 포인터 제공 (쿼리 실행 시 사용)
    DBConnection* GetConnection() { return connection_.get(); }
    const DBConnection* GetConnection() const { return connection_.get(); }

    // 연결 상태 확인
    bool IsConnected() const { return connection_ != nullptr; }

private:
    DBConfig config_;
    std::unique_ptr<DBConnection> connection_;
};

