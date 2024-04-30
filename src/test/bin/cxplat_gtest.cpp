/*++

    Copyright (c) Microsoft Corporation.
    Licensed under the MIT License.

--*/

#include "cxplat_gtest.h"

#ifndef _WIN32
#define _vsnprintf_s(dst, dst_len, flag, format, ...) vsnprintf(dst, dst_len, format, __VA_ARGS__)
#endif

bool TestingKernelMode = false;
const char* OsRunner = nullptr;
uint32_t Timeout = UINT32_MAX;
CxPlatDriverClient DriverClient;

class CxPlatTestEnvironment : public ::testing::Environment {
    CxPlatDriverService DriverService;
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
            CxPlatTestInitialize();
        }
    }
    void TearDown() override {
        if (TestingKernelMode) {
            DriverClient.Uninitialize();
            DriverService.Uninitialize();
        } else {
            CxPlatTestUninitialize();
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
    CxPlatTraceLogError(
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
        CxPlatTraceLogInfo(
            TestCaseStart,
            "[test] START %s",
            TestName);
    }
    ~TestLogger() {
        CxPlatTraceLogInfo(
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
        CxPlatTraceLogInfo(
            TestCaseTStart,
            "[test] START %s, %s",
            TestName,
            stream.str().c_str());
    }
    ~TestLoggerT() {
        CxPlatTraceLogInfo(
            TestCaseTEnd,
            "[test] END %s",
            TestName);
    }
};

TEST(CryptSuite, Random) {
    TestLogger Logger("CxPlatTestCryptRandom");
    if (TestingKernelMode) {
        ASSERT_TRUE(DriverClient.Run(IOCTL_CXPLAT_RUN_CRYPT_RANDOM));
    } else {
        CxPlatTestCryptRandom();
    }
}

TEST(MemorySuite, Basic) {
    TestLogger Logger("CxPlatTestMemoryBasic");
    if (TestingKernelMode) {
        ASSERT_TRUE(DriverClient.Run(IOCTL_CXPLAT_RUN_MEMORY_BASIC));
    } else {
        CxPlatTestMemoryBasic();
    }
}

#if DEBUG
TEST(MemorySuite, FailureInjection) {
    TestLogger Logger("CxPlatTestMemoryFailureInjection");
    if (TestingKernelMode) {
        GTEST_SKIP_("winkernel platform does not currently support failure injection");
    } else {
        CxPlatTestMemoryBasic();
    }
}
#endif

TEST(TimeSuite, Basic) {
    TestLogger Logger("CxPlatTestTimeBasic");
    if (TestingKernelMode) {
        ASSERT_TRUE(DriverClient.Run(IOCTL_CXPLAT_RUN_TIME_BASIC));
    } else {
        CxPlatTestTimeBasic();
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
    ::testing::AddGlobalTestEnvironment(new CxPlatTestEnvironment);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
