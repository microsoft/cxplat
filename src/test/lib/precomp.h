/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#if defined(CX_PLATFORM_WINKERNEL)
#include <wdm.h>
#elif defined(CX_PLATFORM_WINUSER)
#include <windows.h>
#elif defined(CX_PLATFORM_LINUX) || defined(CX_PLATFORM_DARWIN)
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

#include "TestAbstractionLayer.h"

#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
#include "cxplatvector.h"
#endif

#if defined(_ARM64_) || defined(_ARM64EC_)
#pragma optimize("", off)
#endif
