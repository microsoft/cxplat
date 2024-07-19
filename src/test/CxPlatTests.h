/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    Interface for the Platform Independent CxPlat Tests

--*/

#ifdef __cplusplus
extern "C" {
#endif

void CxPlatTestInitialize();
void CxPlatTestUninitialize();

//
// Crypt Tests
//

void CxPlatTestCryptRandom();

//
// Memory/Allocation Tests
//

void CxPlatTestMemoryBasic();
#if DEBUG
void CxPlatTestMemoryFailureInjection();
#endif

//
// Time Tests
//

void CxPlatTestTimeBasic();

//
// Event Tests
//

void CxPlatTestEventBasic();

//
// Processor Tests
//

void CxPlatTestProcBasic();

//
// Thread Tests
//

void CxPlatTestThreadBasic();
void CxPlatTestThreadAsync();
#if defined(CX_PLATFORM_WINUSER) || defined(CX_PLATFORM_WINKERNEL)
void CxPlatTestThreadWaitTimeout();
#endif

//
// Vector Tests
//
void VectorBasic();

//
// Platform Specific Functions
//

void
LogTestFailure(
    _In_z_ const char *File,
    _In_z_ const char *Function,
    int Line,
    _Printf_format_string_ const char *Format,
    ...
    );

#ifdef __cplusplus
}
#endif

//
// Kernel Mode Driver Interface
//

//
// Name of the driver service for cxplattest.sys.
//
#define CXPLAT_DRIVER_NAME            "cxplattest"

#ifdef _WIN32

//
// {3A37B2CB-39A6-426A-BAF4-77D0ED0070B3}
//
static const GUID CXPLAT_TEST_DEVICE_INSTANCE =
{ 0x3a37b2cb, 0x39a6, 0x426a,{ 0xba, 0xf4, 0x77, 0xd0, 0xed, 0x00, 0x70, 0xb3 } };

#ifndef _KERNEL_MODE
#include <winioctl.h>
#endif // _KERNEL_MODE

#define CXPLAT_CTL_CODE(request, method, access) \
    CTL_CODE(FILE_DEVICE_NETWORK, request, method, access)

#define IoGetFunctionCodeFromCtlCode( ControlCode ) (\
    ( ControlCode >> 2) & 0x00000FFF )

#else // _WIN32

#define CXPLAT_CTL_CODE(request, method, access) (request)

#endif // _WIN32

//
// IOCTL Interface
//

#define IOCTL_CXPLAT_RUN_CRYPT_RANDOM \
    CXPLAT_CTL_CODE(1, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_MEMORY_BASIC \
    CXPLAT_CTL_CODE(2, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_TIME_BASIC \
    CXPLAT_CTL_CODE(3, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_EVENT_BASIC \
    CXPLAT_CTL_CODE(4, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_PROC_BASIC \
    CXPLAT_CTL_CODE(5, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_THREAD_BASIC \
    CXPLAT_CTL_CODE(6, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_THREAD_ASYNC \
    CXPLAT_CTL_CODE(7, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_THREAD_WAIT_TIMEOUT \
    CXPLAT_CTL_CODE(8, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_CXPLAT_RUN_VECTOR_BASIC \
    CXPLAT_CTL_CODE(9, METHOD_BUFFERED, FILE_WRITE_DATA)

#define CXPLAT_MAX_IOCTL_FUNC_CODE 9
