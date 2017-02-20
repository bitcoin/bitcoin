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
 * __os_rename --
 *	Rename a file.
 */
int
__os_rename(env, oldname, newname, silent)
	ENV *env;
	const char *oldname, *newname;
	u_int32_t silent;
{
	DB_ENV *dbenv;
	_TCHAR *toldname, *tnewname;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: rename %s to %s", oldname, newname);

	TO_TSTRING(env, oldname, toldname, ret);
	if (ret != 0)
		return (ret);
	TO_TSTRING(env, newname, tnewname, ret);
	if (ret != 0) {
		FREE_STRING(env, toldname);
		return (ret);
	}

	LAST_PANIC_CHECK_BEFORE_IO(env);

	if (!MoveFile(toldname, tnewname))
		ret = __os_get_syserr();

	if (__os_posix_err(ret) == EEXIST) {
		ret = 0;
#ifndef DB_WINCE
		if (__os_is_winnt()) {
			if (!MoveFileEx(
			    toldname, tnewname, MOVEFILE_REPLACE_EXISTING))
				ret = __os_get_syserr();
		} else
#endif
		{
			/*
			 * There is no MoveFileEx for Win9x/Me/CE, so we have to
			 * do the best we can.  Note that the MoveFile call
			 * above would have succeeded if oldname and newname
			 * refer to the same file, so we don't need to check
			 * that here.
			 */
			(void)DeleteFile(tnewname);
			if (!MoveFile(toldname, tnewname))
				ret = __os_get_syserr();
		}
	}

	FREE_STRING(env, tnewname);
	FREE_STRING(env, toldname);

	if (ret != 0) {
		if (silent == 0)
			__db_syserr(
			    env, ret, "MoveFileEx %s %s", oldname, newname);
		ret = __os_posix_err(ret);
	}

	return (ret);
}
