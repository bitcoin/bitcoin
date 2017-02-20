/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __mutex_alloc_pp --
 *	Allocate a mutex, application method.
 *
 * PUBLIC: int __mutex_alloc_pp __P((DB_ENV *, u_int32_t, db_mutex_t *));
 */
int
__mutex_alloc_pp(dbenv, flags, indxp)
	DB_ENV *dbenv;
	u_int32_t flags;
	db_mutex_t *indxp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	if ((ret = __db_fchk(env, "DB_ENV->mutex_alloc",
	    flags, DB_MUTEX_PROCESS_ONLY | DB_MUTEX_SELF_BLOCK)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	ret = __mutex_alloc(env, MTX_APPLICATION, flags, indxp);
	ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __mutex_free_pp --
 *	Destroy a mutex, application method.
 *
 * PUBLIC: int __mutex_free_pp __P((DB_ENV *, db_mutex_t));
 */
int
__mutex_free_pp(dbenv, indx)
	DB_ENV *dbenv;
	db_mutex_t indx;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	if (indx == MUTEX_INVALID)
		return (EINVAL);

	/*
	 * Internally Berkeley DB passes around the db_mutex_t address on
	 * free, because we want to make absolutely sure the slot gets
	 * overwritten with MUTEX_INVALID.  We don't export MUTEX_INVALID,
	 * so we don't export that part of the API, either.
	 */
	ENV_ENTER(env, ip);
	ret = __mutex_free(env, &indx);
	ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __mutex_lock --
 *	Lock a mutex, application method.
 *
 * PUBLIC: int __mutex_lock_pp __P((DB_ENV *, db_mutex_t));
 */
int
__mutex_lock_pp(dbenv, indx)
	DB_ENV *dbenv;
	db_mutex_t indx;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	if (indx == MUTEX_INVALID)
		return (EINVAL);

	ENV_ENTER(env, ip);
	ret = __mutex_lock(env, indx);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __mutex_unlock --
 *	Unlock a mutex, application method.
 *
 * PUBLIC: int __mutex_unlock_pp __P((DB_ENV *, db_mutex_t));
 */
int
__mutex_unlock_pp(dbenv, indx)
	DB_ENV *dbenv;
	db_mutex_t indx;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	if (indx == MUTEX_INVALID)
		return (EINVAL);

	ENV_ENTER(env, ip);
	ret = __mutex_unlock(env, indx);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __mutex_get_align --
 *	DB_ENV->mutex_get_align.
 *
 * PUBLIC: int __mutex_get_align __P((DB_ENV *, u_int32_t *));
 */
int
__mutex_get_align(dbenv, alignp)
	DB_ENV *dbenv;
	u_int32_t *alignp;
{
	ENV *env;

	env = dbenv->env;

	if (MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*alignp = ((DB_MUTEXREGION *)
		    env->mutex_handle->reginfo.primary)->stat.st_mutex_align;
	} else
		*alignp = dbenv->mutex_align;
	return (0);
}

/*
 * __mutex_set_align --
 *	DB_ENV->mutex_set_align.
 *
 * PUBLIC: int __mutex_set_align __P((DB_ENV *, u_int32_t));
 */
int
__mutex_set_align(dbenv, align)
	DB_ENV *dbenv;
	u_int32_t align;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_align");

	if (align == 0 || !POWER_OF_TWO(align)) {
		__db_errx(env,
    "DB_ENV->mutex_set_align: alignment value must be a non-zero power-of-two");
		return (EINVAL);
	}

	dbenv->mutex_align = align;
	return (0);
}

/*
 * __mutex_get_increment --
 *	DB_ENV->mutex_get_increment.
 *
 * PUBLIC: int __mutex_get_increment __P((DB_ENV *, u_int32_t *));
 */
int
__mutex_get_increment(dbenv, incrementp)
	DB_ENV *dbenv;
	u_int32_t *incrementp;
{
	/*
	 * We don't maintain the increment in the region (it just makes
	 * no sense).  Return whatever we have configured on this handle,
	 * nobody is ever going to notice.
	 */
	*incrementp = dbenv->mutex_inc;
	return (0);
}

/*
 * __mutex_set_increment --
 *	DB_ENV->mutex_set_increment.
 *
 * PUBLIC: int __mutex_set_increment __P((DB_ENV *, u_int32_t));
 */
int
__mutex_set_increment(dbenv, increment)
	DB_ENV *dbenv;
	u_int32_t increment;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_increment");

	dbenv->mutex_cnt = 0;
	dbenv->mutex_inc = increment;
	return (0);
}

/*
 * __mutex_get_max --
 *	DB_ENV->mutex_get_max.
 *
 * PUBLIC: int __mutex_get_max __P((DB_ENV *, u_int32_t *));
 */
int
__mutex_get_max(dbenv, maxp)
	DB_ENV *dbenv;
	u_int32_t *maxp;
{
	ENV *env;

	env = dbenv->env;

	if (MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*maxp = ((DB_MUTEXREGION *)
		    env->mutex_handle->reginfo.primary)->stat.st_mutex_cnt;
	} else
		*maxp = dbenv->mutex_cnt;
	return (0);
}

/*
 * __mutex_set_max --
 *	DB_ENV->mutex_set_max.
 *
 * PUBLIC: int __mutex_set_max __P((DB_ENV *, u_int32_t));
 */
