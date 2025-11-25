#pragma once
#include <cstdint>

#include "Ids.h"
#include "Entity.h"
#include "Player.h"

/// <summary>
/// 플레이어 전용 규칙
/// </summary>

class World;
class SpawnSystem;

class PlayerSystem
{
private:
    World& world_;
    SpawnSystem& sp_;

public:
    PlayerSystem(World& world, SpawnSystem& sp) : world_(world), sp_(sp) {}

    // 월드 참여/떠나기
    void OnEnter(UID uid);
    void OnLeave(UID uid);

    // 월드 연결
    void OnWorldConnect(UID uid, uint32_t protoVer);

    // 클라 입력에 따른 스냅샷
    void OnClientTransform(UID uid, const Transform& tfClient, uint32_t tickClient);

};

