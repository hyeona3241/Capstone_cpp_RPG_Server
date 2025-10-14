#pragma once
#include <cstdint>
#include <type_traits>

// =============================
// ��Ŷ ID Enum ����
// =============================


namespace Proto {

	enum class Test : uint16_t {
		// 0���� �׽�Ʈ ��Ŷ ����
	};

	enum class Main : uint16_t {
		// 1000���� main ��Ŷ ����
	};

	enum class World : uint16_t {
		// 2000���� world ��Ŷ ����
	};


	// ê, �α���, ���� �� ��Ŷ ���� �̾ �ϱ�


	// enum�� uint16_t�� ��ȯ
	template<class E>
	constexpr uint16_t to_id(E e) noexcept {
		static_assert(std::is_enum_v<E>, "enum only");
		return static_cast<uint16_t>(e);
	}
}