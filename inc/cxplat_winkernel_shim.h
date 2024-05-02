/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Windows kernel-mode platform shim definitions.
    Defines that are present on other platforms but not this one.

--*/

#ifndef CXPLAT_WINKERNEL_SHIM_H
#define CXPLAT_WINKERNEL_SHIM_H

#include <ntddk.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef INT8 int8_t;
typedef INT16 int16_t;
typedef INT32 int32_t;
typedef INT64 int64_t;

typedef UINT8 uint8_t;
typedef UINT16 uint16_t;
typedef UINT32 uint32_t;
typedef UINT64 uint64_t;

#define UINT8_MAX   0xffui8
#define UINT16_MAX  0xffffui16
#define UINT32_MAX  0xffffffffui32
#define UINT64_MAX  0xffffffffffffffffui64

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_WINKERNEL_SHIM_H
