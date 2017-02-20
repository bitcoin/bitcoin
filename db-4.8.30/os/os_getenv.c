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
 * __os_getenv --
 *	Retrieve an environment variable.
 *
 * PUBLIC: int __os_getenv __P((ENV *, const char *, char **, size_t));
 */
int
__os_getenv(env, name, bpp, buflen)
	ENV *env;
	const char *name;
	char **bpp;
	size_t buflen;
{
	/*
	 * If we have getenv, there's a value and the buffer is large enough:
	 *	copy value into the pointer, return 0
	 * If we have getenv, there's a value  and the buffer is too short:
	 *	set pointer to NULL, return EINVAL
	 * If we have getenv and there's no value:
	 *	set pointer to NULL, return 0
	 * If we don't have getenv:
	 *	set pointer to NULL, return 0
	 */
#ifdef HAVE_GETENV
	char *p;

	if ((p = getenv(name)) != NULL) {
		if (strlen(p) < buflen) {
			(void)strcpy(*bpp, p);
			return (0);
		}

		*bpp = NULL;
		__db_errx(env,
		    "%s: buffer too small to hold environment variable %s",
		    name, p);
		return (EINVAL);
	}
#else
	COMPQUIET(env, NULL);
	COMPQUIET(name, NULL);
	COMPQUIET(buflen, 0);
#endif
	*bpp = NULL;
	return (0);
}
