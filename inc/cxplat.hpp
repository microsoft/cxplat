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

struct CxPlatRwLock {
    CXPLAT_RW_LOCK Handle;
    CxPlatRwLock() noexcept { CxPlatRwLockInitialize(&Handle); }
    ~CxPlatRwLock() noexcept { CxPlatRwLockUninitialize(&Handle); }
    void AcquireShared() noexcept { CxPlatRwLockAcquireShared(&Handle); }
    void AcquireExclusive() noexcept { CxPlatRwLockAcquireExclusive(&Handle); }
    void ReleaseShared() noexcept { CxPlatRwLockReleaseShared(&Handle); }
    void ReleaseExclusive() noexcept { CxPlatRwLockReleaseExclusive(&Handle); }
};

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

typedef void* CxPlatCallback(
    _Inout_ void* Context
);

class CxPlatAsync {
private:
    struct CxPlatAsyncContext {
        void* UserContext;
        CxPlatCallback *UserCallback;
        void* ReturnValue;
    };

    static CXPLAT_THREAD_CALLBACK(CxPlatAsyncWrapperCallback, Context)
    {
        struct CxPlatAsyncContext* AsyncContext = (struct CxPlatAsyncContext*)Context;
        AsyncContext->ReturnValue = AsyncContext->UserCallback(AsyncContext->UserContext);
        CXPLAT_THREAD_RETURN(0);
    }

    CXPLAT_THREAD Thread {0};
    CXPLAT_THREAD_CONFIG ThreadConfig {0};
    struct CxPlatAsyncContext AsyncContext {0};
    bool Initialized = false;
    bool ThreadCompleted = false;
public:
    CxPlatAsync(CxPlatCallback Callback, void* UserContext = nullptr) noexcept {
        AsyncContext.UserContext = UserContext;
        AsyncContext.UserCallback = Callback;
        AsyncContext.ReturnValue = nullptr;

        ThreadConfig.Name = "CxPlatAsync";
        ThreadConfig.Callback = CxPlatAsyncWrapperCallback;
        ThreadConfig.Context = &AsyncContext;
        if (CxPlatThreadCreate(&ThreadConfig, &Thread) != 0) {
            Initialized = false;
            return;
        }
        Initialized = true;
    }
    ~CxPlatAsync() noexcept {
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

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
    bool WaitFor(uint32_t TimeoutMs) noexcept {
        if (Initialized) {
            if (CxPlatThreadWaitWithTimeout(&Thread, TimeoutMs)) {
                return true;
            }
        }
        return false;
    }
#endif

    void* Get() noexcept {
        return AsyncContext.ReturnValue;
    }
};

#endif
