/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    KArray test.

--*/

#include "precomp.h"

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
void VectorBasic()
{
    CxPlatVector<uint32_t> Array;

    for (uint32_t i = 0; i < 10; i++) {
        Array.push_back(i);
    }

    TEST_EQUAL(Array.size(), 10u);

    for (uint32_t i = 0; i < 10; i++) {
        TEST_EQUAL(Array[i], i);
    }
}
#endif
