/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    CXPLAT Kernel Mode Test Driver

--*/

#include <ntddk.h>
#include <wdf.h>
#include <ntstrsafe.h>

#include "CxPlatTests.h"

#include "cxplat_trace.h"

#ifndef KRTL_INIT_SEGMENT
#define KRTL_INIT_SEGMENT "INIT"
#endif
#ifndef KRTL_PAGE_SEGMENT
#define KRTL_PAGE_SEGMENT "PAGE"
#endif
#ifndef KRTL_NONPAGED_SEGMENT
#define KRTL_NONPAGED_SEGMENT ".text"
#endif

// Use on code in the INIT segment. (Code is discarded after DriverEntry returns.)
#define INITCODE __declspec(code_seg(KRTL_INIT_SEGMENT))

// Use on pageable functions.
#define PAGEDX __declspec(code_seg(KRTL_PAGE_SEGMENT))

DECLARE_CONST_UNICODE_STRING(CxPlatTestCtlDeviceName, L"\\Device\\" CXPLAT_DRIVER_NAME);
DECLARE_CONST_UNICODE_STRING(CxPlatTestCtlDeviceSymLink, L"\\DosDevices\\" CXPLAT_DRIVER_NAME);

typedef struct CXPLAT_DEVICE_EXTENSION {
    EX_PUSH_LOCK Lock;

    _Guarded_by_(Lock)
    LIST_ENTRY ClientList;
    ULONG ClientListSize;

} CXPLAT_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CXPLAT_DEVICE_EXTENSION, CxPlatTestCtlGetDeviceContext);

typedef struct CXPLAT_TEST_CLIENT
{
    LIST_ENTRY Link;
    bool TestFailure;

} CXPLAT_TEST_CLIENT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CXPLAT_TEST_CLIENT, CxPlatTestCtlGetFileContext);

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL CxPlatTestCtlEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE CxPlatTestCtlEvtIoCanceled;

PAGEDX EVT_WDF_DEVICE_FILE_CREATE CxPlatTestCtlEvtFileCreate;
PAGEDX EVT_WDF_FILE_CLOSE CxPlatTestCtlEvtFileClose;
PAGEDX EVT_WDF_FILE_CLEANUP CxPlatTestCtlEvtFileCleanup;

WDFDEVICE CxPlatTestCtlDevice = nullptr;
CXPLAT_DEVICE_EXTENSION* CxPlatTestCtlExtension = nullptr;
CXPLAT_TEST_CLIENT* CxPlatTestClient = nullptr;

