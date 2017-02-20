/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_MUTEX_INT_H_
#define _DB_MUTEX_INT_H_

#include "dbinc/atomic.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Mutexes and Shared Latches
 *
 * Mutexes may be test-and-set (spinning & yielding when busy),
 * native versions (pthreads, WaitForSingleObject)
 * or a hybrid which has the lower no-contention overhead of test-and-set
 * mutexes, using operating system calls only to block and wakeup.
 *
 * Hybrid exclusive-only mutexes include a 'tas' field.
 * Hybrid DB_MUTEX_SHARED latches also include a 'shared' field.
 */

/*********************************************************************
 * POSIX.1 pthreads interface.
 *********************************************************************/
#if defined(HAVE_MUTEX_PTHREADS)
/*
 * Pthreads-based mutexes (exclusive-only) and latches (possibly shared)
 * have the same MUTEX_FIELDS union. Different parts of the union are used
 * depending on:
 *    - whether HAVE_SHARED_LATCHES is defined, and
 *    - if HAVE_SHARED_LATCHES, whether this particular instance of a mutex
 *  is a shared mutexDB_MUTEX_SHARED.
 *
 * The rwlock part of the union is used *only* for non-hybrid shared latches;
 * in all other cases the mutex and cond fields are the only ones used.
 *
 *  configuration & Who uses the field
 *  mutex
 *          mutex   cond    rwlock  tas
 * Native mutex     y   y
 * Hybrid mutexes   y   y       y
 * Native sharedlatches         y
 * Hybrid sharedlatches y   y       y
 *
 * They all have a condition variable which is used only for
 * DB_MUTEX_SELF_BLOCK waits.
 *
 * There can be no self-blocking shared latches: the pthread_cond_wait() would
 * require getting a pthread_mutex_t, also it would not make sense.
 */
#define MUTEX_FIELDS                            \
    union {                             \
        struct {                        \
            pthread_mutex_t mutex;  /* Mutex */     \
            pthread_cond_t  cond;   /* Condition variable */ \
        } m;                            \
        pthread_rwlock_t rwlock;    /* Read/write lock */   \
    } u;

#if defined(HAVE_SHARED_LATCHES) && !defined(HAVE_MUTEX_HYBRID)
#define RET_SET_PTHREAD_LOCK(mutexp, ret) do {              \
    if (F_ISSET(mutexp, DB_MUTEX_SHARED))               \
        RET_SET((pthread_rwlock_wrlock(&(mutexp)->u.rwlock)),   \
            ret);                       \
    else                                \
        RET_SET((pthread_mutex_lock(&(mutexp)->u.m.mutex)), ret); \
} while (0)
#define RET_SET_PTHREAD_TRYLOCK(mutexp, ret) do {           \
    if (F_ISSET(mutexp, DB_MUTEX_SHARED))               \
        RET_SET((pthread_rwlock_trywrlock(&(mutexp)->u.rwlock)), \
            ret);                       \
    else                                \
        RET_SET((pthread_mutex_trylock(&(mutexp)->u.m.mutex)),  \
            ret);                       \
} while (0)
#else
#define RET_SET_PTHREAD_LOCK(mutexp, ret)               \
        RET_SET(pthread_mutex_lock(&(mutexp)->u.m.mutex), ret);
#define RET_SET_PTHREAD_TRYLOCK(mutexp, ret)                \
        RET_SET(pthread_mutex_trylock(&(mutexp)->u.m.mutex), ret);
#endif
#endif

#ifdef HAVE_MUTEX_UI_THREADS
#include <thread.h>
#endif

/*********************************************************************
 * Solaris lwp threads interface.
 *
 * !!!
 * We use LWP mutexes on Solaris instead of UI or POSIX mutexes (both of
 * which are available), for two reasons.  First, the Solaris C library
 * includes versions of the both UI and POSIX thread mutex interfaces, but
 * they are broken in that they don't support inter-process locking, and
 * there's no way to detect it, e.g., calls to configure the mutexes for
 * inter-process locking succeed without error.  So, we use LWP mutexes so
 * that we don't fail in fairly undetectable ways because the application
 * wasn't linked with the appropriate threads library.  Second, there were
 * bugs in SunOS 5.7 (Solaris 7) where if an application loaded the C library
 * before loading the libthread/libpthread threads libraries (e.g., by using
 * dlopen to load the DB library), the pwrite64 interface would be translated
 * into a call to pwrite and DB would drop core.
 *********************************************************************/
#ifdef HAVE_MUTEX_SOLARIS_LWP
/*
 * XXX
 * Don't change <synch.h> to <sys/lwp.h> -- although lwp.h is listed in the
 * Solaris manual page as the correct include to use, it causes the Solaris
 * compiler on SunOS 2.6 to fail.
 */
#include <synch.h>

