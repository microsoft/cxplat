/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Posix platform definitions.

--*/

#ifndef CXPLAT_POSIX_H
#define CXPLAT_POSIX_H

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
#include "cxplat_sal_stub.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Defines that are present on other platforms but not this one
//

#define UNREFERENCED_PARAMETER(P) (void)(P)

//
// Status Codes
//

#define CXPLAT_STATUS                         unsigned int
#define CXPLAT_FAILED(X)                      ((int)(X) > 0)
#define CXPLAT_SUCCEEDED(X)                   ((int)(X) <= 0)

#define CXPLAT_STATUS_SUCCESS                 ((CXPLAT_STATUS)0)                // 0

//
// Code Annotations
//

#define INITCODE
#define PAGEDX

//
// Static Analysis Interfaces
//

#if defined(__clang__)
#define CXPLAT_NO_SANITIZE(X) __attribute__((no_sanitize(X)))
#else
#define CXPLAT_NO_SANITIZE(X)
#endif

#define CXPLAT_ANALYSIS_ASSERT(X)
#define CXPLAT_ANALYSIS_ASSUME(X)

//
// Assertion Interfaces
//

__attribute__((noinline, noreturn))
void
cxplat_bugcheck(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    );

void
CxPlatLogAssert(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    );

#define CXPLAT_STATIC_ASSERT(X,Y) static_assert(X, Y);

#define CXPLAT_FRE_ASSERT(exp) ((exp) ? (void)0 : (CxPlatLogAssert(__FILE__, __LINE__, #exp), cxplat_bugcheck(__FILE__, __LINE__, #exp)));
#define CXPLAT_FRE_ASSERTMSG(exp, Y) CXPLAT_FRE_ASSERT(exp)

#ifdef DEBUG
#define CXPLAT_DBG_ASSERT(exp) CXPLAT_FRE_ASSERT(exp)
#define CXPLAT_DBG_ASSERTMSG(exp, msg) CXPLAT_FRE_ASSERT(exp)
#else
#define CXPLAT_DBG_ASSERT(exp)
#define CXPLAT_DBG_ASSERTMSG(exp, msg)
#endif

//
// Crypto Interfaces
//

CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    );

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_POSIX_H
