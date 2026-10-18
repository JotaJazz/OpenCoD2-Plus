#ifndef COD2_GCC40_COMPAT_H
#define COD2_GCC40_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

static __inline__ int cod2_sync_fetch_and_add_i32(volatile int *ptr, int val)
{
    return __sync_fetch_and_add(ptr, val);
}
static __inline__ int cod2_sync_add_and_fetch_i32(volatile int *ptr, int val)
{
    return __sync_add_and_fetch(ptr, val);
}
static __inline__ int cod2_sync_val_compare_and_swap_i32(volatile int *ptr, int oldval, int newval)
{
    return __sync_val_compare_and_swap(ptr, oldval, newval);
}
static __inline__ int cod2_sync_bool_compare_and_swap_i32(volatile int *ptr, int oldval, int newval)
{
    return __sync_bool_compare_and_swap(ptr, oldval, newval);
}
static __inline__ int cod2_sync_lock_test_and_set_i32(volatile int *ptr, int val)
{
    return __sync_lock_test_and_set(ptr, val);
}

static __inline__ unsigned int cod2_bswap32(unsigned int x)
{
    return ((x & 0x000000FFu) << 24) | ((x & 0x0000FF00u) << 8) | ((x & 0x00FF0000u) >> 8) | ((x & 0xFF000000u) >> 24);
}

#ifdef __cplusplus
}
#endif
#endif
