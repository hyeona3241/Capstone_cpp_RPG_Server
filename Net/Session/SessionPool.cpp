#include "SessionPool.h"

// 세션 수정함. 거기에 맞춰서 얘들도 수정해줘야함


SessionPool::SessionPool(size_t maxSession) {
    // maxSession 만큼 Session만들기
    sessions_.reserve(maxSession);

   /* for (size_t i = 0; i < maxSession; ++i) {
        auto* s = new Session();
        sessions_.push_back(s);
        freeList_.push(s);
    }*/
}

SessionPool::~SessionPool() {
    // 모든 Session을 delete로 정리한다
    for (auto* s : sessions_) delete s;
    sessions_.clear();
}

Session* SessionPool::Allocate() {
    // 뮤텍스로 보호 (freeList에서 하나 꺼냄)
    std::lock_guard<std::mutex> lock(mtx_);
    if (freeList_.empty()) return nullptr;

    Session* s = freeList_.front();
    freeList_.pop();

    /*s->ResetForReuse();
    s->id = idGen_++;
    s->inUse = true;

    s->sock_ = INVALID_SOCKET;*/

    return s;
}

void SessionPool::Release(Session* s) {
    if (!s) return; // 잘못된 인자

    // 사용 종료 표시 후 freeList로 되돌림
    std::lock_guard<std::mutex> lock(mtx_);
   /* s->inUse = false;*/
    freeList_.push(s);
}

std::vector<Session*> SessionPool::GetActiveSessions() {
    // 전체 세션 중 inUse == true 인 것만 모으기
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Session*> out;
    out.reserve(sessions_.size());

    for (auto* s : sessions_) {
        /*if (s->inUse) out.push_back(s);*/
    }

    return out;
}

size_t SessionPool::FreeCount() const {
    // 현재 빌릴 수 있는 개수를 반환
    std::lock_guard<std::mutex> lock(mtx_);
    return freeList_.size();
}