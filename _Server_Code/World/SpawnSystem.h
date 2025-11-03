#pragma once
#include <cstdint>
#include <vector>
#include <random>

#include "Ids.h"
#include "Entity.h"
#include "WorldConfig.h"

/// <summary>
/// 엔티티 생성, 초기화 로직
/// </summary>

class World;

class SpawnSystem
{
private:
    World& world_;
    // 난수 생성기 타입 (rand()보다 품질 좋다고 함)
    // 조금더 알아봐야 할듯 (<random> 헤더에 있음)
    std::mt19937 rng_;  // 랜덤 생성기

    struct RespawnItem {
        float remainSec;
        EntityType type;
        UID owner;
    };
    std::vector<RespawnItem> respawn_;

public:
    explicit SpawnSystem(World& world);

    void Init();
    EntityId SpawnPlayer(UID uid);
    EntityId SpawnNpc(const Transform& seed);
    void Despawn(EntityId id);
    void Update(float dt);

private:
    Transform PickSpawnPoint(UID uid, EntityType t);
    bool FindFreeSpot(Transform& inout);
    float Rand(float minVal, float maxVal);

};

