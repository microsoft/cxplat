#include "cxplat_posix.h"
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
