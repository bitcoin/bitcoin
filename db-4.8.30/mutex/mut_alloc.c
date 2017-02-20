/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __mutex_alloc --
 *	Allocate a mutex from the mutex region.
 *
 * PUBLIC: int __mutex_alloc __P((ENV *, int, u_int32_t, db_mutex_t *));
 */
int
__mutex_alloc(env, alloc_id, flags, indxp)
	ENV *env;
	int alloc_id;
	u_int32_t flags;
	db_mutex_t *indxp;
{
	int ret;

	/* The caller may depend on us to initialize. */
	*indxp = MUTEX_INVALID;

	/*
	 * If this is not an application lock, and we've turned off locking,
	 * or the ENV handle isn't thread-safe, and this is a thread lock
	 * or the environment isn't multi-process by definition, there's no
	 * need to mutex at all.
	 */
	if (alloc_id != MTX_APPLICATION &&
	    (F_ISSET(env->dbenv, DB_ENV_NOLOCKING) ||
	    (!F_ISSET(env, ENV_THREAD) &&
	    (LF_ISSET(DB_MUTEX_PROCESS_ONLY) ||
	    F_ISSET(env, ENV_PRIVATE)))))
		return (0);

	/* Private environments never share mutexes. */
	if (F_ISSET(env, ENV_PRIVATE))
		LF_SET(DB_MUTEX_PROCESS_ONLY);

	/*
	 * If we have a region in which to allocate the mutexes, lock it and
	 * do the allocation.
	 */
	if (MUTEX_ON(env))
		return (__mutex_alloc_int(env, 1, alloc_id, flags, indxp));

	/*
	 * We have to allocate some number of mutexes before we have a region
	 * in which to allocate them.  We handle this by saving up the list of
	 * flags and allocating them as soon as we have a handle.
	 *
	 * The list of mutexes to alloc is maintained in pairs: first the
	 * alloc_id argument, second the flags passed in by the caller.
	 */
	if (env->mutex_iq == NULL) {
		env->mutex_iq_max = 50;
		if ((ret = __os_calloc(env, env->mutex_iq_max,
		    sizeof(env->mutex_iq[0]), &env->mutex_iq)) != 0)
			return (ret);
	} else if (env->mutex_iq_next == env->mutex_iq_max - 1) {
		env->mutex_iq_max *= 2;
		if ((ret = __os_realloc(env,
		    env->mutex_iq_max * sizeof(env->mutex_iq[0]),
		    &env->mutex_iq)) != 0)
			return (ret);
	}
	*indxp = env->mutex_iq_next + 1;	/* Correct for MUTEX_INVALID. */
	env->mutex_iq[env->mutex_iq_next].alloc_id = alloc_id;
	env->mutex_iq[env->mutex_iq_next].flags = flags;
	++env->mutex_iq_next;

	return (0);
}

/*
 * __mutex_alloc_int --
 *	Internal routine to allocate a mutex.
 *
 * PUBLIC: int __mutex_alloc_int
 * PUBLIC:	__P((ENV *, int, int, u_int32_t, db_mutex_t *));
 */
