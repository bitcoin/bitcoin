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
 *
 * PUBLIC: int __os_unlink __P((ENV *, const char *, int));
 */
int
__os_unlink(env, path, overwrite_test)
	ENV *env;
	const char *path;
	int overwrite_test;
{
	DB_ENV *dbenv;
	int ret, t_ret;

	dbenv = env == NULL ? NULL : env->dbenv;

	if (dbenv != NULL &&
	    FLD_ISSET(dbenv->verbose, DB_VERB_FILEOPS | DB_VERB_FILEOPS_ALL))
		__db_msg(env, "fileops: unlink %s", path);

	/* Optionally overwrite the contents of the file to enhance security. */
	if (dbenv != NULL && overwrite_test && F_ISSET(dbenv, DB_ENV_OVERWRITE))
		(void)__db_file_multi_write(env, path);

	LAST_PANIC_CHECK_BEFORE_IO(env);

	if (DB_GLOBAL(j_unlink) != NULL)
		ret = DB_GLOBAL(j_unlink)(path);
	else {
		RETRY_CHK((unlink(CHAR_STAR_CAST path)), ret);
#ifdef HAVE_QNX
		/*
		 * The file may be a region file created by shm_open, not a
		 * regular file.  Try and delete using unlink, and if that
		 * fails for an unexpected reason, try a shared memory unlink.
		 */
		if (ret != 0 && __os_posix_err(ret) != ENOENT)
			RETRY_CHK((shm_unlink(path)), ret);
#endif
	}

	/*
	 * !!!
	 * The results of unlink are file system driver specific on VxWorks.
	 * In the case of removing a file that did not exist, some, at least,
	 * return an error, but with an errno of 0, not ENOENT.  We do not
	 * have to test for that explicitly, the RETRY_CHK macro resets "ret"
	 * to be the errno, and so we'll just slide right on through.
	 *
	 * XXX
	 * We shouldn't be testing for an errno of ENOENT here, but ENOENT
	 * signals that a file is missing, and we attempt to unlink things
	 * (such as v. 2.x environment regions, in ENV->remove) that we
	 * are expecting not to be there.  Reporting errors in these cases
	 * is annoying.
	 */
	if (ret != 0) {
		t_ret = __os_posix_err(ret);
		if (t_ret != ENOENT)
			__db_syserr(env, ret, "unlink: %s", path);
		ret = t_ret;
	}

	return (ret);
}
