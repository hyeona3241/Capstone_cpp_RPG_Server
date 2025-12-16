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
		C_PING_REQ = 1000,
		C_PING_ACK = 1001,

		S_PING_REQ = 1002,
		S_PING_ACK = 1003,

		// 1100 ~ 1199 : 로그인 / 계정 관련
		LOGIN_REQ = 1100,
		LOGIN_ACK = 1101,

		REGISTER_REQ = 1102,
		REGISTER_ACK = 1103,

		LOGOUT_REQ = 1104,
		
		// 1200 ~ 1299 : 채팅 관련
		MS_CL_CHAT_CONNECT_INFO_ACK = 1200,

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

		LS_LOGOUT_NOTIFY = 2104,
	};

	enum class DB : uint16_t {
		// 3000번대 DBServer <-> MainServer
		// 3000 ~ 3099 : 내부 검증 
		DB_PING_REQ = 3000,
		DB_PING_ACK = 3001,

		// 3100 ~ 3199 : 로그인 관련
		DB_FIND_ACCOUNT_REQ = 3100,
		DB_FIND_ACCOUNT_ACK = 3101,

		DB_UPDATE_LASTLOGIN_REQ = 3102,
		/*DB_UPDATE_LASTLOGIN_ACK = 3103,*/

		DB_REGISTER_REQ = 3104,
		DB_REGISTER_ACK = 3105,

		// 그 이후 로직 추가하기

	};

	enum class Chat : uint16_t {
		// 4000번대 MainServer <-> ChatServer <-> Client
		// 4000 ~ 4099 : 내부 검증

		CS_MS_PING_REQ = 4000,
		CS_MS_PING_ACK = 4001,

		CS_C_PING_REQ = 4002,
		CS_C_PING_ACK = 4003,

		// 4100 ~ 4199 : 인증/세션 바인딩
		MS_CS_CHAT_ALLOW_TOKEN_REQ = 4100,

		CL_CS_CHAT_AUTH_REQ = 4101,
		CS_CL_CHAT_AUTH_ACK = 4102,

		// 4200 ~ 4299 : 채널(목록/입장/퇴장/닫기)

		CL_CS_CHANNEL_LIST_REQ = 4200,
		CS_CL_CHANNEL_LIST_ACK = 4201,

		CL_CS_CHANNEL_ENTER_REQ = 4202,
		CS_CL_CHANNEL_ENTER_ACK = 4203,

		CL_CS_CHANNEL_LEAVE_REQ = 4204,
		CS_CL_CHANNEL_LEAVE_ACK = 4205,


		// 4300 ~ 4399 : 채팅 메시지(채널 메시지/귓속말 등)
		CL_CS_CHAT_SEND_REQ = 4300,
		CS_CL_CHAT_BROADCAST_NFY = 4301,

	};

	// enum을 uint16_t로 변환
	template<class E>
	constexpr uint16_t to_id(E e) noexcept {
		static_assert(std::is_enum_v<E>, "enum only");
		return static_cast<uint16_t>(e);
	}

}