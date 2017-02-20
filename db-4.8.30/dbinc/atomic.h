/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_ATOMIC_H_
#define _DB_ATOMIC_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *  Atomic operation support for Oracle Berkeley DB
 *
 * HAVE_ATOMIC_SUPPORT configures whether to use the assembly language
 * or system calls to perform:
 *
 *   atomic_inc(env, valueptr)
 *      Adds 1 to the db_atomic_t value, returning the new value.
 *
 *   atomic_dec(env, valueptr)
 *      Subtracts 1 from the db_atomic_t value, returning the new value.
 *
 *   atomic_compare_exchange(env, valueptr, oldval, newval)
 *      If the db_atomic_t's value is still oldval, set it to newval.
 *      It returns 1 for success or 0 for failure.
 *
 * The ENV * paramter is used only when HAVE_ATOMIC_SUPPORT is undefined.
 *
 * If the platform does not natively support any one of these operations,
 * then atomic operations will be emulated with this sequence:
 *      MUTEX_LOCK()
 *      <op>
 *      MUTEX_UNLOCK();
 * Uses where mutexes are not available (e.g. the environment has not yet
 * attached to the mutex region) must be avoided.
 */
#if defined(DB_WIN32)
typedef DWORD   atomic_value_t;
#else
typedef int32_t  atomic_value_t;
#endif

/*
 * Windows CE has strange issues using the Interlocked APIs with variables
 * stored in shared memory. It seems like the page needs to have been written
 * prior to the API working as expected. Work around this by allocating an
 * additional 32-bit value that can be harmlessly written for each value
 * used in Interlocked instructions.
 */
#if defined(DB_WINCE)
typedef struct {
    volatile atomic_value_t value;
    volatile atomic_value_t dummy;
} db_atomic_t;
#else
typedef struct {
    volatile atomic_value_t value;
} db_atomic_t;
#endif

/*
 * These macro hide the db_atomic_t structure layout and help detect
 * non-atomic_t actual argument to the atomic_xxx() calls. DB requires
 * aligned 32-bit reads to be atomic even outside of explicit 'atomic' calls.
 * These have no memory barriers; the caller must include them when necessary.
 */
#define atomic_read(p)      ((p)->value)
#define atomic_init(p, val) ((p)->value = (val))

#ifdef HAVE_ATOMIC_SUPPORT

#if defined(DB_WIN32)
#if defined(DB_WINCE)
#define WINCE_ATOMIC_MAGIC(p)                       \
    /*                              \
     * Memory mapped regions on Windows CE cause problems with  \
     * InterlockedXXX calls. Each page in a mapped region needs to  \
     * have been written to prior to an InterlockedXXX call, or the \
     * InterlockedXXX call hangs. This does not seem to be      \
     * documented anywhere. For now, read/write a non-critical  \
     * piece of memory from the shared region prior to attempting   \
     * shared region prior to attempting an InterlockedExchange \
     * InterlockedXXX operation.                    \
     */                             \
    (p)->dummy = 0
#else
#define WINCE_ATOMIC_MAGIC(p) 0
#endif

#if defined(DB_WINCE) || (defined(_MSC_VER) && _MSC_VER < 1300)
/*
 * The Interlocked instructions on Windows CE have different parameter
 * definitions. The parameters lost their 'volatile' qualifier,
 * cast it away, to avoid compiler warnings.
 * These definitions should match those in dbinc/mutex_int.h for tsl_t, except
 * that the WINCE version drops the volatile qualifier.
 */
typedef PLONG interlocked_val;
#define atomic_inc(env, p)                      \
    (WINCE_ATOMIC_MAGIC(p),                     \
    InterlockedIncrement((interlocked_val)(&(p)->value)))

#else
typedef LONG volatile *interlocked_val;
#define atomic_inc(env, p)  \
    InterlockedIncrement((interlocked_val)(&(p)->value))
#endif

#define atomic_dec(env, p)                      \
    (WINCE_ATOMIC_MAGIC(p),                     \
    InterlockedDecrement((interlocked_val)(&(p)->value)))
