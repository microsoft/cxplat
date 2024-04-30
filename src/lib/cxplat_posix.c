#include "cxplat.h"
#include "cxplat_trace.h"

// For FreeBSD
#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <sched.h>
#include <syslog.h>

typedef struct CX_PLATFORM {

    void* Reserved; // Nothing right now.

#if DEBUG
    //
    // 1/Denominator of allocations to fail.
    // Negative is Nth allocation to fail.
    //
    int32_t AllocFailDenominator;

    //
    // Count of allocations.
    //
    long AllocCounter;
#endif

} CX_PLATFORM;

CX_PLATFORM CxPlatform = { NULL };

//
// Used for reading random numbers.
//
int RandomFd = -1;

#ifdef __clang__
__attribute__((noinline, noreturn, optnone))
#else
__attribute__((noinline, noreturn, optimize("O0")))
#endif
void
cxplat_bugcheck(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    )
{
    //
    // Pass in the error info so it can be seen in the debugger.
    //
    UNREFERENCED_PARAMETER(File);
    UNREFERENCED_PARAMETER(Line);
    UNREFERENCED_PARAMETER(Expr);

    //
    // We want to prevent this routine from being inlined so that we can
    // easily detect when our bugcheck conditions have occurred just by
    // looking at callstack. However, even after specifying inline attribute,
    // it is possible certain optimizations will cause inlining. asm technique
    // is the gcc documented way to prevent such optimizations.
    //
    asm("");

    //
    // abort() sends a SIGABRT signal and it triggers termination and coredump.
    //
    abort();
}

void
CxPlatLogAssert(
    _In_z_ const char* File,
    _In_ int Line,
    _In_z_ const char* Expr
    )
{
    UNREFERENCED_PARAMETER(File);
    UNREFERENCED_PARAMETER(Line);
    UNREFERENCED_PARAMETER(Expr);

    CxPlatTraceEvent(
        "[ lib] ASSERT, %u:%s - %s.",
        (uint32_t)Line,
        File,
        Expr);
}

CXPLAT_STATUS
CxPlatInitialize(
    void
    )
{
#if DEBUG
    CxPlatform.AllocFailDenominator = 0;
    CxPlatform.AllocCounter = 0;
#endif

    RandomFd = open("/dev/urandom", O_RDONLY|O_CLOEXEC);
    if (RandomFd == -1) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            errno,
            "open(/dev/urandom, O_RDONLY|O_CLOEXEC) failed");
        return (CXPLAT_STATUS)errno;
    }

    CxPlatTraceLogInfo(
        "[ dso] Initialized");

    return CXPLAT_STATUS_SUCCESS;
}

void
CxPlatUninitialize(
    void
    )
{
    close(RandomFd);
    CxPlatTraceLogInfo(
        "[ dso] Uninitialized");
}

#if DEBUG
void
CxPlatSetAllocFailDenominator(
    _In_ int32_t Value
    )
{
    CxPlatform.AllocFailDenominator = Value;
    CxPlatform.AllocCounter = 0;
}

int32_t
CxPlatGetAllocFailDenominator(
    )
{
    return CxPlatform.AllocFailDenominator;
}
#endif

void*
CxPlatAlloc(
    _In_ size_t ByteCount,
    _In_ uint32_t Tag
    )
{
    UNREFERENCED_PARAMETER(Tag);
#if DEBUG
    CXPLAT_DBG_ASSERT(ByteCount != 0);
    uint32_t Rand;
    if ((CxPlatform.AllocFailDenominator > 0 && (CxPlatRandom(sizeof(Rand), &Rand), Rand % CxPlatform.AllocFailDenominator) == 1) ||
        (CxPlatform.AllocFailDenominator < 0 && InterlockedIncrement(&CxPlatform.AllocCounter) % CxPlatform.AllocFailDenominator == 0)) {
        return NULL;
    }
#endif
    return malloc(ByteCount);
}

void
CxPlatFree(
    __drv_freesMem(Mem) _Frees_ptr_ void* Mem,
    _In_ uint32_t Tag
    )
{
    UNREFERENCED_PARAMETER(Tag);
    free(Mem);
}

uint64_t
CxPlatTimespecToUs(
    _In_ const struct timespec *Time
    )
{
    return (Time->tv_sec * CXPLAT_MICROSEC_PER_SEC) + (Time->tv_nsec / CXPLAT_NANOSEC_PER_MICROSEC);
}

uint64_t
CxPlatGetTimerResolution(
    void
    )
{
    struct timespec Res = {0};
    int ErrorCode = clock_getres(CLOCK_MONOTONIC, &Res);
    CXPLAT_DBG_ASSERT(ErrorCode == 0);
    UNREFERENCED_PARAMETER(ErrorCode);
    return CxPlatTimespecToUs(&Res);
}

uint64_t
CxPlatTimeUs64(
    void
    )
{
    struct timespec CurrTime = {0};
    int ErrorCode = clock_gettime(CLOCK_MONOTONIC, &CurrTime);
    CXPLAT_DBG_ASSERT(ErrorCode == 0);
    UNREFERENCED_PARAMETER(ErrorCode);
    return CxPlatTimespecToUs(&CurrTime);
}

void
CxPlatGetAbsoluteTime(
    _In_ unsigned long DeltaMs,
    _Out_ struct timespec *Time
    )
{
    int ErrorCode = 0;

    CxPlatZeroMemory(Time, sizeof(struct timespec));

#if defined(CX_PLATFORM_LINUX)
    ErrorCode = clock_gettime(CLOCK_MONOTONIC, Time);
#elif defined(CX_PLATFORM_DARWIN)
    //
    // timespec_get is used on darwin, as CLOCK_MONOTONIC isn't actually
    // monotonic according to our tests.
    //
    timespec_get(Time, TIME_UTC);
#endif // CX_PLATFORM_DARWIN

    CXPLAT_DBG_ASSERT(ErrorCode == 0);
    UNREFERENCED_PARAMETER(ErrorCode);

    Time->tv_sec += (DeltaMs / CXPLAT_MS_PER_SECOND);
    Time->tv_nsec += ((DeltaMs % CXPLAT_MS_PER_SECOND) * CXPLAT_NANOSEC_PER_MS);

    if (Time->tv_nsec >= CXPLAT_NANOSEC_PER_SEC)
    {
        Time->tv_sec += 1;
        Time->tv_nsec -= CXPLAT_NANOSEC_PER_SEC;
    }

    CXPLAT_DBG_ASSERT(Time->tv_sec >= 0);
    CXPLAT_DBG_ASSERT(Time->tv_nsec >= 0);
    CXPLAT_DBG_ASSERT(Time->tv_nsec < CXPLAT_NANOSEC_PER_SEC);
}

void
CxPlatSleep(
    _In_ uint32_t DurationMs
    )
{
    int ErrorCode = 0;
    struct timespec TS = {
        .tv_sec = (DurationMs / CXPLAT_MS_PER_SECOND),
        .tv_nsec = (CXPLAT_NANOSEC_PER_MS * (DurationMs % CXPLAT_MS_PER_SECOND))
    };

    ErrorCode = nanosleep(&TS, &TS);
    CXPLAT_DBG_ASSERT(ErrorCode == 0);
    UNREFERENCED_PARAMETER(ErrorCode);
}

CXPLAT_STATUS
CxPlatRandom(
    _In_ uint32_t BufferLen,
    _Out_writes_bytes_(BufferLen) void* Buffer
    )
{
    if (read(RandomFd, Buffer, BufferLen) == -1) {
        return (CXPLAT_STATUS)errno;
    }
    return CXPLAT_STATUS_SUCCESS;
}
