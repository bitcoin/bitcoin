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
 *
 * PUBLIC: int __os_rename __P((ENV *,
 * PUBLIC:    const char *, const char *, u_int32_t));
 */
int
__os_rename(env, oldname, newname, silent)
	ENV *env;
	const char *oldname, *newname;
	u_int32_t silent;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;
	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: rename %s to %s", oldname, newname);

	LAST_PANIC_CHECK_BEFORE_IO(env);

	if (DB_GLOBAL(j_rename) != NULL)
		ret = DB_GLOBAL(j_rename)(oldname, newname);
	else
		RETRY_CHK((rename(oldname, newname)), ret);

	/*
	 * If "silent" is not set, then errors are OK and we should not output
	 * an error message.
	 */
	if (ret != 0) {
		if (!silent)
			__db_syserr(
			    env, ret, "rename %s %s", oldname, newname);
		ret = __os_posix_err(ret);
	}
	return (ret);
}
