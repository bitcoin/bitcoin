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
 * __os_ctime --
 *	Format a time-stamp.
 *
 * PUBLIC: char *__os_ctime __P((const time_t *, char *));
 */
char *
__os_ctime(tod, time_buf)
	const time_t *tod;
	char *time_buf;
{
	time_buf[CTIME_BUFLEN - 1] = '\0';

	/*
	 * The ctime_r interface is the POSIX standard, thread-safe version of
	 * ctime.  However, it was implemented in three different ways (with
	 * and without a buffer length argument, and where the buffer length
	 * argument was an int vs. a size_t *).  Also, you can't depend on a
	 * return of (char *) from ctime_r, HP-UX 10.XX's version returned an
	 * int.
	 */
#if defined(HAVE_VXWORKS)
	{
	size_t buflen = CTIME_BUFLEN;
	(void)ctime_r(tod, time_buf, &buflen);
	}
#elif defined(HAVE_CTIME_R_3ARG)
	(void)ctime_r(tod, time_buf, CTIME_BUFLEN);
#elif defined(HAVE_CTIME_R)
	(void)ctime_r(tod, time_buf);
#else
	(void)strncpy(time_buf, ctime(tod), CTIME_BUFLEN - 1);
#endif
	return (time_buf);
}
