/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#ifndef _BENCH_H_
#define _BENCH_H_
#include "db_config.h"

#include "db_int.h"

#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 5
/*
 * Older releases of Berkeley DB don't include standard include files in
 * db_int.h.
 */
#ifdef DB_WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <direct.h>
#include <sys/timeb.h>
#else
#include <sys/stat.h>
#include <sys/time.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif
#endif

#define TESTDIR     "TESTDIR"
#define TESTFILE    "test_micro.db"

/*
 * Implement a custom assert to allow consistent behavior across builds and
 * platforms.
 *
 * The BDB library DB_ASSERT implementation is only enabled in diagnostic
 * builds -- so is not suitable here.
 */
#define DB_BENCH_ASSERT(e) do {                     \
    (e) ? (void)0 :                         \
        (fprintf(stderr,                        \
        "assert failure: %s/%d: \"%s\"\n", __FILE__, __LINE__, #e), \
        b_util_abort());                        \
} while (0)

#ifndef NS_PER_SEC
#define NS_PER_SEC  1000000000  /* Nanoseconds in a second */
#endif
#ifndef NS_PER_US
#define NS_PER_US   1000        /* Nanoseconds in a microsecond */
#endif
#ifndef MS_PER_NS
#define MS_PER_NS   1000000     /* Milliseconds in a nanosecond */
#endif

#ifdef DB_TIMEOUT_TO_TIMESPEC
/*
 * We have the timer routines in the Berkeley DB library after their conversion
 * to the POSIX timespec interfaces.  We'd rather use something that gives us
 * better information than elapsed wallclock time, so use getrusage instead if
 * it's available.
 */
#ifdef HAVE_GETRUSAGE
#include <sys/resource.h>

#define SET_TIMER_FROM_GETRUSAGE(tp) do {               \
    struct rusage __usage;                      \
    DB_BENCH_ASSERT(getrusage(RUSAGE_SELF, &__usage) == 0);     \
    (tp)->tv_sec =                          \
        __usage.ru_utime.tv_sec + __usage.ru_stime.tv_sec;      \
    (tp)->tv_nsec = NS_PER_US *                 \
        (__usage.ru_utime.tv_usec + __usage.ru_stime.tv_usec);  \
} while (0);

#define TIMER_START SET_TIMER_FROM_GETRUSAGE(&__start_time);
#define TIMER_STOP  SET_TIMER_FROM_GETRUSAGE(&__end_time);

#elif defined(DB_WIN32) && !defined(DB_WINCE)

#define SET_TIMER_FROM_GETPROCESSTIMES(tp) do {             \
    FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTIme;  \
    LARGE_INTEGER large_int;                    \
    LONGLONG __ns_since_epoch;                  \
    DB_BENCH_ASSERT(                        \
        GetProcessTimes(GetCurrentProcess(), &lpCreationTime,   \
        &lpExitTime, &lpKernelTime, &lpUserTIme) != 0);     \
    memcpy(&large_int, &lpKernelTime, sizeof(lpKernelTime));    \
    __ns_since_epoch = (large_int.QuadPart * 100);          \
    (tp)->tv_sec = (time_t)(__ns_since_epoch / NS_PER_SEC);     \
    (tp)->tv_nsec = (long)(__ns_since_epoch % NS_PER_SEC);      \
    memcpy(&large_int, &lpUserTIme, sizeof(lpUserTIme));        \
    __ns_since_epoch = (large_int.QuadPart * 100);          \
    (tp)->tv_sec += (time_t)(__ns_since_epoch / NS_PER_SEC);    \
    (tp)->tv_nsec += (long)(__ns_since_epoch % NS_PER_SEC);     \
} while (0);

#define TIMER_START SET_TIMER_FROM_GETPROCESSTIMES(&__start_time);
#define TIMER_STOP  SET_TIMER_FROM_GETPROCESSTIMES(&__end_time);

#else /* !HAVEGETRUSAGE && !DB_WIN32 */

#if DB_VERSION_MAJOR > 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 6
#define TIMER_START __os_gettime(NULL, &__start_time, 1)
#define TIMER_STOP  __os_gettime(NULL, &__end_time, 1)
#else
#define TIMER_START __os_gettime(NULL, &__start_time)
#define TIMER_STOP  __os_gettime(NULL, &__end_time)
#endif
#endif /* !HAVE_GETRUSAGE */

