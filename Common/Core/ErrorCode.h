#pragma once

#include <cstdint> 
#include <string_view>

// - ����, Ŭ���̾�Ʈ ��ο��� ���� ���� ���Ǹ� ���� ���� ����
// - enum class �� �Ἥ Ÿ�� ������ �÷��� (�Ϲ��� ���� ��ȯ ����)

namespace Types {

	enum class ErrorCode : uint16_t {
		Ok = 0,			  // ������ �⺻ ��

		InvalidParam,     // �߸��� ����/����
		NotFound,         // ���ҽ�/�����/���� ����
		Conflict,         // ���� �浹(�ߺ� �г��� ��)
		Unauthorized,     // ���� �ȵ�/�ڰ� ����
		Forbidden,        // ���� ����(���������� ������)
		Timeout,          // �ð� �ʰ�(��Ʈ��ũ/DB/�ܺ�)
		Throttled,        // ����Ʈ����/���� ����
		Overflow,         // �뷮 �ʰ�(����/ť/������ ����)
		Unavailable,      // �Ͻ��� �Ұ�(���� ������/����)
		Internal          // ���� ���� ����(����ġ ���� ����)

		// ����� �� �߰�
	};

	// ����� ���� �� �ִ� ���ڿ��� ��ȯ 
	std::string_view ToString(ErrorCode ec) noexcept;


	// ����� �߸�
	bool IsClientFault(ErrorCode ec) noexcept;
	// ���� ����
	bool IsServerFault(ErrorCode ec) noexcept;
	// ��õ� ���� ����? (��õ� ��ġ)
	bool IsRetryable(ErrorCode ec) noexcept;

	// ���� ���� ���� ���� ���� �ڵ����� Ȯ��
	constexpr bool IsValid(ErrorCode ec) noexcept {
		using UT = std::underlying_type_t<ErrorCode>;
		UT v = static_cast<UT>(ec);
		return v >= static_cast<UT>(ErrorCode::Ok)
			&& v <= static_cast<UT>(ErrorCode::Internal);
	}

	// ���� �ڵ带 ��Ŷ�� ���� �� ���� ���� ��ȯ (��Ŷ���� �޾Ƽ� ���� �� �ֵ���?)
	// enum -> uint16_t (���� �� ��ȯ)
	constexpr uint16_t ToWire(ErrorCode ec) noexcept { return static_cast<uint16_t>(ec); }

	// uint16_t -> enum (Ŭ�� �� ��ȯ)
	inline ErrorCode FromWire(uint16_t v) noexcept {
		if (v <= static_cast<uint16_t>(ErrorCode::Internal))
			return static_cast<ErrorCode>(v);
		return ErrorCode::Internal; // ������ ����� ���ο����� ó��
	}
}

