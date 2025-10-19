#pragma once
#include <span>
#include "Util/RingBuffer.h"

class RecvRing
{
private:
    Util::RingBuffer rb_;

public:
    explicit RecvRing(std::size_t capacity) : rb_(capacity) {}

    // 수신한 바이트 누적
    std::size_t Write(std::span<const std::byte> src) { return rb_.write(src); }

    // 연속 구간 보기
    std::span<const std::byte> Seg1() const { return rb_.first_segment(); }
    std::span<const std::byte> Seg2() const { return rb_.second_segment(); }

    // 앞에서 n바이트 소비
    void Consume(std::size_t n) { rb_.consume(n); }

    // 비었는지 확인
    bool Empty() const { return rb_.empty(); }

    // 현재 보유 바이트 수
    std::size_t Size() const { return rb_.size(); }

    // 전체 용량
    std::size_t Capacity() const { return rb_.capacity(); }

    // 초기화(모든 데이터 버림)
    void Clear() { rb_.clear(); }

};

