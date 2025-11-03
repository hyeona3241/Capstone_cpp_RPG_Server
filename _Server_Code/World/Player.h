#pragma once

#include <cstdint>
#include "Ids.h"

/// <summary>
/// UID <-> 엔티티 매핑, 플레이어 로직
/// </summary>

struct Player
{
    UID uid{};  // 계정/플레이어 식별자(서버 권위 키)
    EntityId  controlled{0};    // 내가 조종 중인 엔티티

    // 디버깅용
    uint32_t lastClientTick{0}; // 마지막으로 받은 클라 틱
    bool loggedIn{false};   // 세션/인증 상태 플래그

    char nickname[32] = {0};    // 표시용 닉네임

    inline bool IsControlling() const noexcept { return controlled != 0; }
    inline void Attach(EntityId eid) noexcept { controlled = eid; }
    inline void Detach() noexcept { controlled = 0; }
    inline void OnClientTick(uint32_t t) noexcept { lastClientTick = t; }
};