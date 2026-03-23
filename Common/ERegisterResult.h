#pragma once
#include <cstdint>

enum class ERegisterResult : uint32_t
{
    OK = 0,
    DUPLICATE_LOGIN_ID = 1,
    INVALID_INPUT = 2,
    DB_ERROR = 3,
    INTERNAL_ERROR = 4,
};