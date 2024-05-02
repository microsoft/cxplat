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

#ifdef CXPLAT_NUMA_AWARE
#include <numa.h>               // If missing: `apt-get install -y libnuma-dev`
uint32_t CxPlatNumaNodeCount;
cpu_set_t* CxPlatNumaNodeMasks;
#endif // CXPLAT_NUMA_AWARE

CX_PLATFORM CxPlatform = { NULL };

//
// Used for reading random numbers.
//
int RandomFd = -1;

uint32_t CxPlatProcessorCount;

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

#if defined(CX_PLATFORM_DARWIN)
    //
    // arm64 macOS has no way to get the current proc, so treat as single core.
    // Intel macOS can return incorrect values for CPUID, so treat as single core.
    //
    CxPlatProcessorCount = 1;
#else
    CxPlatProcessorCount = (uint32_t)sysconf(_SC_NPROCESSORS_ONLN);
#endif

#ifdef CXPLAT_NUMA_AWARE
    if (numa_available() >= 0) {
        CxPlatNumaNodeCount = (uint32_t)numa_num_configured_nodes();
        CxPlatNumaNodeMasks =
            CXPLAT_ALLOC_NONPAGED(sizeof(cpu_set_t) * CxPlatNumaNodeCount, CXPLAT_POOL_PROC);
        CXPLAT_FRE_ASSERT(CxPlatNumaNodeMasks);
        for (uint32_t n = 0; n < CxPlatNumaNodeCount; ++n) {
            CPU_ZERO(&CxPlatNumaNodeMasks[n]);
            CXPLAT_FRE_ASSERT(numa_node_to_cpus_compat((int)n, CxPlatNumaNodeMasks[n].__bits, sizeof(cpu_set_t)) >= 0);
        }
    } else {
        CxPlatNumaNodeCount = 0;
    }
#endif // CXPLAT_NUMA_AWARE

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

#ifdef CXPLAT_NUMA_AWARE
    CXPLAT_FREE(CxPlatNumaNodeMasks, CXPLAT_POOL_PROC);
#endif

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

uint32_t
CxPlatProcCurrentNumber(
    void
    )
{
#if defined(CX_PLATFORM_LINUX)
    return (uint32_t)sched_getcpu() % CxPlatProcessorCount;
#elif defined(CX_PLATFORM_DARWIN)
    //
    // arm64 macOS has no way to get the current proc, so treat as single core.
    // Intel macOS can return incorrect values for CPUID, so treat as single core.
    //
    return 0;
#endif // CX_PLATFORM_DARWIN
}

#if defined(CX_PLATFORM_LINUX)

CXPLAT_STATUS
CxPlatThreadCreate(
    _In_ CXPLAT_THREAD_CONFIG* Config,
    _Out_ CXPLAT_THREAD* Thread
    )
{
    CXPLAT_STATUS Status = CXPLAT_STATUS_SUCCESS;

    pthread_attr_t Attr;
    if (pthread_attr_init(&Attr)) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            errno,
            "pthread_attr_init failed");
        return errno;
    }

#ifdef __GLIBC__
    if (Config->Flags & CXPLAT_THREAD_FLAG_SET_AFFINITIZE) {
        cpu_set_t CpuSet;
        CPU_ZERO(&CpuSet);
        CPU_SET(Config->IdealProcessor, &CpuSet);
        if (pthread_attr_setaffinity_np(&Attr, sizeof(CpuSet), &CpuSet)) {
            CxPlatTraceEvent(
                "[ lib] ERROR, %s.",
                "pthread_attr_setaffinity_np failed");
        }
    } else {
        // TODO - Set Linux equivalent of NUMA affinity.
    }
    // There is no way to set an ideal processor in Linux.
#endif

    if (Config->Flags & CXPLAT_THREAD_FLAG_HIGH_PRIORITY) {
        struct sched_param Params;
        Params.sched_priority = sched_get_priority_max(SCHED_FIFO);
        if (pthread_attr_setschedparam(&Attr, &Params)) {
            CxPlatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                errno,
                "pthread_attr_setschedparam failed");
        }
    }

#ifdef CXPLAT_USE_CUSTOM_THREAD_CONTEXT

    CXPLAT_THREAD_CUSTOM_CONTEXT* CustomContext =
        CXPLAT_ALLOC_NONPAGED(sizeof(CXPLAT_THREAD_CUSTOM_CONTEXT), CXPLAT_POOL_CUSTOM_THREAD);
    if (CustomContext == NULL) {
        Status = CXPLAT_STATUS_OUT_OF_MEMORY;
        CxPlatTraceEvent(
            "Allocation of '%s' failed. (%llu bytes)",
            "Custom thread context",
            sizeof(CXPLAT_THREAD_CUSTOM_CONTEXT));
    }
    CustomContext->Callback = Config->Callback;
    CustomContext->Context = Config->Context;

    if (pthread_create(Thread, &Attr, CxPlatThreadCustomStart, CustomContext)) {
        Status = errno;
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            Status,
            "pthread_create failed");
        CXPLAT_FREE(CustomContext, CXPLAT_POOL_CUSTOM_THREAD);
    }

