/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Posix platform definitions.

--*/

#ifndef CXPLAT_POSIX_H
#define CXPLAT_POSIX_H

// For FreeBSD
#if defined(__FreeBSD__)
#include <sys/socket.h>
#include <netinet/in.h>
#define ETIME   ETIMEDOUT
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdalign.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include "cxplat_sal_stub.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Status Codes
//

#define CXPLAT_STATUS                         unsigned int
#define CXPLAT_FAILED(X)                      ((int)(X) > 0)
#define CXPLAT_SUCCEEDED(X)                   ((int)(X) <= 0)

#define CXPLAT_STATUS_SUCCESS                 ((CXPLAT_STATUS)0)                // 0
#define CXPLAT_STATUS_OUT_OF_MEMORY           ((CXPLAT_STATUS)ENOMEM)           // 12
#define CXPLAT_STATUS_NOT_SUPPORTED           ((CXPLAT_STATUS)EOPNOTSUPP)       // 95   (102 on macOS)

//
// Code Annotations
//

#define INITCODE
#define PAGEDX

//
// Static Analysis Interfaces
//

#if defined(__clang__)
#define CXPLAT_NO_SANITIZE(X) __attribute__((no_sanitize(X)))
#else
#define CXPLAT_NO_SANITIZE(X)
#endif

#define CXPLAT_ANALYSIS_ASSERT(X)
#define CXPLAT_ANALYSIS_ASSUME(X)

//
// Assertion Interfaces
//

__attribute__((noinline, noreturn))
void
cxplat_bugcheck(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    );

void
CxPlatLogAssert(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    );

#define CXPLAT_STATIC_ASSERT(X,Y) static_assert(X, Y);

#define CXPLAT_FRE_ASSERT(exp) ((exp) ? (void)0 : (CxPlatLogAssert(__FILE__, __LINE__, #exp), cxplat_bugcheck(__FILE__, __LINE__, #exp)));
#define CXPLAT_FRE_ASSERTMSG(exp, Y) CXPLAT_FRE_ASSERT(exp)

#ifdef DEBUG
#define CXPLAT_DBG_ASSERT(exp) CXPLAT_FRE_ASSERT(exp)
#define CXPLAT_DBG_ASSERTMSG(exp, msg) CXPLAT_FRE_ASSERT(exp)
#else
#define CXPLAT_DBG_ASSERT(exp)
#define CXPLAT_DBG_ASSERTMSG(exp, msg)
#endif

//
// Allocation/Memory Interfaces
//

_Ret_maybenull_
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

#define CxPlatZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#define CxPlatCopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#define CxPlatMoveMemory(Destination, Source, Length) memmove((Destination), (Source), (Length))
#define CxPlatSecureZeroMemory CxPlatZeroMemory // TODO - Something better?

//
// Interrupt ReQuest Level
//

#define CXPLAT_IRQL() 0
#define CXPLAT_PASSIVE_CODE()
#define CXPLAT_AT_DISPATCH() FALSE

//
// Locking interfaces
//

typedef struct CXPLAT_LOCK {
    alignas(16) pthread_mutex_t Mutex;
} CXPLAT_LOCK;

#define CxPlatLockInitialize(Lock) { \
    pthread_mutexattr_t Attr; \
    CXPLAT_FRE_ASSERT(pthread_mutexattr_init(&Attr) == 0); \
    CXPLAT_FRE_ASSERT(pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE) == 0); \
    CXPLAT_FRE_ASSERT(pthread_mutex_init(&(Lock)->Mutex, &Attr) == 0); \
    CXPLAT_FRE_ASSERT(pthread_mutexattr_destroy(&Attr) == 0); \
}
#define CxPlatLockUninitialize(Lock) \
        CXPLAT_FRE_ASSERT(pthread_mutex_destroy(&(Lock)->Mutex) == 0)
#define CxPlatLockAcquire(Lock) \
    CXPLAT_FRE_ASSERT(pthread_mutex_lock(&(Lock)->Mutex) == 0)
#define CxPlatLockRelease(Lock) \
    CXPLAT_FRE_ASSERT(pthread_mutex_unlock(&(Lock)->Mutex) == 0)

typedef CXPLAT_LOCK CXPLAT_DISPATCH_LOCK;

#define CxPlatDispatchLockInitialize CxPlatLockInitialize
#define CxPlatDispatchLockUninitialize CxPlatLockUninitialize
#define CxPlatDispatchLockAcquire CxPlatLockAcquire
#define CxPlatDispatchLockRelease CxPlatLockRelease

typedef struct CXPLAT_RW_LOCK {
    pthread_rwlock_t RwLock;
} CXPLAT_RW_LOCK;

#define CxPlatRwLockInitialize(Lock) \
    CXPLAT_FRE_ASSERT(pthread_rwlock_init(&(Lock)->RwLock, NULL) == 0)
#define CxPlatRwLockUninitialize(Lock) \
    CXPLAT_FRE_ASSERT(pthread_rwlock_destroy(&(Lock)->RwLock) == 0)
