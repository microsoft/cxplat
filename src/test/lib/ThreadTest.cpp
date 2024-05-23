/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Thread test.

--*/

#include "precomp.h"

#define INITIAL_CONTEXT_VALUE ((CXPLAT_THREAD_ID)-1)

CXPLAT_THREAD_CALLBACK(ThreadFn, Context)
{
    TEST_EQUAL_GOTO(*(CXPLAT_THREAD_ID*)Context, INITIAL_CONTEXT_VALUE);

    *(CXPLAT_THREAD_ID*)Context = CxPlatCurThreadID();

Failure:

    CXPLAT_THREAD_RETURN(0);
}

void CxPlatTestThreadBasic()
{
    CXPLAT_THREAD Thread;
    CXPLAT_THREAD_CONFIG ThreadConfig;
    CXPLAT_THREAD_ID Context = INITIAL_CONTEXT_VALUE;

    ThreadConfig.Flags = 0;
    ThreadConfig.IdealProcessor = 0;
    ThreadConfig.Name = "CxPlatTestThreadBasic";
    ThreadConfig.Callback = ThreadFn;
    ThreadConfig.Context = (void*)&Context;

    TEST_CXPLAT(CxPlatThreadCreate(&ThreadConfig, &Thread));

    CxPlatThreadWaitForever(&Thread);

    TEST_TRUE(Context != INITIAL_CONTEXT_VALUE);

    CxPlatThreadDelete(&Thread);
}
