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

namespace cxplat {
    template <class _Tp>
    struct remove_reference
    {
        typedef _Tp type;
    };

    template <class _Tp>
    struct remove_reference<_Tp&>
    {
        typedef _Tp type;
    };

    template <class _Tp>
    struct remove_reference<_Tp&&>
    {
        typedef _Tp type;
    };

    template <class _Tp>
    inline _Tp&& forward(typename remove_reference<_Tp>::type& __t) noexcept
    {
        return static_cast<_Tp&&>(__t);
    }

    template <class _Tp>
    inline _Tp&& forward(typename remove_reference<_Tp>::type&& __t) noexcept
    {
        return static_cast<_Tp&&>(__t);
    }
}

template <typename TLambda>
class CxPlatAsync {
private:
    TLambda UserCallback;
    static CXPLAT_THREAD_CALLBACK(CxPlatAsyncWrapperCallback, Context)
    {
        TLambda *Callback = (TLambda *)Context;
        (*Callback)();
        CXPLAT_THREAD_RETURN(0);
    }

    CXPLAT_THREAD Thread {0};
    CXPLAT_THREAD_CONFIG ThreadConfig {0};
    bool Initialized = false;
    bool ThreadCompleted = false;
public:
    CxPlatAsync(TLambda&& Callback) noexcept 
        : ThreadConfig{0, 0, "CxPlatAsync", CxPlatAsyncWrapperCallback, &UserCallback},
          UserCallback{cxplat::forward<TLambda>(Callback)},
          Initialized(CxPlatThreadCreate(&ThreadConfig, &Thread) == 0),
          ThreadCompleted(false) {
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
};

#endif
