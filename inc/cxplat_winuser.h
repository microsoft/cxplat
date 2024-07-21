/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Windows user-mode platform definitions.

--*/

#ifndef CXPLAT_WINUSER_H
#define CXPLAT_WINUSER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <winternl.h>

#if defined(__cplusplus)
extern "C" {
#endif

//
// Status Codes
//

#define CXPLAT_STATUS                         HRESULT
#define CXPLAT_FAILED(X)                      FAILED(X)
#define CXPLAT_SUCCEEDED(X)                   SUCCEEDED(X)

#define CXPLAT_STATUS_SUCCESS                 S_OK                              // 0x0
#define CXPLAT_STATUS_OUT_OF_MEMORY           E_OUTOFMEMORY                     // 0x8007000e
#define CXPLAT_STATUS_NOT_SUPPORTED           E_NOINTERFACE                     // 0x80004002

//
// Code Annotations
//

#define INITCODE
#define PAGEDX

//
// Static Analysis Interfaces
//

#define CXPLAT_NO_SANITIZE(X)

#if defined(_PREFAST_)
// _Analysis_assume_ will never result in any code generation for _exp,
// so using it will not have runtime impact, even if _exp has side effects.
#define CXPLAT_ANALYSIS_ASSUME(_exp) _Analysis_assume_(_exp)
#else // _PREFAST_
// CXPLAT_ANALYSIS_ASSUME ensures that _exp is parsed in non-analysis compile.
// On DEBUG, it's guaranteed to be parsed as part of the normal compile, but
// with non-DEBUG, use __noop to ensure _exp is parseable but without code
// generation.
#if DEBUG
#define CXPLAT_ANALYSIS_ASSUME(_exp) ((void) 0)
#else // DEBUG
#define CXPLAT_ANALYSIS_ASSUME(_exp) __noop(_exp)
#endif // DEBUG
#endif // _PREFAST_

#define CXPLAT_ANALYSIS_ASSERT(X) __analysis_assert(X)

//
// Assertion Interfaces
//

_IRQL_requires_max_(DISPATCH_LEVEL)
void
CxPlatLogAssert(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    );

#define CXPLAT_WIDE_STRING(_str) L##_str

#define CXPLAT_ASSERT_NOOP(_exp, _msg) \
    (CXPLAT_ANALYSIS_ASSUME(_exp), 0)

#define CXPLAT_ASSERT_CRASH(_exp, _msg) \
    (CXPLAT_ANALYSIS_ASSUME(_exp), \
    ((!(_exp)) ? \
        (CxPlatLogAssert(__FILE__, __LINE__, #_exp), \
         __annotation(L"Debug", L"AssertFail", _msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE))

#ifdef __clang__
#define CXPLAT_STATIC_ASSERT(X,Y) _Static_assert(X,Y)
#else
#define CXPLAT_STATIC_ASSERT(X,Y) static_assert(X,Y)
#endif

#define CXPLAT_FRE_ASSERT(_exp)          CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(#_exp))
#define CXPLAT_FRE_ASSERTMSG(_exp, _msg) CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(_msg))

#if DEBUG
#define CXPLAT_DBG_ASSERT(_exp)          CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(#_exp))
#define CXPLAT_DBG_ASSERTMSG(_exp, _msg) CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(_msg))
#else
#define CXPLAT_DBG_ASSERT(_exp)          CXPLAT_ASSERT_NOOP(_exp, CXPLAT_WIDE_STRING(#_exp))
#define CXPLAT_DBG_ASSERTMSG(_exp, _msg) CXPLAT_ASSERT_NOOP(_exp, CXPLAT_WIDE_STRING(_msg))
#endif

//
// Wrapper functions
//

//
// CloseHandle has an incorrect SAL annotation, so call through a wrapper.
//
_IRQL_requires_max_(PASSIVE_LEVEL)
inline
void
CxPlatCloseHandle(_Pre_notnull_ HANDLE Handle) {
    CloseHandle(Handle);
}

//
// Allocation/Memory Interfaces
//

_Ret_maybenull_
_Post_writable_byte_size_(ByteCount)
DECLSPEC_ALLOCATOR
void*
CxPlatAlloc(
    _In_ size_t ByteCount,
    _In_ uint32_t Tag
    );

void
CxPlatFree(
    __drv_freesMem(Mem) _Frees_ptr_ void* Mem,
    _In_ uint32_t Tag
    );

#define CXPLAT_ALLOC_PAGED(Size, Tag) CxPlatAlloc(Size, Tag)
#define CXPLAT_ALLOC_NONPAGED(Size, Tag) CxPlatAlloc(Size, Tag)
#define CXPLAT_FREE(Mem, Tag) CxPlatFree((void*)Mem, Tag)

#define CxPlatZeroMemory RtlZeroMemory
#define CxPlatCopyMemory RtlCopyMemory
#define CxPlatMoveMemory RtlMoveMemory
#define CxPlatSecureZeroMemory RtlSecureZeroMemory

//
// Interrupt ReQuest Level
//

#define CXPLAT_IRQL() PASSIVE_LEVEL

#define CXPLAT_PASSIVE_CODE() CXPLAT_DBG_ASSERT(CXPLAT_IRQL() == PASSIVE_LEVEL)
#define CXPLAT_AT_DISPATCH() FALSE

//
// Locking interfaces
//

typedef CRITICAL_SECTION CXPLAT_LOCK;

#define CxPlatLockInitialize(Lock) InitializeCriticalSection(Lock)
#define CxPlatLockUninitialize(Lock) DeleteCriticalSection(Lock)
#define CxPlatLockAcquire(Lock) EnterCriticalSection(Lock)
#define CxPlatLockRelease(Lock) LeaveCriticalSection(Lock)

typedef CRITICAL_SECTION CXPLAT_DISPATCH_LOCK;

#define CxPlatDispatchLockInitialize(Lock) InitializeCriticalSection(Lock)
#define CxPlatDispatchLockUninitialize(Lock) DeleteCriticalSection(Lock)
#define CxPlatDispatchLockAcquire(Lock) EnterCriticalSection(Lock)
#define CxPlatDispatchLockRelease(Lock) LeaveCriticalSection(Lock)

typedef SRWLOCK CXPLAT_RW_LOCK;

#define CxPlatRwLockInitialize(Lock) InitializeSRWLock(Lock)
#define CxPlatRwLockUninitialize(Lock)
#define CxPlatRwLockAcquireShared(Lock) AcquireSRWLockShared(Lock)
#define CxPlatRwLockAcquireExclusive(Lock) AcquireSRWLockExclusive(Lock)
#define CxPlatRwLockReleaseShared(Lock) ReleaseSRWLockShared(Lock)
#define CxPlatRwLockReleaseExclusive(Lock) ReleaseSRWLockExclusive(Lock)

typedef SRWLOCK CXPLAT_DISPATCH_RW_LOCK;

#define CxPlatDispatchRwLockInitialize(Lock) InitializeSRWLock(Lock)
#define CxPlatDispatchRwLockUninitialize(Lock)
#define CxPlatDispatchRwLockAcquireShared(Lock) AcquireSRWLockShared(Lock)
#define CxPlatDispatchRwLockAcquireExclusive(Lock) AcquireSRWLockExclusive(Lock)
#define CxPlatDispatchRwLockReleaseShared(Lock) ReleaseSRWLockShared(Lock)
#define CxPlatDispatchRwLockReleaseExclusive(Lock) ReleaseSRWLockExclusive(Lock)

//
// Time Measurement Interfaces
//

#ifdef CXPLAT_UWP_BUILD
WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
GetSystemTimeAdjustment(
    _Out_ PDWORD lpTimeAdjustment,
    _Out_ PDWORD lpTimeIncrement,
    _Out_ PBOOL lpTimeAdjustmentDisabled
    );
#endif

//
// Returns the worst-case system timer resolution (in us).
//
inline
uint64_t
CxPlatGetTimerResolution()
{
    DWORD Adjustment, Increment;
    BOOL AdjustmentDisabled;
    GetSystemTimeAdjustment(&Adjustment, &Increment, &AdjustmentDisabled);
    return NS100_TO_US(Increment);
}

//
// Performance counter frequency.
//
extern uint64_t CxPlatPerfFreq;

//
// Returns the current time in platform specific time units.
//
inline
uint64_t
CxPlatTimePlat(
    void
    )
{
    uint64_t Count;
    QueryPerformanceCounter((LARGE_INTEGER*)&Count);
    return Count;
}

//
// Converts platform time to microseconds.
//
inline
uint64_t
CxPlatTimePlatToUs64(
    uint64_t Count
    )
{
    //
    // Multiply by a big number (1000000, to convert seconds to microseconds)
    // and divide by a big number (CxPlatPerfFreq, to convert counts to secs).
    //
    // Avoid overflow with separate multiplication/division of the high and low
    // bits. Taken from TcpConvertPerformanceCounterToMicroseconds.
    //
    uint64_t High = (Count >> 32) * 1000000;
    uint64_t Low = (Count & 0xFFFFFFFF) * 1000000;
    return
        ((High / CxPlatPerfFreq) << 32) +
        ((Low + ((High % CxPlatPerfFreq) << 32)) / CxPlatPerfFreq);
}

//
// Converts microseconds to platform time.
//
inline
uint64_t
CxPlatTimeUs64ToPlat(
    uint64_t TimeUs
    )
{
    uint64_t High = (TimeUs >> 32) * CxPlatPerfFreq;
    uint64_t Low = (TimeUs & 0xFFFFFFFF) * CxPlatPerfFreq;
    return
        ((High / 1000000) << 32) +
        ((Low + ((High % 1000000) << 32)) / CxPlatPerfFreq);
}

#define CxPlatTimeUs64() CxPlatTimePlatToUs64(CxPlatTimePlat())
#define CxPlatTimeUs32() (uint32_t)CxPlatTimeUs64()
#define CxPlatTimeMs64() US_TO_MS(CxPlatTimeUs64())
#define CxPlatTimeMs32() (uint32_t)CxPlatTimeMs64()

#define UNIX_EPOCH_AS_FILE_TIME 0x19db1ded53e8000ll

inline
int64_t
CxPlatTimeEpochMs64(
    )
{
    LARGE_INTEGER FileTime;
    GetSystemTimeAsFileTime((FILETIME*) &FileTime);
    return NS100_TO_MS(FileTime.QuadPart - UNIX_EPOCH_AS_FILE_TIME);
}

//
// Returns the difference between two timestamps.
//
inline
uint64_t
CxPlatTimeDiff64(
    _In_ uint64_t T1,     // First time measured
    _In_ uint64_t T2      // Second time measured
    )
{
    //
    // Assume no wrap around.
    //
    return T2 - T1;
}

//
// Returns the difference between two timestamps.
//
inline
uint32_t
CxPlatTimeDiff32(
    _In_ uint32_t T1,     // First time measured
    _In_ uint32_t T2      // Second time measured
    )
{
    if (T2 > T1) {
        return T2 - T1;
    } else { // Wrap around case.
        return T2 + (0xFFFFFFFF - T1) + 1;
    }
}

//
// Returns TRUE if T1 came before T2.
//
inline
BOOLEAN
CxPlatTimeAtOrBefore64(
    _In_ uint64_t T1,
    _In_ uint64_t T2
    )
{
    //
    // Assume no wrap around.
    //
    return T1 <= T2;
}

//
// Returns TRUE if T1 came before T2.
//
inline
BOOLEAN
CxPlatTimeAtOrBefore32(
    _In_ uint32_t T1,
    _In_ uint32_t T2
    )
{
    return (int32_t)(T1 - T2) <= 0;
}

#define CxPlatSleep(ms) Sleep(ms)

#define CxPlatSchedulerYield() Sleep(0)


//
// Event Interfaces
//

typedef HANDLE CXPLAT_EVENT;

#define CxPlatEventInitialize(Event, ManualReset, InitialState)     \
    *(Event) = CreateEvent(NULL, ManualReset, InitialState, NULL);  \
    CXPLAT_DBG_ASSERT(*Event != NULL)
#define CxPlatEventUninitialize(Event) CxPlatCloseHandle(Event)
#define CxPlatEventSet(Event) SetEvent(Event)
#define CxPlatEventReset(Event) ResetEvent(Event)
#define CxPlatEventWaitForever(Event) WaitForSingleObject(Event, INFINITE)
inline
BOOLEAN
CxPlatEventWaitWithTimeout(
    _In_ CXPLAT_EVENT Event,
    _In_ uint32_t TimeoutMs
    )
{
    CXPLAT_DBG_ASSERT(TimeoutMs != UINT32_MAX);
    return WAIT_OBJECT_0 == WaitForSingleObject(Event, TimeoutMs);
}

//
// Processor Interfaces
//

typedef struct CXPLAT_PROCESSOR_INFO {
    uint32_t Index;  // Index in the current group
    uint16_t Group;  // The group number this processor is a part of
} CXPLAT_PROCESSOR_INFO;

typedef struct CXPLAT_PROCESSOR_GROUP_INFO {
    KAFFINITY Mask;  // Bit mask of active processors in the group
    uint32_t Count;  // Count of active processors in the group
    uint32_t Offset; // Base process index offset this group starts at
} CXPLAT_PROCESSOR_GROUP_INFO;

extern CXPLAT_PROCESSOR_INFO* CxPlatProcessorInfo;
extern CXPLAT_PROCESSOR_GROUP_INFO* CxPlatProcessorGroupInfo;

extern uint32_t CxPlatProcessorCount;
#define CxPlatProcCount() CxPlatProcessorCount

_IRQL_requires_max_(DISPATCH_LEVEL)
inline
uint32_t
CxPlatProcCurrentNumber(
    void
    ) {
    PROCESSOR_NUMBER ProcNumber;
    GetCurrentProcessorNumberEx(&ProcNumber);
    const CXPLAT_PROCESSOR_GROUP_INFO* Group = &CxPlatProcessorGroupInfo[ProcNumber.Group];
    return Group->Offset + (ProcNumber.Number % Group->Count);
}

//
// Thread Interfaces
//

//
// This is the undocumented interface for setting a thread's name. This is
// essentially what SetThreadDescription does, but that is not available in
// older versions of Windows. These API's are suffixed _PRIVATE in order
// to not colide with the built in windows definitions, which are not gated
// behind any preprocessor macros
//
#if !defined(CXPLAT_RESTRICTED_BUILD)
#define ThreadNameInformationPrivate ((THREADINFOCLASS)38)

typedef struct _THREAD_NAME_INFORMATION_PRIVATE {
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION_PRIVATE, *PTHREAD_NAME_INFORMATION_PRIVATE;

__kernel_entry
NTSTATUS
NTAPI
NtSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );
#endif

#ifdef CXPLAT_UWP_BUILD
WINBASEAPI
BOOL
WINAPI
SetThreadGroupAffinity(
    _In_ HANDLE hThread,
    _In_ CONST GROUP_AFFINITY* GroupAffinity,
    _Out_opt_ PGROUP_AFFINITY PreviousGroupAffinity
    );
#endif

typedef struct CXPLAT_THREAD_CONFIG {
    uint16_t Flags;
    uint16_t IdealProcessor;
    _Field_z_ const char* Name;
    LPTHREAD_START_ROUTINE Callback;
    void* Context;
} CXPLAT_THREAD_CONFIG;

typedef HANDLE CXPLAT_THREAD;
#define CXPLAT_THREAD_CALLBACK(FuncName, CtxVarName)  \
    DWORD                                           \
    WINAPI                                          \
    FuncName(                                       \
      _In_ void* CtxVarName                         \
      )

#define CXPLAT_THREAD_RETURN(Status) return (DWORD)(Status)

#ifdef CXPLAT_USE_CUSTOM_THREAD_CONTEXT

//
// Extension point that allows additional platform specific logic to be executed
// for every thread created. The platform must define CXPLAT_USE_CUSTOM_THREAD_CONTEXT
// and implement the CxPlatThreadCustomStart function. CxPlatThreadCustomStart MUST
// call the Callback passed in. CxPlatThreadCustomStart MUST also free
// CustomContext (via CXPLAT_FREE(CustomContext, CXPLAT_POOL_CUSTOM_THREAD)) before
// returning.
//

typedef struct CXPLAT_THREAD_CUSTOM_CONTEXT {
    LPTHREAD_START_ROUTINE Callback;
    void* Context;
} CXPLAT_THREAD_CUSTOM_CONTEXT;

CXPLAT_THREAD_CALLBACK(CxPlatThreadCustomStart, CustomContext); // CXPLAT_THREAD_CUSTOM_CONTEXT* CustomContext

#endif // CXPLAT_USE_CUSTOM_THREAD_CONTEXT

inline
CXPLAT_STATUS
CxPlatThreadCreate(
    _In_ CXPLAT_THREAD_CONFIG* Config,
    _Out_ CXPLAT_THREAD* Thread
    )
{
#ifdef CXPLAT_USE_CUSTOM_THREAD_CONTEXT
    CXPLAT_THREAD_CUSTOM_CONTEXT* CustomContext =
        CXPLAT_ALLOC_NONPAGED(sizeof(CXPLAT_THREAD_CUSTOM_CONTEXT), CXPLAT_POOL_CUSTOM_THREAD);
    if (CustomContext == NULL) {
        return CXPLAT_STATUS_OUT_OF_MEMORY;
    }
    CustomContext->Callback = Config->Callback;
    CustomContext->Context = Config->Context;
    *Thread =
        CreateThread(
            NULL,
            0,
            CxPlatThreadCustomStart,
            CustomContext,
            0,
            NULL);
    if (*Thread == NULL) {
        CXPLAT_FREE(CustomContext, CXPLAT_POOL_CUSTOM_THREAD);
        return GetLastError();
    }
#else // CXPLAT_USE_CUSTOM_THREAD_CONTEXT
    *Thread =
        CreateThread(
            NULL,
            0,
            Config->Callback,
            Config->Context,
            0,
            NULL);
    if (*Thread == NULL) {
        return GetLastError();
    }
#endif // CXPLAT_USE_CUSTOM_THREAD_CONTEXT
    CXPLAT_DBG_ASSERT(Config->IdealProcessor < CxPlatProcCount());
    const CXPLAT_PROCESSOR_INFO* ProcInfo = &CxPlatProcessorInfo[Config->IdealProcessor];
    GROUP_AFFINITY Group = {0};
    if (Config->Flags & CXPLAT_THREAD_FLAG_SET_AFFINITIZE) {
        Group.Mask = (KAFFINITY)(1ull << ProcInfo->Index);          // Fixed processor
    } else {
        Group.Mask = CxPlatProcessorGroupInfo[ProcInfo->Group].Mask;
    }
    Group.Group = ProcInfo->Group;
    SetThreadGroupAffinity(*Thread, &Group, NULL);
    if (Config->Flags & CXPLAT_THREAD_FLAG_SET_IDEAL_PROC) {
        SetThreadIdealProcessor(*Thread, ProcInfo->Index);
    }
    if (Config->Flags & CXPLAT_THREAD_FLAG_HIGH_PRIORITY) {
        SetThreadPriority(*Thread, THREAD_PRIORITY_HIGHEST);
    }
    if (Config->Name) {
        WCHAR WideName[64] = L"";
        size_t WideNameLength;
        mbstowcs_s(
            &WideNameLength,
            WideName,
            ARRAYSIZE(WideName) - 1,
            Config->Name,
            _TRUNCATE);
#if defined(CXPLAT_RESTRICTED_BUILD)
        SetThreadDescription(*Thread, WideName);
#else
        THREAD_NAME_INFORMATION_PRIVATE ThreadNameInfo;
        RtlInitUnicodeString(&ThreadNameInfo.ThreadName, WideName);
        NtSetInformationThread(
            *Thread,
            ThreadNameInformationPrivate,
            &ThreadNameInfo,
            sizeof(ThreadNameInfo));
#endif
    }
    return CXPLAT_STATUS_SUCCESS;
}
#define CxPlatThreadDelete(Thread) CxPlatCloseHandle(*(Thread))
#define CxPlatThreadWaitForever(Thread) WaitForSingleObject(*(Thread), INFINITE)
inline
BOOLEAN
CxPlatThreadWaitWithTimeout(
    _In_ CXPLAT_THREAD* Thread,
    _In_ uint32_t TimeoutMs
    )
{
    CXPLAT_DBG_ASSERT(TimeoutMs != UINT32_MAX);
    return WAIT_OBJECT_0 == WaitForSingleObject(*Thread, TimeoutMs);
}
typedef uint32_t CXPLAT_THREAD_ID;
#define CxPlatCurThreadID() GetCurrentThreadId()

//
// Crypto Interfaces
//

_IRQL_requires_max_(DISPATCH_LEVEL)
CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    );

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_WINUSER_H
