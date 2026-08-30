/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    NUMA Support abstraction layer.
    Based on .NET runtime's NUMA implementation.

--*/

#include "numasupport.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>

#ifdef __linux__
#include <dirent.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

// The highest NUMA node available
int g_highestNumaNode = 0;
// Is numa available
bool g_numaAvailable = false;

#ifdef __linux__

#ifndef STRING_LENGTH
#define STRING_LENGTH(s) (sizeof(s) / sizeof(s[0]) - 1)
#endif

//
// Helper function to get NUMA node number from a directory path.
// If firstOnly is true, returns the first node found.
// Otherwise, returns the highest node number found.
//
static int GetNodeNum(const char* path, bool firstOnly)
{
    DIR *dir;
    struct dirent *entry;
    int result = -1;

    dir = opendir(path);
    if (dir)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if (strncmp(entry->d_name, "node", STRING_LENGTH("node")) != 0)
                continue;

            unsigned long nodeNum = strtoul(entry->d_name + STRING_LENGTH("node"), NULL, 0);
            if (nodeNum > INT_MAX)
                nodeNum = INT_MAX;

            if (result < (int)nodeNum)
                result = (int)nodeNum;

            if (firstOnly)
                break;
        }

        closedir(dir);
    }

    return result;
}

#endif // __linux__

void NUMASupportInitialize(void)
{
#ifdef __linux__
    // Check if get_mempolicy syscall is available
    if (syscall(__NR_get_mempolicy, NULL, NULL, 0, 0, 0) < 0)
    {
        g_numaAvailable = false;
        g_highestNumaNode = 0;
        return;
    }

    int highestNumaNode = GetNodeNum("/sys/devices/system/node", false);
    // we only use this implementation when there are two or more NUMA nodes available
    if (highestNumaNode < 1)
    {
        g_numaAvailable = false;
        g_highestNumaNode = 0;
        return;
    }

    g_numaAvailable = true;
    g_highestNumaNode = highestNumaNode;
#else
    g_numaAvailable = false;
    g_highestNumaNode = 0;
#endif
}

int GetNumaNodeNumByCpu(int cpu)
{
#ifdef __linux__
    char path[64];
    if (snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d", cpu) < 0)
        return -1;

    return GetNodeNum(path, true);
#else
    (void)cpu; // Suppress unused parameter warning
    return -1;
#endif
}

int GetNumaNodeCount(void)
{
    if (!g_numaAvailable)
        return 0;
    
    // Node count is highest node + 1 (nodes are 0-indexed)
    return g_highestNumaNode + 1;
}

bool IsNumaAvailable(void)
{
    return g_numaAvailable;
}
