/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Event test.

--*/

#include "precomp.h"

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
void KArrayBasic()
{
    Rtl::KArray<uint32_t> Array;

    for (uint32_t i = 0; i < 10; i++) {
        Array.append(i);
    }

    TEST_EQUAL(Array.count(), 10u);

    for (uint32_t i = 0; i < 10; i++) {
        TEST_EQUAL(Array[i], i);
    }
}
#endif