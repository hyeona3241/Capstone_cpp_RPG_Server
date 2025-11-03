#pragma once

#include <cstdint>
#include <string>
#include <vector>

/// <summary>
/// 월드 규칙/기본 설정 정의
/// </summary>

struct Vec3 { float x{}, y{}, z{}; };

enum class SpawnMode : uint8_t { 
	Fixed = 0,  // 고정
	Random = 1, // 랜덤
	Waypoints = 2   // 웨이 포인트
};

struct WorldConfig
{
    // 기본 메타 / 버전
    uint32_t worldId = 1;   // 월드 ID
    std::string name = "DefaultWorld";  // 월드 네임
    uint32_t protoVer = 1;  // 월드 데이터 버전 (클라와 교차 검증)

    // 시뮬레이션 타이밍
    uint32_t tickRate = 30;   // Hz : 시뮬레이션 틱 (게임 로직을 1초에 몇 번 계산할지)
    uint32_t snapshotRate = 15;   // Hz : 브로드캐스트 빈도 (엔티티들의 상태(좌표, 회전 등)를 클라이언트에게 몇 번씩 전송할지)
    // 현재 2틱에 1번씩 보냄 (30 계산 15 전송 / 나중에 값 수정하며 최적화)
    float maxDeltaTime = 0.1f;   // 초 : 틱 클램프 (서버 랙 대비) (0.1초 이상은 계산 안 함)

    // 좌표 및 경계 (얘는 꼭꼭 클라에서 테스트 하면서 맵 크기 확인하기)
    Vec3 boundsMin{ -500.f, -50.f, -500.f };
    Vec3 boundsMax{ 500.f,  200.f,  500.f };

    // 물리 규칙 (쓸 일 있을까 싶지만 나중에 더 추가하기)
    float gravityY = -9.8f; // 중력 가속도
    bool enableCollision = false;  // 충돌 감지 (처음엔 꺼두고 나중에 사용)
    float collisionEpsilon = 0.05f; // 충돌 감지 오차 허용 범위

    // 이동 상수 (얘들도 클라에서 테스트 하면서 값 찾기)
    float walkSpeed = 4.0f;
    float runSpeed = 6.0f;
    float jumpSpeed = 5.0f;

    // 스폰 규칙
    SpawnMode spawnMode = SpawnMode::Fixed; // 고정 스폰
    Vec3 spawnPoint{ 0.f, 0.f, 0.f };   // 스폰 위치
    std::vector<Vec3> spawnPoints; // Waypoints 모드에서 사용
    uint32_t npcDummyCount = 3; // 자동으로 만들어질 더미 NPC (테스트용)
    float respawnTimeSec = 3.0f;    // 리스폰 타임

    // 네트워크 / 레플리케이션
    uint32_t maxPlayers = 32;   // 최대 플레이어 (테스트 하면서 점점 늘리기)
    float interestRadius = 100.0f;  // 플레이어가 브로드 캐스트 받을 거리 (제한적 거리 확인)
    bool transformQuantization = false; // 좌표 데이터를 압축해서 전달 할지 (나중에 최적화 할 때 True)

    // 결정성 / 디버그
    uint32_t seed = 0xC0FFEEu;  // 서버 시드 (리플레이, 동기화 대비)
    // 0=Off(로그 꺼짐), 1=Info(기본 로그), 2=Verbose(상세 로그)
    int logLevelWorld = 1; // 월드 로직 로그
    int logLevelNet = 1;    // 네트워크 로그

    // 유효성 검증
    bool Validate() const noexcept;
};

// 나중에 데이터 베이스 사용해서 불러오기 가능하도록 만들기!!!