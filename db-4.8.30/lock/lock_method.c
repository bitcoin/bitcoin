/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/lock.h"

/*
 * __lock_env_create --
 *	Lock specific creation of the DB_ENV structure.
 *
 * PUBLIC: int __lock_env_create __P((DB_ENV *));
 */
int
__lock_env_create(dbenv)
	DB_ENV *dbenv;
{
	u_int32_t cpu;
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 */
	dbenv->lk_max = DB_LOCK_DEFAULT_N;
	dbenv->lk_max_lockers = DB_LOCK_DEFAULT_N;
	dbenv->lk_max_objects = DB_LOCK_DEFAULT_N;

	/*
	 * Default to 10 partitions per cpu.  This seems to be near
	 * the point of diminishing returns on Xeon type processors.
	 * Cpu count often returns the number of hyper threads and if
	 * there is only one CPU you probably do not want to run partitions.
	 */
	cpu = __os_cpu_count();
	dbenv->lk_partitions = cpu > 1 ? 10 * cpu : 1;

	return (0);
}

/*
 * __lock_env_destroy --
 *	Lock specific destruction of the DB_ENV structure.
 *
 * PUBLIC: void __lock_env_destroy __P((DB_ENV *));
 */
void
__lock_env_destroy(dbenv)
	DB_ENV *dbenv;
{
	ENV *env;

	env = dbenv->env;

	if (dbenv->lk_conflicts != NULL) {
		__os_free(env, dbenv->lk_conflicts);
		dbenv->lk_conflicts = NULL;
	}
}

/*
 * __lock_get_lk_conflicts
 *	Get the conflicts matrix.
 *
 * PUBLIC: int __lock_get_lk_conflicts
 * PUBLIC:     __P((DB_ENV *, const u_int8_t **, int *));
 */
int
__lock_get_lk_conflicts(dbenv, lk_conflictsp, lk_modesp)
	DB_ENV *dbenv;
	const u_int8_t **lk_conflictsp;
	int *lk_modesp;
{
	DB_LOCKTAB *lt;
	ENV *env;

	env = dbenv->env;
	lt = env->lk_handle;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_lk_conflicts", DB_INIT_LOCK);

	if (LOCKING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		if (lk_conflictsp != NULL)
			*lk_conflictsp = lt->conflicts;
		if (lk_modesp != NULL)
			*lk_modesp = ((DB_LOCKREGION *)
			    (lt->reginfo.primary))->nmodes;
	} else {
		if (lk_conflictsp != NULL)
			*lk_conflictsp = dbenv->lk_conflicts;
		if (lk_modesp != NULL)
			*lk_modesp = dbenv->lk_modes;
	}
	return (0);
}

/*
 * __lock_set_lk_conflicts
 *	Set the conflicts matrix.
 *
 * PUBLIC: int __lock_set_lk_conflicts __P((DB_ENV *, u_int8_t *, int));
 */
int
__lock_set_lk_conflicts(dbenv, lk_conflicts, lk_modes)
	DB_ENV *dbenv;
	u_int8_t *lk_conflicts;
	int lk_modes;
{
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lk_conflicts");

	if (dbenv->lk_conflicts != NULL) {
		__os_free(env, dbenv->lk_conflicts);
		dbenv->lk_conflicts = NULL;
	}
	if ((ret = __os_malloc(env,
	    (size_t)(lk_modes * lk_modes), &dbenv->lk_conflicts)) != 0)
		return (ret);
	memcpy(
	    dbenv->lk_conflicts, lk_conflicts, (size_t)(lk_modes * lk_modes));
	dbenv->lk_modes = lk_modes;

	return (0);
}

/*
 * PUBLIC: int __lock_get_lk_detect __P((DB_ENV *, u_int32_t *));
 */
