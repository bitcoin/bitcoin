/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

#if DB_VERSION_MAJOR < 4 || DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 5
/*
 * !!!
 * We build this file in old versions of Berkeley DB when we're doing test
 * runs using the test_micro tool.   Without a prototype in place, we get
 * warnings, and there's no simple workaround.
 */
char *strsep();
#endif

/*
 * __db_util_arg --
 *	Convert a string into an argc/argv pair.
 *
 * PUBLIC: int __db_util_arg __P((char *, char *, int *, char ***));
 */
int
__db_util_arg(arg0, str, argcp, argvp)
	char *arg0, *str, ***argvp;
	int *argcp;
{
	int n, ret;
	char **ap, **argv;

#define	MAXARGS	25
	if ((ret =
	    __os_malloc(NULL, (MAXARGS + 1) * sizeof(char **), &argv)) != 0)
		return (ret);

	ap = argv;
	*ap++ = arg0;
	for (n = 1; (*ap = strsep(&str, " \t")) != NULL;)
		if (**ap != '\0') {
			++ap;
			if (++n == MAXARGS)
				break;
		}
	*ap = NULL;

	*argcp = ap - argv;
	*argvp = argv;

	return (0);
}
