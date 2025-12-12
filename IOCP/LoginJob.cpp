#include "LoginJob.h"
#include "DbJob.h"
#include "AuthService.h"
#include "DBServer.h" 

#include <iostream>

static const char* ToString(AuthService::LoginResult r)
{
    switch (r)
    {
    case AuthService::LoginResult::Success:       return "Success";
    case AuthService::LoginResult::NoSuchId:      return "NoSuchId";
    case AuthService::LoginResult::WrongPassword: return "WrongPassword";
    case AuthService::LoginResult::Banned:        return "Banned";
    case AuthService::LoginResult::DbError:       return "DbError";
    default:                                      return "Unknown";
    }
}

void LoginJob::Execute(DbServices& services, DBServer& server)
{
    auto& auth = services.auth;

    AccountRecord account{}; // AuthService::Login이 여기 채워줌
    AuthService::LoginResult result = AuthService::LoginResult::DbError;

    try
    {
        result = auth.Login(loginId_, plainPassword_, account); // <- 시그니처 딱 일치 :contentReference[oaicite:2]{index=2}
    }
    catch (const std::exception& e)
    {
        std::cout << "[LoginJob] exception: " << e.what() << "\n";
        result = AuthService::LoginResult::DbError;
    }
    catch (...)
    {
        std::cout << "[LoginJob] unknown exception\n";
        result = AuthService::LoginResult::DbError;
    }

    // ---- TODO: 결과 ACK 패킷 만들기 ----
    // DB_LOGIN_ACK (예: 3201) 같은 패킷이 생기면:
    // - resultCode = (int)result
    // - 성공 시 account_id 포함 (account.account_id)
    // - 필요하면 status/model_id 등 추가 포함
    //
    // 예:
    // DB_LOGIN_ACK ack;
    // ack.result = static_cast<int>(result);
    // ack.accountId = (result == Success) ? account.account_id : 0;

    // ---- TODO: MainServer로 응답 전송 ----
    // server.SendToSession(replySessionId_, ackBytes);
    // 또는 server.SendLoginAck(replySessionId_, ack);

    std::cout << "[LoginJob] loginId=" << loginId_
        << " result=" << ToString(result)
        << " account_id=" << account.account_id
        << " replySessionId=" << replySessionId_ << "\n";

    (void)server; // 아직 send가 없으니 경고 방지
}