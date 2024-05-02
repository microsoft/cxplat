/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Platform definitions.

--*/

#ifdef CX_PLATFORM_WINKERNEL
#include "cxplat_winkernel_shim.h"
#elif CX_PLATFORM_LINUX
#include "cxplat_posix_shim.h"
#elif CX_PLATFORM_DARWIN
#include "cxplat_posix_shim.h"
#endif

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

//
// Pool tags.
//
#define CXPLAT_POOL_PROC          '10xC' // Cx01
#define CXPLAT_POOL_TMP_ALLOC     '20xC' // Cx02
#define CXPLAT_POOL_CUSTOM_THREAD '30xC' // Cx03

//
// Thread create flags.
//
typedef enum CXPLAT_THREAD_FLAGS {
    CXPLAT_THREAD_FLAG_NONE               = 0x0000,
    CXPLAT_THREAD_FLAG_SET_IDEAL_PROC     = 0x0001,
    CXPLAT_THREAD_FLAG_SET_AFFINITIZE     = 0x0002,
    CXPLAT_THREAD_FLAG_HIGH_PRIORITY      = 0x0004
} CXPLAT_THREAD_FLAGS;

#ifdef DEFINE_ENUM_FLAG_OPERATORS
DEFINE_ENUM_FLAG_OPERATORS(CXPLAT_THREAD_FLAGS);
#endif

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
