/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

Abstract:

    This file contains helpers for interacting with a kernel mode driver service.
--*/

#pragma once

#include "cxplat_trace.h"

#if defined(_WIN32) && !defined(CXPLAT_RESTRICTED_BUILD)

//#define CXPLAT_DRIVER_FILE_NAME  CXPLAT_DRIVER_NAME ".sys"
//#define CXPLAT_IOCTL_PATH        "\\\\.\\\\" CXPLAT_DRIVER_NAME


class CxplatDriverService {
    SC_HANDLE ScmHandle;
    SC_HANDLE ServiceHandle;
public:
    CxplatDriverService() :
        ScmHandle(nullptr),
        ServiceHandle(nullptr) {
    }
    bool Initialize(
        _In_z_ const char* DriverName,
        _In_z_ const char* DependentFileNames
        ) {
        DWORD Error;
        ScmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
        if (ScmHandle == nullptr) {
            Error = GetLastError();
            CxplatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                Error,
                "GetFullPathName failed");
            return false;
        }
    QueryService:
        ServiceHandle =
            OpenServiceA(
                ScmHandle,
                DriverName,
                SERVICE_ALL_ACCESS);
        if (ServiceHandle == nullptr) {
            CxplatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                 GetLastError(),
                "OpenService failed");
            char DriverFilePath[MAX_PATH] = {0};
            GetModuleFileNameA(NULL, DriverFilePath, MAX_PATH);
            char* PathEnd = strrchr(DriverFilePath, '\\');
            if (!PathEnd) {
                CxplatTraceEvent(
                    "[ lib] ERROR, %s.",
                    "Failed to get currently executing module path");
                return false;
            }
            PathEnd++;
            size_t RemainingLength = sizeof(DriverFilePath) - (PathEnd - DriverFilePath);
            int PathResult =
                snprintf(
                    PathEnd,
                    RemainingLength,
                    "%s.sys",
                    DriverName);
            if (PathResult <= 0 || (size_t)PathResult > RemainingLength) {
                CxplatTraceEvent(
                    "[ lib] ERROR, %s.",
                    "Failed to create driver on disk file path");
                return false;
            }
            if (GetFileAttributesA(DriverFilePath) == INVALID_FILE_ATTRIBUTES) {
                CxplatTraceEvent(
                    "[ lib] ERROR, %s.",
                    "Failed to find driver on disk");
                return false;
            }
            ServiceHandle =
                CreateServiceA(
                    ScmHandle,
                    DriverName,
                    DriverName,
                    SC_MANAGER_ALL_ACCESS,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_DEMAND_START,
                    SERVICE_ERROR_NORMAL,
                    DriverFilePath,
                    nullptr,
                    nullptr,
                    DependentFileNames,
                    nullptr,
                    nullptr);
            if (ServiceHandle == nullptr) {
                Error = GetLastError();
                if (Error == ERROR_SERVICE_EXISTS) {
                    goto QueryService;
                }
                CxplatTraceEvent(
                    "[ lib] ERROR, %u, %s.",
                    Error,
                    "CreateService failed");
                return false;
            }
        }
        return true;
    }
    void Uninitialize() {
        if (ServiceHandle != nullptr) {
            CloseServiceHandle(ServiceHandle);
        }
        if (ScmHandle != nullptr) {
            CloseServiceHandle(ScmHandle);
        }
    }
    bool Start() {
        if (!StartServiceA(ServiceHandle, 0, nullptr)) {
            DWORD Error = GetLastError();
            if (Error != ERROR_SERVICE_ALREADY_RUNNING) {
                CxplatTraceEvent(
                    "[ lib] ERROR, %u, %s.",
                    Error,
                    "StartService failed");
                return false;
            }
        }
        return true;
    }
};