int
__lock_get_lk_detect(dbenv, lk_detectp)
	DB_ENV *dbenv;
	u_int32_t *lk_detectp;
{
	DB_LOCKTAB *lt;
	DB_THREAD_INFO *ip;
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_lk_detect", DB_INIT_LOCK);

	if (LOCKING_ON(env)) {
		lt = env->lk_handle;
		ENV_ENTER(env, ip);
		LOCK_REGION_LOCK(env);
		*lk_detectp = ((DB_LOCKREGION *)lt->reginfo.primary)->detect;
		LOCK_REGION_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		*lk_detectp = dbenv->lk_detect;
	return (0);
}

/*
 * __lock_set_lk_detect
 *	DB_ENV->set_lk_detect.
 *
 * PUBLIC: int __lock_set_lk_detect __P((DB_ENV *, u_int32_t));
 */
int
__lock_set_lk_detect(dbenv, lk_detect)
	DB_ENV *dbenv;
	u_int32_t lk_detect;
{
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->set_lk_detect", DB_INIT_LOCK);

	switch (lk_detect) {
	case DB_LOCK_DEFAULT:
	case DB_LOCK_EXPIRE:
	case DB_LOCK_MAXLOCKS:
	case DB_LOCK_MAXWRITE:
	case DB_LOCK_MINLOCKS:
	case DB_LOCK_MINWRITE:
	case DB_LOCK_OLDEST:
	case DB_LOCK_RANDOM:
	case DB_LOCK_YOUNGEST:
		break;
	default:
		__db_errx(env,
	    "DB_ENV->set_lk_detect: unknown deadlock detection mode specified");
		return (EINVAL);
	}

	ret = 0;
	if (LOCKING_ON(env)) {
		ENV_ENTER(env, ip);

		lt = env->lk_handle;
		region = lt->reginfo.primary;
		LOCK_REGION_LOCK(env);
		/*
		 * Check for incompatible automatic deadlock detection requests.
		 * There are scenarios where changing the detector configuration
		 * is reasonable, but we disallow them guessing it is likely to
		 * be an application error.
		 *
		 * We allow applications to turn on the lock detector, and we
		 * ignore attempts to set it to the default or current value.
		 */
		if (region->detect != DB_LOCK_NORUN &&
		    lk_detect != DB_LOCK_DEFAULT &&
		    region->detect != lk_detect) {
			__db_errx(env,
	    "DB_ENV->set_lk_detect: incompatible deadlock detector mode");
			ret = EINVAL;
		} else
			if (region->detect == DB_LOCK_NORUN)
				region->detect = lk_detect;
		LOCK_REGION_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		dbenv->lk_detect = lk_detect;

	return (ret);
}

/*
 * PUBLIC: int __lock_get_lk_max_locks __P((DB_ENV *, u_int32_t *));
 */
int
__lock_get_lk_max_locks(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_lk_maxlocks", DB_INIT_LOCK);

	if (LOCKING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*lk_maxp = ((DB_LOCKREGION *)
		    env->lk_handle->reginfo.primary)->stat.st_maxlocks;
	} else
		*lk_maxp = dbenv->lk_max;
	return (0);
}

/*
 * __lock_set_lk_max_locks
 *	DB_ENV->set_lk_max_locks.
 *
 * PUBLIC: int __lock_set_lk_max_locks __P((DB_ENV *, u_int32_t));
 */
int
__lock_set_lk_max_locks(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lk_max_locks");

	dbenv->lk_max = lk_max;
	return (0);
}

/*
 * PUBLIC: int __lock_get_lk_max_lockers __P((DB_ENV *, u_int32_t *));
 */
int
__lock_get_lk_max_lockers(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_lk_max_lockers", DB_INIT_LOCK);

	if (LOCKING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*lk_maxp = ((DB_LOCKREGION *)
		    env->lk_handle->reginfo.primary)->stat.st_maxlockers;
	} else
		*lk_maxp = dbenv->lk_max_lockers;
	return (0);
}

/*
 * __lock_set_lk_max_lockers
 *	DB_ENV->set_lk_max_lockers.
 *
 * PUBLIC: int __lock_set_lk_max_lockers __P((DB_ENV *, u_int32_t));
 */
int
__lock_set_lk_max_lockers(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lk_max_lockers");

	dbenv->lk_max_lockers = lk_max;
	return (0);
}

/*
 * PUBLIC: int __lock_get_lk_max_objects __P((DB_ENV *, u_int32_t *));
 */
int
__lock_get_lk_max_objects(dbenv, lk_maxp)
	DB_ENV *dbenv;
	u_int32_t *lk_maxp;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_lk_max_objects", DB_INIT_LOCK);

	if (LOCKING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*lk_maxp = ((DB_LOCKREGION *)
		    env->lk_handle->reginfo.primary)->stat.st_maxobjects;
	} else
		*lk_maxp = dbenv->lk_max_objects;
	return (0);
}

/*
 * __lock_set_lk_max_objects
 *	DB_ENV->set_lk_max_objects.
 *
 * PUBLIC: int __lock_set_lk_max_objects __P((DB_ENV *, u_int32_t));
 */
int
__lock_set_lk_max_objects(dbenv, lk_max)
	DB_ENV *dbenv;
	u_int32_t lk_max;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lk_max_objects");

	dbenv->lk_max_objects = lk_max;
	return (0);
}
/*
 * PUBLIC: int __lock_get_lk_partitions __P((DB_ENV *, u_int32_t *));
 */
