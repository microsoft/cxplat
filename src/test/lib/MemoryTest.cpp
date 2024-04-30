/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Memory/Allocation test.

--*/

#include "precomp.h"

#define CXPLAT_POOLTAG_MEMORY_TEST 'tMxC' // CxMt

void CxPlatTestMemoryBasic()
{
    const char* TestString = "CxPlatTestMemoryBasic";
    const size_t BufferLen = strlen(TestString);

    char* Buffer = (char*)CXPLAT_ALLOC_NONPAGED(BufferLen, CXPLAT_POOLTAG_MEMORY_TEST);
    TEST_TRUE_GOTO(Buffer != NULL);

    CxPlatZeroMemory(Buffer, BufferLen);
    for (size_t i = 0; i < BufferLen; i++) {
        TEST_EQUAL_GOTO(Buffer[i], '\0');
    }

    CxPlatCopyMemory(Buffer, TestString, BufferLen);
    for (size_t i = 0; i < BufferLen; i++) {
        TEST_EQUAL_GOTO(Buffer[i], TestString[i]);
    }

    CxPlatMoveMemory(Buffer + 6, Buffer, 8);
    TEST_EQUAL_GOTO(0, memcmp(Buffer, "CxPlatCxPlatTeryBasic", BufferLen));

    CxPlatSecureZeroMemory(Buffer, BufferLen);
    for (size_t i = 0; i < BufferLen; i++) {
        TEST_EQUAL_GOTO(Buffer[i], '\0');
    }

Failure:
    if (Buffer != NULL) {
        CXPLAT_FREE(Buffer, CXPLAT_POOLTAG_MEMORY_TEST);
    }
}

#if DEBUG
void CxPlatTestMemoryFailureInjection()
{
    void* Buffer = NULL;
    const size_t BufferLen = 2;
    uint32_t AllocsFailed = 0;
    const int32_t FailureIter = 6;
    int32_t OriginalDenominator = CxPlatGetAllocFailDenominator();

    //
    // No failure injection.
    //
    CxPlatSetAllocFailDenominator(0);
    for (int i = 0; i < 100; i++) {
        Buffer = CXPLAT_ALLOC_NONPAGED(BufferLen, CXPLAT_POOLTAG_MEMORY_TEST);
        TEST_TRUE_GOTO(Buffer != NULL);
        CXPLAT_FREE(Buffer, CXPLAT_POOLTAG_MEMORY_TEST);
        Buffer = NULL;
    }

    //
    // Non-zero probability failure injection.
    //
    CxPlatSetAllocFailDenominator(4);
    for (int i = 0; i < 100; i++) {
        Buffer = CXPLAT_ALLOC_NONPAGED(BufferLen, CXPLAT_POOLTAG_MEMORY_TEST);
        if (Buffer == NULL) {
            AllocsFailed++;
        } else {
            CXPLAT_FREE(Buffer, CXPLAT_POOLTAG_MEMORY_TEST);
            Buffer = NULL;
        }
    }
    TEST_TRUE_GOTO(AllocsFailed > 0);
    TEST_TRUE_GOTO(AllocsFailed < 50);

    //
    // Nth pattern failure injection.
    //
    CxPlatSetAllocFailDenominator(-1 * FailureIter);
    for (int i = 0; i < 10; i++) {
        Buffer = CXPLAT_ALLOC_NONPAGED(BufferLen, CXPLAT_POOLTAG_MEMORY_TEST);
        if (i == FailureIter) {
            TEST_TRUE_GOTO(Buffer == NULL);
        } else {
            TEST_TRUE_GOTO(Buffer != NULL);
            CXPLAT_FREE(Buffer, CXPLAT_POOLTAG_MEMORY_TEST);
            Buffer = NULL;
        }
    }

Failure:
    if (Buffer != NULL) {
        CXPLAT_FREE(Buffer, CXPLAT_POOLTAG_MEMORY_TEST);
    }

    CxPlatSetAllocFailDenominator(OriginalDenominator);
}
#endif
