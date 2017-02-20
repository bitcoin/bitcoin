/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/* vxworks API to get system clock rate */
int sysClkRateGet (void);

/*
 * __os_yield --
 *	Yield the processor, optionally pausing until running again.
 */
void
__os_yield(env, secs, usecs)
	ENV *env;
	u_long secs, usecs;		/* Seconds and microseconds. */
{
	int ticks_delay, ticks_per_second;

	COMPQUIET(env, NULL);

	/* Don't require the values be normalized. */
	for (; usecs >= US_PER_SEC; usecs -= US_PER_SEC)
		++secs;

	/*
	 * Yield the processor so other processes or threads can run.
	 *
	 * As a side effect, taskDelay() moves the calling task to the end of
	 * the ready queue for tasks of the same priority. In particular, you
	 * can yield the CPU to any other tasks of the same priority by
	 * "delaying" for zero clock ticks.
	 *
	 * Never wait less than a tick, if we were supposed to wait at all.
	 */
	ticks_per_second = sysClkRateGet();
	ticks_delay =
	    secs * ticks_per_second + (usecs * ticks_per_second) / US_PER_SEC;
	if (ticks_delay == 0 && (secs != 0 || usecs != 0))
		ticks_delay = 1;
	(void)taskDelay(ticks_delay);
}
