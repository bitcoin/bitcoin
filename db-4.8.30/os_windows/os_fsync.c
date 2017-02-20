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
 * __os_fsync --
 *	Flush a file descriptor.
 */
int
__os_fsync(env, fhp)
	ENV *env;
	DB_FH *fhp;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	/*
	 * Do nothing if the file descriptor has been marked as not requiring
	 * any sync to disk.
	 */
	if (F_ISSET(fhp, DB_FH_NOSYNC))
		return (0);

	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: flush %s", fhp->name);

	RETRY_CHK((!FlushFileBuffers(fhp->handle)), ret);
	if (ret != 0) {
		__db_syserr(env, ret, "FlushFileBuffers");
		ret = __os_posix_err(ret);
	}
	return (ret);
}
