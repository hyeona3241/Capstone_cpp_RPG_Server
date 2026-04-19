#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>

enum class GatewayRedirectTarget : uint8_t
{
    Login,
    World,
    Chat
};

struct GatewayRedirectToken
{
    std::string token;
    GatewayRedirectTarget target;
    int64_t accountId;
    uint32_t targetServerId;
    uint64_t expireTick;
};

class GatewayTokenManager
{
public:
    std::string CreateToken(GatewayRedirectTarget target,
        int64_t accountId,
        uint32_t targetServerId,
        uint64_t ttlMs);

    bool ConsumeToken(const std::string& token, GatewayRedirectToken& out);
    void RemoveExpiredTokens();

private:
    std::unordered_map<std::string, GatewayRedirectToken> tokens_;
    std::mutex lock_;
};

