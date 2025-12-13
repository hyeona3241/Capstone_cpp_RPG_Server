#pragma once
#include <cstdint>
#include <type_traits>

namespace PacketType {
	enum class Basic : uint16_t {
		None = 0,

	};

	enum class Client : uint16_t {
		// 1000번대 Client <-> MainServer
		// 1000 ~ 1099 : 핸드셰이크 / 버전체크 / 기본 설정
		// 1100 ~ 1199 : 로그인 / 계정 관련
		// 1200 ~ 1299 : 로비 / 캐릭터 선택
		// 1300 ~ 1399 : 월드 / 인게임 관련
	};

	enum class Login : uint16_t {
		// 2000번대 LoginServer <-> MainServer
		LS_PING_REQ = 2000,
		LS_PING_ACK = 2001,

		// 2100 ~ 2199 : DB 관련
		LS_DB_FIND_ACCOUNT_REQ = 2100,
		LS_DB_FIND_ACCOUNT_ACK = 2101,

		LS_LOGIN_REQ = 2102,
		LS_LOGIN_ACK = 2103,

	};

	enum class DB : uint16_t {
		// 3000번대 DBServer <-> MainServer
		// 3000 ~ 3099 : 내부 검증 
		DB_PING_REQ = 3000,
		DB_PING_ACK = 3001,

		// 3100 ~ 3199 : 로그인 관련
		DB_FIND_ACCOUNT_REQ = 3100,
		DB_FIND_ACCOUNT_ACK = 3101,

		// 그 이후 로직 추가하기

	};

	// enum을 uint16_t로 변환
	template<class E>
	constexpr uint16_t to_id(E e) noexcept {
		static_assert(std::is_enum_v<E>, "enum only");
		return static_cast<uint16_t>(e);
	}

}