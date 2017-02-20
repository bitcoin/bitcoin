/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
/*
 * Copyright (c) 1982, 1986, 1993
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *  @(#)time.h  8.5 (Berkeley) 5/4/95
 * FreeBSD: src/sys/sys/time.h,v 1.65 2004/04/07 04:19:49 imp Exp
 */

#ifndef _DB_CLOCK_H_
#define _DB_CLOCK_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * This declaration is POSIX-compatible.  Because there are lots of different
 * time.h include file patterns out there, it's easier to declare our own name
 * in all cases than to try and discover if a system has a struct timespec.
 * For the same reason, and because we'd have to #include <sys/time.h> in db.h,
 * we don't export any timespec structures in the DB API, even in places where
 * it would make sense, like the replication statistics information.
 */
typedef struct {
    time_t  tv_sec;             /* seconds */
    long    tv_nsec;            /* nanoseconds */
} db_timespec;

/* Operations on timespecs */
#undef  timespecclear
#define timespecclear(tvp)  ((tvp)->tv_sec = (tvp)->tv_nsec = 0)
#undef  timespecisset
#define timespecisset(tvp)  ((tvp)->tv_sec || (tvp)->tv_nsec)
#undef  timespeccmp
#define timespeccmp(tvp, uvp, cmp)                  \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?             \
        ((tvp)->tv_nsec cmp (uvp)->tv_nsec) :           \
        ((tvp)->tv_sec cmp (uvp)->tv_sec))
#undef timespecadd
/*
 * Note that using timespecadd to add to yourself (i.e. doubling)
 * must be supported.
 */
#define timespecadd(vvp, uvp)                       \
    do {                                \
        (vvp)->tv_sec += (uvp)->tv_sec;             \
        (vvp)->tv_nsec += (uvp)->tv_nsec;           \
        if ((vvp)->tv_nsec >= 1000000000) {         \
            (vvp)->tv_sec++;                \
            (vvp)->tv_nsec -= 1000000000;           \
        }                           \
    } while (0)
#undef timespecsub
#define timespecsub(vvp, uvp)                       \
    do {                                \
        (vvp)->tv_sec -= (uvp)->tv_sec;             \
        (vvp)->tv_nsec -= (uvp)->tv_nsec;           \
        if ((vvp)->tv_nsec < 0) {               \
            (vvp)->tv_sec--;                \
            (vvp)->tv_nsec += 1000000000;           \
        }                           \
    } while (0)

#undef timespecset
#define timespecset(vvp, sec, nsec)                 \
    do {                                \
        (vvp)->tv_sec = (time_t)(sec);              \
        (vvp)->tv_nsec = (long)(nsec);              \
    } while (0)

#define DB_TIMEOUT_TO_TIMESPEC(t, vvp)                  \
    do {                                \
        (vvp)->tv_sec = (time_t)((t) / 1000000);        \
        (vvp)->tv_nsec = (long)(((t) % 1000000) * 1000);    \
    } while (0)

#define DB_TIMESPEC_TO_TIMEOUT(t, vvp, prec)                \
    do {                                \
        t = (u_long)((vvp)->tv_sec * 1000000);          \
        t += (u_long)((vvp)->tv_nsec / 1000);           \
        /* Add in 1 usec for lost nsec precision if wanted. */  \
        if (prec)                       \
            t++;                        \
    } while (0)

#define TIMESPEC_ADD_DB_TIMEOUT(vvp, t)                 \
    do {                                    \
        db_timespec __tmp;                      \
        DB_TIMEOUT_TO_TIMESPEC(t, &__tmp);              \
        timespecadd((vvp), &__tmp);                 \
    } while (0)

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_CLOCK_H_ */
