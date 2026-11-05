/* The MSVC-ABI pthread shim's implementation (declarations in pthread.h).
 * Isolated TU: <windows.h> but no engine header, as dirent_msvc.c. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include "pthread.h"
#include "time.h"

typedef char cod2_srwlock_fits[sizeof(SRWLOCK) == sizeof(void *) ? 1 : -1];
typedef char cod2_initonce_fits[sizeof(INIT_ONCE) == sizeof(pthread_once_t) ? 1 : -1];

int pthread_mutexattr_init(pthread_mutexattr_t *a) { if (a) *a = 0; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t *a) { (void)a; return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type) { if (a) *a = type; return 0; }

/* every mutex is recursive: the engine's critical sections need it, the others never relock */
int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a)
{
    (void)a;
    m->lock = 0;
    m->owner = 0;
    m->depth = 0;
    return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *m) { (void)m; return 0; }

int pthread_mutex_lock(pthread_mutex_t *m)
{
    DWORD self = GetCurrentThreadId();
    if (m->depth && m->owner == self) {
        m->depth++;
        return 0;
    }
    AcquireSRWLockExclusive((PSRWLOCK)&m->lock);
    m->owner = self;
    m->depth = 1;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *m)
{
    DWORD self = GetCurrentThreadId();
    if (m->depth && m->owner == self) {
        m->depth++;
        return 0;
    }
    if (!TryAcquireSRWLockExclusive((PSRWLOCK)&m->lock))
        return 16;   /* EBUSY */
    m->owner = self;
    m->depth = 1;
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *m)
{
    if (!m->depth || m->owner != GetCurrentThreadId())
        return 1;   /* EPERM */
    if (--m->depth)
        return 0;
    m->owner = 0;
    ReleaseSRWLockExclusive((PSRWLOCK)&m->lock);
    return 0;
}

int pthread_attr_init(pthread_attr_t *a) { a->detached = 0; a->stacksize = 0; return 0; }
int pthread_attr_destroy(pthread_attr_t *a) { (void)a; return 0; }
int pthread_attr_setdetachstate(pthread_attr_t *a, int state) { a->detached = state == PTHREAD_CREATE_DETACHED; return 0; }
int pthread_attr_setstacksize(pthread_attr_t *a, size_t size) { a->stacksize = size; return 0; }

typedef struct { void *(*fn)(void *); void *arg; } cod2_thread_start;

/* A joinable thread's handle, kept open until pthread_join so its id is not reused before. */
#define COD2_JOINABLE_MAX 64
static struct { DWORD id; HANDLE h; } cod2_joinable[COD2_JOINABLE_MAX];
static SRWLOCK cod2_joinable_lock = SRWLOCK_INIT;

static int cod2_joinable_put(DWORD id, HANDLE h)
{
    int i, put = 0;
    AcquireSRWLockExclusive(&cod2_joinable_lock);
    for (i = 0; i < COD2_JOINABLE_MAX && !put; i++)
        if (!cod2_joinable[i].h) {
            cod2_joinable[i].id = id;
            cod2_joinable[i].h = h;
            put = 1;
        }
    ReleaseSRWLockExclusive(&cod2_joinable_lock);
    return put;
}

static HANDLE cod2_joinable_take(DWORD id)
{
    int i;
    HANDLE h = 0;
    AcquireSRWLockExclusive(&cod2_joinable_lock);
    for (i = 0; i < COD2_JOINABLE_MAX && !h; i++)
        if (cod2_joinable[i].h && cod2_joinable[i].id == id) {
            h = cod2_joinable[i].h;
            cod2_joinable[i].h = 0;
        }
    ReleaseSRWLockExclusive(&cod2_joinable_lock);
    return h;
}

static unsigned __stdcall cod2_thread_main(void *p)
{
    cod2_thread_start start = *(cod2_thread_start *)p;
    free(p);
    /* the return value travels through the exit code: pointers are 32 bits here */
    return (unsigned)(uintptr_t)start.fn(start.arg);
}

int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *arg)
{
    cod2_thread_start *start = (cod2_thread_start *)malloc(sizeof(*start));
    unsigned id;
    uintptr_t h;
    if (!start)
        return 12;   /* ENOMEM */
    start->fn = fn;
    start->arg = arg;
    h = _beginthreadex(0, a ? (unsigned)a->stacksize : 0, cod2_thread_main, start, 0, &id);
    if (!h) {
        free(start);
        return 11;   /* EAGAIN */
    }
    if (a && a->detached)
        CloseHandle((HANDLE)h);
    else if (!cod2_joinable_put(id, (HANDLE)h))
        CloseHandle((HANDLE)h);   /* table full: pthread_join reports ESRCH */
    if (t)
        *t = id;
    return 0;
}

int pthread_join(pthread_t t, void **ret)
{
    DWORD code = 0;
    HANDLE h = cod2_joinable_take(t);
    if (!h)
        return 3;   /* ESRCH */
    WaitForSingleObject(h, INFINITE);
    GetExitCodeThread(h, &code);
    CloseHandle(h);
    if (ret)
        *ret = (void *)(uintptr_t)code;
    return 0;
}

pthread_t pthread_self(void) { return GetCurrentThreadId(); }
int pthread_equal(pthread_t a, pthread_t b) { return a == b; }
void pthread_exit(void *ret) { _endthreadex((unsigned)(uintptr_t)ret); }

static BOOL CALLBACK cod2_once_main(PINIT_ONCE once, PVOID fn, PVOID *ctx)
{
    (void)once;
    (void)ctx;
    ((void (*)(void))fn)();
    return TRUE;
}

int pthread_once(pthread_once_t *once, void (*fn)(void))
{
    return InitOnceExecuteOnce((PINIT_ONCE)once, cod2_once_main, (PVOID)fn, 0) ? 0 : 1;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    DWORD k = FlsAlloc((PFLS_CALLBACK_FUNCTION)destructor);   /* FLS runs the destructor at thread exit */
    if (k == FLS_OUT_OF_INDEXES)
        return 11;
    *key = k;
    return 0;
}

int pthread_setspecific(pthread_key_t key, const void *value) { return FlsSetValue(key, (PVOID)value) ? 0 : 22; }
void *pthread_getspecific(pthread_key_t key) { return FlsGetValue(key); }

int clock_gettime(clockid_t clock, struct timespec *ts)
{
    if (clock == CLOCK_MONOTONIC) {
        LARGE_INTEGER count, freq;
        QueryPerformanceCounter(&count);
        QueryPerformanceFrequency(&freq);
        ts->tv_sec = (time_t)(count.QuadPart / freq.QuadPart);
        ts->tv_nsec = (long)(count.QuadPart % freq.QuadPart * 1000000000 / freq.QuadPart);
        return 0;
    }
    if (clock == CLOCK_REALTIME) {
        FILETIME ft;
        unsigned long long t;
        GetSystemTimePreciseAsFileTime(&ft);
        t = ((unsigned long long)ft.dwHighDateTime << 32 | ft.dwLowDateTime) - 116444736000000000ULL;   /* 1601 -> 1970 */
        ts->tv_sec = (time_t)(t / 10000000);
        ts->tv_nsec = (long)(t % 10000000 * 100);
        return 0;
    }
    return 22;   /* EINVAL */
}

#endif
