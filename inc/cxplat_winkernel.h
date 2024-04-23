/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Windows kernel-mode platform definitions.

--*/

#ifndef CXPLAT_WINKERNEL_H
#define CXPLAT_WINKERNEL_H

#include <ntddk.h>

#if defined(__cplusplus)
extern "C" {
#endif

//
// Defines that are present on other platforms but not this one
//

typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;

typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

//
// Status Codes
//

#define CXPLAT_STATUS                         NTSTATUS
#define CXPLAT_FAILED(X)                      (!NT_SUCCESS(X))
#define CXPLAT_SUCCEEDED(X)                   NT_SUCCESS(X)

#define CXPLAT_STATUS_SUCCESS                 STATUS_SUCCESS                    // 0x0

//
// Code Annotations
//

#ifndef KRTL_INIT_SEGMENT
#define KRTL_INIT_SEGMENT "INIT"
#endif
#ifndef KRTL_PAGE_SEGMENT
#define KRTL_PAGE_SEGMENT "PAGE"
#endif
#ifndef KRTL_NONPAGED_SEGMENT
#define KRTL_NONPAGED_SEGMENT ".text"
#endif

// Use on code in the INIT segment. (Code is discarded after DriverEntry returns.)
#define INITCODE __declspec(code_seg(KRTL_INIT_SEGMENT))

// Use on pageable functions.
#define PAGEDX __declspec(code_seg(KRTL_PAGE_SEGMENT))

//
// Static Analysis Interfaces
//

#define CXPLAT_NO_SANITIZE(X)

#if defined(_PREFAST_)
// _Analysis_assume_ will never result in any code generation for _exp,
// so using it will not have runtime impact, even if _exp has side effects.
#define CXPLAT_ANALYSIS_ASSUME(_exp) _Analysis_assume_(_exp)
#else // _PREFAST_
// CXPLAT_ANALYSIS_ASSUME ensures that _exp is parsed in non-analysis compile.
// On DEBUG, it's guaranteed to be parsed as part of the normal compile, but
// with non-DEBUG, use __noop to ensure _exp is parseable but without code
// generation.
#if DEBUG
#define CXPLAT_ANALYSIS_ASSUME(_exp) ((void) 0)
#else // DEBUG
#define CXPLAT_ANALYSIS_ASSUME(_exp) __noop(_exp)
#endif // DEBUG
#endif // _PREFAST_

#define CXPLAT_ANALYSIS_ASSERT(X) __analysis_assert(X)

//
// Assertion Interfaces
//

_IRQL_requires_max_(DISPATCH_LEVEL)
void
CxPlatLogAssert(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    );

#define CXPLAT_WIDE_STRING(_str) L##_str

#define CXPLAT_ASSERT_NOOP(_exp, _msg) \
    (CXPLAT_ANALYSIS_ASSUME(_exp), 0)

#define CXPLAT_ASSERT_CRASH(_exp, _msg) \
    (CXPLAT_ANALYSIS_ASSUME(_exp), \
    ((!(_exp)) ? \
        (CxPlatLogAssert(__FILE__, __LINE__, #_exp), \
         __annotation(L"Debug", L"AssertFail", _msg), \
         DbgRaiseAssertionFailure(), FALSE) : \
        TRUE))

#define CXPLAT_STATIC_ASSERT(X,Y) static_assert(X,Y)

#define CXPLAT_FRE_ASSERT(_exp)          CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(#_exp))
#define CXPLAT_FRE_ASSERTMSG(_exp, _msg) CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(_msg))

#if DEBUG
#define CXPLAT_DBG_ASSERT(_exp)          CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(#_exp))
#define CXPLAT_DBG_ASSERTMSG(_exp, _msg) CXPLAT_ASSERT_CRASH(_exp, CXPLAT_WIDE_STRING(_msg))
#else
#define CXPLAT_DBG_ASSERT(_exp)          CXPLAT_ASSERT_NOOP(_exp, CXPLAT_WIDE_STRING(#_exp))
#define CXPLAT_DBG_ASSERTMSG(_exp, _msg) CXPLAT_ASSERT_NOOP(_exp, CXPLAT_WIDE_STRING(_msg))
#endif

//
// Crypto Interfaces
//

_IRQL_requires_max_(DISPATCH_LEVEL)
CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    );

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_WINKERNEL_H