#define MUTEX_FIELDS                            \
    lwp_mutex_t mutex;      /* Mutex. */            \
    lwp_cond_t cond;        /* Condition variable. */
#endif

/*********************************************************************
 * Solaris/Unixware threads interface.
 *********************************************************************/
#ifdef HAVE_MUTEX_UI_THREADS
#include <thread.h>
#include <synch.h>

#define MUTEX_FIELDS                            \
    mutex_t mutex;          /* Mutex. */            \
    cond_t  cond;           /* Condition variable. */
#endif

/*********************************************************************
 * AIX C library functions.
 *********************************************************************/
#ifdef HAVE_MUTEX_AIX_CHECK_LOCK
#include <sys/atomic_op.h>
typedef int tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)   0
#define MUTEX_SET(x)    (!_check_lock(x, 0, 1))
#define MUTEX_UNSET(x)  _clear_lock(x, 0)
#endif
#endif

/*********************************************************************
 * Apple/Darwin library functions.
 *********************************************************************/
#ifdef HAVE_MUTEX_DARWIN_SPIN_LOCK_TRY
typedef u_int32_t tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
extern int _spin_lock_try(tsl_t *);
extern void _spin_unlock(tsl_t *);
#define MUTEX_SET(tsl)          _spin_lock_try(tsl)
#define MUTEX_UNSET(tsl)        _spin_unlock(tsl)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * General C library functions (msemaphore).
 *
 * !!!
 * Check for HPPA as a special case, because it requires unusual alignment,
 * and doesn't support semaphores in malloc(3) or shmget(2) memory.
 *
 * !!!
 * Do not remove the MSEM_IF_NOWAIT flag.  The problem is that if a single
 * process makes two msem_lock() calls in a row, the second one returns an
 * error.  We depend on the fact that we can lock against ourselves in the
 * locking subsystem, where we set up a mutex so that we can block ourselves.
 * Tested on OSF1 v4.0.
 *********************************************************************/
#ifdef HAVE_MUTEX_HPPA_MSEM_INIT
#define MUTEX_ALIGN 16
#endif

#if defined(HAVE_MUTEX_MSEM_INIT) || defined(HAVE_MUTEX_HPPA_MSEM_INIT)
#include <sys/mman.h>
typedef msemaphore tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)   (msem_init(x, MSEM_UNLOCKED) <= (msemaphore *)0)
#define MUTEX_SET(x)    (!msem_lock(x, MSEM_IF_NOWAIT))
#define MUTEX_UNSET(x)  msem_unlock(x, 0)
#endif
#endif

/*********************************************************************
 * Plan 9 library functions.
 *********************************************************************/
#ifdef HAVE_MUTEX_PLAN9
typedef Lock tsl_t;

#define MUTEX_INIT(x)   (memset(x, 0, sizeof(Lock)), 0)
#define MUTEX_SET(x)    canlock(x)
#define MUTEX_UNSET(x)  unlock(x)
#endif

/*********************************************************************
 * Reliant UNIX C library functions.
 *********************************************************************/
#ifdef HAVE_MUTEX_RELIANTUNIX_INITSPIN
#include <ulocks.h>
typedef spinlock_t tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)   (initspin(x, 1), 0)
#define MUTEX_SET(x)    (cspinlock(x) == 0)
#define MUTEX_UNSET(x)  spinunlock(x)
#endif
#endif

/*********************************************************************
 * General C library functions (POSIX 1003.1 sema_XXX).
 *
 * !!!
 * Never selected by autoconfig in this release (semaphore calls are known
 * to not work in Solaris 5.5).
 *********************************************************************/
#ifdef HAVE_MUTEX_SEMA_INIT
#include <synch.h>
typedef sema_t tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_DESTROY(x) sema_destroy(x)
#define MUTEX_INIT(x)    (sema_init(x, 1, USYNC_PROCESS, NULL) != 0)
#define MUTEX_SET(x)     (sema_wait(x) == 0)
#define MUTEX_UNSET(x)   sema_post(x)
#endif
#endif

/*********************************************************************
 * SGI C library functions.
 *********************************************************************/
#ifdef HAVE_MUTEX_SGI_INIT_LOCK
#include <abi_mutex.h>
typedef abilock_t tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)   (init_lock(x) != 0)
#define MUTEX_SET(x)    (!acquire_lock(x))
#define MUTEX_UNSET(x)  release_lock(x)
#endif
#endif

/*********************************************************************
 * Solaris C library functions.
 *
 * !!!
 * These are undocumented functions, but they're the only ones that work
 * correctly as far as we know.
 *********************************************************************/
#ifdef HAVE_MUTEX_SOLARIS_LOCK_TRY
#include <sys/atomic.h>
#define MUTEX_MEMBAR(x) membar_enter()
#define MEMBAR_ENTER()  membar_enter()
#define MEMBAR_EXIT()   membar_exit()
#include <sys/machlock.h>
typedef lock_t tsl_t;

