/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    External definition of C99 inline functions.
    See "Clang" / "Language Compatibility" / "C99 inline functions"
    ( https://clang.llvm.org/compatibility.html#inline .)
    It seems that C99 standard requires that every inline function defined
    in a header have a corresponding non-inline definition in a C source file.
    Observed behavior is that Clang is enforcing this, but not MSVC.
    Until an alternative solution is found, this file is required for Clang.

--*/

#include "cxplat.h"

short
InterlockedIncrement16(
    _Inout_ _Interlocked_operand_ short volatile *Addend
    );

short
InterlockedDecrement16(
    _Inout_ _Interlocked_operand_ short volatile *Addend
    );

long
InterlockedIncrement(
    _Inout_ _Interlocked_operand_ long volatile *Addend
    );

long
InterlockedDecrement(
    _Inout_ _Interlocked_operand_ long volatile *Addend
    );

int64_t
InterlockedIncrement64(
    _Inout_ _Interlocked_operand_ int64_t volatile *Addend
    );

int64_t
InterlockedDecrement64(
    _Inout_ _Interlocked_operand_ int64_t volatile *Addend
    );

long
InterlockedAnd(
    _Inout_ _Interlocked_operand_ long volatile *Destination,
    _In_ long Value
    );

long
InterlockedOr(
    _Inout_ _Interlocked_operand_ long volatile *Destination,
    _In_ long Value
    );

short
InterlockedCompareExchange16(
    _Inout_ _Interlocked_operand_ short volatile *Destination,
    _In_ short ExChange,
    _In_ short Comperand
    );

short
InterlockedCompareExchange(
    _Inout_ _Interlocked_operand_ long volatile *Destination,
    _In_ long ExChange,
    _In_ long Comperand
    );

int64_t
InterlockedCompareExchange64(
    _Inout_ _Interlocked_operand_ int64_t volatile *Destination,
    _In_ int64_t ExChange,
    _In_ int64_t Comperand
    );

int64_t
InterlockedExchangeAdd64(
    _Inout_ _Interlocked_operand_ int64_t volatile *Addend,
    _In_ int64_t Value
    );

void*
InterlockedExchangePointer(
    _Inout_ _Interlocked_operand_ void* volatile *Target,
    _In_opt_ void* Value
    );

void*
InterlockedFetchAndClearPointer(
    _Inout_ _Interlocked_operand_ void* volatile *Target
    );

BOOLEAN
InterlockedFetchAndClearBoolean(
    _Inout_ _Interlocked_operand_ BOOLEAN volatile *Target
    );

BOOLEAN
InterlockedFetchAndSetBoolean(
    _Inout_ _Interlocked_operand_ BOOLEAN volatile *Target
    );

int64_t
CxPlatTimeEpochMs64(
    void
    );

uint64_t
CxPlatTimeDiff64(
    _In_ uint64_t T1,
    _In_ uint64_t T2
    );

uint32_t
CxPlatTimeDiff32(
    _In_ uint32_t T1,
    _In_ uint32_t T2
    );

BOOLEAN
CxPlatTimeAtOrBefore64(
    _In_ uint64_t T1,
    _In_ uint64_t T2
    );

BOOLEAN
CxPlatTimeAtOrBefore32(
    _In_ uint32_t T1,
    _In_ uint32_t T2
    );

void
CxPlatEventInitialize(
    _Out_ CXPLAT_EVENT* Event,
    _In_ BOOLEAN ManualReset,
    _In_ BOOLEAN InitialState
    );

void
CxPlatInternalEventUninitialize(
    _Inout_ CXPLAT_EVENT* Event
    );

void
CxPlatInternalEventSet(
    _Inout_ CXPLAT_EVENT* Event
    );

void
CxPlatInternalEventReset(
    _Inout_ CXPLAT_EVENT* Event
    );

void
CxPlatInternalEventWaitForever(
    _Inout_ CXPLAT_EVENT* Event
    );

BOOLEAN
CxPlatInternalEventWaitWithTimeout(
    _Inout_ CXPLAT_EVENT* Event,
    _In_ uint32_t TimeoutMs
    );
