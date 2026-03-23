#include "ErrorCode.h"

namespace Types {

    std::string_view ToString(ErrorCode ec) noexcept {
        switch (ec) {
        case ErrorCode::Ok:           return "Ok";
        case ErrorCode::InvalidParam: return "InvalidParam";
        case ErrorCode::NotFound:     return "NotFound";
        case ErrorCode::Conflict:     return "Conflict";
        case ErrorCode::Unauthorized: return "Unauthorized";
        case ErrorCode::Forbidden:    return "Forbidden";
        case ErrorCode::Timeout:      return "Timeout";
        case ErrorCode::Throttled:    return "Throttled";
        case ErrorCode::Overflow:     return "Overflow";
        case ErrorCode::Unavailable:  return "Unavailable";
        case ErrorCode::Internal:     return "Internal";
        }
        // enum 확장시 default로 빠질 수 있으므로 기본값 설정
        return "Internal";
    }

    // 사용자 잘못 (입력/상태/권한 문제) 사용자에게 수정 요청
    bool IsClientFault(ErrorCode ec) noexcept {
        switch (ec) {
        case ErrorCode::InvalidParam:
        case ErrorCode::NotFound:
        case ErrorCode::Conflict:
        case ErrorCode::Unauthorized:
        case ErrorCode::Forbidden:
        case ErrorCode::Throttled:
            return true;
        default:
            return false;
        }
    }

    // 서버 잘못 (인프라/예상외 오류) 알람/자동회복/재시도 고려
    bool IsServerFault(ErrorCode ec) noexcept {
        switch (ec) {
        case ErrorCode::Timeout:
        case ErrorCode::Unavailable:
        case ErrorCode::Overflow:
        case ErrorCode::Internal:
            return true;
        default:
            return false;
        }
    }

    // 재시도 가치가 있는 에러만 true (네트워크/혼잡/일시 불가)
    bool IsRetryable(ErrorCode ec) noexcept {
        
        switch (ec) {
        case ErrorCode::Timeout:
        case ErrorCode::Unavailable:
        case ErrorCode::Throttled:
            return true;
        default:
            return false;
        }
    }
}