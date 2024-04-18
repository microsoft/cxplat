/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#if defined(CX_PLATFORM_WINKERNEL)
#error TODO_WINKERNEL
#elif defined(CX_PLATFORM_WINUSER)
#include <windows.h>
#include <stdio.h>
#elif defined(CX_PLATFORM_LINUX)
#error TODO_LINUX
#elif defined(CX_PLATFORM_DARWIN)
#error TODO_DARWIN
#else
#error "Unsupported Platform"
#endif

#include "cxplat.h"
#include "CxplatTests.h"
#include "cxplat_trace.h"
#include "cxplat_driver_helpers.h"
#undef min // gtest headers conflict with previous definitions of min/max.
#undef max
#include "gtest/gtest.h"

extern bool TestingKernelMode;
