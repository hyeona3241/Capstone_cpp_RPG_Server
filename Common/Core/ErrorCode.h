#pragma once

#include <cstdint> 
#include <string_view>

// - 서버, 클라이언트 모두에서 같은 에러 정의를 쓰기 위해 정의
// - enum class 를 써서 타입 안전성 올려줌 (암묵적 정수 변환 방지)

namespace Types {

	enum class ErrorCode : uint16_t {
		Ok = 0,			  // 성공한 기본 값

		InvalidParam,     // 잘못된 인자/포맷
		NotFound,         // 리소스/사용자/세션 없음
		Conflict,         // 상태 충돌(중복 닉네임 등)
		Unauthorized,     // 인증 안됨/자격 없음
		Forbidden,        // 권한 부족(인증됐지만 금지됨)
		Timeout,          // 시간 초과(네트워크/DB/외부)
		Throttled,        // 레이트리밋/스팸 제한
		Overflow,         // 용량 초과(버퍼/큐/사이즈 상한)
		Unavailable,      // 일시적 불가(서버 과부하/정검)
		Internal          // 서버 내부 오류(예상치 못한 에러)

		// 생기면 더 추가
	};

	// 사람이 읽을 수 있는 문자열로 변환 
	std::string_view ToString(ErrorCode ec) noexcept;


	// 사용자 잘못
	bool IsClientFault(ErrorCode ec) noexcept;
	// 서버 문제
	bool IsServerFault(ErrorCode ec) noexcept;
	// 재시도 가능 여부? (재시도 가치)
	bool IsRetryable(ErrorCode ec) noexcept;

	// 내가 만든 범위 내의 에러 코드인지 확인
	constexpr bool IsValid(ErrorCode ec) noexcept {
		using UT = std::underlying_type_t<ErrorCode>;
		UT v = static_cast<UT>(ec);
		return v >= static_cast<UT>(ErrorCode::Ok)
			&& v <= static_cast<UT>(ErrorCode::Internal);
	}

	// 에러 코드를 패킷에 담을 때 쓰기 쉽게 변환 (패킷에서 받아서 읽을 수 있도록?)
	// enum -> uint16_t (서버 쪽 변환)
	constexpr uint16_t ToWire(ErrorCode ec) noexcept { return static_cast<uint16_t>(ec); }

	// uint16_t -> enum (클라 쪽 변환)
	inline ErrorCode FromWire(uint16_t v) noexcept {
		if (v <= static_cast<uint16_t>(ErrorCode::Internal))
			return static_cast<ErrorCode>(v);
		return ErrorCode::Internal; // 범위를 벗어나면 내부오류로 처리
	}
}

