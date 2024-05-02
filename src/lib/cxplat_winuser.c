#include "cxplat.h"
#include "cxplat_trace.h"
#include <bcrypt.h>

#define CXPLAT_POOL_PROC      '10xC' // Cx01
#define CXPLAT_POOL_TMP_ALLOC '20xC' // Cx02

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
CXPLAT_PROCESSOR_INFO* CxPlatProcessorInfo;
CXPLAT_PROCESSOR_GROUP_INFO* CxPlatProcessorGroupInfo;
uint32_t CxPlatProcessorCount;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
CXPLAT_STATUS
CxPlatGetProcessorGroupInfo(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP Relationship,
    _Outptr_ _At_(*Buffer, __drv_allocatesMem(Mem)) _Pre_defensive_
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Buffer,
    _Out_ PDWORD BufferLength
    )
{
    *BufferLength = 0;
    GetLogicalProcessorInformationEx(Relationship, NULL, BufferLength);
    if (*BufferLength == 0) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %s.",
            "Failed to determine PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX size");
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *Buffer = CXPLAT_ALLOC_NONPAGED(*BufferLength, CXPLAT_POOL_TMP_ALLOC);
    if (*Buffer == NULL) {
        CxPlatTraceEvent(
            "Allocation of '%s' failed. (%llu bytes)",
            "PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX",
            *BufferLength);
        return CXPLAT_STATUS_OUT_OF_MEMORY;
    }

    if (!GetLogicalProcessorInformationEx(
            Relationship,
            *Buffer,
            BufferLength)) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            GetLastError(),
            "GetLogicalProcessorInformationEx failed");
        CXPLAT_FREE(*Buffer, CXPLAT_POOL_TMP_ALLOC);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return CXPLAT_STATUS_SUCCESS;
}

