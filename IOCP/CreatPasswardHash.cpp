//#include <iostream>
//#include <string>
//
//#include "PasswordHasher.h"
//#include "DBManager.h"
//
//static std::string ToHex(const std::string& bin)
//{
//    static const char* hex = "0123456789abcdef";
//    std::string out;
//    out.reserve(bin.size() * 2);
//    for (unsigned char c : bin)
//    {
//        out.push_back(hex[c >> 4]);
//        out.push_back(hex[c & 0x0F]);
//    }
//    return out;
//}
//
//
//// 테스트 전용
//int main()
//{
//    DBConfig cfg;
//    cfg.host = "100.77.71.68";
//
//    DBManager dbManager;
//    if (!dbManager.Initialize(cfg))
//    {
//        std::cerr << "DB Init failed\n";
//        return 1;
//    }
//
//    DBConnection* conn = dbManager.GetConnection();
//
//    std::string loginId, password;
//    std::cout << "Input login_id: ";
//    std::getline(std::cin, loginId);
//    std::cout << "Input password: ";
//    std::getline(std::cin, password);
//
//    // 1) salt/hash 생성
//    std::string saltBin = PasswordHasher::GenerateSalt();              // 16 bytes binary
//    std::string saltHex = ToHex(saltBin);                              // 32 hex chars
//    std::string hashHex = PasswordHasher::HashPassword(password, saltBin); // 64 hex chars
//
//    // 2) INSERT (pw_salt는 UNHEX로 넣어야 BINARY(16)에 안전)
//    std::string query =
//        "INSERT INTO account (login_id, pw_hash, pw_salt, status) VALUES ('" +
//        loginId + "', '" + hashHex + "', UNHEX('" + saltHex + "'), 0);";
//
//    long long affected = 0;
//    if (conn->ExecuteUpdate(query, affected))
//    {
//        std::cout << "INSERT OK, affected rows = " << affected << "\n";
//        std::cout << "pw_hash=" << hashHex << "\n";
//        std::cout << "pw_salt_hex=" << saltHex << "\n";
//    }
//    else
//    {
//        std::cout << "INSERT FAILED\n";
//    }
//
//    dbManager.Finalize();
//}
