/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Platform independent test abstraction layer.

--*/

#include "CxPlatTests.h"

#define TEST_FAILURE(Format, ...) \
    LogTestFailure(__FILE__, __FUNCTION__, __LINE__, Format, ##__VA_ARGS__)

#define TEST_EQUAL(__expected, __condition) { \
    if (__condition != __expected) { \
        TEST_FAILURE(#__condition " not equal to " #__expected); \
        return; \
    } \
}

#define TEST_NOT_EQUAL(__expected, __condition) { \
    if (__condition == __expected) { \
        TEST_FAILURE(#__condition " equals " #__expected); \
        return; \
    } \
}

#define TEST_TRUE(__condition) { \
    if (!(__condition)) { \
        TEST_FAILURE(#__condition " not true"); \
        return; \
    } \
}

#define TEST_FALSE(__condition) { \
    if (__condition) { \
        TEST_FAILURE(#__condition " not false"); \
        return; \
    } \
}

#define TEST_CXPLAT(__condition) { \
    CXPLAT_STATUS __status = __condition; \
    if (CXPLAT_FAILED(__status)) { \
        TEST_FAILURE(#__condition " failed, 0x%x", __status); \
        return; \
    } \
}

//
// goto variants
//

#define TEST_EQUAL_GOTO(__expected, __condition) { \
    if (__condition != __expected) { \
        TEST_FAILURE(#__condition " not equal to " #__expected); \
        goto Failure; \
    } \
}

#define TEST_NOT_EQUAL_GOTO(__expected, __condition) { \
    if (__condition == __expected) { \
        TEST_FAILURE(#__condition " equals " #__expected); \
        goto Failure; \
    } \
}

#define TEST_TRUE_GOTO(__condition) { \
    if (!(__condition)) { \
        TEST_FAILURE(#__condition " not true"); \
        goto Failure; \
    } \
}

#define TEST_FALSE_GOTO(__condition) { \
    if (__condition) { \
        TEST_FAILURE(#__condition " not false"); \
        goto Failure; \
    } \
}

#define TEST_CXPLAT_GOTO(__condition) { \
    CXPLAT_STATUS __status = __condition; \
    if (CXPLAT_FAILED(__status)) { \
        TEST_FAILURE(#__condition " failed, 0x%x", __status); \
        goto Failure; \
    } \
}
