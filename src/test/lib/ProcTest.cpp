/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Processor test.

--*/

#include "precomp.h"

void CxPlatTestProcBasic()
{
    TEST_TRUE(CxPlatProcCount() > 0);
    TEST_TRUE(CxPlatProcCurrentNumber() < CxPlatProcCount());
}
