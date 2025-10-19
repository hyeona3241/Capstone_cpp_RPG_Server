#include "PacketFramer.h"

namespace Proto {
    // ���ӵǾ� ���� ��
    PacketFramer::Result PacketFramer::TryPopFromContiguous(std::span<const std::byte> buf,
        size_t& consumed, std::vector<std::byte>& out) const noexcept 
    {
        consumed = 0;
        out.clear();

        // ����� �����ϸ� �� �ʿ�
        if (buf.size() < kHeaderSize) {
            return Result::NeedMore;
        }

        // ��� �б�
        PacketHeader hdr{};
        std::memcpy(&hdr, buf.data(), kHeaderSize);

        // ũ�� ��ȿ�� �˻�
        if (!IsValidPacketSize(hdr.size)) {
            return Result::Fatal; // ��� �̸� �Ǵ� ���� �ʰ�
        }

        // ���� ����
        if (buf.size() < hdr.size) {
            return Result::NeedMore;
        }

        //  ��Ŷ 1�� ����
        out.resize(hdr.size);
        std::memcpy(out.data(), buf.data(), hdr.size);

        consumed = hdr.size;
        return Result::Ok;
    }

    // �� �������� �������� ��
    PacketFramer::Result PacketFramer::TryPopFromTwoSegments(std::span<const std::byte> seg1, std::span<const std::byte> seg2,
        size_t& consumed, std::vector<std::byte>& out) const noexcept 
    {
        consumed = 0;
        out.clear();

        const size_t totalAvail = seg1.size() + seg2.size();

        if (totalAvail < kHeaderSize) {
            return Result::NeedMore;
        }

        // ��� ����/�ؼ�
        PacketHeader hdr{};
        if (!ReadHeaderFromTwo(seg1, seg2, hdr)) {
            return Result::Fatal;
        }

        if (!IsValidPacketSize(hdr.size)) {
            return Result::Fatal;
        }

        if (totalAvail < hdr.size) {
            return Result::NeedMore;
        }

        // ��Ŷ ����
        out.resize(hdr.size);

        // seg1���� ���� ���� ��
        size_t firstTake = (hdr.size <= seg1.size()) ? hdr.size : seg1.size();

        if (firstTake)
            std::memcpy(out.data(), seg1.data(), firstTake);

        size_t remain = hdr.size - firstTake;
        if (remain) {
            std::memcpy(out.data() + firstTake, seg2.data(), remain);
        }

        consumed = hdr.size;
        return Result::Ok;
    }

    bool PacketFramer::PeekHeader(std::span<const std::byte> buf, PacketHeader& hdr) const noexcept {
        if (buf.size() < kHeaderSize) return false;
        std::memcpy(&hdr, buf.data(), kHeaderSize);

        return true;
    }

    bool PacketFramer::ReadHeaderFromTwo(std::span<const std::byte> a, std::span<const std::byte> b, PacketHeader& hdr) noexcept
    {
        // a������ ����ϸ� �ٷ�
        if (a.size() >= kHeaderSize) {
            std::memcpy(&hdr, a.data(), kHeaderSize);
            return true;
        }

        // a�� �Ϻ�, b���� ������
        if (a.size() + b.size() < kHeaderSize) return false;

        std::byte tmp[kHeaderSize];

        if (!a.empty())
            std::memcpy(tmp, a.data(), a.size());

        std::memcpy(tmp + a.size(), b.data(), kHeaderSize - a.size());
        std::memcpy(&hdr, tmp, kHeaderSize);

        return true;
    }

}
