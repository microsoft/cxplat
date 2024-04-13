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

//
// Static Analysis Interfaces
//

#if defined(__clang__)
#define CXPLAT_NO_SANITIZE(X) __attribute__((no_sanitize(X)))
#else
#define CXPLAT_NO_SANITIZE(X)
#endif

#define CXPLAT_ANALYSIS_ASSUME(X)

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_POSIX_H
