#pragma once
#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <cstring>
#include <type_traits>
#include "PacketDef.h"

// 서버 -> 클라

namespace Proto {
    class BinaryWriter {
    private:
        // 헤더 시작된 위치 기억
        size_t hdrPos_ = npos;
        // 유효하지 않은 위치를 나타내는 상수값
        static constexpr size_t npos = static_cast<size_t>(-1);
        
        // 데이터 써넣는 바이트 버퍼
        std::vector<std::byte> buf_;

    public:
        BinaryWriter() { buf_.reserve(1024); }  // 재할당 방지
        explicit BinaryWriter(size_t preReserve) { buf_.reserve(preReserve); }

        // 상태
        // 데이터 삭제
        void Clear() { buf_.clear(); hdrPos_ = npos; }
        // 내부 버퍼 용량 미리 확보
        void Reserve(size_t bytes) { buf_.reserve(bytes); }
        // 읽기 전용 상수로 반환 (전송이나 로그 사용 등)
        const std::vector<std::byte>& Buffer() const noexcept { return buf_; }
        // 쓰기 가능한 상수 반환 (되도록 사용x)
        std::vector<std::byte>& Buffer() noexcept { return buf_; }
        // 사이즈 반환 (기록된 총 바이트)
        size_t Size() const noexcept { return buf_.size(); }
        // 버퍼가 비었는가
        bool   Empty() const noexcept { return buf_.empty(); }

        // 패킷 헤더 자리 확보하고 id 기록
        void BeginPacket(uint16_t id);
        // 패킷 사이즈 기록하고 사이즈가 kMaxPacket 이하인지 확인
        void EndPacket();
        // 사이즈와 아이디 기록
        void WriteHeader(uint16_t id, uint16_t totalSize);

        // 모두 리틀엔디언으로 해석
        // 스칼라 타입 (C#)
        void WriteU8(uint8_t  v) { writeScalarLE(v); }
        void WriteI8(int8_t   v) { writeScalarLE(v); }

        void WriteU16(uint16_t v) { writeScalarLE(v); }
        void WriteI16(int16_t  v) { writeScalarLE(v); }

        void WriteU32(uint32_t v) { writeScalarLE(v); }
        void WriteI32(int32_t  v) { writeScalarLE(v); }

        void WriteU64(uint64_t v) { writeScalarLE(v); }
        void WriteI64(int64_t  v) { writeScalarLE(v); }

        void WriteF32(float    v) { writeScalarLE(v); }
        void WriteF64(double   v) { writeScalarLE(v); }


        // 복사
        void WriteBytes(std::span<const std::byte> bytes);

        // 짧은 문자열(65535B) 전송
        void WriteStringLp16(std::string_view s);

        // 긴 텍스트(4GB) 전송
        void WriteStringLp32(std::string_view s);

    private:
        // 리틀엔디언으로 POD 값을 기록
        template<class T>
        void writeScalarLE(T v) {
            static_assert(std::is_trivially_copyable_v<T>, "POD only");
            const size_t old = buf_.size();

            buf_.resize(old + sizeof(T));
            
            std::memcpy(buf_.data() + old, &v, sizeof(T));
        }
        
    };
}
