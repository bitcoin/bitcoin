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
 * __os_qnx_region_open --
 *	Open a shared memory region file using POSIX shm_open.
 *
 * PUBLIC: #ifdef HAVE_QNX
 * PUBLIC: int __os_qnx_region_open
 * PUBLIC:     __P((ENV *, const char *, int, int, DB_FH **));
 * PUBLIC: #endif
 */
int
__os_qnx_region_open(env, name, oflags, mode, fhpp)
	ENV *env;
	const char *name;
	int oflags, mode;
	DB_FH **fhpp;
{
	DB_FH *fhp;
	int fcntl_flags;
	int ret;

	/*
	 * Allocate the file handle and copy the file name.  We generally only
	 * use the name for verbose or error messages, but on systems where we
	 * can't unlink temporary files immediately, we use the name to unlink
	 * the temporary file when the file handle is closed.
	 *
	 * Lock the ENV handle and insert the new file handle on the list.
	 */
	if ((ret = __os_calloc(env, 1, sizeof(DB_FH), &fhp)) != 0)
		return (ret);
	if ((ret = __os_strdup(env, name, &fhp->name)) != 0)
		goto err;
	if (env != NULL) {
		MUTEX_LOCK(env, env->mtx_env);
		TAILQ_INSERT_TAIL(&env->fdlist, fhp, q);
		MUTEX_UNLOCK(env, env->mtx_env);
		F_SET(fhp, DB_FH_ENVLINK);
	}

	/*
	 * Once we have created the object, we don't need the name
	 * anymore.  Other callers of this will convert themselves.
	 */
	if ((fhp->fd = shm_open(name, oflags, mode)) == -1) {
		ret = __os_posix_err(__os_get_syserr());
err:		(void)__os_closehandle(env, fhp);
		return (ret);
	}

	F_SET(fhp, DB_FH_OPENED);

#ifdef HAVE_FCNTL_F_SETFD
	/* Deny file descriptor access to any child process. */
	if ((fcntl_flags = fcntl(fhp->fd, F_GETFD)) == -1 ||
	    fcntl(fhp->fd, F_SETFD, fcntl_flags | FD_CLOEXEC) == -1) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "fcntl(F_SETFD)");
		(void)__os_closehandle(env, fhp);
		return (__os_posix_err(ret));
	}
#else
	COMPQUIET(fcntl_flags, 0);
#endif
	F_SET(fhp, DB_FH_OPENED);
	*fhpp = fhp;
	return (0);
}
