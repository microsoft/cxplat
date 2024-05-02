#include "cxplat.h"
#include "cxplat_trace.h"
#include <bcrypt.h>

typedef struct CX_PLATFORM {
    //
    // Random number algorithm loaded for DISPATCH_LEVEL usage.
    //
    BCRYPT_ALG_HANDLE RngAlgorithm;

#if DEBUG
    //
    // 1/Denominator of allocations to fail.
    // Negative is Nth allocation to fail.
    //
    int32_t AllocFailDenominator;

    //
    // Count of allocations.
    //
    long AllocCounter;
#endif

} CX_PLATFORM;

uint64_t CxPlatPerfFreq;
uint32_t CxPlatProcessorCount;
CX_PLATFORM CxPlatform = { NULL };

PAGEDX
_IRQL_requires_max_(PASSIVE_LEVEL)
CXPLAT_STATUS
CxPlatInitialize(
    void
    )
{
    PAGED_CODE();

    CxPlatProcessorCount =
        (uint32_t)KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    (VOID)KeQueryPerformanceCounter((LARGE_INTEGER*)&CxPlatPerfFreq);

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
void
CxPlatLogAssert(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    )
{
    UNREFERENCED_PARAMETER(File);
    UNREFERENCED_PARAMETER(Line);
    UNREFERENCED_PARAMETER(Expr);
    CxPlatTraceEvent(
        "[ lib] ASSERT, %u:%s - %s.",
        (uint32_t)Line,
        File,
        Expr);
}

#if DEBUG
void
CxPlatSetAllocFailDenominator(
    _In_ int32_t Value
    )
{
    CxPlatform.AllocFailDenominator = Value;
    CxPlatform.AllocCounter = 0;
}

int32_t
CxPlatGetAllocFailDenominator(
    )
{
    return CxPlatform.AllocFailDenominator;
}

#endif

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
