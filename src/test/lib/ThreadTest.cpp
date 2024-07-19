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
        CxPlatAsync Async([](void*) -> void* {
            return nullptr;
        });
    }

    {
        struct TempCtx {
            uint32_t Value;
        } TempCtx = { 0 };
        CxPlatAsync Async([](void* Ctx) -> void* {
            struct TempCtx* TempCtx = (struct TempCtx*)Ctx;
            TempCtx->Value = 123;
            return nullptr;
        }, &TempCtx);
        Async.Wait();
        TEST_EQUAL(123, TempCtx.Value);
    }   

    {
        CXPLAT_THREAD_ID ThreadId = INITIAL_THREAD_ID_VALUE;
        CxPlatAsync Async([](void* Ctx) -> void* {
            CXPLAT_THREAD_ID* ThreadId = (CXPLAT_THREAD_ID*)Ctx;
            *ThreadId = CxPlatCurThreadID();
            return (void*)(intptr_t)(*ThreadId);
        }, &ThreadId);

        Async.Wait();
        TEST_EQUAL((CXPLAT_THREAD_ID)((intptr_t)Async.Get()), ThreadId);
        TEST_NOT_EQUAL(INITIAL_THREAD_ID_VALUE, ThreadId);
    }

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
    {
        CxPlatAsync Async([](void*) -> void* {
            CxPlatSleep(2000);   
            return (void*)(intptr_t)(0xdeadbeaf);
        });

        Async.WaitFor(50);
        TEST_EQUAL(Async.Get(), nullptr);
        Async.Wait();
        TEST_NOT_EQUAL(Async.Get(), nullptr);
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
