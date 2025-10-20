#include "SessionPool.h"

// ���� ������. �ű⿡ ���缭 ��鵵 �����������


SessionPool::SessionPool(size_t maxSession) {
    // maxSession ��ŭ Session�����
    sessions_.reserve(maxSession);

   /* for (size_t i = 0; i < maxSession; ++i) {
        auto* s = new Session();
        sessions_.push_back(s);
        freeList_.push(s);
    }*/
}

SessionPool::~SessionPool() {
    // ��� Session�� delete�� �����Ѵ�
    for (auto* s : sessions_) delete s;
    sessions_.clear();
}

Session* SessionPool::Allocate() {
    // ���ؽ��� ��ȣ (freeList���� �ϳ� ����)
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
    if (!s) return; // �߸��� ����

    // ��� ���� ǥ�� �� freeList�� �ǵ���
    std::lock_guard<std::mutex> lock(mtx_);
   /* s->inUse = false;*/
    freeList_.push(s);
}

std::vector<Session*> SessionPool::GetActiveSessions() {
    // ��ü ���� �� inUse == true �� �͸� ������
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<Session*> out;
    out.reserve(sessions_.size());

    for (auto* s : sessions_) {
        /*if (s->inUse) out.push_back(s);*/
    }

    return out;
}

size_t SessionPool::FreeCount() const {
    // ���� ���� �� �ִ� ������ ��ȯ
    std::lock_guard<std::mutex> lock(mtx_);
    return freeList_.size();
}