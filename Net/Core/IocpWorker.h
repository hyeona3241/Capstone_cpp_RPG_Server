#pragma once
#include <windows.h>
#include <thread>
#include <atomic>

class IocpWorker
{
private:
    // IOCP �ڵ�
    HANDLE iocp_ = INVALID_HANDLE_VALUE; 
    // ���� ���� ������
    std::atomic<bool> running_{ false };  
    // ��Ŀ ������ ��ü
    std::thread th_;     

public:
    explicit IocpWorker(HANDLE iocp) : iocp_(iocp) {}
    ~IocpWorker();

    // ��Ŀ ������ ����(�̹� ���۵Ǿ� ������ ����)
    void Start();

    // ��Ŀ ������ ���� ��û(ť�� ���� ��ȣ�� �ְ� ����)
    void Stop();

private:
    // GQCS ����: �Ϸ� �̺�Ʈ�� ��ٷȴٰ� ������(Session ��)�� ����
    void Loop();

};

