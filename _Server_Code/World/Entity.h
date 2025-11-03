#pragma once

#include "Ids.h"

/// <summary>
/// 엔티티(캐릭터, 오브젝트) 구조체
/// </summary>

// 나중에 더미 플레이어, 몬스터 등 추가
enum class EntityType : uint8_t
{
    None = 0,
    Player = 1,
    Npc = 2,
    Object = 3,
};

// 위치 및 회전
struct Transform
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float rx = 0.f;   // rotation X
    float ry = 0.f;
    float rz = 0.f;
};

struct Entity {
    EntityId id = 0;    // 월드 내부 ID
    EntityType type = EntityType::None; // 엔티티 종류
    UID ownerUid = 0;   // Player면 자신의 UID, 그 외 0
    Transform tf;    // 위치/회전 상태
    bool active = true; // 활성 여부 (퇴장 시 false)

    // 맵 경계 클램프
    inline void ClampToBounds(float minX, float maxX, float minZ, float maxZ)
    {
        if (tf.x < minX) tf.x = minX;
        if (tf.x > maxX) tf.x = maxX;
        if (tf.z < minZ) tf.z = minZ;
        if (tf.z > maxZ) tf.z = maxZ;
    }
};