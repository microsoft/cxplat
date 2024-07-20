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

template <typename T, typename R = void>
class CxPlatAsyncT {
private:
    typedef R CallbackT(_Inout_ T* Context);

    struct ContextT {
        T* UserContext;
        CallbackT *UserCallback;
        R ReturnValue;
    };

    static CXPLAT_THREAD_CALLBACK(ThreadCallback, Context) {
        auto AsyncContext = (ContextT*)Context;
        AsyncContext->ReturnValue = AsyncContext->UserCallback(AsyncContext->UserContext);
        CXPLAT_THREAD_RETURN(0);
    }

    struct ContextT AsyncContext {0, 0, 0};
    CXPLAT_THREAD_CONFIG ThreadConfig {0, 0, "CxPlatAsync", ThreadCallback, &AsyncContext};
    CXPLAT_THREAD Thread {0};
    bool ThreadCompleted {false};
    bool Initialized;
public:
    CxPlatAsyncT(CallbackT Callback, T* UserContext = nullptr) noexcept
        : AsyncContext({UserContext, Callback, 0}),
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

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
    bool WaitFor(uint32_t TimeoutMs) noexcept {
        if (Initialized) {
            return (ThreadCompleted = CxPlatThreadWaitWithTimeout(&Thread, TimeoutMs));
        }
        return false;
    }
#endif

    R Get() noexcept {
        return AsyncContext.ReturnValue;
    }
};

template <typename T>
class CxPlatAsyncT <T, void>{
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

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
    bool WaitFor(uint32_t TimeoutMs) noexcept {
        if (Initialized) {
            return (ThreadCompleted = CxPlatThreadWaitWithTimeout(&Thread, TimeoutMs));
        }
        return false;
    }
#endif
};

#endif
