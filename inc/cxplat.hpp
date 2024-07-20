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

template <typename T, typename R>
class CxPlatAsyncT {
private:

    typedef R CxPlatCallback(
        _Inout_ T* Context
    );

    struct CxPlatAsyncContext {
        T* UserContext;
        CxPlatCallback *UserCallback;
        R ReturnValue;
    };

    static CXPLAT_THREAD_CALLBACK(CxPlatAsyncWrapperCallback, Context) {
        auto AsyncContext = (CxPlatAsyncContext*)Context;
        AsyncContext->ReturnValue = AsyncContext->UserCallback(AsyncContext->UserContext);
        CXPLAT_THREAD_RETURN(0);
    }

    CXPLAT_THREAD Thread {0};
    CXPLAT_THREAD_CONFIG ThreadConfig {0};
    struct CxPlatAsyncContext AsyncContext {0};
    bool Initialized = false;
    bool ThreadCompleted = false;
public:
    CxPlatAsyncT(CxPlatCallback Callback, T* UserContext = nullptr) noexcept {
        AsyncContext.UserContext = UserContext;
        AsyncContext.UserCallback = Callback;
        AsyncContext.ReturnValue = (R)0;

        ThreadConfig.Name = "CxPlatAsync";
        ThreadConfig.Callback = CxPlatAsyncWrapperCallback;
        ThreadConfig.Context = &AsyncContext;
        if (CxPlatThreadCreate(&ThreadConfig, &Thread) != 0) {
            Initialized = false;
            return;
        }
        Initialized = true;
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
            if (CxPlatThreadWaitWithTimeout(&Thread, TimeoutMs)) {
                return true;
            }
        }
        return false;
    }
#endif

    R Get() noexcept {
        return AsyncContext.ReturnValue;
    }
};

typedef CxPlatAsyncT<void, void*> CxPlatAsync;

#endif
