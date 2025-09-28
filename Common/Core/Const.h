#pragma once

#include <cstdint>
#include <chrono>
#include <cstddef>
#include <string_view>
#include <functional>
#include <string>


// 1) ���� ��� / ���� ����
namespace Const {
	// �������� ����
	inline constexpr uint32_t kProtocolVersion = 1;		// Ŭ���� ��Ŷ ������ ȣȯ�� üũ
	// ���� ä�� (Dev:���߿� ���� / Staging:�������� ���� / Prod:�����)
	inline constexpr std::string_view kBuildChannel = "dev";
}



// 2) ���� / ���� �ð� ��� ����
namespace Const {
	// ���� ���� �ֱ�
	inline constexpr std::chrono::milliseconds kNetTick{ 10 };     // 100Hz (�ʴ� 100��)
	// ���� (connect)���� ���� ���� �ð� (���� ������ ű�ϵ���)
	inline constexpr std::chrono::seconds      kHandshakeTimeout{ 5 };
	// ���� �� �α��� �õ� ���ϸ� ���� ���� (���ʿ� �α��� / ȸ������ ��ư ������ �� ������ ����? ���) 
	inline constexpr std::chrono::seconds kAuthTimeout{ 10 };
	// �÷��� �� �� �ð����� Ȱ�� ������ ű
	inline constexpr std::chrono::seconds kSessionIdleKick{ 300 };


	// �̸� ���� ���ؾ���� �� �ְ���..?
	
	// ��Ƽ �ʴ� �� ���� �޴� �ð�
	inline constexpr std::chrono::seconds kPartyInviteTimeout{ 30 };
	// ���� ���� �� Ŭ���� ���� �ð� (�ϴ� ���� �ϳ��ϱ�)
	inline constexpr std::chrono::seconds kDungeonClearLimit{ 600 };

	// DB ���� �ֱ� (�ϴ� 1��. ���߿� �ٲٱ�)
	inline constexpr std::chrono::seconds kSaveInterval{ 60 };
}



// 3) ��Ʈ��ũ ���� ��
namespace Const {
	// 1024 �̸�(Well-known)�� ���ϰ� 1024~49151(Registered) ���� ���
	// ����/� �и��� �ʿ��ϸ� �⺻���� Const�� �ΰ� config�� override
	inline constexpr uint16_t kLoginServerPort = 31000;
	inline constexpr uint16_t kMainServerPort = 32000;
	inline constexpr uint16_t kWorldServerPort = 33000;
	// ä�� ������ �ϴ� ����
	//inline constexpr uint16_t kChatServerPort = 34000;

	// �ִ� ������ (per-conn �޸� �� kMaxClientsPerWorld)
	// ������ 500�� ������ ������� ���߿� �ø��鼭 �׽�Ʈ�ϱ�
	inline constexpr int      kMaxClientsPerWorld = 500;
	// �� ũ�⸦ ������ �������� ����/���� �ǽ� (�뷮 ���� �޽��� ���۽� ���� ���� ����)
	inline constexpr size_t   kMaxPacketSize = 64 * 1024;     // UDP ��� ����� 64KB ����
	// ���� �� ��� ���� ������ ũ�� (�뿪�� �� �պ�����(RTT) �� ���(2~3)) 
	inline constexpr size_t   kRecvBufferSize = 256 * 1024;    // ���� �޸� ����غ��� ��ġ �ٲٱ�
	// �翬�� �õ� ���� (���� ���� �� �α� ǥ�� �� ���� ����)
	inline constexpr int      kConnectRetryMax = 5;
}


// 4) ����ȭ / �������� Ÿ��
namespace Const {
	// ��Ʈ��ũ�� ��� �� �� � ����Ʈ ������ �� ������
	enum class Endian : uint8_t { Little = 0, Big = 1 };
	inline constexpr Endian kWireEndian = Endian::Little; //������ = ��Ʋ �����

	// �������� ��ȣ Ÿ�� (65535�� ��Ŷ ���� �� �ִٰ���)
	using PacketType = uint16_t;   // �갡 �������� ������ ���� (�����Ǹ� ���� �÷������)
	// ���̷ε� ���� Ÿ�� (�꺸�� �� ū ����Ʈ�� ��Ŷ�̿��� �������� ����)
	using PayloadSize = uint16_t;   // ���� �� ū �����͸� �������ϸ� ûũ ����
	// ���� �ش� ������ ũ�� (type(2) + len(2) + crc(4) = 8����Ʈ / crc : ������ ���� �� ���� ������ �ʾҴ°�)
	inline constexpr size_t kHeaderSize = sizeof(PacketType) + sizeof(PayloadSize) + sizeof(uint32_t);
}


// 5) �ĺ���(UID, ����ID, ��ID) ����
namespace Types {
	// ��Ÿ������ ����ü ���ؼ� ���� �� ���̰� ����
	struct Uid { 
		uint64_t v{ 0 }; 

