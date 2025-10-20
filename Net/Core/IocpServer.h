#pragma once
#include <windows.h>
#include <vector>
#include <thread>
#include <atomic>
#include <cstdint>
#include "Config/NetConfig.h" 
#include "IocpWorker.h" 
#include "SocketUtils.h" 
#include "Session/SessionManager.h" 
#include "App/INetListener.h" 

// ���߿� AcceptEx�� ��ü
class IocpServer
{
private:
    // IOCP �ڵ�
    HANDLE iocp_ = INVALID_HANDLE_VALUE;
    // ���� ����
    SOCKET listen_ = INVALID_SOCKET;
    // ��Ŀ �������
    std::vector<IocpWorker*> workers_; 
    // accept ���� ������
    std::thread acceptThread_;
    // ���� ���� ���ΰ�
    std::atomic<bool> running_{ false };     

    // ���� �Ŵ���(���/����)
    SessionManager* mgr_ = nullptr; 
    //// ���� �� �ݹ�
    //INetListener* listener_ = nullptr;

public:
    IocpServer() = default; 
    ~IocpServer(); 

    // ���� ����
    bool Start(const char* ip, uint16_t port, int workerCount, SessionManager* mgr, INetListener* listener);

    // ���� ����
    void Stop();

private:
    // accept ���� ����
    void AcceptLoop();

    // ������ IOCP�� ����
    bool RegisterSocketToIocp(SOCKET s, void* completionKeyOwner);

};

