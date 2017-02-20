/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __db_util_logset --
 *	Log that we're running.
 *
 * PUBLIC: int __db_util_logset __P((const char *, char *));
 */
int
__db_util_logset(progname, fname)
	const char *progname;
	char *fname;
{
	pid_t pid;
	FILE *fp;
	time_t now;
	char time_buf[CTIME_BUFLEN];

	if ((fp = fopen(fname, "w")) == NULL)
		goto err;

	(void)time(&now);

	__os_id(NULL, &pid, NULL);
	fprintf(fp,
	    "%s: %lu %s", progname, (u_long)pid, __os_ctime(&now, time_buf));

	if (fclose(fp) == EOF)
		goto err;

	return (0);

err:	fprintf(stderr, "%s: %s: %s\n", progname, fname, strerror(errno));
	return (1);
}