/*
 * The functions are declared in <sys/machlock.h>, but under #ifdef KERNEL.
 * Re-declare them here to avoid warnings.
 */
extern  int _lock_try(lock_t *);
extern void _lock_clear(lock_t *);

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)   0
#define MUTEX_SET(x)    _lock_try(x)
#define MUTEX_UNSET(x)  _lock_clear(x)
#endif
#endif

/*********************************************************************
 * VMS.
 *********************************************************************/
#ifdef HAVE_MUTEX_VMS
#include <sys/mman.h>
#include <builtins.h>
typedef volatile unsigned char tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#ifdef __ALPHA
#define MUTEX_SET(tsl)      (!__TESTBITSSI(tsl, 0))
#else /* __VAX */
#define MUTEX_SET(tsl)      (!(int)_BBSSI(0, tsl))
#endif
#define MUTEX_UNSET(tsl)    (*(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * VxWorks
 * Use basic binary semaphores in VxWorks, as we currently do not need
 * any special features.  We do need the ability to single-thread the
 * entire system, however, because VxWorks doesn't support the open(2)
 * flag O_EXCL, the mechanism we normally use to single thread access
 * when we're first looking for a DB environment.
 *********************************************************************/
#ifdef HAVE_MUTEX_VXWORKS
#include "taskLib.h"
typedef SEM_ID tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * Uses of this MUTEX_SET() need to have a local 'nowait' variable,
 * which determines whether to return right away when the semaphore
 * is busy or to wait until it is available.
 */
#define MUTEX_SET(tsl)                          \
    (semTake((*(tsl)), nowait ? NO_WAIT : WAIT_FOREVER) == OK)
#define MUTEX_UNSET(tsl)    (semGive((*tsl)))
#define MUTEX_INIT(tsl)                         \
    ((*(tsl) = semBCreate(SEM_Q_FIFO, SEM_FULL)) == NULL)
#define MUTEX_DESTROY(tsl)  semDelete(*tsl)
#endif
#endif

/*********************************************************************
 * Win16
 *
 * Win16 spinlocks are simple because we cannot possibly be preempted.
 *
 * !!!
 * We should simplify this by always returning a no-need-to-lock lock
 * when we initialize the mutex.
 *********************************************************************/
#ifdef HAVE_MUTEX_WIN16
typedef unsigned int tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)       0
#define MUTEX_SET(tsl)      (*(tsl) = 1)
#define MUTEX_UNSET(tsl)    (*(tsl) = 0)
#endif
#endif

/*********************************************************************
 * Win32 - always a hybrid mutex
 *********************************************************************/
#if defined(HAVE_MUTEX_WIN32) || defined(HAVE_MUTEX_WIN32_GCC)
typedef LONG volatile tsl_t;
#define MUTEX_FIELDS                            \
    LONG nwaiters;                          \
    u_int32_t id;   /* ID used for creating events */       \

#if defined(LOAD_ACTUAL_MUTEX_CODE)
#define MUTEX_SET(tsl)      (!InterlockedExchange((PLONG)tsl, 1))
#define MUTEX_UNSET(tsl)    InterlockedExchange((PLONG)tsl, 0)
#define MUTEX_INIT(tsl)     MUTEX_UNSET(tsl)

/*
 * From Intel's performance tuning documentation (and see SR #6975):
 * ftp://download.intel.com/design/perftool/cbts/appnotes/sse2/w_spinlock.pdf
 *
 * "For this reason, it is highly recommended that you insert the PAUSE
 * instruction into all spin-wait code immediately. Using the PAUSE
 * instruction does not affect the correctness of programs on existing
 * platforms, and it improves performance on Pentium 4 processor platforms."
 */
#ifdef HAVE_MUTEX_WIN32
#if !defined(_WIN64) && !defined(DB_WINCE)
#define MUTEX_PAUSE     {__asm{_emit 0xf3}; __asm{_emit 0x90}}
#endif
#endif
#ifdef HAVE_MUTEX_WIN32_GCC
#define MUTEX_PAUSE     __asm__ volatile ("rep; nop" : : );
#endif
#endif
#endif

/*********************************************************************
 * 68K/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_68K_GCC_ASSEMBLY
typedef unsigned char tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/* gcc/68K: 0 is clear, 1 is set. */
#define MUTEX_SET(tsl) ({                       \
    register tsl_t *__l = (tsl);                    \
    int __r;                            \
        __asm__ volatile("tas  %1; \n               \
              seq  %0"                  \
        : "=dm" (__r), "=m" (*__l)              \
        : "1" (*__l)                        \
        );                          \
    __r & 1;                            \
})

