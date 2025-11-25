#pragma once
#include "Pch.h"

enum class IoType : uint8_t {
    Recv,
    Send,
    Accept,
    Connect,
    Quit
};

//오버랩드ex에서 버퍼 만들고 그 클래스를 인자로 가지고 있게 만들기 위해서 버퍼 먼저 만들기
