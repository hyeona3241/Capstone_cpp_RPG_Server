#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <memory>

#include "WorldConfig.h"
#include "Entity.h"
#include "Player.h"

/// <summary>
/// 전체 월드 상태
/// </summary>

class SpawnSystem;
class PlayerSystem;

// 네트워크 전송 추상화
struct IBroadcaster {
    virtual ~IBroadcaster() = default;
    virtual void SendTo(UID uid, uint16_t pid, const void* data, size_t len) = 0;
    virtual void SendToMany(const std::vector<UID>& uids, uint16_t pid, const void* data, size_t len) = 0;
    virtual void SendToAll(uint16_t pid, const void* data, size_t len) = 0;
};

class World
{
private:
    WorldConfig cfg_;
    IBroadcaster& net_;

    std::unordered_map<EntityId, Entity> entities_;

public:
    // 일시적으로 퍼블릭으로 해놓음. 나중에 호출 함수 형태로 바꾸고 PlayerSystem쪽 함수도 수정해줘야함.
    std::unordered_map<UID, Player> players_;

private:
    EntityId nextId_ = 1;

    float accTick_ = 0.f;
    float accSnapshot_ = 0.f;

    std::unique_ptr<SpawnSystem> spawn_;
    std::unique_ptr<PlayerSystem> playersys_;

public:
    explicit World(const WorldConfig& cfg, IBroadcaster& net);

    // 라이프사이클
    void Init(); 
    void Update(float dt); 
    void Snapshot(float dt);

    // 스폰/퇴장
    EntityId Spawn(EntityType t, UID owner, const Transform& tf);
    void     Despawn(EntityId id);
    void     SpawnDummies(uint32_t count);

    // 플레이어 입장/퇴장
    void OnPlayerEnter(UID uid);
    void OnPlayerLeave(UID uid);

    // 패킷 엔트리 (핸들러에서 호출할거임)
    void OnWorldConnect(UID uid, uint32_t ver);
    void OnClientTransform(UID uid, const Transform& tfClient, uint32_t tickClient);

    // 조회/유틸
    const WorldConfig& Config() const { return cfg_; }
    const Entity* FindEntity(EntityId id) const;
    const Player* FindPlayer(UID uid) const;

    Player* FindPlayerMutable(UID uid) {
        auto it = players_.find(uid);
        return (it == players_.end()) ? nullptr : &it->second;
        
    }

    Entity * FindEntityMutable(EntityId id) {
        auto it = entities_.find(id);
        return (it == entities_.end()) ? nullptr : &it->second;
        
    }


private:
    // 패킷 만들고 만들기
    /*void BroadcastSpawnSelf(UID uid, EntityId selfId, const Transform& tf);
    void SendExistingTo(UID uid);
    void BroadcastSpawnEntity(EntityId id, const Entity& e, UID except);
    void BroadcastDespawn(EntityId id, UID except);
    void BroadcastTransformsInterest();

    std::vector<UID> CollectInterested(UID center, float radius) const;*/

};

