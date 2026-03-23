#include "IocpCommon.h"

OverlappedEx* MakeOv(IoType t, void* owner) noexcept {
    // new ½ÇÆÐ ½Ã nullptr
    return new (std::nothrow) OverlappedEx(t, owner);
}

void FreeOv(OverlappedEx* ov) noexcept {
    if (ov) delete ov;
}

bool PostQuitToIocp(HANDLE iocp) noexcept {
    if (!IsValidHandle(iocp)) return false;

    return ::PostQueuedCompletionStatus(iocp, 0, 0, nullptr) == TRUE;
}