/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_gettime --
 *	Return the current time-of-day clock in seconds and nanoseconds.
 */
void
__os_gettime(env, tp, monotonic)
	ENV *env;
	db_timespec *tp;
	int monotonic;
{
	/*
	 * Berkeley DB uses POSIX time values internally; convert a BREW time
	 * value into a POSIX time value.
	 */
	 tp->tv_sec =
#ifdef HAVE_BREW_SDK2
	    (time_t)GETTIMESECONDS() + BREW_EPOCH_OFFSET;
#else
	    (time_t)GETUTCSECONDS() + BREW_EPOCH_OFFSET;
#endif
	tp->tv_nsec = 0;
}
