/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
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
	if (monotonic) {
		/*
		 * The elapsed time is stored as a DWORD value, so time wraps
		 * around to zero if the system runs for 49.7 days.  Initialize
		 * a base value with 50 days worth of seconds, and add 50 more
		 * days every time the counter wraps.  That ensures we always
		 * move forward.
		 *
		 * It's possible this code could race, but the danger is we
		 * would increment base_seconds more than once per wrap and
		 * eventually overflow, which is a pretty remote possibility.
		 */
#define	TIMER_WRAP_SECONDS	(50 * 24 * 60 * 60)
		static DWORD last_ticks;
		static time_t base_seconds;
		DWORD ticks;

		ticks = GetTickCount();
		if (ticks < last_ticks)
			base_seconds += TIMER_WRAP_SECONDS;
		last_ticks = ticks;
		tp->tv_sec = base_seconds + (u_int32_t)(ticks / 1000);
		tp->tv_nsec = (u_int32_t)((ticks % 1000) * NS_PER_MS);
	} else {
#ifdef DB_WINCE
		FILETIME ft;
		LARGE_INTEGER large_int;
		LONGLONG ns_since_epoch, utc1970;
		SYSTEMTIME st;

		(void)GetSystemTime(&st);
		(void)SystemTimeToFileTime(&st, &ft);

		/*
		 * A FILETIME expresses time as 100 nanosecond chunks from
		 * Jan 1, 1601; convert to a timespec where the time is
		 * is expressed in seconds and nanoseconds from Jan 1, 1970.
		 *
		 * UTC_1970 is the number of 100-nano-second chunks from
		 * 1601 to 1970.
		 */
#define	NS100_PER_SEC	(NS_PER_SEC / 100)
#define	UTC_1970	(((LONGLONG)27111902 << 32) + (LONGLONG)3577643008)
		memcpy(&large_int, &ft, sizeof(large_int));
		utc1970 = UTC_1970;
		ns_since_epoch = (large_int.QuadPart - utc1970);
		tp->tv_sec = (time_t)(ns_since_epoch / NS100_PER_SEC);
		tp->tv_nsec = (long)(ns_since_epoch % NS100_PER_SEC);
#else
		struct _timeb now;

		_ftime(&now);
		tp->tv_sec = now.time;
		tp->tv_nsec = now.millitm * NS_PER_MS;
#endif
	}
}
