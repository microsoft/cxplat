/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Posix platform definitions.

--*/

#ifndef CXPLAT_POSIX_H
#define CXPLAT_POSIX_H

#if defined(__cplusplus)
extern "C" {
#endif

#define FOO 3

extern char Bar;

int Baz();

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_POSIX_H
