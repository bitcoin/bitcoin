/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __mut_failchk --
 *	Check for mutexes held by dead processes.
 *
 * PUBLIC: int __mut_failchk __P((ENV *));
 */
int
__mut_failchk(env)
	ENV *env;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	db_mutex_t i;
	int ret;
	char buf[DB_THREADID_STRLEN];

	dbenv = env->dbenv;
	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	ret = 0;

	MUTEX_SYSTEM_LOCK(env);
	for (i = 1; i <= mtxregion->stat.st_mutex_cnt; ++i, ++mutexp) {
		mutexp = MUTEXP_SET(mtxmgr, i);

		/*
		 * We're looking for per-process mutexes where the process
		 * has died.
		 */
		if (!F_ISSET(mutexp, DB_MUTEX_ALLOCATED) ||
		    !F_ISSET(mutexp, DB_MUTEX_PROCESS_ONLY))
			continue;

		/*
		 * The thread that allocated the mutex may have exited, but
		 * we cannot reclaim the mutex if the process is still alive.
		 */
		if (dbenv->is_alive(
		    dbenv, mutexp->pid, 0, DB_MUTEX_PROCESS_ONLY))
			continue;

		__db_msg(env, "Freeing mutex for process: %s",
		    dbenv->thread_id_string(dbenv, mutexp->pid, 0, buf));

		/* Unlock and free the mutex. */
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED))
			MUTEX_UNLOCK(env, i);

		if ((ret = __mutex_free_int(env, 0, &i)) != 0)
			break;
	}
	MUTEX_SYSTEM_UNLOCK(env);

	return (ret);
}
