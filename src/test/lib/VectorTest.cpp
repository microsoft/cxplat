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

    Array.eraseAt(3);

    TEST_EQUAL(Array.size(), 9u);

    for (uint32_t i = 0; i < 3; i++) {
        TEST_EQUAL(Array[i], i);
    }

    for (uint32_t i = 4; i < 10; i++) {
        TEST_EQUAL(Array[i - 1], i);
    }

    CxPlatVector<uint32_t> ArrayFill(10);
    TEST_EQUAL(ArrayFill.size(), 10u);

    // Verify that the newly appended element will be inserted at 10-th slot.
    ArrayFill.push_back(10);
    TEST_EQUAL(ArrayFill.size(), 11u);
    TEST_EQUAL(ArrayFill[10], 10u);
}
