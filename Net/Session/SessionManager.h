#pragma once
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <span>
#include "SessionPool.h"

#include "Pch.h"


// 활성 세션(연결된 클라이언트)을 관리
// 세션의 등록/해제/검색/브로드캐스트

class SessionManager
{
private:
    mutable std::mutex mtx_; // 세션 목록 보호용 뮤텍스
    std::unordered_map<uint64_t, Session*> sessions_; // 활성 세션 목록 (ID -> Session)
    std::unordered_map<SOCKET, uint64_t> bySock_;  // 소켓으로 세션 찾기용 (SOCKET -> ID)

    Util::BufferPool& bufPool_;
    SessionPool pool_; // 세션 객체 재사용 풀

    std::atomic<uint64_t> created_{ 0 }; // 생성된 세션 수
    std::atomic<uint64_t> destroyed_{ 0 }; // 해제된 세션 수

public:
    explicit SessionManager(size_t maxSession, Util::BufferPool& bufPool);

    // 세션 생성(새 클라가 접속했을 때 호출)
    Session* CreateSession(SOCKET sock);

    // 세션 제거(클라가 끊어졌을 때 호출)
    void DestroySession(uint64_t id);

    // 세션 검색 (ID로 Session 객체 포인터찾음)
    Session* Find(uint64_t id) const;

    // 세션 검색 (소켓으로 Session 찾기)
    Session* FindBySocket(SOCKET sock) const;

    // 활성 세션 전체를 순회하며 주어진 함수를 적용
    template<typename F>
    void ForEachActive(F&& fn);

    // 브로드캐스트
    void Broadcast(std::span<const std::byte> payload);

    // 현재 연결된 세션의 개수
    size_t ActiveCount() const;

    // 지금까지 생성된 세션 수 (디버깅용)
    uint64_t TotalCreated() const { return created_.load(); }

    // 지금까지 해제된 세션 수 (디버깅용)
    uint64_t TotalDestroyed() const { return destroyed_.load(); }

    // 내부 SessionPool 접근( 디버깅용)
    SessionPool& Pool() { return pool_; }
};

