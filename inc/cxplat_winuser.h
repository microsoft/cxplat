/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Windows user-mode platform definitions.

--*/

#ifndef CXPLAT_WINUSER_H
#define CXPLAT_WINUSER_H

#if defined(__cplusplus)
extern "C" {
#endif

#define FOO 3

extern char Bar;

int Baz();

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_WINUSER_H
