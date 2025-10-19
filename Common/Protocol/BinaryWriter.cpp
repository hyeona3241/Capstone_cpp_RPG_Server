#include "BinaryWriter.h"

namespace Proto {

    void BinaryWriter::BeginPacket(uint16_t id) {
        // �̹� ���� ���̸� ��ø ����
        if (hdrPos_ != npos) {
            // ���� �� �Ǽ� ������
            throw std::logic_error("BeginPacket called while a packet is already open");
        }

        // ��� �ڸ� Ȯ��(4B)
        hdrPos_ = buf_.size();
        buf_.resize(hdrPos_ + kHeaderSize);

        // id ���
        PacketHeader tmp{};
        tmp.size = 0;   // ���� payload ���̸� ��
        tmp.id = id;

        // ��� ������ ���� [size(2)][id(2)]
        std::memcpy(buf_.data() + hdrPos_, &tmp, sizeof(tmp));
    }

    void BinaryWriter::EndPacket() {
        // BeginPacket ���� EndPacket ���� ���
        if (hdrPos_ == npos) {
            throw std::logic_error("EndPacket called without BeginPacket");
        }

        // �� ���� = ���� ���� ���� - ��� ���� ��ġ�� ������
        const size_t total = buf_.size() - hdrPos_;
        if (total > kMaxPacket) {
            // kMaxPacket�� �Ѵ� ��Ŷ ũ��
            throw std::out_of_range("EndPacket: packet too large");
        }
        if (total < kHeaderSize) {
            // ������� ���� ��Ŷ ũ��
            throw std::out_of_range("EndPacket: invalid packet size");
        }

        uint16_t size16 = static_cast<uint16_t>(total);
        std::memcpy(buf_.data() + hdrPos_, &size16, sizeof(uint16_t));

        hdrPos_ = npos;
    }

    void BinaryWriter::WriteHeader(uint16_t id, uint16_t totalSize) {
        // ��� ��� (payload ���̸� �̹� �˰� ���� �� ���)
        if (totalSize < kHeaderSize || totalSize > kMaxPacket) {
            // ��Ŷ ũ��
            throw std::out_of_range("WriteHeader: invalid totalSize");
        }
        PacketHeader h{ totalSize, id };
        const size_t old = buf_.size();
        buf_.resize(old + kHeaderSize);
        std::memcpy(buf_.data() + old, &h, sizeof(h));
    }

    void BinaryWriter::WriteBytes(std::span<const std::byte> bytes) {
        if (bytes.empty()) return;
        const size_t old = buf_.size();
        buf_.resize(old + bytes.size());
        std::memcpy(buf_.data() + old, bytes.data(), bytes.size());
    }

    void BinaryWriter::WriteStringLp16(std::string_view s) {
        if (s.size() > 0xFFFF) {
            // ���� �ʰ� ���ڿ�
            throw std::out_of_range("WriteStringLp16: string too long");
        }
        WriteU16(static_cast<uint16_t>(s.size()));
        if (!s.empty()) {
            const size_t old = buf_.size();
            buf_.resize(old + s.size());
            std::memcpy(buf_.data() + old, s.data(), s.size());
        }
    }

    void BinaryWriter::WriteStringLp32(std::string_view s) {
        WriteU32(static_cast<uint32_t>(s.size()));
        if (!s.empty()) {
            const size_t old = buf_.size();
            buf_.resize(old + s.size());
            std::memcpy(buf_.data() + old, s.data(), s.size());
        }
    }

}
