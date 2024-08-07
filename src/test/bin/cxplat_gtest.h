/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#if _WIN32
#include <windows.h>
#include <stdio.h>
#elif __linux__ || __APPLE__ || __FreeBSD__
// For FreeBSD
#if defined(__FreeBSD__)
#include <sys/socket.h>
#include <netinet/in.h>
#define ETIME   ETIMEDOUT
#endif
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdalign.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/time.h>
#else
#error "Unsupported Platform"
#endif

#include "cxplat.h"
#include "CxPlatTests.h"
#include "cxplat_trace.h"
#include "cxplat_driver_helpers.h"
#undef min // gtest headers conflict with previous definitions of min/max.
#undef max
#include "gtest/gtest.h"

extern bool TestingKernelMode;
