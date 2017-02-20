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
 */
int
__os_getenv(env, name, bpp, buflen)
	ENV *env;
	const char *name;
	char **bpp;
	size_t buflen;
{
#ifdef DB_WINCE
	COMPQUIET(name, NULL);
	/* WinCE does not have a getenv implementation. */
	return (0);
#else
	_TCHAR *tname, tbuf[1024];
	int ret;
	char *p;

	/*
	 * If there's a value and the buffer is large enough:
	 *	copy value into the pointer, return 0
	 * If there's a value and the buffer is too short:
	 *	set pointer to NULL, return EINVAL
	 * If there's no value:
	 *	set pointer to NULL, return 0
	 */
	if ((p = getenv(name)) != NULL) {
		if (strlen(p) < buflen) {
			(void)strcpy(*bpp, p);
			return (0);
		}
		goto small_buf;
	}

	TO_TSTRING(env, name, tname, ret);
	if (ret != 0)
		return (ret);
	/*
	 * The declared size of the tbuf buffer limits the maximum environment
	 * variable size in Berkeley DB on Windows.  If that's too small, or if
	 * we need to get rid of large allocations on the BDB stack, we should
	 * malloc the tbuf memory.
	 */
	ret = GetEnvironmentVariable(tname, tbuf, sizeof(tbuf));
	FREE_STRING(env, tname);

	/*
	 * If GetEnvironmentVariable succeeds, the return value is the number
	 * of characters stored in the buffer pointed to by lpBuffer, not
	 * including the terminating null character.  If the buffer is not
	 * large enough to hold the data, the return value is the buffer size,
	 * in characters, required to hold the string and its terminating null
	 * character.  If GetEnvironmentVariable fails, the return value is
	 * zero.  If the specified environment variable was not found in the
	 * environment block, GetLastError returns ERROR_ENVVAR_NOT_FOUND.
	 */
	if (ret == 0) {
		if ((ret = __os_get_syserr()) == ERROR_ENVVAR_NOT_FOUND) {
			*bpp = NULL;
			return (0);
		}
		__db_syserr(env, ret, "GetEnvironmentVariable");
		return (__os_posix_err(ret));
	}
	if (ret > (int)sizeof(tbuf))
		goto small_buf;

	FROM_TSTRING(env, tbuf, p, ret);
	if (ret != 0)
		return (ret);
	if (strlen(p) < buflen)
		(void)strcpy(*bpp, p);
	else
		*bpp = NULL;
	FREE_STRING(env, p);
	if (*bpp == NULL)
		goto small_buf;

	return (0);

small_buf:
	*bpp = NULL;
	__db_errx(env,
	    "%s: buffer too small to hold environment variable %s",
	    name, p);
	return (EINVAL);
#endif
}
