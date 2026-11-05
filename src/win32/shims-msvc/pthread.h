/* MSVC-ABI pthread shim over the Win32 thread API (MinGW brings winpthread).
 *
 * Declarations only: pthread_msvc.c implements them, the only file that
 * includes <windows.h>, as the engine's own Win32 types collide with the SDK's.
 * A mutex is an SRWLOCK plus owner and depth (recursive, and valid when
 * zero-initialized, as PTHREAD_MUTEX_INITIALIZER needs); pthread_t is the
 * thread id, so pthread_equal compares threads. The surface is what the
 * engine, the bridge and libcod use.
 */
#ifndef COD2_MSVC_PTHREAD_H
#define COD2_MSVC_PTHREAD_H

#ifndef _WIN32
#error "shims-msvc/pthread.h is for the MSVC-ABI Windows build only"
#endif

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { void *lock; unsigned long owner; unsigned depth; } pthread_mutex_t;
typedef int pthread_mutexattr_t;
typedef unsigned long pthread_t;
typedef struct { int detached; size_t stacksize; } pthread_attr_t;
typedef void *pthread_once_t;
typedef unsigned long pthread_key_t;

#define PTHREAD_MUTEX_INITIALIZER { 0, 0, 0 }
#define PTHREAD_ONCE_INIT 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_CREATE_JOINABLE 0
#define PTHREAD_CREATE_DETACHED 1

int pthread_mutexattr_init(pthread_mutexattr_t *a);
int pthread_mutexattr_destroy(pthread_mutexattr_t *a);
int pthread_mutexattr_settype(pthread_mutexattr_t *a, int type);
int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a);
int pthread_mutex_destroy(pthread_mutex_t *m);
int pthread_mutex_lock(pthread_mutex_t *m);
int pthread_mutex_trylock(pthread_mutex_t *m);
int pthread_mutex_unlock(pthread_mutex_t *m);

int pthread_attr_init(pthread_attr_t *a);
int pthread_attr_destroy(pthread_attr_t *a);
int pthread_attr_setdetachstate(pthread_attr_t *a, int state);
int pthread_attr_setstacksize(pthread_attr_t *a, size_t size);
int pthread_create(pthread_t *t, const pthread_attr_t *a, void *(*fn)(void *), void *arg);
int pthread_join(pthread_t t, void **ret);
pthread_t pthread_self(void);
int pthread_equal(pthread_t a, pthread_t b);
void pthread_exit(void *ret);

int pthread_once(pthread_once_t *once, void (*fn)(void));
int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_setspecific(pthread_key_t key, const void *value);
void *pthread_getspecific(pthread_key_t key);

#ifdef __cplusplus
}
#endif

/* pthread_main_np is provided by headers/win32_compat.h. */

#endif /* COD2_MSVC_PTHREAD_H */