int
__lock_get_lk_partitions(dbenv, lk_partitionp)
	DB_ENV *dbenv;
	u_int32_t *lk_partitionp;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_lk_partitions", DB_INIT_LOCK);

	if (LOCKING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*lk_partitionp = ((DB_LOCKREGION *)
		    env->lk_handle->reginfo.primary)->stat.st_partitions;
	} else
		*lk_partitionp = dbenv->lk_partitions;
	return (0);
}

/*
 * __lock_set_lk_partitions
 *	DB_ENV->set_lk_partitions.
 *
 * PUBLIC: int __lock_set_lk_partitions __P((DB_ENV *, u_int32_t));
 */
int
__lock_set_lk_partitions(dbenv, lk_partitions)
	DB_ENV *dbenv;
	u_int32_t lk_partitions;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lk_partitions");

	dbenv->lk_partitions = lk_partitions;
	return (0);
}

/*
 * PUBLIC: int __lock_get_env_timeout
 * PUBLIC:     __P((DB_ENV *, db_timeout_t *, u_int32_t));
 */
int
__lock_get_env_timeout(dbenv, timeoutp, flag)
	DB_ENV *dbenv;
	db_timeout_t *timeoutp;
	u_int32_t flag;
{
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->get_env_timeout", DB_INIT_LOCK);

	ret = 0;
	if (LOCKING_ON(env)) {
		lt = env->lk_handle;
		region = lt->reginfo.primary;
		ENV_ENTER(env, ip);
		LOCK_REGION_LOCK(env);
		switch (flag) {
		case DB_SET_LOCK_TIMEOUT:
			*timeoutp = region->lk_timeout;
			break;
		case DB_SET_TXN_TIMEOUT:
			*timeoutp = region->tx_timeout;
			break;
		default:
			ret = 1;
			break;
		}
		LOCK_REGION_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		switch (flag) {
		case DB_SET_LOCK_TIMEOUT:
			*timeoutp = dbenv->lk_timeout;
			break;
		case DB_SET_TXN_TIMEOUT:
			*timeoutp = dbenv->tx_timeout;
			break;
		default:
			ret = 1;
			break;
		}

	if (ret)
		ret = __db_ferr(env, "DB_ENV->get_timeout", 0);

	return (ret);
}

/*
 * __lock_set_env_timeout
 *	DB_ENV->set_lock_timeout.
 *
 * PUBLIC: int __lock_set_env_timeout __P((DB_ENV *, db_timeout_t, u_int32_t));
 */
int
__lock_set_env_timeout(dbenv, timeout, flags)
	DB_ENV *dbenv;
	db_timeout_t timeout;
	u_int32_t flags;
{
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lk_handle, "DB_ENV->set_env_timeout", DB_INIT_LOCK);

	ret = 0;
	if (LOCKING_ON(env)) {
		lt = env->lk_handle;
		region = lt->reginfo.primary;
		ENV_ENTER(env, ip);
		LOCK_REGION_LOCK(env);
		switch (flags) {
		case DB_SET_LOCK_TIMEOUT:
			region->lk_timeout = timeout;
			break;
		case DB_SET_TXN_TIMEOUT:
			region->tx_timeout = timeout;
			break;
		default:
			ret = 1;
			break;
		}
		LOCK_REGION_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		switch (flags) {
		case DB_SET_LOCK_TIMEOUT:
			dbenv->lk_timeout = timeout;
			break;
		case DB_SET_TXN_TIMEOUT:
			dbenv->tx_timeout = timeout;
			break;
		default:
			ret = 1;
			break;
		}

	if (ret)
		ret = __db_ferr(env, "DB_ENV->set_timeout", 0);

	return (ret);
}
