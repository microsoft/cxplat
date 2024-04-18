/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#if defined(CX_PLATFORM_WINKERNEL)
#include <wdm.h>
#elif defined(CX_PLATFORM_WINUSER)
#include <windows.h>
#elif defined(CX_PLATFORM_LINUX)
#error TODO_LINUX
#elif defined(CX_PLATFORM_DARWIN)
#error TODO_DARWIN
#else
#error "Unsupported Platform"
#endif

#include "TestAbstractionLayer.h"

#include "cxplat.h"

#if defined(_ARM64_) || defined(_ARM64EC_)
#pragma optimize("", off)
#endif
