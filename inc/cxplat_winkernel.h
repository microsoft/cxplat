/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Windows kernel-mode platform definitions.

--*/

#ifndef CXPLAT_WINKERNEL_H
#define CXPLAT_WINKERNEL_H

#include <ntddk.h>

#if defined(__cplusplus)
extern "C" {
#endif

//
// Defines that are present on other platforms but not this one
//

typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;

typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

#define UINT8_MAX   0xffui8
#define UINT16_MAX  0xffffui16
#define UINT32_MAX  0xffffffffui32
#define UINT64_MAX  0xffffffffffffffffui64

//
// Status Codes
//

#define CXPLAT_STATUS                         NTSTATUS
#define CXPLAT_FAILED(X)                      (!NT_SUCCESS(X))
#define CXPLAT_SUCCEEDED(X)                   NT_SUCCESS(X)

#define CXPLAT_STATUS_SUCCESS                 STATUS_SUCCESS                    // 0x0
#define CXPLAT_STATUS_OUT_OF_MEMORY           STATUS_NO_MEMORY                  // 0xc0000017

//
// Code Annotations
//

#ifndef KRTL_INIT_SEGMENT
#define KRTL_INIT_SEGMENT "INIT"
#endif
#ifndef KRTL_PAGE_SEGMENT
#define KRTL_PAGE_SEGMENT "PAGE"
#endif
#ifndef KRTL_NONPAGED_SEGMENT
#define KRTL_NONPAGED_SEGMENT ".text"
#endif

// Use on code in the INIT segment. (Code is discarded after DriverEntry returns.)
#define INITCODE __declspec(code_seg(KRTL_INIT_SEGMENT))

// Use on pageable functions.
#define PAGEDX __declspec(code_seg(KRTL_PAGE_SEGMENT))

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

#define CXPLAT_STATIC_ASSERT(X,Y) static_assert(X,Y)

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
// Allocation/Memory Interfaces
//

#define CXPLAT_ALLOC_PAGED(Size, Tag) ExAllocatePool2(POOL_FLAG_PAGED | POOL_FLAG_UNINITIALIZED, Size, Tag)
#define CXPLAT_ALLOC_NONPAGED(Size, Tag) ExAllocatePool2(POOL_FLAG_NON_PAGED | POOL_FLAG_UNINITIALIZED, Size, Tag)
#define CXPLAT_FREE(Mem, Tag) ExFreePoolWithTag((void*)Mem, Tag)

#define CxPlatZeroMemory RtlZeroMemory
#define CxPlatCopyMemory RtlCopyMemory
#define CxPlatMoveMemory RtlMoveMemory
#define CxPlatSecureZeroMemory RtlSecureZeroMemory

//
// Event Interfaces
//

typedef KEVENT CXPLAT_EVENT;

inline
NTSTATUS
CxPlatInternalEventWaitWithTimeout(
    _In_ CXPLAT_EVENT* Event,
    _In_ uint32_t TimeoutMs
    )
{
    LARGE_INTEGER Timeout100Ns;
    CXPLAT_DBG_ASSERT(TimeoutMs != UINT32_MAX);
    Timeout100Ns.QuadPart = -1 * UInt32x32To64(TimeoutMs, 10000);
    return KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, &Timeout100Ns);
}

#define CxPlatEventInitialize(Event, ManualReset, InitialState) \
    KeInitializeEvent(Event, ManualReset ? NotificationEvent : SynchronizationEvent, InitialState)
#define CxPlatEventUninitialize(Event) UNREFERENCED_PARAMETER(Event)
#define CxPlatEventSet(Event) KeSetEvent(&(Event), IO_NO_INCREMENT, FALSE)
#define CxPlatEventReset(Event) KeResetEvent(&(Event))
#define CxPlatEventWaitForever(Event) \
    KeWaitForSingleObject(&(Event), Executive, KernelMode, FALSE, NULL)
#define CxPlatEventWaitWithTimeout(Event, TimeoutMs) \
    (STATUS_SUCCESS == CxPlatInternalEventWaitWithTimeout(&Event, TimeoutMs))

//
// Time Measurement Interfaces
//

//
// Returns the worst-case system timer resolution (in us).
//
inline
uint64_t
CxPlatGetTimerResolution()
{
    ULONG MaximumTime, MinimumTime, CurrentTime;
    ExQueryTimerResolution(&MaximumTime, &MinimumTime, &CurrentTime);
    return NS100_TO_US(MaximumTime);
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
    return (uint64_t)KeQueryPerformanceCounter(NULL).QuadPart;
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
        ((Low + ((High % 1000000) << 32)) / 1000000);
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
    LARGE_INTEGER SystemTime;
    KeQuerySystemTime(&SystemTime);
    return NS100_TO_MS(SystemTime.QuadPart - UNIX_EPOCH_AS_FILE_TIME);
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

_IRQL_requires_max_(PASSIVE_LEVEL)
inline
void
CxPlatSleep(
    _In_ uint32_t DurationMs
    )
{
    CXPLAT_DBG_ASSERT(DurationMs != (uint32_t)-1);

    KTIMER SleepTimer;
    LARGE_INTEGER TimerValue;

    KeInitializeTimerEx(&SleepTimer, SynchronizationTimer);
    TimerValue.QuadPart = Int32x32To64(DurationMs, -10000);
    KeSetTimer(&SleepTimer, TimerValue, NULL);

    KeWaitForSingleObject(&SleepTimer, Executive, KernelMode, FALSE, NULL);
}

#define CxPlatSchedulerYield() // no-op

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

#endif // CXPLAT_WINKERNEL_H
