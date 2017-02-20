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
 * __os_dirlist --
 *	Return a list of the files in a directory.
 */
int
__os_dirlist(env, dir, returndir, namesp, cntp)
	ENV *env;
	const char *dir;
	int returndir, *cntp;
	char ***namesp;
{
	FileInfo fi;
	IFileMgr *pIFileMgr;
	int arraysz, cnt, ret;
	char *filename, *p, **names;

	FILE_MANAGER_CREATE(env, pIFileMgr, ret);
	if (ret != 0)
		return (ret);

	if ((ret = IFILEMGR_EnumInit(pIFileMgr, dir, FALSE)) != SUCCESS) {
		IFILEMGR_Release(pIFileMgr);
		__db_syserr(env, ret, "IFILEMGR_EnumInit");
		return (__os_posix_err(ret));
	}

	names = NULL;
	arraysz = cnt = 0;
	while (IFILEMGR_EnumNext(pIFileMgr, &fi) != FALSE) {
		if (++cnt >= arraysz) {
			arraysz += 100;
			if ((ret = __os_realloc(env,
			    (u_int)arraysz * sizeof(char *), &names)) != 0)
				goto nomem;
		}
		for (filename = fi.szName;
		    (p = strchr(filename, '\\')) != NULL; filename = p + 1)
			;
		for (; (p = strchr(filename, '/')) != NULL; filename = p + 1)
			;
		if ((ret = __os_strdup(env, filename, &names[cnt - 1])) != 0)
			goto nomem;
	}
	IFILEMGR_Release(pIFileMgr);

	*namesp = names;
	*cntp = cnt;
	return (ret);

nomem:	if (names != NULL)
		__os_dirfree(env, names, cnt);
	IFILEMGR_Release(pIFileMgr);

	COMPQUIET(returndir, 0);

	return (ret);
}

/*
 * __os_dirfree --
 *	Free the list of files.
 */
void
__os_dirfree(env, names, cnt)
	ENV *env;
	char **names;
	int cnt;
{
	while (cnt > 0)
		__os_free(env, names[--cnt]);
	__os_free(env, names);
}
