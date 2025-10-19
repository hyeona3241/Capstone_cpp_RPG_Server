#pragma once
#include <span>
#include "Util/RingBuffer.h"

class RecvRing
{
private:
    Util::RingBuffer rb_;

public:
    explicit RecvRing(std::size_t capacity) : rb_(capacity) {}

    // ������ ����Ʈ ����
    std::size_t Write(std::span<const std::byte> src) { return rb_.write(src); }

    // ���� ���� ����
    std::span<const std::byte> Seg1() const { return rb_.first_segment(); }
    std::span<const std::byte> Seg2() const { return rb_.second_segment(); }

    // �տ��� n����Ʈ �Һ�
    void Consume(std::size_t n) { rb_.consume(n); }

    // ������� Ȯ��
    bool Empty() const { return rb_.empty(); }

    // ���� ���� ����Ʈ ��
    std::size_t Size() const { return rb_.size(); }

    // ��ü �뷮
    std::size_t Capacity() const { return rb_.capacity(); }

    // �ʱ�ȭ(��� ������ ����)
    void Clear() { rb_.clear(); }

};

