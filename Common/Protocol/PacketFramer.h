#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <cstring>
#include "PacketDef.h"

// TCP ����Ʈ ��Ʈ������ ��Ŷ�� �ϼ� ������ �и�
// �����ۿ��� ��Ŷ �и�

namespace Proto {
    class PacketFramer final {
    public:
        enum class Result {
            NeedMore, // ���� ��Ŷ �� �ȵ���(���/���� ����)
            Ok,       // ��Ŷ 1�� �ϼ�(out�� ä��), consumed��ŭ ���ۿ��� ����
            Fatal     // ������ size(��� �̸�/���� �ʰ�). ���� ���� ����
        };

        // �ϼ��� ��Ŷ�� �մ��� Ȯ��
        Result TryPopFromContiguous(std::span<const std::byte> buf, size_t& consumed, std::vector<std::byte>& out) const noexcept;
        // �� �������� ���� ���� ����
        Result TryPopFromTwoSegments(std::span<const std::byte> seg1, std::span<const std::byte> seg2, size_t& consumed, std::vector<std::byte>& out) const noexcept;
        // ����� �̸� Ȯ��
        bool PeekHeader(std::span<const std::byte> buf, PacketHeader& hdr) const noexcept;

    private:
        // �� �������� ���� ����� ������ �� �ؼ�
        static bool ReadHeaderFromTwo(std::span<const std::byte> a, std::span<const std::byte> b, PacketHeader& hdr) noexcept;
    };
}