class CxplatDriverClient {
    HANDLE DeviceHandle;
public:
    CxplatDriverClient() : DeviceHandle(INVALID_HANDLE_VALUE) { }
    ~CxplatDriverClient() { Uninitialize(); }
    bool Initialize(
        _In_z_ const char* DriverName
        ) {
        DWORD Error;
        char IoctlPath[MAX_PATH];
        int PathResult =
            snprintf(
                IoctlPath,
                sizeof(IoctlPath),
                "\\\\.\\\\%s",
                DriverName);
        if (PathResult < 0 || PathResult >= sizeof(IoctlPath)) {
            CxplatTraceEvent(
                "[ lib] ERROR, %s.",
                "Creating Driver File Path failed");
            return false;
        }
        DeviceHandle =
            CreateFileA(
                IoctlPath,
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,                // no SECURITY_ATTRIBUTES structure
                OPEN_EXISTING,          // No special create flags
                FILE_FLAG_OVERLAPPED,   // Allow asynchronous requests
                nullptr);
        if (DeviceHandle == INVALID_HANDLE_VALUE) {
            Error = GetLastError();
            CxplatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                Error,
                "CreateFile failed");
            return false;
        }
        return true;
    }
    void Uninitialize() {
        if (DeviceHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(DeviceHandle);
            DeviceHandle = INVALID_HANDLE_VALUE;
        }
    }
    bool Run(
        _In_ DWORD IoControlCode,
        _In_reads_bytes_opt_(InBufferSize)
            void* InBuffer,
        _In_ DWORD InBufferSize,
        _In_ DWORD TimeoutMs = 30000
        ) {
        DWORD Error;
        OVERLAPPED Overlapped = { 0 };
        Overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (Overlapped.hEvent == nullptr) {
            Error = GetLastError();
            CxplatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                Error,
                "CreateEvent failed");
            return false;
        }
        CxplatTraceLogVerbose(
            "[test] Sending Write IOCTL %u with %u bytes.",
            IoGetFunctionCodeFromCtlCode(IoControlCode),
            InBufferSize);
        if (!DeviceIoControl(
                DeviceHandle,
                IoControlCode,
                InBuffer, InBufferSize,
                nullptr, 0,
                nullptr,
                &Overlapped)) {
            Error = GetLastError();
            if (Error != ERROR_IO_PENDING) {
                CloseHandle(Overlapped.hEvent);
                CxplatTraceEvent(
                    "[ lib] ERROR, %u, %s.",
                    Error,
                    "DeviceIoControl Write failed");
                return false;
            }
        }
        DWORD dwBytesReturned;
        if (!GetOverlappedResultEx(
                DeviceHandle,
                &Overlapped,
                &dwBytesReturned,
                TimeoutMs,
                FALSE)) {
            Error = GetLastError();
            if (Error == WAIT_TIMEOUT) {
                Error = ERROR_TIMEOUT;
                CancelIoEx(DeviceHandle, &Overlapped);
            }
            CxplatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                Error,
                "GetOverlappedResultEx Write failed");
        } else {
            Error = ERROR_SUCCESS;
        }
        CloseHandle(Overlapped.hEvent);
        return Error == ERROR_SUCCESS;
    }
    bool Run(
        _In_ DWORD IoControlCode,
        _In_ DWORD TimeoutMs = 30000
        ) {
        return Run(IoControlCode, nullptr, 0, TimeoutMs);
    }
    template<class T>
    bool Run(
        _In_ DWORD IoControlCode,
        _In_ const T& Data,
        _In_ DWORD TimeoutMs = 30000
        ) {
        return Run(IoControlCode, (void*)&Data, sizeof(Data), TimeoutMs);
    }
    bool Read(
        _In_ DWORD IoControlCode,
        _Out_writes_bytes_opt_(OutBufferSize)
            void* OutBuffer,
        _In_ DWORD OutBufferSize,
        _Out_opt_ DWORD* OutBufferWritten,
        _In_ DWORD TimeoutMs = 30000
        ) {
        DWORD Error;
        OVERLAPPED Overlapped = { 0 };
        Overlapped.hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!Overlapped.hEvent) {
            Error = GetLastError();
            CxplatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                Error,
                "CreateEvent failed");
            return false;
        }
        CxplatTraceLogVerbose(
            "[test] Sending Read IOCTL %u.",
            IoGetFunctionCodeFromCtlCode(IoControlCode));
        if (!DeviceIoControl(
                DeviceHandle,
                IoControlCode,
                nullptr, 0,
                OutBuffer, OutBufferSize,
                nullptr,
                &Overlapped)) {
            Error = GetLastError();
            if (Error != ERROR_IO_PENDING) {
                CloseHandle(Overlapped.hEvent);
                CxplatTraceEvent(
                    "[ lib] ERROR, %u, %s.",
                    Error,
                    "DeviceIoControl Write failed");
                return false;
            }
        }
        DWORD dwBytesReturned;
        if (!GetOverlappedResultEx(
                DeviceHandle,
                &Overlapped,
                &dwBytesReturned,
                TimeoutMs,
                FALSE)) {
            Error = GetLastError();
            if (Error == WAIT_TIMEOUT) {
                Error = ERROR_TIMEOUT;
                if (CancelIoEx(DeviceHandle, &Overlapped)) {
                    GetOverlappedResult(DeviceHandle, &Overlapped, &dwBytesReturned, true);
                }
            } else {
                CxplatTraceEvent(
                    "[ lib] ERROR, %u, %s.",
                    Error,
                    "GetOverlappedResultEx Read failed");
            }
        } else {
            Error = ERROR_SUCCESS;
            *OutBufferWritten = dwBytesReturned;
        }
        CloseHandle(Overlapped.hEvent);
        return Error == ERROR_SUCCESS;
    }
};

