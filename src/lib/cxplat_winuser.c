#include "cxplat.h"
#include "cxplat_trace.h"
#include <bcrypt.h>

typedef struct CX_PLATFORM {

    //
    // Heap used for all allocations.
    //
    HANDLE Heap;

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
CX_PLATFORM CxPlatform = { NULL };

_IRQL_requires_max_(PASSIVE_LEVEL)
CXPLAT_STATUS
CxPlatInitialize(
    void
    )
{
    CXPLAT_STATUS Status;

    (void)QueryPerformanceFrequency((LARGE_INTEGER*)&CxPlatPerfFreq);

    CxPlatform.Heap = HeapCreate(0, 0, 0);
    if (CxPlatform.Heap == NULL) {
        Status = CXPLAT_STATUS_OUT_OF_MEMORY;
        goto Error;
    }

#if DEBUG
    CxPlatform.AllocFailDenominator = 0;
    CxPlatform.AllocCounter = 0;
#endif

    CxPlatTraceLogInfo(
        "[ dll] Initialized");

    Status = CXPLAT_STATUS_SUCCESS;

Error:

    if (CXPLAT_FAILED(Status)) {
        if (CxPlatform.Heap) {
            HeapDestroy(CxPlatform.Heap);
            CxPlatform.Heap = NULL;
        }
    }

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
CxPlatUninitialize(
    void
    )
{
    HeapDestroy(CxPlatform.Heap);
    CxPlatform.Heap = NULL;

    CxPlatTraceLogInfo(
        "[ dll] Uninitialized");
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

#if DEBUG
#define AllocOffset (sizeof(void*) * 2)
#endif

_Ret_maybenull_
_Post_writable_byte_size_(ByteCount)
DECLSPEC_ALLOCATOR
void*
CxPlatAlloc(
    _In_ size_t ByteCount,
    _In_ uint32_t Tag
    )
{
#if DEBUG
    CXPLAT_DBG_ASSERT(CxPlatform.Heap);
    CXPLAT_DBG_ASSERT(ByteCount != 0);
    uint32_t Rand;
    if ((CxPlatform.AllocFailDenominator > 0 && (CxPlatRandom(sizeof(Rand), &Rand), Rand % CxPlatform.AllocFailDenominator) == 1) ||
        (CxPlatform.AllocFailDenominator < 0 && InterlockedIncrement(&CxPlatform.AllocCounter) % CxPlatform.AllocFailDenominator == 0)) {
        return NULL;
    }

    void* Alloc = HeapAlloc(CxPlatform.Heap, 0, ByteCount + AllocOffset);
    if (Alloc == NULL) {
        return NULL;
    }
    *((uint32_t*)Alloc) = Tag;
    return (void*)((uint8_t*)Alloc + AllocOffset);
#else
    UNREFERENCED_PARAMETER(Tag);
    return HeapAlloc(CxPlatform.Heap, 0, ByteCount);
#endif
}

void
CxPlatFree(
    __drv_freesMem(Mem) _Frees_ptr_ void* Mem,
    _In_ uint32_t Tag
    )
{
#if DEBUG
    void* ActualAlloc = (void*)((uint8_t*)Mem - AllocOffset);
    if (Mem != NULL) {
        uint32_t TagToCheck = *((uint32_t*)ActualAlloc);
        CXPLAT_DBG_ASSERT(TagToCheck == Tag);
    } else {
        ActualAlloc = NULL;
    }
    (void)HeapFree(CxPlatform.Heap, 0, ActualAlloc);
#else
    UNREFERENCED_PARAMETER(Tag);
    (void)HeapFree(CxPlatform.Heap, 0, Mem);
#endif
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