		// ������ �����ε�
		friend inline bool operator==(Uid a, Uid b) { return a.v == b.v; }
		friend inline bool operator!=(Uid a, Uid b) { return !(a == b); }
		// ���� ��ȿ���� �ٷ� Ȯ��
		inline bool IsValid() const noexcept { return v != 0; }
		// UID Ÿ������ ���� ���丮 (Uid::From(x)�� ���)
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
// �ؽ� �����̳�
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

// Ŭ���� �и�
//// 6) ����/��� ���� Ÿ��
//namespace Types {
//	// Ŭ�� / ���� �� ���� ���� ����
//	// ���߿� ���� �߰��ϰų� �����ϱ�
//	enum class ErrorCode : uint16_t {
//		Ok = 0,
//
//		InvalidParam,     // �߸��� ����/����
//		NotFound,         // ���ҽ�/�����/���� ����
//		Conflict,         // ���� �浹(�ߺ� �г��� ��)
//		Unauthorized,     // ���� �ȵ�/�ڰ� ����
//		Forbidden,        // ���� ����(���������� ������)
//		Timeout,          // �ð� �ʰ�(��Ʈ��ũ/DB/�ܺ�)
//		Throttled,        // ����Ʈ����/���� ����
//		Overflow,         // �뷮 �ʰ�(����/ť/������ ����)
//		Unavailable,      // �Ͻ��� �Ұ�(���� ������/����)
//		Internal          // ���� ���� ����(����ġ ���� ����)
//	};
//
//	// ���� ���и� Ÿ������ ����
//	template<class T>
//	using Result = std::expected<T, ErrorCode>;
//}

// 7~8�� ���� �ٲٸ鼭 ����ȭ �ϱ�! (������� ���� �������Ϸ�)
// 7) ������/Ÿ�̸��� �Ķ����
namespace Const {
	// ���� ���� (���� ���� Ŀ���ϴ� ���� = slots �� tick)
	inline constexpr int  kTimerWheelSlots = 512;
	// �� �� ĭ ���� �ֱ�
	inline constexpr auto kTimerWheelTick = std::chrono::milliseconds{ 10 };
	// �ּ� ���� ���� �ð� (�ʹ� ª�� Ÿ�̸� x)
	inline constexpr auto kTimerMinDelay = std::chrono::milliseconds{ 1 };
	// �ִ� ���� ���� �ð� (Ÿ�� �ƿ�)
	inline constexpr auto kTimerMaxDelay = std::chrono::seconds{ 600 };
}


// 8) ������/�۾�ť �ѵ�
namespace Const {
	// ��Ʈ��ũ ���� I/O ������ ��
	inline constexpr int kIoThreadsDefault = 2;
	// ��Ŀ ������ ����
	inline constexpr int kWorkerThreadsPerCPU = 4; // ��κ� ���ھ� * 2
	// �۾� ť �ִ� ����
	inline constexpr size_t kJobQueueMax = 100000;
}


// 9) RateLimiter �⺻��
namespace Const {
	struct RateLimitParam {
		// ��Ŷ�� ��� �� �� �ִ� �ִ� ��ū ����
		uint32_t capacity;
		// �ʴ� ���� �Ǵ� ��ū ����
		uint32_t refillPerSec;
	};

	// ����� ä�ý� 5������ �������� ���� �� ���� / �׸��� �ʴ� 1���� ȸ��
	inline constexpr RateLimitParam kChatMsgRL{ .capacity = 5, .refillPerSec = 1 };

	// �α��� �õ� 3�� �������� ����
	inline constexpr RateLimitParam kLoginReqRL{ .capacity = 3, .refillPerSec = 1 };

	// �� ������ �𸣰����� ����� �̾� ���̱�
}


// 10) ���/���ϸ� ��Ģ
namespace Const {
	// �α����� ���� ���
	inline constexpr std::string_view kLogDir = "./logs";
	// ���� ���� ���� ��� (�浹/���� �߻� �� ���� �ϴ� ����)
	inline constexpr std::string_view kDumpDir = "./dumps";

	// �α� ���� �ִ� ũ��
	inline constexpr size_t kLogRotateSize = 10 * 1024 * 1024; // 10MB
	// ������ �α� ���� ���� (���� 5���� �ʿ��ұ�?)
	inline constexpr int    kLogKeepFiles = 5;
}


// 11) ���� ��ƿ Ÿ�� ��Ī
namespace Types {
	// ����ϱ� ���ϰ�
	using Timestamp = std::chrono::time_point<std::chrono::steady_clock>;
	using Millis = std::chrono::milliseconds;
	using Seconds = std::chrono::seconds;
	using Byte = std::byte;
	using Bytes = std::basic_string<std::byte>;
}


// 12) ���� ����/���(Contract) ���
namespace Const {
	// �����/��ü �̸� ���� �ִ� ����
	inline constexpr size_t kMaxNameLen = 32;	// �г���
	inline constexpr size_t kMaxRoomTitleLen = 32;   // �� ����
	inline constexpr size_t kMaxGuildNameLen = 24;   // ��� �̸�

	// �޽��� ����
	inline constexpr size_t kMaxChatMsgLen = 256;  // ä�� �޽���
}


