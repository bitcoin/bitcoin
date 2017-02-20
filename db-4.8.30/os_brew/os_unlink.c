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
 * __os_unlink --
 *	Remove a file.
 */
int
__os_unlink(env, path, overwrite_test)
	ENV *env;
	const char *path;
	int overwrite_test;
{
	IFileMgr *ifmp;
	int ret;

	FILE_MANAGER_CREATE(env, ifmp, ret);
	if (ret != 0)
		return (ret);

	LAST_PANIC_CHECK_BEFORE_IO(env);

	if (IFILEMGR_Remove(ifmp, path) == EFAILED)
		FILE_MANAGER_ERR(env, ifmp, path, "IFILEMGR_Remove", ret);

	IFILEMGR_Release(ifmp);

	COMPQUIET(overwrite_test, 0);

	return (ret);
}
