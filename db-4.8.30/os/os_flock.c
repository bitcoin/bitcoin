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
 * __os_fdlock --
 *	Acquire/release a lock on a byte in a file.
 *
 * PUBLIC: int __os_fdlock __P((ENV *, DB_FH *, off_t, int, int));
 */
int
__os_fdlock(env, fhp, offset, acquire, nowait)
	ENV *env;
	DB_FH *fhp;
	int acquire, nowait;
	off_t offset;
{
#ifdef HAVE_FCNTL
	DB_ENV *dbenv;
	struct flock fl;
	int ret, t_ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	DB_ASSERT(env, F_ISSET(fhp, DB_FH_OPENED) && fhp->fd != -1);

	if (dbenv != NULL && FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS_ALL))
		__db_msg(env,
		    "fileops: flock %s %s offset %lu",
		    fhp->name, acquire ? "acquire": "release", (u_long)offset);

	fl.l_start = offset;
	fl.l_len = 1;
	fl.l_type = acquire ? F_WRLCK : F_UNLCK;
	fl.l_whence = SEEK_SET;

	RETRY_CHK_EINTR_ONLY(
	    (fcntl(fhp->fd, nowait ? F_SETLK : F_SETLKW, &fl)), ret);

	if (ret == 0)
		return (0);

	if ((t_ret = __os_posix_err(ret)) != EACCES && t_ret != EAGAIN)
		__db_syserr(env, ret, "fcntl");
	return (t_ret);
#else
	COMPQUIET(fhp, NULL);
	COMPQUIET(acquire, 0);
	COMPQUIET(nowait, 0);
	COMPQUIET(offset, 0);
	__db_syserr(env, DB_OPNOTSUP, "advisory file locking unavailable");
	return (DB_OPNOTSUP);
#endif
}
