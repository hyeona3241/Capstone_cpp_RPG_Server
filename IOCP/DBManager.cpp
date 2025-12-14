#include "DBManager.h"
#include <iostream>
#include <Logger.h>

DBManager::~DBManager()
{
    Finalize();
}

bool DBManager::Initialize(const DBConfig& config)
{
    // 이미 초기화 되어 있다면 한 번 정리하고 다시 시도할 수도 있음
    if (connection_)
    {
        std::cout << "[DBManager] Reinitializing DB connection...\n";
        LOG_INFO("[DBManager] Reinitializing DB connection...");
        connection_->Disconnect();
        connection_.reset();
    }

    config_ = config;

    connection_ = std::make_unique<DBConnection>();
    bool ok = connection_->Connect(
        config_.host,
        config_.port,
        config_.user,
        config_.password,
        config_.schema,
        config_.charset
    );

    if (!ok)
    {
        std::cerr << "[DBManager] Failed to connect DB. ("
            << config_.host << ":" << config_.port
            << ", schema=" << config_.schema << ")\n";

        LOG_ERROR(std::string("[DBManager] Failed to connect DB. (") + config_.host +
            ":" + std::to_string(config_.port) + ", schema=" + config_.schema + ")");
        connection_.reset(); // 실패 시 nullptr로 되돌림
        return false;
    }

    std::cout << "[DBManager] DB initialized successfully.\n";
    LOG_INFO("[DBManager] DB initialized successfully.");
    return true;
}

void DBManager::Finalize()
{
    if (connection_)
    {
        connection_->Disconnect();
        connection_.reset();
        std::cout << "[DBManager] DB connection finalized.\n";
        LOG_INFO("[DBManager] DB connection finalized.");
    }
}