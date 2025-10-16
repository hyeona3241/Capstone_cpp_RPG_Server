#include "BinaryReader.h"

namespace Proto {

    BinaryReader::BinaryReader(const uint8_t* buffer, size_t size, size_t maxStringLen)
        : buffer_(buffer), size_(size), offset_(0), maxStringLen_(maxStringLen)
    {
        if (buffer_ == nullptr || size_ == 0) {
            // �߸��� �ʱ�ȭ�� Reader�� ���� �Ұ����� ��� ����
            throw std::invalid_argument("BinaryReader: buffer is null or size is zero");
        }
    }

    void BinaryReader::Reset(const uint8_t* buffer, size_t size) {
        if (buffer == nullptr || size == 0) {
            // �߸��� ���� ���� �� ����
            throw std::invalid_argument("BinaryReader::Reset: buffer is null or size is zero");
        }
        buffer_ = buffer;
        size_ = size;
        offset_ = 0;
    }

    PacketHeader BinaryReader::ReadHeader() {
        if (Remain() < sizeof(PacketHeader)) {
            // size ���� �ּ�/�ִ� ������ ���
            throw std::out_of_range("ReadHeader: not enough bytes");
        }

        uint16_t size = ReadUInt16();
        if (!IsValidPacketSize(size)) {
            // ��ȿ���� ���� ��Ŷ ũ��
            throw std::out_of_range("ReadHeader: invalid packet size");
        }
        uint16_t id = ReadUInt16();

        PacketHeader h{};
        h.size = size;
        h.id = id;
        return h;
    }

    uint8_t  BinaryReader::ReadUInt8() { return readScalarLE<uint8_t>(); }
    int8_t   BinaryReader::ReadInt8() { return readScalarLE<int8_t>(); }

    uint16_t BinaryReader::ReadUInt16() { return readScalarLE<uint16_t>(); }
    int16_t  BinaryReader::ReadInt16() { return readScalarLE<int16_t>(); }

    uint32_t BinaryReader::ReadUInt32() { return readScalarLE<uint32_t>(); }
    int32_t  BinaryReader::ReadInt32() { return readScalarLE<int32_t>(); }

    uint64_t BinaryReader::ReadUInt64() { return readScalarLE<uint64_t>(); }
    int64_t  BinaryReader::ReadInt64() { return readScalarLE<int64_t>(); }

    float    BinaryReader::ReadFloat() { return readScalarLE<float>(); }
    double   BinaryReader::ReadDouble() { return readScalarLE<double>(); }


    std::string BinaryReader::ReadString() {
        if (Remain() < sizeof(uint16_t)) {
            // �������� ���� �� ����
            throw std::out_of_range("ReadString: not enough bytes for length");
        }
        uint16_t len = ReadUInt16();

        if (len > maxStringLen_) {
            // ���ڿ� ���̰� maxStringLen_ �ʰ�
            throw std::out_of_range("ReadString: length exceeds cap");
        }
        if (Remain() < len) {
            // ���� ����Ʈ ����
            throw std::out_of_range("ReadString: not enough bytes for body");
        }

        std::string s;
        s.resize(len);
        std::memcpy(s.data(), buffer_ + offset_, len);
        offset_ += len;
        return s;
    }

    std::vector<uint8_t> BinaryReader::ReadBytes(size_t n) {
        if (Remain() < n) {
            // ���� ����Ʈ < n
            throw std::out_of_range("ReadBytes: not enough bytes");
        }
        std::vector<uint8_t> out(n);
        std::memcpy(out.data(), buffer_ + offset_, n);
        offset_ += n;
        return out;
    }

    void BinaryReader::Skip(size_t n) {
        if (Remain() < n) {
            // ���� ����Ʈ < n
            throw std::out_of_range("Skip: not enough bytes");
        }
        offset_ += n;
    }


    template <class T>
    T BinaryReader::readScalarLE() {
        static_assert(std::is_trivially_copyable_v<T>, "POD required");
        if (Remain() < sizeof(T)) {
            throw std::out_of_range("readScalarLE: not enough bytes");
        }

        T v{};
        std::memcpy(&v, buffer_ + offset_, sizeof(T));
        offset_ += sizeof(T);
        return v;
    }

    // ����� �ν��Ͻ�ȭ (���Ǵ� �������� �ڵ尡 ����)
    template uint8_t  BinaryReader::readScalarLE<uint8_t>();
    template int8_t   BinaryReader::readScalarLE<int8_t>();
    template uint16_t BinaryReader::readScalarLE<uint16_t>();
    template int16_t  BinaryReader::readScalarLE<int16_t>();
    template uint32_t BinaryReader::readScalarLE<uint32_t>();
    template int32_t  BinaryReader::readScalarLE<int32_t>();
    template uint64_t BinaryReader::readScalarLE<uint64_t>();
    template int64_t  BinaryReader::readScalarLE<int64_t>();
    template float    BinaryReader::readScalarLE<float>();
    template double   BinaryReader::readScalarLE<double>();

} 
