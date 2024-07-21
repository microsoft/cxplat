/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Thread test.

--*/

#include "precomp.h"

typedef struct THREAD_CONTEXT {
    CXPLAT_THREAD_ID ThreadId;
    uint32_t DelayMs;
} THREAD_CONTEXT;

#define INITIAL_THREAD_ID_VALUE ((CXPLAT_THREAD_ID)-1)

CXPLAT_THREAD_CALLBACK(ThreadFn, Ctx)
{
    THREAD_CONTEXT* Context = (THREAD_CONTEXT*)Ctx;

    if (Context->DelayMs > 0) {
        CxPlatSleep(Context->DelayMs);
    }

    TEST_EQUAL_GOTO(Context->ThreadId, INITIAL_THREAD_ID_VALUE);

    *(CXPLAT_THREAD_ID*)Ctx = CxPlatCurThreadID();

Failure:

    CXPLAT_THREAD_RETURN(0);
}

void CxPlatTestThreadBasic()
{
    CXPLAT_THREAD Thread;
    CXPLAT_THREAD_CONFIG ThreadConfig;
    THREAD_CONTEXT Context;

    Context.ThreadId = INITIAL_THREAD_ID_VALUE;
    Context.DelayMs = 0;

    ThreadConfig.Flags = 0;
    ThreadConfig.IdealProcessor = 0;
    ThreadConfig.Name = "CxPlatTestThreadBasic";
    ThreadConfig.Callback = ThreadFn;
    ThreadConfig.Context = (void*)&Context;

    TEST_CXPLAT(CxPlatThreadCreate(&ThreadConfig, &Thread));

    CxPlatThreadWaitForever(&Thread);

    TEST_TRUE_GOTO(Context.ThreadId != INITIAL_THREAD_ID_VALUE);

Failure:

    CxPlatThreadDelete(&Thread);
}

void CxPlatTestThreadAsync()
{
    {
        CxPlatAsync Async([](void*) {
            // no-op
        });
    }

    {
        uint32_t Ctx = 0;
        CxPlatAsyncT<uint32_t> Async([](uint32_t* Ctx) {
            *Ctx = 123;
        }, &Ctx);
        Async.Wait();
        TEST_EQUAL(123, Ctx);
    }

    {
        CXPLAT_THREAD_ID ThreadId = INITIAL_THREAD_ID_VALUE;
        CxPlatAsyncT<CXPLAT_THREAD_ID> Async([](CXPLAT_THREAD_ID* Ctx) {
            *Ctx = CxPlatCurThreadID();
        }, &ThreadId);

        Async.Wait();
        TEST_NOT_EQUAL(INITIAL_THREAD_ID_VALUE, ThreadId);
    }

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
    {
        intptr_t Ctx = 0;
        CxPlatAsyncT<intptr_t> Async([](intptr_t* Ctx) {
            CxPlatSleep(2000);
            *Ctx = (intptr_t)(0xdeadbeaf);
        }, &Ctx);

        TEST_FALSE(Async.WaitFor(50));
        TEST_EQUAL(Ctx, 0);
        Async.Wait();
        TEST_NOT_EQUAL(Ctx, 0);
    }
#endif
}

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
void CxPlatTestThreadWaitTimeout()
{
    CXPLAT_THREAD Thread;
    CXPLAT_THREAD_CONFIG ThreadConfig;
    THREAD_CONTEXT Context;

    Context.ThreadId = INITIAL_THREAD_ID_VALUE;
    Context.DelayMs = 1000;

    ThreadConfig.Flags = 0;
    ThreadConfig.IdealProcessor = 0;
    ThreadConfig.Name = "CxPlatTestThreadWaitTimeout";
    ThreadConfig.Callback = ThreadFn;
    ThreadConfig.Context = (void*)&Context;

    TEST_CXPLAT(CxPlatThreadCreate(&ThreadConfig, &Thread));

    TEST_FALSE_GOTO(CxPlatThreadWaitWithTimeout(&Thread, 50));

    TEST_TRUE_GOTO(CxPlatThreadWaitWithTimeout(&Thread, 5000));

    TEST_TRUE_GOTO(Context.ThreadId != INITIAL_THREAD_ID_VALUE);

Failure:

    CxPlatThreadDelete(&Thread);
}
#endif
