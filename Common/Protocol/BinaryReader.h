#pragma once
#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstring> 

#include "PacketDef.h"

// 나중에 IOCP에 맞춰 수정
// 예외처리 부분에 Logger추가해서 로그 남기기

namespace Proto {

	// Unity C#으로 만든 클라와 사용
	// 리틀엔디언으로 직렬화/역직렬화

	class BinaryReader {
	private: 
		const uint8_t* buffer_ = nullptr;
		size_t size_ = 0;
		size_t offset_ = 0;
		size_t maxStringLen_ = 2048; // 기본 2KB 상한

	public:
		explicit BinaryReader(const uint8_t* buffer, size_t size, size_t maxStringLen = 2048);

		// 새로운 버퍼로 재사용
		void   Reset(const uint8_t* buffer, size_t size);

		// 상태
		// 현재 읽기 위치
		size_t Tell()   const noexcept { return offset_; }
		// 아직 남은 바이트 수
		size_t Remain() const noexcept { return (size_ > offset_) ? (size_ - offset_) : 0; }
		// 끝났는지
		bool   End()    const noexcept { return Remain() == 0; }

		PacketHeader ReadHeader();

		// 모두 리틀엔디언으로 해석
		// 정수
		uint8_t  ReadUInt8();
		int8_t   ReadInt8();

		uint16_t ReadUInt16();
		int16_t  ReadInt16();

		uint32_t ReadUInt32();
		int32_t  ReadInt32();

		uint64_t ReadUInt64();
		int64_t  ReadInt64();

		// 부동소수
		float    ReadFloat();
		double   ReadDouble();

		// 문자열
		std::string ReadString();

		// n 바이트를 벡터로 반환
		std::vector<uint8_t> ReadBytes(size_t n);

		// 이동/정렬 (프로토콜 버전/확장 시 이전 필드를 스킵)
		void Skip(size_t n);

		// 길이 상한 접근자 (과도한 문자열(DoS) 방어)
		size_t MaxStringLen() const noexcept { return maxStringLen_; }
		void   SetMaxStringLen(size_t cap) noexcept { maxStringLen_ = cap; }

	private:
		template <class T>
		T readScalarLE(); // 리틀엔디언 POD 읽기
	};

}
