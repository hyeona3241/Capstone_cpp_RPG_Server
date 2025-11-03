#include "SpawnSystem.h"
#include "World.h"
#include <algorithm> 
#include <limits>

SpawnSystem::SpawnSystem(World& world)
    : world_(world)
{
    // 월드 시드 기반 RNG 초기화
    const uint32_t seed = world_.Config().seed ? world_.Config().seed : 0xA5A5A5A5u;
    rng_.seed(seed);
}

// 더미 NPC 스폰
void SpawnSystem::Init() {
    const auto& cfg = world_.Config();

    for (uint32_t i = 0; i < cfg.npcDummyCount; ++i) {
        Transform t = PickSpawnPoint(0, EntityType::Npc);
        
        t.x += static_cast<float>(i) * 2.f;
        world_.Spawn(EntityType::Npc, 0, t);
    }
}

// 플레이어 스폰
EntityId SpawnSystem::SpawnPlayer(UID uid) {
    Transform t = PickSpawnPoint(uid, EntityType::Player);
    FindFreeSpot(t);

    return world_.Spawn(EntityType::Player, uid, t);
}

// NPC 스폰
EntityId SpawnSystem::SpawnNpc(const Transform& seed) {
    Transform t = seed;

    FindFreeSpot(t);
    return world_.Spawn(EntityType::Npc, 0, t);
}

// 디스폰 
void SpawnSystem::Despawn(EntityId id) {
    world_.Despawn(id);
}

// 리스폰 큐 처리
void SpawnSystem::Update(float dt) {
    if (respawn_.empty()) return;

    for (auto& r : respawn_) {
        r.remainSec -= dt;
    }
    
    auto it = respawn_.begin();

    while (it != respawn_.end()) {
        if (it->remainSec <= 0.f) {
            Transform t = PickSpawnPoint(it->owner, it->type);

            FindFreeSpot(t);
            world_.Spawn(it->type, it->owner, t);
            it = respawn_.erase(it);
        }
        else {
            ++it;
        }
    }
}

// 스폰 위치 선택
Transform SpawnSystem::PickSpawnPoint(UID , EntityType ) {
    const auto& cfg = world_.Config();
    Transform t{};

    switch (cfg.spawnMode) {
    case SpawnMode::Fixed:
        t.x = cfg.spawnPoint.x; t.y = cfg.spawnPoint.y; t.z = cfg.spawnPoint.z;
        t.rx = t.ry = t.rz = 0.f;
        break;

    case SpawnMode::Random: {
        // bounds 내 랜덤
        const float x = Rand(cfg.boundsMin.x, cfg.boundsMax.x);
        const float z = Rand(cfg.boundsMin.z, cfg.boundsMax.z);

        t.x = x; t.y = cfg.spawnPoint.y; t.z = z;
        t.rx = t.ry = t.rz = 0.f;
        break;
    }

    case SpawnMode::Waypoints:
        if (!cfg.spawnPoints.empty()) {
            const auto& p = cfg.spawnPoints[0];
            t.x = p.x; t.y = p.y; t.z = p.z;
            t.rx = t.ry = t.rz = 0.f;
        }
        else {
            t.x = cfg.spawnPoint.x; t.y = cfg.spawnPoint.y; t.z = cfg.spawnPoint.z;
            t.rx = t.ry = t.rz = 0.f;
        }
        break;
    }

    // 경계 클램프
    t.x = std::clamp(t.x, cfg.boundsMin.x, cfg.boundsMax.x);
    t.z = std::clamp(t.z, cfg.boundsMin.z, cfg.boundsMax.z);
    return t;
}

// 자리지정 보정 (겹침/충돌 회피) (나중에 수정 필요)
bool SpawnSystem::FindFreeSpot(Transform& inout) {
    const auto& cfg = world_.Config();
    if (!cfg.enableCollision) return true;

    for (int i = 0; i < 8; ++i) {
        const float ox = Rand(-0.5f, 0.5f);
        const float oz = Rand(-0.5f, 0.5f);
        Transform cand = inout;

        cand.x = std::clamp(cand.x + ox, cfg.boundsMin.x, cfg.boundsMax.x);
        cand.z = std::clamp(cand.z + oz, cfg.boundsMin.z, cfg.boundsMax.z);
        
        inout = cand;
        return true; 
    }
    return true;
}

// RNG 유틸
float SpawnSystem::Rand(float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(rng_);
}