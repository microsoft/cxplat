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

#if defined(__cplusplus)
}
#endif

#endif // CXPLAT_WINUSER_H