_No_competing_thread_
INITCODE
NTSTATUS
CxPlatTestCtlInitialize(
    _In_ WDFDRIVER Driver
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWDFDEVICE_INIT DeviceInit = nullptr;
    WDF_FILEOBJECT_CONFIG FileConfig;
    WDF_OBJECT_ATTRIBUTES Attribs;
    WDFDEVICE Device;
    CXPLAT_DEVICE_EXTENSION* DeviceContext;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    WDFQUEUE Queue;

    DeviceInit =
        WdfControlDeviceInitAllocate(
            Driver,
            &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
    if (DeviceInit == nullptr) {
        CxPlatTraceEvent(
            LibraryError,
            "[ lib] ERROR, %s.",
            "WdfControlDeviceInitAllocate failed");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    Status =
        WdfDeviceInitAssignName(
            DeviceInit,
            &CxPlatTestCtlDeviceName);
    if (!NT_SUCCESS(Status)) {
        CxPlatTraceEvent(
            LibraryErrorStatus,
            "[ lib] ERROR, %u, %s.",
            Status,
            "WdfDeviceInitAssignName failed");
        goto Error;
    }

    WDF_FILEOBJECT_CONFIG_INIT(
        &FileConfig,
        CxPlatTestCtlEvtFileCreate,
        CxPlatTestCtlEvtFileClose,
        CxPlatTestCtlEvtFileCleanup);
    FileConfig.FileObjectClass = WdfFileObjectWdfCanUseFsContext2;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attribs, CXPLAT_TEST_CLIENT);
    WdfDeviceInitSetFileObjectConfig(
        DeviceInit,
        &FileConfig,
        &Attribs);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attribs, CXPLAT_DEVICE_EXTENSION);

    Status =
        WdfDeviceCreate(
            &DeviceInit,
            &Attribs,
            &Device);
    if (!NT_SUCCESS(Status)) {
        CxPlatTraceEvent(
            LibraryErrorStatus,
            "[ lib] ERROR, %u, %s.",
            Status,
            "WdfDeviceCreate failed");
        goto Error;
    }

    DeviceContext = CxPlatTestCtlGetDeviceContext(Device);
    RtlZeroMemory(DeviceContext, sizeof(CXPLAT_DEVICE_EXTENSION));
    ExInitializePushLock(&DeviceContext->Lock);
    InitializeListHead(&DeviceContext->ClientList);

    Status = WdfDeviceCreateSymbolicLink(Device, &CxPlatTestCtlDeviceSymLink);
    if (!NT_SUCCESS(Status)) {
        CxPlatTraceEvent(
            LibraryErrorStatus,
            "[ lib] ERROR, %u, %s.",
            Status,
            "WdfDeviceCreateSymbolicLink failed");
        goto Error;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);
    QueueConfig.EvtIoDeviceControl = CxPlatTestCtlEvtIoDeviceControl;
    QueueConfig.EvtIoCanceledOnQueue = CxPlatTestCtlEvtIoCanceled;

    __analysis_assume(QueueConfig.EvtIoStop != 0);
    Status =
        WdfIoQueueCreate(
            Device,
            &QueueConfig,
            WDF_NO_OBJECT_ATTRIBUTES,
            &Queue);
    __analysis_assume(QueueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(Status)) {
        CxPlatTraceEvent(
            LibraryErrorStatus,
            "[ lib] ERROR, %u, %s.",
            Status,
            "WdfIoQueueCreate failed");
        goto Error;
    }

    CxPlatTestCtlDevice = Device;
    CxPlatTestCtlExtension = DeviceContext;

    WdfControlFinishInitializing(Device);

    CxPlatTraceLogVerbose(
        TestControlInitialized,
        "[test] Control interface initialized");

Error:

    if (DeviceInit) {
        WdfDeviceInitFree(DeviceInit);
    }

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
CxPlatTestCtlUninitialize(
    )
{
    CxPlatTraceLogVerbose(
        TestControlUninitializing,
        "[test] Control interface uninitializing");

    if (CxPlatTestCtlDevice != nullptr) {
        NT_ASSERT(CxPlatTestCtlExtension != nullptr);
        CxPlatTestCtlExtension = nullptr;

        WdfObjectDelete(CxPlatTestCtlDevice);
        CxPlatTestCtlDevice = nullptr;
    }

    CxPlatTraceLogVerbose(
        TestControlUninitialized,
        "[test] Control interface uninitialized");
}

PAGEDX
_Use_decl_annotations_
VOID
CxPlatTestCtlEvtFileCreate(
    _In_ WDFDEVICE /* Device */,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT FileObject
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    KeEnterGuardedRegion();
    ExAcquirePushLockExclusive(&CxPlatTestCtlExtension->Lock);

    do
    {
        if (CxPlatTestCtlExtension->ClientListSize >= 1) {
            CxPlatTraceEvent(
                LibraryError,
                "[ lib] ERROR, %s.",
                "Already have max clients");
            Status = STATUS_TOO_MANY_SESSIONS;
            break;
        }

        CXPLAT_TEST_CLIENT* Client = CxPlatTestCtlGetFileContext(FileObject);
        if (Client == nullptr) {
            CxPlatTraceEvent(
                LibraryError,
                "[ lib] ERROR, %s.",
                "nullptr File context in FileCreate");
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        RtlZeroMemory(Client, sizeof(CXPLAT_TEST_CLIENT));

        //
        // Insert into the client list
        //
        InsertTailList(&CxPlatTestCtlExtension->ClientList, &Client->Link);
        CxPlatTestCtlExtension->ClientListSize++;

        CxPlatTraceLogInfo(
            TestControlClientCreated,
            "[test] Client %p created",
            Client);

        //
        // TODO: Add multiple device client support?
        //
        CxPlatTestClient = Client;
    }
    while (false);

    ExReleasePushLockExclusive(&CxPlatTestCtlExtension->Lock);
    KeLeaveGuardedRegion();

    WdfRequestComplete(Request, Status);
}

PAGEDX
_Use_decl_annotations_
VOID
CxPlatTestCtlEvtFileClose(
    _In_ WDFFILEOBJECT /* FileObject */
    )
{
    PAGED_CODE();
}

PAGEDX
_Use_decl_annotations_
VOID
CxPlatTestCtlEvtFileCleanup(
    _In_ WDFFILEOBJECT FileObject
    )
{
    PAGED_CODE();

    KeEnterGuardedRegion();

    CXPLAT_TEST_CLIENT* Client = CxPlatTestCtlGetFileContext(FileObject);
    if (Client != nullptr) {

        ExAcquirePushLockExclusive(&CxPlatTestCtlExtension->Lock);

        //
        // Remove the device client from the list
        //
        RemoveEntryList(&Client->Link);
        CxPlatTestCtlExtension->ClientListSize--;

        ExReleasePushLockExclusive(&CxPlatTestCtlExtension->Lock);

        CxPlatTraceLogInfo(
            TestControlClientCleaningUp,
            "[test] Client %p cleaning up",
            Client);

        CxPlatTestClient = nullptr;
    }

    KeLeaveGuardedRegion();
}

VOID
CxPlatTestCtlEvtIoCanceled(
    _In_ WDFQUEUE /* Queue */,
    _In_ WDFREQUEST Request
    )
{
    NTSTATUS Status;

    WDFFILEOBJECT FileObject = WdfRequestGetFileObject(Request);
    if (FileObject == nullptr) {
        Status = STATUS_DEVICE_NOT_READY;
        goto error;
    }

    CXPLAT_TEST_CLIENT* Client = CxPlatTestCtlGetFileContext(FileObject);
    if (Client == nullptr) {
        Status = STATUS_DEVICE_NOT_READY;
        goto error;
    }

    CxPlatTraceLogWarning(
        TestControlClientCanceledRequest,
        "[test] Client %p canceled request %p",
        Client,
        Request);

    Status = STATUS_CANCELLED;

error:

    WdfRequestComplete(Request, Status);
}

size_t CXPLAT_IOCTL_BUFFER_SIZES[] =
{
    0,
    0,
    0,
    0,
    0,
    0,
};

static_assert(
    CXPLAT_MAX_IOCTL_FUNC_CODE + 1 == (sizeof(CXPLAT_IOCTL_BUFFER_SIZES)/sizeof(size_t)),
    "CXPLAT_IOCTL_BUFFER_SIZES must be kept in sync with the IOCTLs");

typedef union {
} CXPLAT_IOCTL_PARAMS;

#define CxPlatTestCtlRun(X) \
    Client->TestFailure = false; \
    X; \
    Status = Client->TestFailure ? STATUS_FAIL_FAST_EXCEPTION : STATUS_SUCCESS;

VOID
CxPlatTestCtlEvtIoDeviceControl(
    _In_ WDFQUEUE /* Queue */,
    _In_ WDFREQUEST Request,
    _In_ size_t /* OutputBufferLength */,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDFFILEOBJECT FileObject = nullptr;
    CXPLAT_TEST_CLIENT* Client = nullptr;

    if (KeGetCurrentIrql() > PASSIVE_LEVEL) {
        Status = STATUS_NOT_SUPPORTED;
        CxPlatTraceEvent(
            LibraryError,
            "[ lib] ERROR, %s.",
            "IOCTL not supported greater than PASSIVE_LEVEL");
        goto Error;
    }

    FileObject = WdfRequestGetFileObject(Request);
    if (FileObject == nullptr) {
        Status = STATUS_DEVICE_NOT_READY;
        CxPlatTraceEvent(
            LibraryError,
            "[ lib] ERROR, %s.",
            "WdfRequestGetFileObject failed");
        goto Error;
    }

    Client = CxPlatTestCtlGetFileContext(FileObject);
    if (Client == nullptr) {
        Status = STATUS_DEVICE_NOT_READY;
        CxPlatTraceEvent(
            LibraryError,
            "[ lib] ERROR, %s.",
            "CxPlatTestCtlGetFileContext failed");
        goto Error;
    }

    ULONG FunctionCode = IoGetFunctionCodeFromCtlCode(IoControlCode);
    if (FunctionCode == 0 || FunctionCode > CXPLAT_MAX_IOCTL_FUNC_CODE) {
        Status = STATUS_NOT_IMPLEMENTED;
        CxPlatTraceEvent(
            LibraryErrorStatus,
            "[ lib] ERROR, %u, %s.",
            FunctionCode,
            "Invalid FunctionCode");
        goto Error;
    }

    if (InputBufferLength < CXPLAT_IOCTL_BUFFER_SIZES[FunctionCode]) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        CxPlatTraceEvent(
            LibraryErrorStatus,
            "[ lib] ERROR, %u, %s.",
            FunctionCode,
            "Invalid buffer size for FunctionCode");
        goto Error;
    }

    CXPLAT_IOCTL_PARAMS* Params = nullptr;
    if (CXPLAT_IOCTL_BUFFER_SIZES[FunctionCode] != 0) {
        Status =
            WdfRequestRetrieveInputBuffer(
                Request,
                CXPLAT_IOCTL_BUFFER_SIZES[FunctionCode],
                (void**)&Params,
                nullptr);
        if (!NT_SUCCESS(Status)) {
            CxPlatTraceEvent(
                LibraryErrorStatus,
                "[ lib] ERROR, %u, %s.",
                Status,
                "WdfRequestRetrieveInputBuffer failed");
            goto Error;
        } else if (Params == nullptr) {
            CxPlatTraceEvent(
                LibraryError,
                "[ lib] ERROR, %s.",
                "WdfRequestRetrieveInputBuffer failed to return parameter buffer");
            Status = STATUS_INVALID_PARAMETER;
            goto Error;
        }
    }

    CxPlatTraceLogInfo(
        TestControlClientIoctl,
        "[test] Client %p executing IOCTL %u",
        Client,
        FunctionCode);

    switch (IoControlCode) {

    case IOCTL_CXPLAT_RUN_CRYPT_RANDOM:
        CxPlatTestCtlRun(CxPlatTestCryptRandom());
        break;

    case IOCTL_CXPLAT_RUN_MEMORY_BASIC:
        CxPlatTestCtlRun(CxPlatTestMemoryBasic());
        break;

    case IOCTL_CXPLAT_RUN_TIME_BASIC:
        CxPlatTestCtlRun(CxPlatTestTimeBasic());
        break;

    case IOCTL_CXPLAT_RUN_EVENT_BASIC:
        CxPlatTestCtlRun(CxPlatTestEventBasic());
        break;

    case IOCTL_CXPLAT_RUN_PROC_BASIC:
        CxPlatTestCtlRun(CxPlatTestProcBasic());
        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

Error:

    CxPlatTraceLogInfo(
        TestControlClientIoctlComplete,
        "[test] Client %p completing request, 0x%x",
        Client,
        Status);

    WdfRequestComplete(Request, Status);
}

void
LogTestFailure(
    _In_z_ const char *File,
    _In_z_ const char *Function,
    int Line,
    _Printf_format_string_ const char *Format,
    ...
    )
/*++

Routine Description:

    Records a test failure from the platform independent test code.

Arguments:

    File - The file where the failure occurred.

    Function - The function where the failure occurred.

    Line - The line (in File) where the failure occurred.

Return Value:

    None

--*/
{
    char Buffer[128];

    UNREFERENCED_PARAMETER(File);
    UNREFERENCED_PARAMETER(Function);
    UNREFERENCED_PARAMETER(Line);

    NT_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    CxPlatTestClient->TestFailure = true;

    va_list Args;
    va_start(Args, Format);
    (void)_vsnprintf_s(Buffer, sizeof(Buffer), _TRUNCATE, Format, Args);
    va_end(Args);

    CxPlatTraceLogError(
        TestDriverFailureLocation,
        "[test] File: %s, Function: %s, Line: %d",
        File,
        Function,
        Line);
    CxPlatTraceLogError(
        TestDriverFailure,
        "[test] FAIL: %s",
        Buffer);

#if CXPLAT_BREAK_TEST
    NT_FRE_ASSERT(FALSE);
#endif
}
