#pragma once

#include "Pch.h"
#include <memory>

class Buffer
{


private:
    std::size_t capacity_; // 전체 크기
    std::size_t size_; // 사용 가능 데이터 크기
    std::unique_ptr<char[]> data_; // 실제 메모리 블록
};

