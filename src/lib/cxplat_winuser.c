#include "cxplat_winuser.h"
#include "cxplat_trace.h"
#include <bcrypt.h>

_IRQL_requires_max_(PASSIVE_LEVEL)
CXPLAT_STATUS
CxPlatInitialize(
    void
    )
{
    CxPlatTraceLogInfo(
        "[ dll] Initialized");

    return CXPLAT_STATUS_SUCCESS;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
CxPlatUninitialize(
    void
    )
{
    CxPlatTraceLogInfo(
        "[ dll] Uninitialized");
}

_IRQL_requires_max_(DISPATCH_LEVEL)
CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    )
{
    //
    // Just use the system-preferred random number generator algorithm.
    //
    return (CXPLAT_STATUS)
        BCryptGenRandom(
            NULL,
            (uint8_t*)Buffer,
            BufferLen,
            BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}