#define MUTEX_UNSET(tsl)    (*(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * ALPHA/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_ALPHA_GCC_ASSEMBLY
typedef u_int32_t tsl_t;

#define MUTEX_ALIGN 4

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * For gcc/alpha.  Should return 0 if could not acquire the lock, 1 if
 * lock was acquired properly.
 */
static inline int
MUTEX_SET(tsl_t *tsl) {
    register tsl_t *__l = tsl;
    register tsl_t __r;
    __asm__ volatile(
        "1: ldl_l   %0,%2\n"
        "   blbs    %0,2f\n"
        "   or  $31,1,%0\n"
        "   stl_c   %0,%1\n"
        "   beq %0,3f\n"
        "   mb\n"
        "   br  3f\n"
        "2: xor %0,%0\n"
        "3:"
        : "=&r"(__r), "=m"(*__l) : "1"(*__l) : "memory");
    return __r;
}

/*
 * Unset mutex. Judging by Alpha Architecture Handbook, the mb instruction
 * might be necessary before unlocking
 */
static inline int
MUTEX_UNSET(tsl_t *tsl) {
    __asm__ volatile("  mb\n");
    return *tsl = 0;
}

#define MUTEX_INIT(tsl)     MUTEX_UNSET(tsl)
#endif
#endif

/*********************************************************************
 * Tru64/cc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_TRU64_CC_ASSEMBLY
typedef volatile u_int32_t tsl_t;

#define MUTEX_ALIGN 4

#ifdef LOAD_ACTUAL_MUTEX_CODE
#include <alpha/builtins.h>
#define MUTEX_SET(tsl)      (__LOCK_LONG_RETRY((tsl), 1) != 0)
#define MUTEX_UNSET(tsl)    (__UNLOCK_LONG(tsl))

#define MUTEX_INIT(tsl)     (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * ARM/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_ARM_GCC_ASSEMBLY
typedef unsigned char tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/* gcc/arm: 0 is clear, 1 is set. */
#define MUTEX_SET(tsl) ({                       \
    int __r;                            \
    __asm__ volatile(                       \
        "swpb   %0, %1, [%2]\n\t"               \
        "eor    %0, %0, #1\n\t"                 \
        : "=&r" (__r)                       \
        : "r" (1), "r" (tsl)                    \
        );                              \
    __r & 1;                            \
})

#define MUTEX_UNSET(tsl)    (*(volatile tsl_t *)(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * HPPA/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_HPPA_GCC_ASSEMBLY
typedef u_int32_t tsl_t;

#define MUTEX_ALIGN 16

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * The PA-RISC has a "load and clear" instead of a "test and set" instruction.
 * The 32-bit word used by that instruction must be 16-byte aligned.  We could
 * use the "aligned" attribute in GCC but that doesn't work for stack variables.
 */
#define MUTEX_SET(tsl) ({                       \
    register tsl_t *__l = (tsl);                    \
    int __r;                            \
    __asm__ volatile("ldcws 0(%1),%0" : "=r" (__r) : "r" (__l));    \
    __r & 1;                            \
})

#define MUTEX_UNSET(tsl)        (*(volatile tsl_t *)(tsl) = -1)
#define MUTEX_INIT(tsl)     (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * IA64/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_IA64_GCC_ASSEMBLY
typedef volatile unsigned char tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/* gcc/ia64: 0 is clear, 1 is set. */
#define MUTEX_SET(tsl) ({                       \
    register tsl_t *__l = (tsl);                    \
    long __r;                           \
    __asm__ volatile("xchg1 %0=%1,%2" :             \
             "=r"(__r), "+m"(*__l) : "r"(1));           \
    __r ^ 1;                            \
})

/*
 * Store through a "volatile" pointer so we get a store with "release"
 * semantics.
 */
#define MUTEX_UNSET(tsl)    (*(tsl_t *)(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * PowerPC/gcc assembly.
 *********************************************************************/
#if defined(HAVE_MUTEX_PPC_GCC_ASSEMBLY)
typedef u_int32_t tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * The PowerPC does a sort of pseudo-atomic locking.  You set up a
 * 'reservation' on a chunk of memory containing a mutex by loading the
 * mutex value with LWARX.  If the mutex has an 'unlocked' (arbitrary)
 * value, you then try storing into it with STWCX.  If no other process or
 * thread broke your 'reservation' by modifying the memory containing the
 * mutex, then the STCWX succeeds; otherwise it fails and you try to get
 * a reservation again.
 *
 * While mutexes are explicitly 4 bytes, a 'reservation' applies to an
 * entire cache line, normally 32 bytes, aligned naturally.  If the mutex
 * lives near data that gets changed a lot, there's a chance that you'll
 * see more broken reservations than you might otherwise.  The only
 * situation in which this might be a problem is if one processor is
 * beating on a variable in the same cache block as the mutex while another
 * processor tries to acquire the mutex.  That's bad news regardless
 * because of the way it bashes caches, but if you can't guarantee that a
 * mutex will reside in a relatively quiescent cache line, you might
 * consider padding the mutex to force it to live in a cache line by
 * itself.  No, you aren't guaranteed that cache lines are 32 bytes.  Some
 * embedded processors use 16-byte cache lines, while some 64-bit
 * processors use 128-bit cache lines.  But assuming a 32-byte cache line
 * won't get you into trouble for now.
 *
 * If mutex locking is a bottleneck, then you can speed it up by adding a
 * regular LWZ load before the LWARX load, so that you can test for the
 * common case of a locked mutex without wasting cycles making a reservation.
 *
 * gcc/ppc: 0 is clear, 1 is set.
 */