#define CxPlatRwLockAcquireShared(Lock) \
    CXPLAT_FRE_ASSERT(pthread_rwlock_rdlock(&(Lock)->RwLock) == 0)
#define CxPlatRwLockAcquireExclusive(Lock) \
    CXPLAT_FRE_ASSERT(pthread_rwlock_wrlock(&(Lock)->RwLock) == 0)
#define CxPlatRwLockReleaseShared(Lock) \
    CXPLAT_FRE_ASSERT(pthread_rwlock_unlock(&(Lock)->RwLock) == 0)
#define CxPlatRwLockReleaseExclusive(Lock) \
    CXPLAT_FRE_ASSERT(pthread_rwlock_unlock(&(Lock)->RwLock) == 0)

typedef CXPLAT_RW_LOCK CXPLAT_DISPATCH_RW_LOCK;

#define CxPlatDispatchRwLockInitialize CxPlatRwLockInitialize
#define CxPlatDispatchRwLockUninitialize CxPlatRwLockUninitialize
#define CxPlatDispatchRwLockAcquireShared CxPlatRwLockAcquireShared
#define CxPlatDispatchRwLockAcquireExclusive CxPlatRwLockAcquireExclusive
#define CxPlatDispatchRwLockReleaseShared CxPlatRwLockReleaseShared
#define CxPlatDispatchRwLockReleaseExclusive CxPlatRwLockReleaseExclusive

//
// Time Measurement Interfaces
//

#define CXPLAT_NANOSEC_PER_MS       (1000000)
#define CXPLAT_NANOSEC_PER_MICROSEC (1000)
#define CXPLAT_NANOSEC_PER_SEC      (1000000000)
#define CXPLAT_MICROSEC_PER_MS      (1000)
#define CXPLAT_MICROSEC_PER_SEC     (1000000)
#define CXPLAT_MS_PER_SECOND        (1000)

uint64_t
CxPlatGetTimerResolution(
    void
    );

void
CxPlatGetAbsoluteTime(
    _In_ unsigned long DeltaMs,
    _Out_ struct timespec *Time
    );

uint64_t
CxPlatTimeUs64(
    void
    );

#define CxPlatTimeUs32() (uint32_t)CxPlatTimeUs64()
#define CxPlatTimeMs64()  (CxPlatTimeUs64() / CXPLAT_MICROSEC_PER_MS)
#define CxPlatTimeMs32() (uint32_t)CxPlatTimeMs64()
#define CxPlatTimeUs64ToPlat(x) (x)

inline
int64_t
CxPlatTimeEpochMs64(
    void
    )
{
    struct timeval tv;
    CxPlatZeroMemory(&tv, sizeof(tv));
    gettimeofday(&tv, NULL);
    return S_TO_MS(tv.tv_sec) + US_TO_MS(tv.tv_usec);
}

inline
uint64_t
CxPlatTimeDiff64(
    _In_ uint64_t T1,
    _In_ uint64_t T2
    )
{
    //
    // Assume no wrap around.
    //

    return T2 - T1;
}

inline
uint32_t
CXPLAT_NO_SANITIZE("unsigned-integer-overflow")
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

inline
BOOLEAN
CXPLAT_NO_SANITIZE("unsigned-integer-overflow")
CxPlatTimeAtOrBefore32(
    _In_ uint32_t T1,
    _In_ uint32_t T2
    )
{
    return (int32_t)(T1 - T2) <= 0;
}

void
CxPlatSleep(
    _In_ uint32_t DurationMs
    );

#define CxPlatSchedulerYield() sched_yield()

//
// Event Interfaces
//

typedef struct CXPLAT_EVENT {

    //
    // Mutex and condition. The alignas is important, as the perf tanks
    // if the event is not aligned.
    //
    alignas(16) pthread_mutex_t Mutex;
    pthread_cond_t Cond;

    //
    // Denotes if the event object is in signaled state.
    //
    BOOLEAN Signaled;

    //
    // Denotes if the event object should be auto reset after it's signaled.
    //
    BOOLEAN AutoReset;

} CXPLAT_EVENT;

inline
void
CxPlatEventInitialize(
    _Out_ CXPLAT_EVENT* Event,
    _In_ BOOLEAN ManualReset,
    _In_ BOOLEAN InitialState
    )
{
    pthread_condattr_t Attr;
    int Result;

    CxPlatZeroMemory(&Attr, sizeof(Attr));
    Event->AutoReset = !ManualReset;
    Event->Signaled = InitialState;

    Result = pthread_mutex_init(&Event->Mutex, NULL);
    CXPLAT_FRE_ASSERT(Result == 0);
    Result = pthread_condattr_init(&Attr);
    CXPLAT_FRE_ASSERT(Result == 0);
#if defined(CX_PLATFORM_LINUX)
    Result = pthread_condattr_setclock(&Attr, CLOCK_MONOTONIC);
    CXPLAT_FRE_ASSERT(Result == 0);
#endif // CX_PLATFORM_LINUX
    Result = pthread_cond_init(&Event->Cond, &Attr);
    CXPLAT_FRE_ASSERT(Result == 0);
    Result = pthread_condattr_destroy(&Attr);
    CXPLAT_FRE_ASSERT(Result == 0);
}

