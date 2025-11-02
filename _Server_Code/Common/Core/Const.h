#pragma once

#include <cstdint>
#include <chrono>
#include <cstddef>
#include <string_view>
#include <functional>
#include <string>


// 1) 빌드 상수 / 버전 관리
namespace Const {
	// 프로토콜 버전
	inline constexpr uint32_t kProtocolVersion = 1;		// 클라의 패킷 버전과 호환성 체크
	// 빌드 채널 (Dev:개발용 빌드 / Staging:사전검증 빌드 / Prod:운영빌드)
	inline constexpr std::string_view kBuildChannel = "dev";
}



// 2) 단위 / 응답 시간 상수 관리
namespace Const {
	// 서버 루프 주기
	inline constexpr std::chrono::milliseconds kNetTick{ 10 };     // 100Hz (초당 100번)
	// 연결 (connect)이후 응답 제한 시간 (오지 않으면 킥하도록)
	inline constexpr std::chrono::seconds      kHandshakeTimeout{ 5 };
	// 접속 후 로그인 시도 안하면 접속 끊기 (애초에 로그인 / 회원가입 버튼 눌렀을 때 서버와 연결? 고민) 
	inline constexpr std::chrono::seconds kAuthTimeout{ 10 };
	// 플레이 중 이 시간동안 활동 없으면 킥
	inline constexpr std::chrono::seconds kSessionIdleKick{ 300 };


	// 미리 만들어도 안잊어버릴 수 있겠찌..?
	
	// 파티 초대 후 응답 받는 시간
	inline constexpr std::chrono::seconds kPartyInviteTimeout{ 30 };
	// 던전 입장 후 클리어 제한 시간 (일단 던전 하나니까)
	inline constexpr std::chrono::seconds kDungeonClearLimit{ 600 };

	// DB 저장 주기 (일단 1분. 나중에 바꾸기)
	inline constexpr std::chrono::seconds kSaveInterval{ 60 };
}



// 3) 네트워크 공통 값
namespace Const {
	// 1024 미만(Well-known)은 피하고 1024~49151(Registered) 값을 사용
	// 개발/운영 분리가 필요하면 기본값은 Const에 두고 config로 override
	inline constexpr uint16_t kLoginServerPort = 31000;
	inline constexpr uint16_t kMainServerPort = 32000;
	inline constexpr uint16_t kWorldServerPort = 33000;
	// 채팅 서버는 일단 보류
	//inline constexpr uint16_t kChatServerPort = 34000;

	// 최대 접속자 (per-conn 메모리 × kMaxClientsPerWorld)
	// 지금은 500명 정도로 맞춰놓고 나중에 늘리면서 테스트하기
	inline constexpr int      kMaxClientsPerWorld = 500;
	// 이 크기를 넘으면 프로토콜 위반/공격 의심 (용량 많은 메시지 전송시 분할 구조 설계)
	inline constexpr size_t   kMaxPacketSize = 64 * 1024;     // UDP 사용 고려한 64KB 상한
	// 유저 한 명당 수신 링버퍼 크기 (대역폭 × 왕복지연(RTT) × 계수(2~3)) 
	inline constexpr size_t   kRecvBufferSize = 256 * 1024;    // 서버 메모리 계산해보고 수치 바꾸기
	// 재연결 시도 상한 (상한 도달 시 로그 표출 후 연결 포기)
	inline constexpr int      kConnectRetryMax = 5;
}


// 4) 직렬화 / 프로토콜 타입
namespace Const {
	// 네트워크를 기록 할 때 어떤 바이트 순서를 쓸 것인지
	enum class Endian : uint8_t { Little = 0, Big = 1 };
	inline constexpr Endian kWireEndian = Endian::Little; //윈도우 = 리틀 엔디안

	// 프로토콜 번호 타입 (65535개 패킷 만들 수 있다고함)
	using PacketType = uint16_t;   // 얘가 프로토콜 버전과 연관 (수정되면 버전 올려줘야함)
	// 페이로드 길이 타입 (얘보다 더 큰 바이트의 패킷이오면 공격으로 간주)
	using PayloadSize = uint16_t;   // 내가 더 큰 데이터를 보내야하면 청크 분할
	// 공통 해더 바이터 크기 (type(2) + len(2) + crc(4) = 8바이트 / crc : 데이터 전송 중 값이 깨지지 않았는가)
	inline constexpr size_t kHeaderSize = sizeof(PacketType) + sizeof(PayloadSize) + sizeof(uint32_t);
}


// 5) 식별자(UID, 세션ID, 룸ID) 정의
namespace Types {
	// 강타입으로 구조체 통해서 서로 안 섞이게 구분
	struct Uid { 
		uint64_t v{ 0 }; 

		// 연산자 오버로딩
		friend inline bool operator==(Uid a, Uid b) { return a.v == b.v; }
		friend inline bool operator!=(Uid a, Uid b) { return !(a == b); }
		// 값이 유효한지 바로 확인
		inline bool IsValid() const noexcept { return v != 0; }
		// UID 타입으로 감싼 팩토리 (Uid::From(x)로 사용)
		static inline Uid From(uint64_t x) noexcept { return Uid{ x }; }
	};


	struct SessionId { 
		uint64_t v{ 0 }; 

		friend inline bool operator==(SessionId a, SessionId b) { return a.v == b.v; }
		friend inline bool operator!=(SessionId a, SessionId b) { return !(a == b); }
		inline bool IsValid() const noexcept { return v != 0; }
		static inline SessionId From(uint64_t x) noexcept { return SessionId{ x }; }
	
	};