static inline int
MUTEX_SET(int *tsl)  {
    int __r;
    __asm__ volatile (
"0:                             \n\t"
"       lwarx   %0,0,%1         \n\t"
"       cmpwi   %0,0            \n\t"
"       bne-    1f              \n\t"
"       stwcx.  %1,0,%1         \n\t"
"       isync                   \n\t"
"       beq+    2f              \n\t"
"       b       0b              \n\t"
"1:                             \n\t"
"       li      %1,0            \n\t"
"2:                             \n\t"
     : "=&r" (__r), "+r" (tsl)
     :
     : "cr0", "memory");
     return (int)tsl;
}

static inline int
MUTEX_UNSET(tsl_t *tsl) {
     __asm__ volatile("sync" : : : "memory");
     return *tsl = 0;
}
#define MUTEX_INIT(tsl)     MUTEX_UNSET(tsl)
#endif
#endif

/*********************************************************************
 * OS/390 C.
 *********************************************************************/
#ifdef HAVE_MUTEX_S390_CC_ASSEMBLY
typedef int tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * cs() is declared in <stdlib.h> but is built in to the compiler.
 * Must use LANGLVL(EXTENDED) to get its declaration.
 */
#define MUTEX_SET(tsl)      (!cs(&zero, (tsl), 1))
#define MUTEX_UNSET(tsl)    (*(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * S/390 32-bit assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_S390_GCC_ASSEMBLY
typedef int tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/* gcc/S390: 0 is clear, 1 is set. */
static inline int
MUTEX_SET(tsl_t *tsl) {                         \
    register tsl_t *__l = (tsl);                    \
    int __r;                            \
  __asm__ volatile(                         \
       "    la    1,%1\n"                       \
       "    lhi   0,1\n"                        \
       "    l     %0,%1\n"                      \
       "0:  cs    %0,0,0(1)\n"                      \
       "    jl    0b"                           \
       : "=&d" (__r), "+m" (*__l)                   \
       : : "0", "1", "cc");                     \
    return !__r;                            \
}

#define MUTEX_UNSET(tsl)    (*(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * SCO/cc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_SCO_X86_CC_ASSEMBLY
typedef unsigned char tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * UnixWare has threads in libthread, but OpenServer doesn't (yet).
 *
 * cc/x86: 0 is clear, 1 is set.
 */
#if defined(__USLC__)
asm int
_tsl_set(void *tsl)
{
%mem tsl
    movl    tsl, %ecx
    movl    $1, %eax
    lock
    xchgb   (%ecx),%al
    xorl    $1,%eax
}
#endif

#define MUTEX_SET(tsl)      _tsl_set(tsl)
#define MUTEX_UNSET(tsl)    (*(tsl) = 0)
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#endif
#endif

/*********************************************************************
 * Sparc/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_SPARC_GCC_ASSEMBLY
typedef unsigned char tsl_t;

#define MUTEX_ALIGN 8

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * The ldstub instruction takes the location specified by its first argument
 * (a register containing a memory address) and loads its contents into its
 * second argument (a register) and atomically sets the contents the location
 * specified by its first argument to a byte of 1s.  (The value in the second
 * argument is never read, but only overwritten.)
 *
 * Hybrid mutexes require membar #StoreLoad and #LoadStore ordering on multi-
 * processor v9 systems.
 *
 * gcc/sparc: 0 is clear, 1 is set.
 */
#define MUTEX_SET(tsl) ({                       \
    register tsl_t *__l = (tsl);                    \
    register tsl_t __r;                     \
    __asm__ volatile                        \
        ("ldstub [%1],%0; stbar"                    \
        : "=r"( __r) : "r" (__l));                  \
    !__r;                               \
})

#define MUTEX_UNSET(tsl)    (*(tsl) = 0, MUTEX_MEMBAR(tsl))
#define MUTEX_INIT(tsl)         (MUTEX_UNSET(tsl), 0)
#define MUTEX_MEMBAR(x) \
    ({ __asm__ volatile ("membar #StoreStore|#StoreLoad|#LoadStore"); })
