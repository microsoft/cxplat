/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    KArray test.

--*/

#include "precomp.h"

void VectorBasic()
{
    CxPlatVector<uint32_t> Array;

    for (uint32_t i = 0; i < 10; i++) {
        TEST_EQUAL(Array.push_back(i), true);
    }

    TEST_EQUAL(Array.size(), 10u);

    for (uint32_t i = 0; i < 10; i++) {
        TEST_EQUAL(Array[i], i);
    }
}
