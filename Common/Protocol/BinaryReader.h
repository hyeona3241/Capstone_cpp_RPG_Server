#pragma once
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring> 

#include "PacketDef.h"

// ���߿� IOCP�� ���� ����
// ����ó�� �κп� Logger�߰��ؼ� �α� �����

namespace Proto {

	// Unity C#���� ���� Ŭ��� ���
	// ��Ʋ��������� ����ȭ/������ȭ

	class BinaryReader {
	private: 
		const uint8_t* buffer_ = nullptr;
		size_t size_ = 0;
		size_t offset_ = 0;
		size_t maxStringLen_ = 2048; // �⺻ 2KB ����

	public:
		explicit BinaryReader(const uint8_t* buffer, size_t size, size_t maxStringLen = 2048);

		// ���ο� ���۷� ����
		void   Reset(const uint8_t* buffer, size_t size);

		// ����
		// ���� �б� ��ġ
		size_t Tell()   const noexcept { return offset_; }
		// ���� ���� ����Ʈ ��
		size_t Remain() const noexcept { return (size_ > offset_) ? (size_ - offset_) : 0; }
		// ��������
		bool   End()    const noexcept { return Remain() == 0; }

		PacketHeader ReadHeader();

		// ��� ��Ʋ��������� �ؼ�
		// ����
		uint8_t  ReadUInt8();
		int8_t   ReadInt8();

		uint16_t ReadUInt16();
		int16_t  ReadInt16();

		uint32_t ReadUInt32();
		int32_t  ReadInt32();

		uint64_t ReadUInt64();
		int64_t  ReadInt64();

		// �ε��Ҽ�
		float    ReadFloat();
		double   ReadDouble();

		// ���ڿ�
		std::string ReadString();

		// n ����Ʈ�� ���ͷ� ��ȯ
		std::vector<uint8_t> ReadBytes(size_t n);

		// �̵�/���� (�������� ����/Ȯ�� �� ���� �ʵ带 ��ŵ)
		void Skip(size_t n);

		// ���� ���� ������ (������ ���ڿ�(DoS) ���)
		size_t MaxStringLen() const noexcept { return maxStringLen_; }
		void   SetMaxStringLen(size_t cap) noexcept { maxStringLen_ = cap; }

	private:
		template <class T>
		T readScalarLE(); // ��Ʋ����� POD �б�
	};

}