#else

class CxplatDriverService {
public:
    bool Initialize(
        _In_z_ const char* DriverName,
        _In_z_ const char* DependentFileNames
        ) {
        UNREFERENCED_PARAMETER(DriverName);
        UNREFERENCED_PARAMETER(DependentFileNames);
        return false;
        }
    void Uninitialize() { }
    bool Start() { return false; }
};

class CxplatDriverClient {
public:
    bool Initialize(
        _In_z_ const char* DriverName
    ) {
        UNREFERENCED_PARAMETER(DriverName);
        return false;
    }
    void Uninitialize() { }
    bool Run(
        _In_ DWORD IoControlCode,
        _In_ void* InBuffer,
        _In_ DWORD InBufferSize,
        _In_ DWORD TimeoutMs = 30000
        ) {
        UNREFERENCED_PARAMETER(IoControlCode);
        UNREFERENCED_PARAMETER(InBuffer);
        UNREFERENCED_PARAMETER(InBufferSize);
        UNREFERENCED_PARAMETER(TimeoutMs);
        return false;
    }
    bool
    Run(
        _In_ DWORD IoControlCode,
        _In_ DWORD TimeoutMs = 30000
        ) {
        return Run(IoControlCode, nullptr, 0, TimeoutMs);
    }
    template<class T>
    bool
    Run(
        _In_ DWORD IoControlCode,
        _In_ const T& Data,
        _In_ DWORD TimeoutMs = 30000
        ) {
        return Run(IoControlCode, (void*)&Data, sizeof(Data), TimeoutMs);
    }
    bool Read(
        _In_ DWORD IoControlCode,
        _Out_writes_bytes_opt_(OutBufferSize)
            void* OutBuffer,
        _In_ DWORD OutBufferSize,
        _Out_ DWORD* OutBufferWritten,
        _In_ DWORD TimeoutMs = 30000
        ) {
        UNREFERENCED_PARAMETER(IoControlCode);
        UNREFERENCED_PARAMETER(OutBuffer);
        UNREFERENCED_PARAMETER(OutBufferSize);
        UNREFERENCED_PARAMETER(OutBufferWritten);
        UNREFERENCED_PARAMETER(TimeoutMs);
        return false;
    }
};

#endif