#define MEMBAR_ENTER() \
    ({ __asm__ volatile ("membar #StoreStore|#StoreLoad"); })
#define MEMBAR_EXIT() \
    ({ __asm__ volatile ("membar #StoreStore|#LoadStore"); })
#endif
#endif

/*********************************************************************
 * UTS/cc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_UTS_CC_ASSEMBLY
typedef int tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
#define MUTEX_INIT(x)   0
#define MUTEX_SET(x)    (!uts_lock(x, 1))
#define MUTEX_UNSET(x)  (*(x) = 0)
#endif
#endif

/*********************************************************************
 * MIPS/gcc assembly.
 *********************************************************************/
#ifdef HAVE_MUTEX_MIPS_GCC_ASSEMBLY
typedef u_int32_t tsl_t;

#define MUTEX_ALIGN 4

#ifdef LOAD_ACTUAL_MUTEX_CODE
/*
 * For gcc/MIPS.  Should return 0 if could not acquire the lock, 1 if
 * lock was acquired properly.
 */
static inline int
MUTEX_SET(tsl_t *tsl) {
       register tsl_t *__l = tsl;
       register tsl_t __r, __t;
       __asm__ volatile(
           "       .set push           \n"
           "       .set mips2          \n"
           "       .set noreorder      \n"
           "       .set nomacro        \n"
           "1:     ll      %0, %3      \n"
           "       ori     %2, %0, 1   \n"
           "       sc      %2, %1      \n"
           "       beqzl   %2, 1b      \n"
           "       nop                 \n"
           "       andi    %2, %0, 1   \n"
           "       sync                \n"
           "       .set reorder        \n"
           "       .set pop            \n"
           : "=&r" (__t), "=m" (*tsl), "=&r" (__r)
           : "m" (*tsl)
           : "memory");
       return (!__r);
}

static inline void
MUTEX_UNSET(tsl_t *tsl) {
    __asm__ volatile(
           "       .set noreorder      \n"
           "       sync                \n"
           "       sw      $0, %0      \n"
           "       .set reorder        \n"
           : "=m" (*tsl)
           : "m" (*tsl)
           : "memory");
}

#define        MUTEX_INIT(tsl)         (*(tsl) = 0)
#endif
#endif

/*********************************************************************
 * x86/gcc (32- and 64-bit) assembly.
 *********************************************************************/
#if defined(HAVE_MUTEX_X86_GCC_ASSEMBLY) || \
    defined(HAVE_MUTEX_X86_64_GCC_ASSEMBLY)
typedef volatile unsigned char tsl_t;

#ifdef LOAD_ACTUAL_MUTEX_CODE
/* gcc/x86: 0 is clear, 1 is set. */
#define MUTEX_SET(tsl) ({                       \
    tsl_t __r;                          \
    __asm__ volatile("movb $1, %b0\n\t"             \
        "xchgb %b0,%1"                      \
        : "=&q" (__r)                       \
        : "m" (*(tsl_t *)(tsl))                 \
        : "memory", "cc");                      \
    !__r;   /* return 1 on success, 0 on failure */         \
})

#define MUTEX_UNSET(tsl)        (*(tsl_t *)(tsl) = 0)
#define MUTEX_INIT(tsl)     (MUTEX_UNSET(tsl), 0)
/*
 * We need to pass a valid address to generate the memory barrier
 * otherwise PURIFY will complain.  Use something referenced recently
 * and initialized.
 */
#if defined(HAVE_MUTEX_X86_GCC_ASSEMBLY)
#define MUTEX_MEMBAR(addr)                      \
    ({ __asm__ volatile ("lock; addl $0, %0" ::"m" (addr): "memory"); 1; })
#else
#define MUTEX_MEMBAR(addr)                      \
    ({ __asm__ volatile ("mfence" ::: "memory"); 1; })
#endif

/*
 * From Intel's performance tuning documentation (and see SR #6975):
 * ftp://download.intel.com/design/perftool/cbts/appnotes/sse2/w_spinlock.pdf
 *
 * "For this reason, it is highly recommended that you insert the PAUSE
 * instruction into all spin-wait code immediately. Using the PAUSE
 * instruction does not affect the correctness of programs on existing
 * platforms, and it improves performance on Pentium 4 processor platforms."
 */
#define MUTEX_PAUSE     __asm__ volatile ("rep; nop" : : );
#endif
#endif

/* End of operating system & hardware architecture-specific definitions */

/*
 * Mutex alignment defaults to sizeof(unsigned int).
 *
 * !!!
 * Various systems require different alignments for mutexes (the worst we've
 * seen so far is 16-bytes on some HP architectures).  Malloc(3) is assumed
 * to return reasonable alignment, all other mutex users must ensure proper
 * alignment locally.
 */
