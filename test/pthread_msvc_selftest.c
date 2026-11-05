/* src/win32/shims-msvc/pthread.h on the MSVC ABI: built and run by test_pthread_msvc.py. */
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static volatile int onceRuns, trylockResult = -1, detachedRan;
static pthread_t mainThread, otherThread;

static void once_fn(void) { onceRuns++; }

static void *other(void *arg)
{
    otherThread = pthread_self();
    trylockResult = pthread_mutex_trylock(&m);   /* the main thread holds it */
    pthread_once(&once, once_fn);
    return arg;
}

static void *detached(void *arg) { detachedRan = 1; return arg; }
static void *noop(void *arg) { return arg; }

int main(void)
{
    pthread_t t;
    pthread_attr_t attr;
    void *ret = 0;
    int i;

    mainThread = pthread_self();
    CHECK(pthread_equal(mainThread, pthread_self()));

    /* recursive on a statically initialized mutex */
    CHECK(pthread_mutex_lock(&m) == 0);
    CHECK(pthread_mutex_lock(&m) == 0);
    pthread_once(&once, once_fn);
    CHECK(pthread_create(&t, 0, other, &m) == 0);
    CHECK(pthread_join(t, &ret) == 0);
    CHECK(ret == &m);
    CHECK(trylockResult != 0);
    CHECK(!pthread_equal(mainThread, otherThread));
    CHECK(onceRuns == 1);
    CHECK(pthread_mutex_unlock(&m) == 0);
    CHECK(pthread_mutex_unlock(&m) == 0);
    CHECK(pthread_mutex_trylock(&m) == 0);
    CHECK(pthread_mutex_unlock(&m) == 0);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 256 * 1024);
    CHECK(pthread_create(&t, &attr, detached, 0) == 0);
    pthread_attr_destroy(&attr);
    for (i = 0; i < 1000 && !detachedRan; i++) {
        pthread_t w;
        pthread_create(&w, 0, noop, 0);   /* waits a little by joining another thread */
        pthread_join(w, 0);
    }
    CHECK(detachedRan);

    {
        struct timespec a, b;
        CHECK(clock_gettime(CLOCK_MONOTONIC, &a) == 0);
        CHECK(clock_gettime(CLOCK_MONOTONIC, &b) == 0);
        CHECK(b.tv_sec > a.tv_sec || (b.tv_sec == a.tv_sec && b.tv_nsec >= a.tv_nsec));
        CHECK(a.tv_nsec >= 0 && a.tv_nsec < 1000000000);
        CHECK(clock_gettime(CLOCK_REALTIME, &a) == 0);
        CHECK(a.tv_sec > 1577836800 && a.tv_sec < 4102444800LL);   /* between 2020 and 2100 */
        CHECK(clock_gettime(7, &a) != 0);
    }
    printf("ok\n");
    return 0;
}
