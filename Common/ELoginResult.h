#pragma once
#include <cstdint>

// 실패 사유 코드(클라가 화면에 띄우거나 디버깅용)
enum class ELoginResult : uint32_t
{
    OK = 0,
    NO_SUCH_USER = 1,
    INVALID_PASSWORD = 2,
    BANNED = 3,
    DB_ERROR = 4,
    INTERNAL_ERROR = 5,
};