#ifndef MUTEX_ALIGN
#define MUTEX_ALIGN sizeof(unsigned int)
#endif

/*
 * Mutex destruction defaults to a no-op.
 */
#ifndef MUTEX_DESTROY
#define MUTEX_DESTROY(x)
#endif

/*
 * Mutex pause defaults to a no-op.
 */
#ifndef MUTEX_PAUSE
#define MUTEX_PAUSE
#endif

/*
 * If no native atomic support is available then use mutexes to
 * emulate atomic increment, decrement, and compare-and-exchange.
 * The address of the atomic value selects which of a small number
 * of mutexes to use to protect the updates.
 * The number of mutexes should be somewhat larger than the number of
 * processors in the system in order to minimize unnecessary contention.
 * It defaults to 8 to handle most small (1-4) cpu systems, if it hasn't
 * already been defined (e.g. in db_config.h)
 */
#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT) && \
    !defined(MAX_ATOMIC_MUTEXES)
#define MAX_ATOMIC_MUTEXES  1
#endif

/*
 * DB_MUTEXMGR --
 *  The mutex manager encapsulates the mutex system.
 */
struct __db_mutexmgr {
    /* These fields are never updated after creation, so not protected. */
    DB_ENV  *dbenv;         /* Environment */
    REGINFO  reginfo;       /* Region information */

    void    *mutex_array;       /* Base of the mutex array */
};

/* Macros to lock/unlock the mutex region as a whole. */
#define MUTEX_SYSTEM_LOCK(dbenv)                    \
    MUTEX_LOCK(dbenv, ((DB_MUTEXREGION *)               \
        (dbenv)->mutex_handle->reginfo.primary)->mtx_region)
#define MUTEX_SYSTEM_UNLOCK(dbenv)                  \
    MUTEX_UNLOCK(dbenv, ((DB_MUTEXREGION *)             \
        (dbenv)->mutex_handle->reginfo.primary)->mtx_region)

/*
 * DB_MUTEXREGION --
 *  The primary mutex data structure in the shared memory region.
 */
typedef struct __db_mutexregion {
    /* These fields are initialized at create time and never modified. */
    roff_t      mutex_off_alloc;/* Offset of mutex array */
    roff_t      mutex_off;  /* Adjusted offset of mutex array */
    size_t      mutex_size; /* Size of the aligned mutex */
    roff_t      thread_off; /* Offset of the thread area. */

    db_mutex_t  mtx_region; /* Region mutex. */

    /* Protected using the region mutex. */
    u_int32_t   mutex_next; /* Next free mutex */

#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
    /* Mutexes for emulating atomic operations. */
    db_mutex_t  mtx_atomic[MAX_ATOMIC_MUTEXES];
#endif

    DB_MUTEX_STAT   stat;       /* Mutex statistics */
} DB_MUTEXREGION;

#ifdef HAVE_MUTEX_SUPPORT
struct __db_mutex_t {           /* Mutex. */
#ifdef MUTEX_FIELDS
    MUTEX_FIELDS            /* Opaque thread mutex structures. */
#endif
#ifndef HAVE_MUTEX_FCNTL
#if defined(HAVE_MUTEX_HYBRID) || \
    (defined(HAVE_SHARED_LATCHES) && !defined(HAVE_MUTEX_PTHREADS))
    /*
     * For hybrid and test-and-set shared latches it is a counter:
     * 0 means it is free,
     * -1 is exclusively locked,
     * > 0 is the number of shared readers.
     * Pthreads shared latches use pthread_rwlock instead.
     */
    db_atomic_t sharecount;
    tsl_t       tas;
#elif !defined(MUTEX_FIELDS)
    /*
     * This is the Test and Set flag for exclusive latches (mutexes):
     * there is a free value (often 0, 1, or -1) and a set value.
     */
    tsl_t       tas;
#endif
#endif
#ifdef HAVE_MUTEX_HYBRID
    volatile u_int32_t wait;    /* Count of waiters. */
#endif
    pid_t       pid;        /* Process owning mutex */
    db_threadid_t   tid;        /* Thread owning mutex */

    db_mutex_t mutex_next_link; /* Linked list of free mutexes. */

#ifdef HAVE_STATISTICS
    int   alloc_id;     /* Allocation ID. */

    u_int32_t mutex_set_wait;   /* Granted after wait. */
    u_int32_t mutex_set_nowait; /* Granted without waiting. */
#ifdef HAVE_SHARED_LATCHES
    u_int32_t mutex_set_rd_wait;    /* Granted shared lock after wait. */
    u_int32_t mutex_set_rd_nowait;  /* Granted shared lock w/out waiting. */
#endif
#ifdef HAVE_MUTEX_HYBRID
    u_int32_t hybrid_wait;
    u_int32_t hybrid_wakeup;    /* for counting spurious wakeups */
#endif
#endif

