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
 * __db_mkpath -- --
 *	Create intermediate directories.
 *
 * PUBLIC: int __db_mkpath __P((ENV *, const char *));
 */
int
__db_mkpath(env, name)
	ENV *env;
	const char *name;
{
	size_t len;
	int ret;
	char *p, *t, savech;

	/*
	 * Get a copy so we can modify the string.  It's a path and potentially
	 * quite long, so don't allocate the space on the stack.
	 */
	len = strlen(name) + 1;
	if ((ret = __os_malloc(env, len, &t)) != 0)
		return (ret);
	memcpy(t, name, len);

	/*
	 * Cycle through the path, creating intermediate directories.
	 *
	 * Skip the first byte if it's a path separator, it's the start of an
	 * absolute pathname.
	 */
	if (PATH_SEPARATOR[1] == '\0') {
		for (p = t + 1; p[0] != '\0'; ++p)
			if (p[0] == PATH_SEPARATOR[0]) {
				savech = *p;
				*p = '\0';
				if (__os_exists(env, t, NULL) &&
				    (ret = __os_mkdir(
					env, t, env->dir_mode)) != 0)
					break;
				*p = savech;
			}
	} else
		for (p = t + 1; p[0] != '\0'; ++p)
			if (strchr(PATH_SEPARATOR, p[0]) != NULL) {
				savech = *p;
				*p = '\0';
				if (__os_exists(env, t, NULL) &&
				    (ret = __os_mkdir(
					env, t, env->dir_mode)) != 0)
					break;
				*p = savech;
			}

	__os_free(env, t);
	return (ret);
}
