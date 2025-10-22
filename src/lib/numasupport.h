/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    NUMA Support abstraction layer.
    Based on .NET runtime's NUMA implementation.

--*/

#ifndef __NUMASUPPORT_H__
#define __NUMASUPPORT_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// The highest NUMA node available
extern int g_highestNumaNode;
// Is numa available
extern bool g_numaAvailable;

//
// Initialize NUMA support.
//
void NUMASupportInitialize(void);

//
// Get the NUMA node number for a given CPU.
// Returns -1 if NUMA is not available or on error.
//
int GetNumaNodeNumByCpu(int cpu);

//
// Get the number of configured NUMA nodes.
// Returns 0 if NUMA is not available.
//
int GetNumaNodeCount(void);

//
// Check if NUMA is available.
// Returns true if NUMA is available, false otherwise.
//
bool IsNumaAvailable(void);

#ifdef __cplusplus
}
#endif

#endif // __NUMASUPPORT_H__