int
__mutex_alloc_int(env, locksys, alloc_id, flags, indxp)
	ENV *env;
	int locksys, alloc_id;
	u_int32_t flags;
	db_mutex_t *indxp;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	int ret;

	dbenv = env->dbenv;
	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	ret = 0;

	/*
	 * If we're not initializing the mutex region, then lock the region to
	 * allocate new mutexes.  Drop the lock before initializing the mutex,
	 * mutex initialization may require a system call.
	 */
	if (locksys)
		MUTEX_SYSTEM_LOCK(env);

	if (mtxregion->mutex_next == MUTEX_INVALID) {
		__db_errx(env,
		    "unable to allocate memory for mutex; resize mutex region");
		if (locksys)
			MUTEX_SYSTEM_UNLOCK(env);
		return (ENOMEM);
	}

	*indxp = mtxregion->mutex_next;
	mutexp = MUTEXP_SET(mtxmgr, *indxp);
	DB_ASSERT(env,
	    ((uintptr_t)mutexp & (dbenv->mutex_align - 1)) == 0);
	mtxregion->mutex_next = mutexp->mutex_next_link;

	--mtxregion->stat.st_mutex_free;
	++mtxregion->stat.st_mutex_inuse;
	if (mtxregion->stat.st_mutex_inuse > mtxregion->stat.st_mutex_inuse_max)
		mtxregion->stat.st_mutex_inuse_max =
		    mtxregion->stat.st_mutex_inuse;
	if (locksys)
		MUTEX_SYSTEM_UNLOCK(env);

	/* Initialize the mutex. */
	memset(mutexp, 0, sizeof(*mutexp));
	F_SET(mutexp, DB_MUTEX_ALLOCATED |
	    LF_ISSET(DB_MUTEX_LOGICAL_LOCK |
		DB_MUTEX_PROCESS_ONLY | DB_MUTEX_SHARED));

	/*
	 * If the mutex is associated with a single process, set the process
	 * ID.  If the application ever calls DbEnv::failchk, we'll need the
	 * process ID to know if the mutex is still in use.
	 */
	if (LF_ISSET(DB_MUTEX_PROCESS_ONLY))
		dbenv->thread_id(dbenv, &mutexp->pid, NULL);

#ifdef HAVE_STATISTICS
	mutexp->alloc_id = alloc_id;
#else
	COMPQUIET(alloc_id, 0);
#endif

	if ((ret = __mutex_init(env, *indxp, flags)) != 0)
		(void)__mutex_free_int(env, locksys, indxp);

	return (ret);
}

/*
 * __mutex_free --
 *	Free a mutex.
 *
 * PUBLIC: int __mutex_free __P((ENV *, db_mutex_t *));
 */
int
__mutex_free(env, indxp)
	ENV *env;
	db_mutex_t *indxp;
{
	/*
	 * There is no explicit ordering in how the regions are cleaned up
	 * up and/or discarded when an environment is destroyed (either a
	 * private environment is closed or a public environment is removed).
	 * The way we deal with mutexes is to clean up all remaining mutexes
	 * when we close the mutex environment (because we have to be able to
	 * do that anyway, after a crash), which means we don't have to deal
	 * with region cleanup ordering on normal environment destruction.
	 * All that said, what it really means is we can get here without a
	 * mpool region.  It's OK, the mutex has been, or will be, destroyed.
	 *
	 * If the mutex has never been configured, we're done.
	 */
	if (!MUTEX_ON(env) || *indxp == MUTEX_INVALID)
		return (0);

	return (__mutex_free_int(env, 1, indxp));
}

/*
 * __mutex_free_int --
 *	Internal routine to free a mutex.
 *
 * PUBLIC: int __mutex_free_int __P((ENV *, int, db_mutex_t *));
 */
int
__mutex_free_int(env, locksys, indxp)
	ENV *env;
	int locksys;
	db_mutex_t *indxp;
{
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	db_mutex_t mutex;
	int ret;

	mutex = *indxp;
	*indxp = MUTEX_INVALID;

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	DB_ASSERT(env, F_ISSET(mutexp, DB_MUTEX_ALLOCATED));
	F_CLR(mutexp, DB_MUTEX_ALLOCATED);

	ret = __mutex_destroy(env, mutex);

	if (locksys)
		MUTEX_SYSTEM_LOCK(env);

	/* Link the mutex on the head of the free list. */
	mutexp->mutex_next_link = mtxregion->mutex_next;
	mtxregion->mutex_next = mutex;
	++mtxregion->stat.st_mutex_free;
	--mtxregion->stat.st_mutex_inuse;

	if (locksys)
		MUTEX_SYSTEM_UNLOCK(env);

	return (ret);
}
