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


// World
// 내부 유틸
static inline float sqr(float v) { return v * v; }

// 두 위치 수평거리 제곱
static float Dist2(const Transform& a, const Transform& b) {
    // 일단 수평 거리만 (필요시 y 포함)
    return sqr(a.x - b.x) + sqr(a.z - b.z);
}

