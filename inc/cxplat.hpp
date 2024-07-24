/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    C++ header only cxplat wrappers

--*/

#ifdef _WIN32
#pragma once
#endif

#ifndef CXPLATCPP_H
#define CXPLATCPP_H

#include "cxplat.h"
#include "cxplat_sal_stub.h"

struct CxPlatLock {
    CXPLAT_LOCK Handle;
    CxPlatLock() noexcept { CxPlatLockInitialize(&Handle); }
    ~CxPlatLock() noexcept { CxPlatLockUninitialize(&Handle); }
    void Acquire() noexcept { CxPlatLockAcquire(&Handle); }
    void Release() noexcept { CxPlatLockRelease(&Handle); }
};

#pragma warning(push)
#pragma warning(disable:C26110) // TODO - Fix SAL annotations for locks
struct CxPlatRwLock {
    CXPLAT_RW_LOCK Handle;
    CxPlatRwLock() noexcept { CxPlatRwLockInitialize(&Handle); }
    ~CxPlatRwLock() noexcept { CxPlatRwLockUninitialize(&Handle); }
    void AcquireShared() noexcept { CxPlatRwLockAcquireShared(&Handle); }
    void AcquireExclusive() noexcept { CxPlatRwLockAcquireExclusive(&Handle); }
    void ReleaseShared() noexcept { CxPlatRwLockReleaseShared(&Handle); }
    void ReleaseExclusive() noexcept { CxPlatRwLockReleaseExclusive(&Handle); }
};
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:28167) // TODO - Fix SAL annotations for IRQL changes
struct CxPlatLockDispatch {
    CXPLAT_DISPATCH_LOCK Handle;
    CxPlatLockDispatch() noexcept { CxPlatDispatchLockInitialize(&Handle); }
    ~CxPlatLockDispatch() noexcept { CxPlatDispatchLockUninitialize(&Handle); }
    void Acquire() noexcept { CxPlatDispatchLockAcquire(&Handle); }
    void Release() noexcept { CxPlatDispatchLockRelease(&Handle); }
};

struct CxPlatRwLockDispatch {
    CXPLAT_DISPATCH_RW_LOCK Handle;
    CxPlatRwLockDispatch() noexcept { CxPlatDispatchRwLockInitialize(&Handle); }
    ~CxPlatRwLockDispatch() noexcept { CxPlatDispatchRwLockUninitialize(&Handle); }
    void AcquireShared() noexcept { CxPlatDispatchRwLockAcquireShared(&Handle); }
    void AcquireExclusive() noexcept { CxPlatDispatchRwLockAcquireExclusive(&Handle); }
    void ReleaseShared() noexcept { CxPlatDispatchRwLockReleaseShared(&Handle); }
    void ReleaseExclusive() noexcept { CxPlatDispatchRwLockReleaseExclusive(&Handle); }
};
#pragma warning(pop)

struct CxPlatEvent {
    CXPLAT_EVENT Handle;
    CxPlatEvent() noexcept { CxPlatEventInitialize(&Handle, FALSE, FALSE); }
    CxPlatEvent(bool ManualReset) noexcept { CxPlatEventInitialize(&Handle, ManualReset, FALSE); }
    CxPlatEvent(CXPLAT_EVENT event) noexcept : Handle(event) { }
    ~CxPlatEvent() noexcept { CxPlatEventUninitialize(Handle); }
    operator CXPLAT_EVENT() const noexcept { return Handle; }
    void Set() { CxPlatEventSet(Handle); }
    void Reset() { CxPlatEventReset(Handle); }
    void WaitForever() { CxPlatEventWaitForever(Handle); }
    bool WaitTimeout(uint32_t TimeoutMs) { return CxPlatEventWaitWithTimeout(Handle, TimeoutMs); }
};

template <typename T>
class CxPlatAsyncT {
private:
    typedef void CallbackT(_Inout_ T* Context);

    struct ContextT {
        T* UserContext;
        CallbackT *UserCallback;
    };

    static CXPLAT_THREAD_CALLBACK(ThreadCallback, Context) {
        auto AsyncContext = (ContextT*)Context;
        AsyncContext->UserCallback(AsyncContext->UserContext);
        CXPLAT_THREAD_RETURN(0);
    }

    struct ContextT AsyncContext {0, 0};
    CXPLAT_THREAD_CONFIG ThreadConfig {0, 0, "CxPlatAsync", ThreadCallback, &AsyncContext};
    CXPLAT_THREAD Thread {0};
    bool ThreadCompleted {false};
    bool Initialized;
public:
    CxPlatAsyncT(CallbackT Callback, T* UserContext = nullptr) noexcept
        : AsyncContext({UserContext, Callback}),
          Initialized(CxPlatThreadCreate(&ThreadConfig, &Thread) == 0) {
    }
    ~CxPlatAsyncT() noexcept {
        if (Initialized) {
            if (!ThreadCompleted) {
                CxPlatThreadWaitForever(&Thread);
            }
            CxPlatThreadDelete(&Thread);
        }
    }

    void Wait() noexcept {
        if (Initialized) {
            CxPlatThreadWaitForever(&Thread);
            ThreadCompleted = true;
        }
    }

#if _WIN32
    bool WaitFor(uint32_t TimeoutMs) noexcept {
        if (Initialized) {
            return (ThreadCompleted = CxPlatThreadWaitWithTimeout(&Thread, TimeoutMs));
        }
        return false;
    }
#endif // _WIN32
};

typedef CxPlatAsyncT<void> CxPlatAsync;

#endif