#if defined(_MSC_VER) && _MSC_VER < 1300
#define atomic_compare_exchange(env, p, oldval, newval)         \
    (WINCE_ATOMIC_MAGIC(p),                     \
    (InterlockedCompareExchange((PVOID *)(&(p)->value),     \
    (PVOID)(newval), (PVOID)(oldval)) == (PVOID)(oldval)))
#else
#define atomic_compare_exchange(env, p, oldval, newval)         \
    (WINCE_ATOMIC_MAGIC(p),                     \
    (InterlockedCompareExchange((interlocked_val)(&(p)->value), \
    (newval), (oldval)) == (oldval)))
#endif
#endif

#if defined(HAVE_ATOMIC_SOLARIS)
/* Solaris sparc & x86/64 */
#include <atomic.h>
#define atomic_inc(env, p)  \
    atomic_inc_uint_nv((volatile unsigned int *) &(p)->value)
#define atomic_dec(env, p)  \
    atomic_dec_uint_nv((volatile unsigned int *) &(p)->value)
#define atomic_compare_exchange(env, p, oval, nval)     \
    (atomic_cas_32((volatile unsigned int *) &(p)->value,   \
        (oval), (nval)) == (oval))
#endif

#if defined(HAVE_ATOMIC_X86_GCC_ASSEMBLY)
/* x86/x86_64 gcc  */
#define atomic_inc(env, p)  __atomic_inc(p)
#define atomic_dec(env, p)  __atomic_dec(p)
#define atomic_compare_exchange(env, p, o, n)   \
    __atomic_compare_exchange((p), (o), (n))
static inline int __atomic_inc(db_atomic_t *p)
{
    int temp;

    temp = 1;
    __asm__ __volatile__("lock; xadd %0, (%1)"
        : "+r"(temp)
        : "r"(p));
    return (temp + 1);
}

static inline int __atomic_dec(db_atomic_t *p)
{
    int temp;

    temp = -1;
    __asm__ __volatile__("lock; xadd %0, (%1)"
        : "+r"(temp)
        : "r"(p));
    return (temp - 1);
}

/*
 * x86/gcc Compare exchange for shared latches. i486+
 *  Returns 1 for success, 0 for failure
 *
 * GCC 4.1+ has an equivalent  __sync_bool_compare_and_swap() as well as
 * __sync_val_compare_and_swap() which returns the value read from *dest
 * http://gcc.gnu.org/onlinedocs/gcc-4.1.0/gcc/Atomic-Builtins.html
 * which configure could be changed to use.
 */
static inline int __atomic_compare_exchange(
    db_atomic_t *p, atomic_value_t oldval, atomic_value_t newval)
{
    atomic_value_t was;

    if (p->value != oldval) /* check without expensive cache line locking */
        return 0;
    __asm__ __volatile__("lock; cmpxchgl %1, (%2);"
        :"=a"(was)
        :"r"(newval), "r"(p), "a"(oldval)
        :"memory", "cc");
    return (was == oldval);
}
#endif

#else
/*
 * No native hardware support for atomic increment, decrement, and
 * compare-exchange. Emulate them when mutexes are supported;
 * do them without concern for atomicity when no mutexes.
 */
#ifndef HAVE_MUTEX_SUPPORT
/*
 * These minimal versions are correct to use only for single-threaded,
 * single-process environments.
 */
#define atomic_inc(env, p)  (++(p)->value)
#define atomic_dec(env, p)  (--(p)->value)
#define atomic_compare_exchange(env, p, oldval, newval)     \
    (DB_ASSERT(env, atomic_read(p) == (oldval)),        \
    atomic_init(p, (newval)), 1)
#else
#define atomic_inc(env, p)  __atomic_inc(env, p)
#define atomic_dec(env, p)  __atomic_dec(env, p)
#endif
#endif

#if defined(__cplusplus)
}
#endif

#endif /* !_DB_ATOMIC_H_ */
