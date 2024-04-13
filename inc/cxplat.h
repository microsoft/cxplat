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
