#include "cxplat_winkernel.h"
#include "cxplat_trace.h"
#include <bcrypt.h>

typedef struct CX_PLATFORM {
    //
    // Random number algorithm loaded for DISPATCH_LEVEL usage.
    //
    BCRYPT_ALG_HANDLE RngAlgorithm;
} CX_PLATFORM;

CX_PLATFORM CxPlatform = { NULL };

PAGEDX
_IRQL_requires_max_(PASSIVE_LEVEL)
CXPLAT_STATUS
CxPlatInitialize(
    void
    )
{
    PAGED_CODE();

    CXPLAT_STATUS Status =
        BCryptOpenAlgorithmProvider(
            &CxPlatform.RngAlgorithm,
            BCRYPT_RNG_ALGORITHM,
            NULL,
            BCRYPT_PROV_DISPATCH);
    if (CXPLAT_FAILED(Status)) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            Status,
            "BCryptOpenAlgorithmProvider (RNG)");
        goto Error;
    }
    CXPLAT_DBG_ASSERT(CxPlatform.RngAlgorithm != NULL);

    CxPlatTraceLogInfo(
        "[ sys] Initialized");

Error:

    if (CXPLAT_FAILED(Status)) {
        if (CxPlatform.RngAlgorithm != NULL) {
            BCryptCloseAlgorithmProvider(CxPlatform.RngAlgorithm, 0);
            CxPlatform.RngAlgorithm = NULL;
        }
    }

    return Status;
}

PAGEDX
_IRQL_requires_max_(PASSIVE_LEVEL)
void
CxPlatUninitialize(
    void
    )
{
    PAGED_CODE();
    BCryptCloseAlgorithmProvider(CxPlatform.RngAlgorithm, 0);
    CxPlatform.RngAlgorithm = NULL;
    CxPlatTraceLogInfo(
        "[ sys] Uninitialized");
}

_IRQL_requires_max_(DISPATCH_LEVEL)
CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    )
{
    //
    // Use the algorithm we initialized for DISPATCH_LEVEL usage.
    //
    CXPLAT_DBG_ASSERT(CxPlatform.RngAlgorithm != NULL);
    return (CXPLAT_STATUS)
        BCryptGenRandom(
            CxPlatform.RngAlgorithm,
            (uint8_t*)Buffer,
            BufferLen,
            0);
}
