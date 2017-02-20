/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#define	__INCLUDE_SELECT_H	1
#include "db_int.h"

#if defined(HAVE_SYSTEM_INCLUDE_FILES) && defined(HAVE_SCHED_YIELD)
#include <sched.h>
#endif

static void __os_sleep __P((ENV *, u_long, u_long));

/*
 * __os_yield --
 *	Yield the processor, optionally pausing until running again.
 *
 * PUBLIC: void __os_yield __P((ENV *, u_long, u_long));
 */
void
__os_yield(env, secs, usecs)
	ENV *env;
	u_long secs, usecs;		/* Seconds and microseconds. */
{
	/*
	 * Don't require the values be normalized (some operating systems
	 * return an error if the usecs argument to select is too large).
	 */
	for (; usecs >= US_PER_SEC; usecs -= US_PER_SEC)
		++secs;

	if (DB_GLOBAL(j_yield) != NULL) {
		(void)DB_GLOBAL(j_yield)(secs, usecs);
		return;
	}

	/*
	 * Yield the processor so other processes or threads can run.  Use
	 * the local yield call if not pausing, otherwise call the select
	 * function.
	 */
	if (secs != 0 || usecs != 0)
		__os_sleep(env, secs, usecs);
	else {
#if defined(HAVE_MUTEX_UI_THREADS)
		thr_yield();
#elif defined(HAVE_PTHREAD_YIELD)
		pthread_yield();
#elif defined(HAVE_SCHED_YIELD)
		(void)sched_yield();
#elif defined(HAVE_YIELD)
		yield();
#else
		__os_sleep(env, 0, 0);
#endif
	}
}

/*
 * __os_sleep --
 *	Pause the thread of control.
 */
static void
__os_sleep(env, secs, usecs)
	ENV *env;
	u_long secs, usecs;		/* Seconds and microseconds. */
{
	struct timeval t;
	int ret;

	/*
	 * Sheer raving paranoia -- don't select for 0 time, in case some
	 * implementation doesn't yield the processor in that case.
	 */
	t.tv_sec = (long)secs;
	t.tv_usec = (long)usecs + 1;

	/*
	 * We don't catch interrupts and restart the system call here, unlike
	 * other Berkeley DB system calls.  This may be a user attempting to
	 * interrupt a sleeping DB utility (for example, db_checkpoint), and
	 * we want the utility to see the signal and quit.  This assumes it's
	 * always OK for DB to sleep for less time than originally scheduled.
	 */
	if (select(0, NULL, NULL, NULL, &t) == -1) {
		ret = __os_get_syserr();
		if (__os_posix_err(ret) != EINTR)
			__db_syserr(env, ret, "select");
	}
}
