/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Platform definitions.

--*/

//
// Time unit conversion.
//
#define NS_TO_US(x)     ((x) / 1000)
#define US_TO_NS(x)     ((x) * 1000)
#define NS100_TO_US(x)  ((x) / 10)
#define US_TO_NS100(x)  ((x) * 10)
#define MS_TO_NS100(x)  ((x)*10000)
#define NS100_TO_MS(x)  ((x)/10000)
#define US_TO_MS(x)     ((x) / 1000)
#define MS_TO_US(x)     ((x) * 1000)
#define US_TO_S(x)      ((x) / (1000 * 1000))
#define S_TO_US(x)      ((x) * 1000 * 1000)
#define S_TO_NS(x)      ((x) * 1000 * 1000 * 1000)
#define MS_TO_S(x)      ((x) / 1000)
#define S_TO_MS(x)      ((x) * 1000)

#ifdef CX_PLATFORM_WINKERNEL
#include "cxplat_winkernel.h"
#elif CX_PLATFORM_WINUSER
#include "cxplat_winuser.h"
#elif CX_PLATFORM_LINUX
#include "cxplat_posix.h"
#elif CX_PLATFORM_DARWIN
#include "cxplat_posix.h"
#else
#error "Unsupported Platform"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

//
// Initializes the cxplat library. Calls to this and CxPlatUninitialize must be
// serialized and cannot overlap.
//
PAGEDX
_IRQL_requires_max_(PASSIVE_LEVEL)
CXPLAT_STATUS
CxPlatInitialize(
    void
    );

//
// Uninitializes the cxplat library. Calls to this and CxPlatInitialize must be
// serialized and cannot overlap.
//
PAGEDX
_IRQL_requires_max_(PASSIVE_LEVEL)
void
CxPlatUninitialize(
    void
    );

#ifdef DEBUG
void
CxPlatSetAllocFailDenominator(
    _In_ int32_t Value
    );

int32_t
CxPlatGetAllocFailDenominator(
    );
#endif

#if defined(__cplusplus)
}
#endif
