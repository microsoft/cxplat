/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Time test.

--*/

#include "precomp.h"

void CxPlatTestTimeBasic()
{
    const uint64_t SleepTimeMs = 1000;
    const uint64_t FudgeMs = 500;

    TEST_TRUE(CxPlatGetTimerResolution() > 0);

    TEST_TRUE(CxPlatTimeUs64() > 0);

    TEST_TRUE(CxPlatTimeEpochMs64() > 1714503647);

    uint32_t T1Us32 = CxPlatTimeUs32();
    uint64_t T1Us64 = CxPlatTimeUs64();
    uint32_t T1Ms32 = CxPlatTimeMs32();
    uint64_t T1Ms64 = CxPlatTimeMs64();

    CxPlatSleep(SleepTimeMs);

    uint32_t T2Us32 = CxPlatTimeUs32();
    uint64_t T2Us64 = CxPlatTimeUs64();
    uint32_t T2Ms32 = CxPlatTimeMs32();
    uint64_t T2Ms64 = CxPlatTimeMs64();

    TEST_TRUE(US_TO_MS(CxPlatTimeDiff64(T1Us64, T2Us64)) > SleepTimeMs - FudgeMs);
    TEST_TRUE(US_TO_MS(CxPlatTimeDiff64(T1Us64, T2Us64)) < SleepTimeMs + FudgeMs);
    TEST_TRUE(US_TO_MS(CxPlatTimeDiff32(T1Us32, T2Us32)) > SleepTimeMs - FudgeMs);
    TEST_TRUE(US_TO_MS(CxPlatTimeDiff32(T1Us32, T2Us32)) < SleepTimeMs + FudgeMs);
    TEST_TRUE(CxPlatTimeDiff64(T1Ms64, T2Ms64) > SleepTimeMs - FudgeMs);
    TEST_TRUE(CxPlatTimeDiff64(T1Ms64, T2Ms64) < SleepTimeMs + FudgeMs);
    TEST_TRUE(CxPlatTimeDiff32(T1Ms32, T2Ms32) > SleepTimeMs - FudgeMs);
    TEST_TRUE(CxPlatTimeDiff32(T1Ms32, T2Ms32) < SleepTimeMs + FudgeMs);
}