int
__mutex_set_max(dbenv, max)
	DB_ENV *dbenv;
	u_int32_t max;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mutex_max");

	dbenv->mutex_cnt = max;
	dbenv->mutex_inc = 0;
	return (0);
}

/*
 * __mutex_get_tas_spins --
 *	DB_ENV->mutex_get_tas_spins.
 *
 * PUBLIC: int __mutex_get_tas_spins __P((DB_ENV *, u_int32_t *));
 */
int
__mutex_get_tas_spins(dbenv, tas_spinsp)
	DB_ENV *dbenv;
	u_int32_t *tas_spinsp;
{
	ENV *env;

	env = dbenv->env;

	if (MUTEX_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*tas_spinsp = ((DB_MUTEXREGION *)env->
		    mutex_handle->reginfo.primary)->stat.st_mutex_tas_spins;
	} else
		*tas_spinsp = dbenv->mutex_tas_spins;
	return (0);
}

/*
 * __mutex_set_tas_spins --
 *	DB_ENV->mutex_set_tas_spins.
 *
 * PUBLIC: int __mutex_set_tas_spins __P((DB_ENV *, u_int32_t));
 */
int
__mutex_set_tas_spins(dbenv, tas_spins)
	DB_ENV *dbenv;
	u_int32_t tas_spins;
{
	ENV *env;

	env = dbenv->env;

	/*
	 * Bound the value -- less than 1 makes no sense, greater than 1M
	 * makes no sense.
	 */
	if (tas_spins == 0)
		tas_spins = 1;
	else if (tas_spins > 1000000)
		tas_spins = 1000000;

	/*
	 * There's a theoretical race here, but I'm not interested in locking
	 * the test-and-set spin count.  The worst possibility is a thread
	 * reads out a bad spin count and spins until it gets the lock, but
	 * that's awfully unlikely.
	 */
	if (MUTEX_ON(env))
		((DB_MUTEXREGION *)env->mutex_handle
		    ->reginfo.primary)->stat.st_mutex_tas_spins = tas_spins;
	else
		dbenv->mutex_tas_spins = tas_spins;
	return (0);
}

#if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
/*
 * Provide atomic operations for platforms which have mutexes yet do not have
 * native atomic operations configured. They are emulated by protected the
 * operation with a mutex.  The address of the atomic value selects which
 * mutex to use.
 */
/*
 * atomic_get_mutex -
 *	Map an address to the mutex to use to atomically modify it
 */
static inline db_mutex_t atomic_get_mutex(env, v)
	ENV *env;
	db_atomic_t *v;
{
	u_int	index;
	DB_MUTEXREGION *mtxreg;

	if (!MUTEX_ON(env))
		return (MUTEX_INVALID);
	index = (u_int)(((uintptr_t) (v)) >> 6) % MAX_ATOMIC_MUTEXES;
	mtxreg = (DB_MUTEXREGION *)env->mutex_handle->reginfo.primary;
	return (mtxreg->mtx_atomic[index]);
}

/*
 * __atomic_inc
 *	Use a mutex to provide an atomic increment function
 *
 * PUBLIC: #if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
 * PUBLIC: atomic_value_t __atomic_inc __P((ENV *, db_atomic_t *));
 * PUBLIC: #endif
 */
atomic_value_t
__atomic_inc(env, v)
	ENV *env;
	db_atomic_t *v;
{
	db_mutex_t mtx;
	int ret;

	mtx = atomic_get_mutex(env, v);
	MUTEX_LOCK(env, mtx);
	ret = ++v->value;
	MUTEX_UNLOCK(env, mtx);

	return (ret);
}

/*
 * __atomic_dec
 *	Use a mutex to provide an atomic decrement function
 *
 * PUBLIC: #if !defined(HAVE_ATOMIC_SUPPORT) && defined(HAVE_MUTEX_SUPPORT)
 * PUBLIC: atomic_value_t __atomic_dec __P((ENV *, db_atomic_t *));
 * PUBLIC: #endif
 */
atomic_value_t
__atomic_dec(env, v)
	ENV *env;
	db_atomic_t *v;
{
	db_mutex_t mtx;
	int ret;

	mtx = atomic_get_mutex(env, v);
	MUTEX_LOCK(env, mtx);
	ret = --v->value;
	MUTEX_UNLOCK(env, mtx);

	return (ret);
}

/*
 * atomic_compare_exchange
 *	Use a mutex to provide an atomic decrement function
 *
 * PRIVATE: int atomic_compare_exchange
 * PRIVATE:     __P((ENV *, db_atomic_t *, atomic_value_t, atomic_value_t));
 *	Returns 1 if the *v was equal to oldval, else 0
 *
 *	Side Effect:
 *		Sets the value to newval if and only if returning 1
 */
int
atomic_compare_exchange(env, v, oldval, newval)
	ENV *env;
	db_atomic_t *v;
	atomic_value_t oldval;
	atomic_value_t newval;
{
	db_mutex_t mtx;
	int ret;

	if (atomic_read(v) != oldval)
		return (0);

	mtx = atomic_get_mutex(env, v);
	MUTEX_LOCK(env, mtx);
	ret = atomic_read(v) == oldval;
	if (ret)
		atomic_init(v, newval);
	MUTEX_UNLOCK(env, mtx);

	return (ret);
}
#endif
