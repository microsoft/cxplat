/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Platform definitions.

--*/

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

#if defined(__cplusplus)
}
#endif
