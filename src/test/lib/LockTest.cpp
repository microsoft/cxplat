/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Lock test.

--*/

#include "precomp.h"

void CxPlatTestLockBasic()
{
#if defined(CX_PLATFORM_WINKERNEL)
    TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif

    {
        CxPlatLock Lock;
        Lock.Acquire();
#if defined(CX_PLATFORM_WINKERNEL)
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
        Lock.Release();
    }

    {
        CxPlatLockDispatch Lock;
        Lock.Acquire();
#if defined(CX_PLATFORM_WINKERNEL)
        TEST_TRUE(CXPLAT_AT_DISPATCH());
#endif
        Lock.Release();
#if defined(CX_PLATFORM_WINKERNEL)
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
    }
}

void CxPlatTestLockReadWrite()
{
#if defined(CX_PLATFORM_WINKERNEL)
    TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif

    {
        CxPlatRwLock Lock;
        Lock.AcquireShared();
#if defined(CX_PLATFORM_WINKERNEL)
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
        Lock.AcquireExclusive();
    }

    {
        CxPlatRwLockDispatch Lock;
        Lock.AcquireShared();
#if defined(CX_PLATFORM_WINKERNEL)
        TEST_TRUE(CXPLAT_AT_DISPATCH());
#endif
        Lock.AcquireExclusive();
#if defined(CX_PLATFORM_WINKERNEL)
        TEST_FALSE(CXPLAT_AT_DISPATCH());
#endif
    }
}
