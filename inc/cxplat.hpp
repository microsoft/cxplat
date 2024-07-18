/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    C++ header only cxplat wrappers 

--*/

#ifndef CXPLATCPP_H
#define CXPLATCPP_H

#include "cxplat.h"
#include "cxplat_sal_stub.h"

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

    static
    CXPLAT_THREAD_CALLBACK(CxPlatAsyncWrapperCallback, Context)
    {
        struct CxPlatAsyncContext *AsyncContext = (struct CxPlatAsyncContext*)Context;
        AsyncContext->ReturnValue = AsyncContext->UserCallback(AsyncContext->UserContext);
        CXPLAT_THREAD_RETURN(0);
    }

    CXPLAT_THREAD Thread {0};
    CXPLAT_THREAD_CONFIG ThreadConfig {0};
    struct CxPlatAsyncContext AsyncContext {0};
    bool Initialized : 1;
    bool WaitOnDelete : 1;
public:
    CxPlatAsync(CxPlatCallback Callback, void* UserContext = nullptr, bool WaitOnDelete = true) noexcept {
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
        this->WaitOnDelete = WaitOnDelete;
    }
    ~CxPlatAsync() noexcept {
        if (Initialized) {
            if (WaitOnDelete) {
                CxPlatThreadWaitForever(&Thread);
            }
            CxPlatThreadDelete(&Thread);
        }
    }

    void Wait() noexcept {
        if (Initialized) {
            CxPlatThreadWaitForever(&Thread);
        }
    }

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
    void WaitFor(uint32_t TimeoutMs) noexcept {
        if (Initialized) {
            CxPlatThreadWaitWithTimeout(&Thread, TimeoutMs);
        }
    }
#endif

    void* Get() noexcept {
        return AsyncContext.ReturnValue;
    }
};

#endif