CXPLAT_STATUS
CxPlatProcessorInfoInit(
    void
    )
{
    CXPLAT_STATUS Status = CXPLAT_STATUS_SUCCESS;
    DWORD InfoLength = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* Info = NULL;
    uint32_t ActiveProcessorCount = 0, MaxProcessorCount = 0;
    Status =
        CxPlatGetProcessorGroupInfo(
            RelationGroup,
            &Info,
            &InfoLength);
    if (CXPLAT_FAILED(Status)) {
        goto Error;
    }

    CXPLAT_DBG_ASSERT(InfoLength != 0);
    CXPLAT_DBG_ASSERT(Info->Relationship == RelationGroup);
    CXPLAT_DBG_ASSERT(Info->Group.ActiveGroupCount != 0);
    CXPLAT_DBG_ASSERT(Info->Group.ActiveGroupCount <= Info->Group.MaximumGroupCount);
    if (Info->Group.ActiveGroupCount == 0) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %s.",
            "Failed to determine processor group count");
        Status = CXPLAT_STATUS_NOT_SUPPORTED;
        goto Error;
    }

    for (WORD i = 0; i < Info->Group.ActiveGroupCount; ++i) {
        ActiveProcessorCount += Info->Group.GroupInfo[i].ActiveProcessorCount;
        MaxProcessorCount += Info->Group.GroupInfo[i].MaximumProcessorCount;
    }

    CXPLAT_DBG_ASSERT(ActiveProcessorCount > 0);
    CXPLAT_DBG_ASSERT(ActiveProcessorCount <= UINT16_MAX);
    if (ActiveProcessorCount == 0 || ActiveProcessorCount > UINT16_MAX) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            ActiveProcessorCount,
            "Invalid active processor count");
        Status = CXPLAT_STATUS_NOT_SUPPORTED;
        goto Error;
    }

    CxPlatTraceLogInfo(
        "[ dll] Processors: (%u active, %u max), Groups: (%hu active, %hu max)",
        ActiveProcessorCount,
        MaxProcessorCount,
        Info->Group.ActiveGroupCount,
        Info->Group.MaximumGroupCount);

    CXPLAT_FRE_ASSERT(CxPlatProcessorInfo == NULL);
    CxPlatProcessorInfo =
        CXPLAT_ALLOC_NONPAGED(
            ActiveProcessorCount * sizeof(CXPLAT_PROCESSOR_INFO),
            CXPLAT_POOL_PROC);
    if (CxPlatProcessorInfo == NULL) {
        CxPlatTraceEvent(
            "Allocation of '%s' failed. (%llu bytes)",
            "CxPlatProcessorInfo",
            ActiveProcessorCount * sizeof(CXPLAT_PROCESSOR_INFO));
        Status = CXPLAT_STATUS_OUT_OF_MEMORY;
        goto Error;
    }

    CXPLAT_DBG_ASSERT(CxPlatProcessorGroupInfo == NULL);
    CxPlatProcessorGroupInfo =
        CXPLAT_ALLOC_NONPAGED(
            Info->Group.ActiveGroupCount * sizeof(CXPLAT_PROCESSOR_GROUP_INFO),
            CXPLAT_POOL_PROC);
    if (CxPlatProcessorGroupInfo == NULL) {
        CxPlatTraceEvent(
            "Allocation of '%s' failed. (%llu bytes)",
            "CxPlatProcessorGroupInfo",
            Info->Group.ActiveGroupCount * sizeof(CXPLAT_PROCESSOR_GROUP_INFO));
        Status = CXPLAT_STATUS_OUT_OF_MEMORY;
        goto Error;
    }

    CxPlatProcessorCount = 0;
    for (WORD i = 0; i < Info->Group.ActiveGroupCount; ++i) {
        CxPlatProcessorGroupInfo[i].Mask = Info->Group.GroupInfo[i].ActiveProcessorMask;
        CxPlatProcessorGroupInfo[i].Count = Info->Group.GroupInfo[i].ActiveProcessorCount;
        CxPlatProcessorGroupInfo[i].Offset = CxPlatProcessorCount;
        CxPlatProcessorCount += Info->Group.GroupInfo[i].ActiveProcessorCount;
    }

    for (uint32_t Proc = 0; Proc < ActiveProcessorCount; ++Proc) {
        for (WORD Group = 0; Group < Info->Group.ActiveGroupCount; ++Group) {
            if (Proc >= CxPlatProcessorGroupInfo[Group].Offset &&
                Proc < CxPlatProcessorGroupInfo[Group].Offset + Info->Group.GroupInfo[Group].ActiveProcessorCount) {
                CxPlatProcessorInfo[Proc].Group = Group;
                CxPlatProcessorInfo[Proc].Index = (Proc - CxPlatProcessorGroupInfo[Group].Offset);
                CxPlatTraceLogInfo(
                    "[ dll] Proc[%u] Group[%hu] Index[%u] Active=%hhu",
                    Proc,
                    (uint16_t)Group,
                    CxPlatProcessorInfo[Proc].Index,
                    (uint8_t)!!(CxPlatProcessorGroupInfo[Group].Mask & (1ULL << CxPlatProcessorInfo[Proc].Index)));
                break;
            }
        }
    }

    Status = CXPLAT_STATUS_SUCCESS;

Error:

    if (Info) {
        CXPLAT_FREE(Info, CXPLAT_POOL_TMP_ALLOC);
    }

    if (CXPLAT_FAILED(Status)) {
        if (CxPlatProcessorGroupInfo) {
            CXPLAT_FREE(CxPlatProcessorGroupInfo, CXPLAT_POOL_PROC);
            CxPlatProcessorGroupInfo = NULL;
        }
        if (CxPlatProcessorInfo) {
            CXPLAT_FREE(CxPlatProcessorInfo, CXPLAT_POOL_PROC);
            CxPlatProcessorInfo = NULL;
        }
    }

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
CxPlatProcessorInfoUnInit(
    void
    )
{
    CXPLAT_FREE(CxPlatProcessorGroupInfo, CXPLAT_POOL_PROC);
    CxPlatProcessorGroupInfo = NULL;
    CXPLAT_FREE(CxPlatProcessorInfo, CXPLAT_POOL_PROC);
    CxPlatProcessorInfo = NULL;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
CXPLAT_STATUS
CxPlatInitialize(
    void
    )
{
    CXPLAT_STATUS Status;
    BOOLEAN ProcInfoInitialized = FALSE;

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

    if (CXPLAT_FAILED(Status = CxPlatProcessorInfoInit())) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %s.",
            "CxPlatProcessorInfoInit failed");
        goto Error;
    }
    ProcInfoInitialized = TRUE;

    CxPlatTraceLogInfo(
        "[ dll] Initialized");

    Status = CXPLAT_STATUS_SUCCESS;

Error:

    if (CXPLAT_FAILED(Status)) {
        if (ProcInfoInitialized) {
            CxPlatProcessorInfoUnInit();
        }
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
    CxPlatProcessorInfoUnInit();
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