#else /* !DB_TIMEOUT_TO_TIMESPEC */

#if defined(HAVE_CLOCK_GETTIME)
typedef struct timespec db_timespec;
#else
typedef struct {
    time_t  tv_sec;             /* seconds */
    long    tv_nsec;            /* nanoseconds */
} db_timespec;
#endif

#define timespecadd(vvp, uvp)                       \
    do {                                \
        (vvp)->tv_sec += (uvp)->tv_sec;             \
        (vvp)->tv_nsec += (uvp)->tv_nsec;           \
        if ((vvp)->tv_nsec >= NS_PER_SEC) {         \
            (vvp)->tv_sec++;                \
            (vvp)->tv_nsec -= NS_PER_SEC;           \
        }                           \
    } while (0)
#define timespecsub(vvp, uvp)                       \
    do {                                \
        (vvp)->tv_sec -= (uvp)->tv_sec;             \
        (vvp)->tv_nsec -= (uvp)->tv_nsec;           \
        if ((vvp)->tv_nsec < 0) {               \
            (vvp)->tv_sec--;                \
            (vvp)->tv_nsec += NS_PER_SEC;           \
        }                           \
    } while (0)

#define TIMER_START CLOCK(__start_time)
#define TIMER_STOP  CLOCK(__end_time)

#if defined(HAVE_CLOCK_GETTIME)
#define CLOCK(tm) do {                          \
    DB_BENCH_ASSERT(clock_gettime(                  \
        CLOCK_REALTIME, (struct timespec *)&(tm)) == 0);        \
} while (0)
#elif defined(DB_WIN32)
#define CLOCK(tm) do {                          \
    struct _timeb __now;                        \
    _ftime(&__now);                         \
    (tm).tv_sec = __now.time;                   \
    (tm).tv_nsec = __now.millitm * MS_PER_NS;           \
} while (0)
#else
#define CLOCK(tm) do {                          \
    struct timeval __tp;                        \
    DB_BENCH_ASSERT(gettimeofday(&__tp, NULL) == 0);        \
    (tm).tv_sec = __tp.tv_sec;                  \
    (tm).tv_nsec = __tp.tv_usec * NS_PER_US;            \
} while (0)
#endif
#endif /* !DB_TIMEOUT_TO_TIMESPEC */

extern db_timespec __start_time, __end_time;

#define TIMER_GET(tm) do {                      \
    tm = __end_time;                        \
    timespecsub(&(tm), &__start_time);              \
} while (0)
#define TIMER_DISPLAY(ops) do {                     \
    db_timespec __tmp_time;                     \
    __tmp_time = __end_time;                    \
    timespecsub(&__tmp_time, &__start_time);            \
    TIME_DISPLAY(ops, __tmp_time);                  \
} while (0)
#define TIME_DISPLAY(ops, tm) do {                  \
    double __secs;                          \
    int __major, __minor, __patch;                  \
    __secs = (tm).tv_sec + (double)(tm).tv_nsec / NS_PER_SEC;   \
    (void)db_version(&__major, &__minor, &__patch);         \
    printf("%d.%d.%d\t%.2f\n", __major, __minor, __patch,       \
        (__secs == 0) ? 0.0 : (ops) / __secs);          \
} while (0)

extern char *progname;                  /* program name */

int  b_curalloc __P((int, char *[]));
int  b_curwalk __P((int, char *[]));
int  b_del __P((int, char *[]));
int  b_get __P((int, char *[]));
int  b_inmem __P((int, char *[]));
int  b_latch __P((int, char *[]));
int  b_load __P((int, char *[]));
int  b_open __P((int, char *[]));
int  b_put __P((int, char *[]));
int  b_recover __P((int, char *[]));
int  b_txn __P((int, char *[]));
int  b_txn_write __P((int, char *[]));
int  b_uname __P((void));
void b_util_abort __P((void));
int  b_util_dir_setup __P((void));
int  b_util_dir_teardown __P((void));
int  b_util_have_hash __P((void));
int  b_util_have_queue __P((void));
int  b_util_unlink __P((char *));
int  b_workload __P((int, char *[]));
u_int32_t part_callback __P((DB *, DBT *));

#endif /* !_BENCH_H_ */
