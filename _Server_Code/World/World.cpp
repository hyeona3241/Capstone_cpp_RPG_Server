#include "World.h"
#include "SpawnSystem.h"
#include <algorithm> 
#include <cmath>

// 내부 유틸
static inline float sqr(float v) { return v * v; }

// 두 위치 수평거리 제곱
static float Dist2(const Transform& a, const Transform& b) {
    // 일단 수평 거리만 (필요시 y 포함)
    return sqr(a.x - b.x) + sqr(a.z - b.z);
}


// 나중에 패킷 구현 후 수정하기

World::World(const WorldConfig& cfg, IBroadcaster& net)
    : cfg_(cfg), net_(net) {

}

// 초기화
void World::Init() {
    spawn_ = std::make_unique<SpawnSystem>(*this);
    spawn_->Init();
}

// 디버깅용 더미 스폰
void World::SpawnDummies(uint32_t count) {
    if (!spawn_) return;

    for (uint32_t i = 0; i < count; ++i) {
        Transform seed{ cfg_.spawnPoint.x + static_cast<float>(i) * 2.f, cfg_.spawnPoint.y, cfg_.spawnPoint.z, 0,0,0 };
        spawn_->SpawnNpc(seed);
    }
}

// 스폰
EntityId World::Spawn(EntityType t, UID owner, const Transform& tf) {
    Entity e{};
    e.id = nextId_++;
    e.type = t;
    e.ownerUid = owner;
    e.tf = tf;

    entities_.emplace(e.id, e);

    // 새로운 스폰 시 브로드 캐스트로 클라들에게 알려줘야함

    return e.id;
}

// 퇴장
void World::Despawn(EntityId id) {
    auto it = entities_.find(id);
    if (it == entities_.end()) return;

    // 디스폰 시 브로드캐스트 해주기

    entities_.erase(it);
}

// 플레이어가 월드에 들어옴
void World::OnPlayerEnter(UID uid) {
    // 월드 참여 표시(아직 엔티티 스폰 전)
    players_[uid] = Player{ uid, 0, 0, true };
}

// 플레이어가 월드에서 나감
void World::OnPlayerLeave(UID uid) {
    auto pit = players_.find(uid);
    if (pit == players_.end()) return;

    if (pit->second.controlled != 0) {
        Despawn(pit->second.controlled);
    }
    players_.erase(pit);

    // 다른 클라들에게 떠나는 거 알리기
}

// 월드와 연결 (좀 더 생각하고 수정해봐야 할듯)
void World::OnWorldConnect(UID uid, uint32_t /*protoVer*/) {
    auto& p = players_[uid]; // 없으면 기본 생성
    p.uid = uid;
    p.loggedIn = true;

    EntityId selfId = spawn_ ? spawn_->SpawnPlayer(uid) : Spawn(EntityType::Player, uid, Transform{ cfg_.spawnPoint.x, cfg_.spawnPoint.y, cfg_.spawnPoint.z, 0,0,0 });
    p.Attach(selfId);

    // 다른 클라들에게 새로운 유저 스폰 브로드 캐스트
    // 월드 접속 서버와 통신
}

// 월드 엔티티에 적용
void World::OnClientTransform(UID uid, const Transform& tfClient, uint32_t tickClient) {
    (void)tickClient; // 지금은 사용 안할듯

    auto pit = players_.find(uid);
    if (pit == players_.end() || pit->second.controlled == 0) return;

    Entity& me = entities_.at(pit->second.controlled);

    // 클라이언트 좌표를 그대로 적용 + 경계 클램프 (나중에 수정 가능성 있음)
    me.tf = tfClient;
    me.tf.x = std::clamp(me.tf.x, cfg_.boundsMin.x, cfg_.boundsMax.x);
    me.tf.z = std::clamp(me.tf.z, cfg_.boundsMin.z, cfg_.boundsMax.z);

    // 범위 내 대상들에게 위치 브로드 캐스트
}

void World::Update(float dtRaw) {
    // 틱 클램프
    float dt = dtRaw;
    if (dt > cfg_.maxDeltaTime) dt = cfg_.maxDeltaTime;

    accTick_ += dt;
    const float tickStep = 1.f / static_cast<float>(cfg_.tickRate);

    while (accTick_ >= tickStep) {

        // 물리나 충돌 처리 나중에 여기서 추가

        accTick_ -= tickStep;
    }

    if (spawn_) spawn_->Update(dt);
}

// 네트워크 스냅샷 제어
void World::Snapshot(float dtRaw) {
    float dt = dtRaw;
    if (dt > cfg_.maxDeltaTime) dt = cfg_.maxDeltaTime;

    accSnapshot_ += dt;
    const float snapStep = 1.f / static_cast<float>(cfg_.snapshotRate);

    if (accSnapshot_ >= snapStep) {
        // interestRadius에 맞춰 Transform 스냅샷 브로드캐스트
        accSnapshot_ = 0.f;
    }
}

// 엔티티 찾기
const Entity* World::FindEntity(EntityId id) const {
    auto it = entities_.find(id);
    return (it == entities_.end()) ? nullptr : &it->second;
}

// 플레이어 찾기
const Player* World::FindPlayer(UID uid) const {
    auto it = players_.find(uid);
    return (it == players_.end()) ? nullptr : &it->second;
}