inline
void
CxPlatInternalEventUninitialize(
    _Inout_ CXPLAT_EVENT* Event
    )
{
    int Result;

    Result = pthread_cond_destroy(&Event->Cond);
    CXPLAT_FRE_ASSERT(Result == 0);
    Result = pthread_mutex_destroy(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);
}

inline
void
CxPlatInternalEventSet(
    _Inout_ CXPLAT_EVENT* Event
    )
{
    int Result;

    Result = pthread_mutex_lock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);

    Event->Signaled = true;

    //
    // Signal the condition while holding the lock for predictable scheduling,
    // better performance and removing possibility of use after free for the
    // condition.
    //

    Result = pthread_cond_broadcast(&Event->Cond);
    CXPLAT_FRE_ASSERT(Result == 0);

    Result = pthread_mutex_unlock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);
}

inline
void
CxPlatInternalEventReset(
    _Inout_ CXPLAT_EVENT* Event
    )
{
    int Result;

    Result = pthread_mutex_lock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);
    Event->Signaled = false;
    Result = pthread_mutex_unlock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);
}

inline
void
CxPlatInternalEventWaitForever(
    _Inout_ CXPLAT_EVENT* Event
    )
{
    int Result;

    Result = pthread_mutex_lock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);

    //
    // Spurious wake ups from pthread_cond_wait can occur. So the function needs
    // to be called in a loop until the predicate 'Signaled' is satisfied.
    //

    while (!Event->Signaled) {
        Result = pthread_cond_wait(&Event->Cond, &Event->Mutex);
        CXPLAT_FRE_ASSERT(Result == 0);
    }

    if(Event->AutoReset) {
        Event->Signaled = false;
    }

    Result = pthread_mutex_unlock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);
}

inline
BOOLEAN
CxPlatInternalEventWaitWithTimeout(
    _Inout_ CXPLAT_EVENT* Event,
    _In_ uint32_t TimeoutMs
    )
{
    BOOLEAN WaitSatisfied = FALSE;
    struct timespec Ts = {0, 0};
    int Result;

    CXPLAT_DBG_ASSERT(TimeoutMs != UINT32_MAX);

    //
    // Get absolute time.
    //

    CxPlatGetAbsoluteTime(TimeoutMs, &Ts);

    Result = pthread_mutex_lock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);

    while (!Event->Signaled) {

        Result = pthread_cond_timedwait(&Event->Cond, &Event->Mutex, &Ts);

        if (Result == ETIMEDOUT) {
            WaitSatisfied = FALSE;
            goto Exit;
        }

        CXPLAT_DBG_ASSERT(Result == 0);
        UNREFERENCED_PARAMETER(Result);
    }

    if (Event->AutoReset) {
        Event->Signaled = FALSE;
    }

    WaitSatisfied = TRUE;

Exit:

    Result = pthread_mutex_unlock(&Event->Mutex);
    CXPLAT_FRE_ASSERT(Result == 0);

    return WaitSatisfied;
}

#define CxPlatEventUninitialize(Event) CxPlatInternalEventUninitialize(&Event)
#define CxPlatEventSet(Event) CxPlatInternalEventSet(&Event)
#define CxPlatEventReset(Event) CxPlatInternalEventReset(&Event)
#define CxPlatEventWaitForever(Event) CxPlatInternalEventWaitForever(&Event)
#define CxPlatEventWaitWithTimeout(Event, TimeoutMs) CxPlatInternalEventWaitWithTimeout(&Event, TimeoutMs)

//
// Processor Interfaces
//

extern uint32_t CxPlatProcessorCount;
#define CxPlatProcCount() CxPlatProcessorCount

uint32_t
CxPlatProcCurrentNumber(
    void
    );

//
// Thread Interfaces
//

typedef pthread_t CXPLAT_THREAD;

#define CXPLAT_THREAD_CALLBACK(FuncName, CtxVarName) \
    void* \
    FuncName( \
        void* CtxVarName \
        )

#define CXPLAT_THREAD_RETURN(Status) return NULL;

typedef void* (* LPTHREAD_START_ROUTINE)(void *);

typedef struct CXPLAT_THREAD_CONFIG {
    uint16_t Flags;
    uint16_t IdealProcessor;
    _Field_z_ const char* Name;
    LPTHREAD_START_ROUTINE Callback;
    void* Context;
} CXPLAT_THREAD_CONFIG;

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

CXPLAT_STATUS
CxPlatThreadCreate(
    _In_ CXPLAT_THREAD_CONFIG* Config,
    _Out_ CXPLAT_THREAD* Thread
    );

void
CxPlatThreadDelete(
    _Inout_ CXPLAT_THREAD* Thread
    );

void
CxPlatThreadWaitForever(
    _Inout_ CXPLAT_THREAD* Thread
    );

typedef uint32_t CXPLAT_THREAD_ID;

CXPLAT_THREAD_ID
CxPlatCurThreadID(
    void
    );

//
// Crypto Interfaces
//

CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    );

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_POSIX_H
