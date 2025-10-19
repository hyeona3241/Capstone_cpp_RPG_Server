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

// ���� -> Ŭ��

namespace Proto {
    class BinaryWriter {
    private:
        // ��� ���۵� ��ġ ���
        size_t hdrPos_ = npos;
        // ��ȿ���� ���� ��ġ�� ��Ÿ���� �����
        static constexpr size_t npos = static_cast<size_t>(-1);
        
        // ������ ��ִ� ����Ʈ ����
        std::vector<std::byte> buf_;

    public:
        BinaryWriter() { buf_.reserve(1024); }  // ���Ҵ� ����
        explicit BinaryWriter(size_t preReserve) { buf_.reserve(preReserve); }

        // ����
        // ������ ����
        void Clear() { buf_.clear(); hdrPos_ = npos; }
        // ���� ���� �뷮 �̸� Ȯ��
        void Reserve(size_t bytes) { buf_.reserve(bytes); }
        // �б� ���� ����� ��ȯ (�����̳� �α� ��� ��)
        const std::vector<std::byte>& Buffer() const noexcept { return buf_; }
        // ���� ������ ��� ��ȯ (�ǵ��� ���x)
        std::vector<std::byte>& Buffer() noexcept { return buf_; }
        // ������ ��ȯ (��ϵ� �� ����Ʈ)
        size_t Size() const noexcept { return buf_.size(); }
        // ���۰� ����°�
        bool   Empty() const noexcept { return buf_.empty(); }

        // ��Ŷ ��� �ڸ� Ȯ���ϰ� id ���
        void BeginPacket(uint16_t id);
        // ��Ŷ ������ ����ϰ� ����� kMaxPacket �������� Ȯ��
        void EndPacket();
        // ������� ���̵� ���
        void WriteHeader(uint16_t id, uint16_t totalSize);

        // ��� ��Ʋ��������� �ؼ�
        // ��Į�� Ÿ�� (C#)
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


        // ����
        void WriteBytes(std::span<const std::byte> bytes);

        // ª�� ���ڿ�(65535B) ����
        void WriteStringLp16(std::string_view s);

        // �� �ؽ�Ʈ(4GB) ����
        void WriteStringLp32(std::string_view s);

    private:
        // ��Ʋ��������� POD ���� ���
        template<class T>
        void writeScalarLE(T v) {
            static_assert(std::is_trivially_copyable_v<T>, "POD only");
            const size_t old = buf_.size();

            buf_.resize(old + sizeof(T));
            
            std::memcpy(buf_.data() + old, &v, sizeof(T));
        }
        
    };
}
