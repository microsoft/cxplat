/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Windows kernel-mode platform definitions.

--*/

#ifndef CXPLAT_WINKERNEL_H
#define CXPLAT_WINKERNEL_H

#if defined(__cplusplus)
extern "C" {
#endif

#define FOO 3

extern char Bar;

int Baz();

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_WINKERNEL_H
