/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Cryptography test.

--*/

#include "precomp.h"

void CxPlatTestInitialize()
{
    TEST_CXPLAT(CxPlatInitialize());
    return;
}

void CxPlatTestUninitialize()
{
    CxPlatUninitialize();
    return;
}

void CxPlatTestCryptRandom()
{
    uint32_t RandomValue = 0;
    TEST_CXPLAT(CxPlatRandom(sizeof(RandomValue), &RandomValue));
}
