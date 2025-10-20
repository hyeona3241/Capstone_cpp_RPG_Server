#include "SessionManager.h"

// ���� ������. �ű⿡ ���缭 ��鵵 �����������

SessionManager::SessionManager(size_t maxSession)
    : pool_(maxSession)
{
}

Session* SessionManager::CreateSession(SOCKET sock)
{
    Session* s = pool_.Allocate(); 
    if (!s) return nullptr;  
    /*s->sock_ = sock;*/

    {
        std::lock_guard<std::mutex> lock(mtx_); 
        /*sessions_[s->id] = s;
        bySock_[sock_] = s->id;*/
        ++created_;
    }

    s->Start();
    return s;
}

void SessionManager::DestroySession(uint64_t id)
{
    Session* s = nullptr;

    {
        std::lock_guard<std::mutex> lock(mtx_);

        auto it = sessions_.find(id);
        if (it == sessions_.end())
            return; 

        s = it->second; 
        sessions_.erase(it); 
        /*bySock_.erase(s->sock_);*/
        ++destroyed_;
    }

    s->Close();
    // ���� ������ �� Ǯ�� �ݳ�
    pool_.Release(s);
}

Session* SessionManager::Find(uint64_t id) const
{
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = sessions_.find(id);
    if (it == sessions_.end())
        return nullptr;

    return it->second;
}

Session* SessionManager::FindBySocket(SOCKET sock) const
{
    std::lock_guard<std::mutex> lock(mtx_);

    auto it = bySock_.find(sock);
    if (it == bySock_.end())
        return nullptr;

    auto it2 = sessions_.find(it->second);
    if (it2 == sessions_.end())
        return nullptr;

    return it2->second;
}

template<typename F>
void SessionManager::ForEachActive(F&& fn)
{
    std::vector<Session*> snapshot;

    {
        std::lock_guard<std::mutex> lock(mtx_);
        snapshot.reserve(sessions_.size());

        for (auto& kv : sessions_)
            snapshot.push_back(kv.second);
    }

    // ���� ������ �� ���� �۾� ����
    for (auto* s : snapshot)
        fn(s);
}

void SessionManager::Broadcast(std::span<const std::byte> payload)
{
    ForEachActive([&](Session* s) { s->Send(payload); });
}

size_t SessionManager::ActiveCount() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return sessions_.size();
}