	struct RoomId { 
		uint64_t v{ 0 }; 

		friend inline bool operator==(RoomId a, RoomId b) { return a.v == b.v; }
		friend inline bool operator!=(RoomId a, RoomId b) { return !(a == b); }
		inline bool IsValid() const noexcept { return v != 0; }
		static inline RoomId From(uint64_t x) noexcept { return RoomId{ x }; }
	};
}
// 해시 컨테이너
namespace std {

	template<>
	struct hash<Types::Uid> {
		size_t operator()(const Types::Uid& id) const noexcept {
			return std::hash<uint64_t>{}(id.v);
		}
	};

	template<> 
	struct hash<Types::SessionId> {
		size_t operator()(const Types::SessionId& x) const noexcept {
			return std::hash<uint64_t>{}(x.v);
		}
	};

	template<> 
	struct hash<Types::RoomId> {
		size_t operator()(const Types::RoomId& x) const noexcept {
			return std::hash<uint64_t>{}(x.v);
		}
	};
}

// 클래스 분리
//// 6) 에러/결과 공통 타입
//namespace Types {
//	// 클라 / 서버 등 공통 언어로 소통
//	// 나중에 차차 추가하거나 수정하기
//	enum class ErrorCode : uint16_t {
//		Ok = 0,
//
//		InvalidParam,     // 잘못된 인자/포맷
//		NotFound,         // 리소스/사용자/세션 없음
//		Conflict,         // 상태 충돌(중복 닉네임 등)
//		Unauthorized,     // 인증 안됨/자격 없음
//		Forbidden,        // 권한 부족(인증됐지만 금지됨)
//		Timeout,          // 시간 초과(네트워크/DB/외부)
//		Throttled,        // 레이트리밋/스팸 제한
//		Overflow,         // 용량 초과(버퍼/큐/사이즈 상한)
//		Unavailable,      // 일시적 불가(서버 과부하/정검)
//		Internal          // 서버 내부 오류(예상치 못한 에러)
//	};
//
//	// 성공 실패를 타입으로 강제
//	template<class T>
//	using Result = std::expected<T, ErrorCode>;
//}

// 7~8의 값들 바꾸면서 최적화 하기! (디버그의 성능 프로파일러)
// 7) 스케줄/타이머휠 파라미터
namespace Const {
	// 슬롯 갯수 (단일 휠이 커버하는 범위 = slots × tick)
	inline constexpr int  kTimerWheelSlots = 512;
	// 휠 한 칸 도는 주기
	inline constexpr auto kTimerWheelTick = std::chrono::milliseconds{ 10 };
	// 최소 지연 가능 시간 (너무 짧은 타이머 x)
	inline constexpr auto kTimerMinDelay = std::chrono::milliseconds{ 1 };
	// 최대 지연 가능 시간 (타임 아웃)
	inline constexpr auto kTimerMaxDelay = std::chrono::seconds{ 600 };
}


// 8) 스레딩/작업큐 한도
namespace Const {
	// 네트워크 전용 I/O 스레드 수
	inline constexpr int kIoThreadsDefault = 2;
	// 워커 스레드 갯수
	inline constexpr int kWorkerThreadsPerCPU = 4; // 대부분 논리코어 * 2
	// 작업 큐 최대 길이
	inline constexpr size_t kJobQueueMax = 100000;
}


// 9) RateLimiter 기본값
namespace Const {
	struct RateLimitParam {
		// 버킷에 담아 둘 수 있는 최대 토큰 갯수
		uint32_t capacity;
		// 초당 보충 되는 토큰 갯수
		uint32_t refillPerSec;
	};

	// 사용자 채팅시 5개까지 연속으로 보낼 수 있음 / 그리고 초당 1개씩 회복
	inline constexpr RateLimitParam kChatMsgRL{ .capacity = 5, .refillPerSec = 1 };

	// 로그인 시도 3번 연속으로 가능
	inline constexpr RateLimitParam kLoginReqRL{ .capacity = 3, .refillPerSec = 1 };

	// 또 있을진 모르겠으나 생기면 이어 붙이기
}


// 10) 경로/파일명 규칙
namespace Const {
	// 로그파일 저장 경로
	inline constexpr std::string_view kLogDir = "./logs";
	// 덤프 파일 저장 경로 (충돌/예외 발생 시 생성 하는 파일)
	inline constexpr std::string_view kDumpDir = "./dumps";

	// 로그 파일 최대 크기
	inline constexpr size_t kLogRotateSize = 10 * 1024 * 1024; // 10MB
	// 보관할 로그 파일 갯수 (굳이 5개가 필요할까?)
	inline constexpr int    kLogKeepFiles = 5;
}


// 11) 공통 유틸 타입 별칭
namespace Types {
	// 사용하기 편하게
	using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;
	using Millis = std::chrono::milliseconds;
	using Seconds = std::chrono::seconds;
	using Byte = std::byte;
	using Bytes = std::basic_string<std::byte>;
}


// 12) 안전 가드/계약(Contract) 상수
namespace Const {
	// 사용자/개체 이름 관련 최대 길이
	inline constexpr size_t kMaxNameLen = 32;	// 닉네임
	inline constexpr size_t kMaxRoomTitleLen = 32;   // 방 제목
	inline constexpr size_t kMaxGuildNameLen = 24;   // 길드 이름

	// 메시지 관련
	inline constexpr size_t kMaxChatMsgLen = 256;  // 채팅 메시지
}


