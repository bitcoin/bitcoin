/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_yield --
 *	Yield the processor, optionally pausing until running again.
 */
void
__os_yield(env, secs, usecs)
	ENV *env;
	u_long secs, usecs;		/* Seconds and microseconds. */
{
	COMPQUIET(env, NULL);

#ifdef HAVE_BREW_SDK2
	COMPQUIET(secs, 0);
	COMPQUIET(usecs, 0);
#else
	/* Don't require the values be normalized. */
	for (; usecs >= US_PER_SEC; usecs -= US_PER_SEC)
		++secs;

	/*
	 * Yield the processor so other processes or threads can run.
	 *
	 * Sheer raving paranoia -- don't sleep for 0 time, in case some
	 * implementation doesn't yield the processor in that case.
	 */
	MSLEEP(secs * MS_PER_SEC + (usecs / US_PER_MS) + 1);
#endif
}
