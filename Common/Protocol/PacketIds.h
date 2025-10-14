#pragma once
#include <cstdint>
#include <type_traits>

// =============================
// 패킷 ID Enum 정의
// =============================


namespace Proto {

	enum class Test : uint16_t {
		// 0번대 테스트 패킷 정의
	};

	enum class Main : uint16_t {
		// 1000번대 main 패킷 정의
	};

	enum class World : uint16_t {
		// 2000번대 world 패킷 정의
	};


	// 챗, 로그인, 던전 등 패킷 정의 이어서 하기


	// enum을 uint16_t로 변환
	template<class E>
	constexpr uint16_t to_id(E e) noexcept {
		static_assert(std::is_enum_v<E>, "enum only");
		return static_cast<uint16_t>(e);
	}
}