    /*
     * A subset of the flag arguments for __mutex_alloc().
     *
     * Flags should be an unsigned integer even if it's not required by
     * the possible flags values, getting a single byte on some machines
     * is expensive, and the mutex structure is a MP hot spot.
     */
    volatile u_int32_t flags;       /* MUTEX_XXX */
};
#endif

/* Macro to get a reference to a specific mutex. */
#define MUTEXP_SET(mtxmgr, indx)                    \
    ((DB_MUTEX *)((u_int8_t *)mtxmgr->mutex_array +         \
        (indx) * ((DB_MUTEXREGION *)mtxmgr->reginfo.primary)->mutex_size))

/* Inverse of the above: get the mutex index from a mutex pointer */
#define MUTEXP_GET(mtxmgr, mutexp)                  \
    (((u_int8_t *) (mutexp) - (u_int8_t *)mtxmgr->mutex_array) /    \
     ((DB_MUTEXREGION *)mtxmgr->reginfo.primary)->mutex_size)

/*
 * Check that a particular mutex is exclusively held at least by someone, not
 * necessarily the current thread.
 */
#ifdef HAVE_MUTEX_SUPPORT
#define MUTEX_IS_OWNED(env, mutex)                  \
    (mutex == MUTEX_INVALID || !MUTEX_ON(env) ||            \
    F_ISSET(env->dbenv, DB_ENV_NOLOCKING) ||            \
    F_ISSET(MUTEXP_SET(env->mutex_handle, mutex), DB_MUTEX_LOCKED))
#else
#define MUTEX_IS_OWNED(env, mutex)  0
#endif

#if defined(HAVE_MUTEX_HYBRID) ||  defined(DB_WIN32) ||     \
    (defined(HAVE_SHARED_LATCHES) && !defined(HAVE_MUTEX_PTHREADS))
#define MUTEXP_IS_BUSY(mutexp)                  \
    (F_ISSET(mutexp, DB_MUTEX_SHARED) ?         \
    (atomic_read(&(mutexp)->sharecount) != 0) :     \
    F_ISSET(mutexp, DB_MUTEX_LOCKED))
#define MUTEXP_BUSY_FIELD(mutexp)       \
    (F_ISSET(mutexp, DB_MUTEX_SHARED) ? \
    (atomic_read(&(mutexp)->sharecount)) : (mutexp)->flags)
#else
/* Pthread_rwlocks don't have an low-cost 'is it being shared?' predicate. */
#define MUTEXP_IS_BUSY(mutexp)  (F_ISSET((mutexp), DB_MUTEX_LOCKED))
#define MUTEXP_BUSY_FIELD(mutexp)   ((mutexp)->flags)
#endif

#define MUTEX_IS_BUSY(env, mutex)                   \
    (mutex == MUTEX_INVALID || !MUTEX_ON(env) ||            \
    F_ISSET(env->dbenv, DB_ENV_NOLOCKING) ||            \
    MUTEXP_IS_BUSY(MUTEXP_SET(env->mutex_handle, mutex)))

#define MUTEX_REQUIRED(env, mutex)                  \
    DB_ASSERT(env, MUTEX_IS_OWNED(env, mutex))

#define MUTEX_REQUIRED_READ(env, mutex)                 \
    DB_ASSERT(env, MUTEX_IS_OWNED(env, mutex) || MUTEX_IS_BUSY(env, mutex))

/*
 * Test and set (and thus hybrid) shared latches use compare & exchange
 * to acquire; the others the mutex-setting primitive defined above.
 */
#ifdef LOAD_ACTUAL_MUTEX_CODE

#if defined(HAVE_SHARED_LATCHES)
/* This is the value of the 'sharecount' of an exclusively held tas latch.
 * The particular value is not special; it is just unlikely to be caused
 * by releasing or acquiring a shared latch too many times.
 */
#define MUTEX_SHARE_ISEXCLUSIVE (-1024)

/*
 * Get an exclusive lock on a possibly sharable latch. We use the native
 * MUTEX_SET() operation for non-sharable latches; it usually is faster.
 */
#define MUTEXP_ACQUIRE(mutexp)  \
    (F_ISSET(mutexp, DB_MUTEX_SHARED) ?         \
    atomic_compare_exchange(env,                \
        &(mutexp)->sharecount, 0, MUTEX_SHARE_ISEXCLUSIVE) :    \
    MUTEX_SET(&(mutexp)->tas))
#else
#define MUTEXP_ACQUIRE(mutexp)      MUTEX_SET(&(mutexp)->tas)
#endif

#ifndef MEMBAR_ENTER
#define MEMBAR_ENTER()
#define MEMBAR_EXIT()
#endif

#endif

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_MUTEX_INT_H_ */
