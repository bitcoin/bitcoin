/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_truncate --
 *	Truncate the file.
 *
 * PUBLIC: int __os_truncate __P((ENV *, DB_FH *, db_pgno_t, u_int32_t));
 */
int
__os_truncate(env, fhp, pgno, pgsize)
	ENV *env;
	DB_FH *fhp;
	db_pgno_t pgno;
	u_int32_t pgsize;
{
	DB_ENV *dbenv;
	off_t offset;
	int ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	/*
	 * Truncate a file so that "pgno" is discarded from the end of the
	 * file.
	 */
	offset = (off_t)pgsize * pgno;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env,
		    "fileops: truncate %s to %lu", fhp->name, (u_long)offset);

	LAST_PANIC_CHECK_BEFORE_IO(env);

	if (DB_GLOBAL(j_ftruncate) != NULL)
		ret = DB_GLOBAL(j_ftruncate)(fhp->fd, offset);
	else {
#ifdef HAVE_FTRUNCATE
		RETRY_CHK((ftruncate(fhp->fd, offset)), ret);
#else
		ret = DB_OPNOTSUP;
#endif
	}

	if (ret != 0) {
		__db_syserr(env, ret, "ftruncate: %lu", (u_long)offset);
		ret = __os_posix_err(ret);
	}

	return (ret);
}
