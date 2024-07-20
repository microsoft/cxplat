/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Event test.

--*/

#include "precomp.h"

void CxPlatTestEventBasic()
{
    CXPLAT_EVENT Event;
    uint32_t TimeoutMs = 100;

    CxPlatEventInitialize(&Event, FALSE, FALSE);

    //
    // Timeout.
    //
    TEST_FALSE(CxPlatEventWaitWithTimeout(Event, TimeoutMs));

    //
    // Immediate satisfy (WithTimeout).
    //
    CxPlatEventReset(Event);
    CxPlatEventSet(Event);
    TEST_TRUE(CxPlatEventWaitWithTimeout(Event, TimeoutMs));

    //
    // Immediate satisfy (Forever).
    //
    CxPlatEventReset(Event);
    CxPlatEventSet(Event);
    CxPlatEventWaitForever(Event);

    CxPlatEventUninitialize(Event);
}

void CxPlatTestEventCpp()
{
    const uint32_t TimeoutMs = 100;

    {
        CxPlatEvent Event;
        TEST_FALSE(Event.WaitTimeout(TimeoutMs));

        Event.Reset();
        Event.Set();
        TEST_TRUE(Event.WaitTimeout(TimeoutMs));

        Event.Reset();
        Event.Set();
        Event.WaitForever();
    }
}
