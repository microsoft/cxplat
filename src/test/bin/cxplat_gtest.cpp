/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#include "cxplat_gtest.h"

bool TestingKernelMode = false;
const char* OsRunner = nullptr;
uint32_t Timeout = UINT32_MAX;
CxplatDriverClient DriverClient;

bool IsWindows2019() { return OsRunner && strcmp(OsRunner, "windows-2019") == 0; }
bool IsWindows2022() { return OsRunner && strcmp(OsRunner, "windows-2022") == 0; }

class CxplatTestEnvironment : public ::testing::Environment {
    CxplatDriverService DriverService;
public:
    void SetUp() override {
        if (TestingKernelMode) {
            printf("Initializing for Kernel Mode tests\n");
            const char* DriverName = CXPLAT_DRIVER_NAME;
            const char* DependentDriverNames = NULL;
            ASSERT_TRUE(DriverService.Initialize(DriverName, DependentDriverNames));
            ASSERT_TRUE(DriverService.Start());
            ASSERT_TRUE(DriverClient.Initialize(DriverName));
        } else {
            printf("Initializing for User Mode tests\n");
            CxplatTestInitialize();
        }
    }
    void TearDown() override {
        if (TestingKernelMode) {
            DriverClient.Uninitialize();
            DriverService.Uninitialize();
        } else {
            CxplatTestUninitialize();
        }
    }
};

//
// This function is called by the platform independent test code when it
// encounters kind of failure. Note - It may be called on any thread.
//
void
LogTestFailure(
    _In_z_ const char* File,
    _In_z_ const char* Function,
    int Line,
    _Printf_format_string_ const char* Format,
    ...
    )
{
    UNREFERENCED_PARAMETER(Function);
    char Buffer[256];
    va_list Args;
    va_start(Args, Format);
    (void)_vsnprintf_s(Buffer, sizeof(Buffer), _TRUNCATE, Format, Args);
    va_end(Args);
    CxplatTraceLogError(
        TestLogFailure,
        "[test] FAILURE - %s:%d - %s",
        File,
        Line,
        Buffer);
    GTEST_MESSAGE_AT_(File, Line, Buffer, ::testing::TestPartResult::kFatalFailure);
}

struct TestLogger {
    const char* TestName;
    TestLogger(const char* Name) : TestName(Name) {
        CxplatTraceLogInfo(
            TestCaseStart,
            "[test] START %s",
            TestName);
    }
    ~TestLogger() {
        CxplatTraceLogInfo(
            TestCaseEnd,
            "[test] END %s",
            TestName);
    }
};

template<class T>
struct TestLoggerT {
    const char* TestName;
    TestLoggerT(const char* Name, const T& Params) : TestName(Name) {
        std::ostringstream stream; stream << Params;
        CxplatTraceLogInfo(
            TestCaseTStart,
            "[test] START %s, %s",
            TestName,
            stream.str().c_str());
    }
    ~TestLoggerT() {
        CxplatTraceLogInfo(
            TestCaseTEnd,
            "[test] END %s",
            TestName);
    }
};

TEST(DummySuite, Dummy) {
    TestLogger Logger("CxplatTestDummy");
    if (TestingKernelMode) {
        ASSERT_TRUE(DriverClient.Run(IOCTL_CXPLAT_RUN_DUMMY));
    } else {
        CxplatTestDummy();
    }
}

int main(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        if (strcmp("--kernel", argv[i]) == 0) {
            TestingKernelMode = true;
        } else if (strstr(argv[i], "--osRunner")) {
            OsRunner = argv[i] + sizeof("--osRunner");
        } else if (strcmp("--timeout", argv[i]) == 0) {
            if (i + 1 < argc) {
                Timeout = atoi(argv[i + 1]);
                ++i;
            }
        }
    }
    ::testing::AddGlobalTestEnvironment(new CxplatTestEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}