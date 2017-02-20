/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static inline int __db_fcntl_mutex_lock_int __P((ENV *, db_mutex_t, int));

/*
 * __db_fcntl_mutex_init --
 *	Initialize a fcntl mutex.
 *
 * PUBLIC: int __db_fcntl_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
 */
int
__db_fcntl_mutex_init(env, mutex, flags)
	ENV *env;
	db_mutex_t mutex;
	u_int32_t flags;
{
	COMPQUIET(env, NULL);
	COMPQUIET(mutex, MUTEX_INVALID);
	COMPQUIET(flags, 0);

	return (0);
}

/*
 * __db_fcntl_mutex_lock_int
 *	Internal function to lock a mutex, blocking only when requested
 */
inline int
__db_fcntl_mutex_lock_int(env, mutex, wait)
	ENV *env;
	db_mutex_t mutex;
	int wait;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_THREAD_INFO *ip;
	struct flock k_lock;
	int locked, ms, ret;

	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	CHECK_MTX_THREAD(env, mutexp);

#ifdef HAVE_STATISTICS
	if (F_ISSET(mutexp, DB_MUTEX_LOCKED))
		++mutexp->mutex_set_wait;
	else
		++mutexp->mutex_set_nowait;
#endif

	/* Initialize the lock. */
	k_lock.l_whence = SEEK_SET;
	k_lock.l_start = mutex;
	k_lock.l_len = 1;

	/*
	 * Only check the thread state once, by initializing the thread
	 * control block pointer to null.  If it is not the failchk
	 * thread, then ip will have a valid value subsequent times
	 * in the loop.
	 */
	ip = NULL;

	for (locked = 0;;) {
		/*
		 * Wait for the lock to become available; wait 1ms initially,
		 * up to 1 second.
		 */
		for (ms = 1; F_ISSET(mutexp, DB_MUTEX_LOCKED);) {
			if (F_ISSET(dbenv, DB_ENV_FAILCHK) &&
			    ip == NULL && dbenv->is_alive(dbenv,
			    mutexp->pid, mutexp->tid, 0) == 0) {
				ret = __env_set_state(env, &ip, THREAD_VERIFY);
				if (ret != 0 ||
				    ip->dbth_state == THREAD_FAILCHK)
					return (DB_RUNRECOVERY);
			}
			if (!wait)
				return (DB_LOCK_NOTGRANTED);
			__os_yield(NULL, 0, ms * US_PER_MS);
			if ((ms <<= 1) > MS_PER_SEC)
				ms = MS_PER_SEC;
		}

		/* Acquire an exclusive kernel lock on the byte. */
		k_lock.l_type = F_WRLCK;
		if (fcntl(env->lockfhp->fd, F_SETLKW, &k_lock))
			goto err;

		/* If the resource is still available, it's ours. */
		if (!F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			locked = 1;

			F_SET(mutexp, DB_MUTEX_LOCKED);
			dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
		}

		/* Release the kernel lock. */
		k_lock.l_type = F_UNLCK;
		if (fcntl(env->lockfhp->fd, F_SETLK, &k_lock))
			goto err;

		/*
		 * If we got the resource lock we're done.
		 *
		 * !!!
		 * We can't check to see if the lock is ours, because we may
		 * be trying to block ourselves in the lock manager, and so
		 * the holder of the lock that's preventing us from getting
		 * the lock may be us!  (Seriously.)
		 */
		if (locked)
			break;
	}

#ifdef DIAGNOSTIC
	/*
	 * We want to switch threads as often as possible.  Yield every time
	 * we get a mutex to ensure contention.
	 */
	if (F_ISSET(dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
#endif
	return (0);

err:	ret = __os_get_syserr();
	__db_syserr(env, ret, "fcntl lock failed");
	return (__env_panic(env, __os_posix_err(ret)));
}

/*
 * __db_fcntl_mutex_lock
 *	Lock a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_fcntl_mutex_lock __P((ENV *, db_mutex_t));
 */
int
__db_fcntl_mutex_lock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_fcntl_mutex_lock_int(env, mutex, 1));
}

/*
 * __db_fcntl_mutex_trylock
 *	Try to lock a mutex, without blocking when it is busy.
 *
 * PUBLIC: int __db_fcntl_mutex_trylock __P((ENV *, db_mutex_t));
 */
int
__db_fcntl_mutex_trylock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_fcntl_mutex_lock_int(env, mutex, 0));
}

/*
 * __db_fcntl_mutex_unlock --
 *	Release a mutex.
 *
 * PUBLIC: int __db_fcntl_mutex_unlock __P((ENV *, db_mutex_t));
 */
int
__db_fcntl_mutex_unlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;

	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

#ifdef DIAGNOSTIC
	if (!F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
		__db_errx(env, "fcntl unlock failed: lock already unlocked");
		return (__env_panic(env, EACCES));
	}
#endif

	/*
	 * Release the resource.  We don't have to acquire any locks because
	 * processes trying to acquire the lock are waiting for the flag to
	 * go to 0.  Once that happens the waiters will serialize acquiring
	 * an exclusive kernel lock before locking the mutex.
	 */
	F_CLR(mutexp, DB_MUTEX_LOCKED);

	return (0);
}

/*
 * __db_fcntl_mutex_destroy --
 *	Destroy a mutex.
 *
 * PUBLIC: int __db_fcntl_mutex_destroy __P((ENV *, db_mutex_t));
 */
int
__db_fcntl_mutex_destroy(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	COMPQUIET(env, NULL);
	COMPQUIET(mutex, MUTEX_INVALID);

	return (0);
}