#else // CXPLAT_USE_CUSTOM_THREAD_CONTEXT

    //
    // If pthread_create fails with an error code, then try again without the attribute
    // because the CPU might be offline.
    //
    if (pthread_create(Thread, &Attr, Config->Callback, Config->Context)) {
        CxPlatTraceLogWarning(
            "[ lib] pthread_create failed, retrying without affinitization");
        if (pthread_create(Thread, NULL, Config->Callback, Config->Context)) {
            Status = errno;
            CxPlatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                Status,
                "pthread_create failed");
        }
    }

#endif // !CXPLAT_USE_CUSTOM_THREAD_CONTEXT

#if !defined(__ANDROID__)
    if (Status == CXPLAT_STATUS_SUCCESS) {
        if (Config->Flags & CXPLAT_THREAD_FLAG_SET_AFFINITIZE) {
            cpu_set_t CpuSet;
            CPU_ZERO(&CpuSet);
            CPU_SET(Config->IdealProcessor, &CpuSet);
            if (pthread_setaffinity_np(*Thread, sizeof(CpuSet), &CpuSet)) {
                CxPlatTraceEvent(
                    "[ lib] ERROR, %s.",
                    "pthread_setaffinity_np failed");
            }
#ifdef CXPLAT_NUMA_AWARE
        } else if (CxPlatNumaNodeCount != 0) {
            int IdealNumaNode = numa_node_of_cpu((int)Config->IdealProcessor);
            if (pthread_setaffinity_np(*Thread, sizeof(cpu_set_t), &CxPlatNumaNodeMasks[IdealNumaNode])) {
                CxPlatTraceEvent(
                    "[ lib] ERROR, %s.",
                    "pthread_setaffinity_np failed");
            }
#endif
        }
    }
#endif

    pthread_attr_destroy(&Attr);

    return Status;
}

#elif defined(CX_PLATFORM_DARWIN)

CXPLAT_STATUS
CxPlatThreadCreate(
    _In_ CXPLAT_THREAD_CONFIG* Config,
    _Out_ CXPLAT_THREAD* Thread
    )
{
    CXPLAT_STATUS Status = CXPLAT_STATUS_SUCCESS;
    pthread_attr_t Attr;
    if (pthread_attr_init(&Attr)) {
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            errno,
            "pthread_attr_init failed");
        return errno;
    }

    // XXX: Set processor affinity

    if (Config->Flags & CXPLAT_THREAD_FLAG_HIGH_PRIORITY) {
        struct sched_param Params;
        Params.sched_priority = sched_get_priority_max(SCHED_FIFO);
        if (!pthread_attr_setschedparam(&Attr, &Params)) {
            CxPlatTraceEvent(
                "[ lib] ERROR, %u, %s.",
                errno,
                "pthread_attr_setschedparam failed");
        }
    }

    if (pthread_create(Thread, &Attr, Config->Callback, Config->Context)) {
        Status = errno;
        CxPlatTraceEvent(
            "[ lib] ERROR, %u, %s.",
            Status,
            "pthread_create failed");
    }

    pthread_attr_destroy(&Attr);

    return Status;
}

#endif // CX_PLATFORM

void
CxPlatThreadDelete(
    _Inout_ CXPLAT_THREAD* Thread
    )
{
    UNREFERENCED_PARAMETER(Thread);
}

void
CxPlatThreadWait(
    _Inout_ CXPLAT_THREAD* Thread
    )
{
    CXPLAT_DBG_ASSERT(pthread_equal(*Thread, pthread_self()) == 0);
    CXPLAT_FRE_ASSERT(pthread_join(*Thread, NULL) == 0);
}

CXPLAT_THREAD_ID
CxPlatCurThreadID(
    void
    )
{

// For FreeBSD
#if defined(__FreeBSD__)
    return pthread_getthreadid_np();

#elif defined(CX_PLATFORM_LINUX)

    CXPLAT_STATIC_ASSERT(sizeof(pid_t) <= sizeof(CXPLAT_THREAD_ID), "PID size exceeds the expected size");
    return syscall(SYS_gettid);

#elif defined(CX_PLATFORM_DARWIN)
    // cppcheck-suppress duplicateExpression
    CXPLAT_STATIC_ASSERT(sizeof(uint32_t) == sizeof(CXPLAT_THREAD_ID), "The cast depends on thread id being 32 bits");
    uint64_t Tid;
    int Res = pthread_threadid_np(NULL, &Tid);
    UNREFERENCED_PARAMETER(Res);
    CXPLAT_DBG_ASSERT(Res == 0);
    CXPLAT_DBG_ASSERT(Tid <= UINT32_MAX);
    return (CXPLAT_THREAD_ID)Tid;

#endif // CX_PLATFORM_DARWIN
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
