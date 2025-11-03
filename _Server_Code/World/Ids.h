#pragma once
#include <cstdint>

// 나중에 Common의 UidGenerator로 옮기기!!!

using UID = std::uint64_t; // 계정/플레이어 식별자
using EntityId = std::uint32_t; // 월드 엔티티 식별자

// id 정책 상수
inline constexpr EntityId kInvalidEntity = 0;

struct EntityIdGen {
    EntityId next{ 1 };
    EntityId issue() { return next++; }
};