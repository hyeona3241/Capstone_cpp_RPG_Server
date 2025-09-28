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
        // enum Ȯ��� default�� ���� �� �����Ƿ� �⺻�� ����
        return "Internal";
    }

    // ����� �߸� (�Է�/����/���� ����) ����ڿ��� ���� ��û
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

    // ���� �߸� (������/����� ����) �˶�/�ڵ�ȸ��/��õ� ���
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

    // ��õ� ��ġ�� �ִ� ������ true (��Ʈ��ũ/ȥ��/�Ͻ� �Ұ�)
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