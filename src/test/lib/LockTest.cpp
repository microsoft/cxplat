/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Lock test.

--*/

#include "precomp.h"

void CxPlatTestLockBasic()
{
#ifdef _KERNEL_MODE
    TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif

    {
        CxPlatLock Lock;
        Lock.Acquire();
#ifdef _KERNEL_MODE
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
        Lock.Release();
    }

    {
        CxPlatLockDispatch Lock;
        Lock.Acquire();
#ifdef _KERNEL_MODE
        TEST_TRUE(CXPLAT_AT_DISPATCH());
#endif
        Lock.Release();
#ifdef _KERNEL_MODE
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
    }

#ifdef _KERNEL_MODE
    {
        CxPlatLockDispatch Lock;
        CXPLAT_RAISE_IRQL();
        Lock.Acquire();
        TEST_TRUE(CXPLAT_AT_DISPATCH());
        Lock.Release();
        TEST_TRUE(CXPLAT_AT_DISPATCH());
        CXPLAT_LOWER_IRQL();
        TEST_FALSE(CXPLAT_AT_DISPATCH());
    }
#endif

    {
        CxPlatEvent Event;
        CxPlatLock Lock;
        struct Context {
            CxPlatEvent* Event;
            CxPlatLock* Lock;
        } Ctx = { &Event, &Lock };
        Lock.Acquire();
        CxPlatAsyncT<Context> Async([](Context* Ctx) {
            Ctx->Lock->Acquire();
            Ctx->Event->Set();
            Ctx->Lock->Release();
        }, &Ctx);
        TEST_FALSE(Event.WaitTimeout(500));
        Lock.Release();
        TEST_TRUE(Event.WaitTimeout(2000));
    }
}

void CxPlatTestLockReadWrite()
{
#ifdef _KERNEL_MODE
    TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif

    {
        CxPlatRwLock Lock;
        Lock.AcquireShared();
#ifdef _KERNEL_MODE
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
        Lock.ReleaseShared();
    }

    {
        CxPlatRwLockDispatch Lock;
        Lock.AcquireShared();
#ifdef _KERNEL_MODE
        TEST_TRUE(CXPLAT_AT_DISPATCH());
#endif
        Lock.ReleaseShared();
#ifdef _KERNEL_MODE
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
    }

#ifdef _KERNEL_MODE
    {
        CxPlatRwLockDispatch Lock;
        CXPLAT_RAISE_IRQL();
        Lock.AcquireShared();
        TEST_TRUE(CXPLAT_AT_DISPATCH());
        Lock.ReleaseShared();
        TEST_TRUE(CXPLAT_AT_DISPATCH());
        CXPLAT_LOWER_IRQL();
        TEST_FALSE(CXPLAT_AT_DISPATCH());
    }
#endif
}
