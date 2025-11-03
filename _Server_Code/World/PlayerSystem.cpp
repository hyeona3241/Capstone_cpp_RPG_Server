#include "PlayerSystem.h"
#include "World.h"
#include "SpawnSystem.h"
#include <algorithm>

// World에 있던 player 관련 로직 옮겨오기

void PlayerSystem::OnEnter(UID uid) {
    Player* p = world_.FindPlayerMutable(uid);
    if (!p) {
        world_.players_[uid] = Player{ uid, 0, 0, true };
    }
    else {
        p->loggedIn = true;
    }
    // 월드에 들어왔다고 패킷
}

void PlayerSystem::OnLeave(UID uid) {
    Player* p = world_.FindPlayerMutable(uid);
    if (!p) return;

    if (p->controlled != 0) {
        world_.Despawn(p->controlled);
        p->Detach();
    }

    world_.players_.erase(uid);
    // 월드에 떠난다고 패킷
}

void PlayerSystem::OnWorldConnect(UID uid, uint32_t protoVer) {
    Player* p = world_.FindPlayerMutable(uid);
    if (!p) {
        world_.players_[uid] = Player{ uid, 0, 0, true };
        p = world_.FindPlayerMutable(uid);
    }

    // 스폰은 SpawnSystem에서
    EntityId self = sp_.SpawnPlayer(uid);
    p->Attach(self);

    // 월드 접속 시 패킷으로 통신
}

void PlayerSystem::OnClientTransform(UID uid, const Transform& tfClient, uint32_t tickClient) {
    Player* p = world_.FindPlayerMutable(uid);
    if (!p || p->controlled == 0) return;

    Entity* me = world_.FindEntityMutable(p->controlled);
    if (!me) return;

    // 클라 좌표를 그대로 적용 + 경계 클램프
    me->tf = tfClient;
    const auto& cfg = world_.Config();
    me->tf.x = std::clamp(me->tf.x, cfg.boundsMin.x, cfg.boundsMax.x);
    me->tf.z = std::clamp(me->tf.z, cfg.boundsMin.z, cfg.boundsMax.z);

    p->lastClientTick = tickClient;

    // 브로드 캐스트 하기
}