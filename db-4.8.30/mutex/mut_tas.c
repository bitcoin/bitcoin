/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"

static inline int __db_tas_mutex_lock_int __P((ENV *, db_mutex_t, int));
static inline int __db_tas_mutex_readlock_int __P((ENV *, db_mutex_t, int));

/*
 * __db_tas_mutex_init --
 *	Initialize a test-and-set mutex.
 *
 * PUBLIC: int __db_tas_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
 */
int
__db_tas_mutex_init(env, mutex, flags)
	ENV *env;
	db_mutex_t mutex;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	int ret;

#ifndef HAVE_MUTEX_HYBRID
	COMPQUIET(flags, 0);
#endif

	dbenv = env->dbenv;
	mtxmgr = env->mutex_handle;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	/* Check alignment. */
	if (((uintptr_t)mutexp & (dbenv->mutex_align - 1)) != 0) {
		__db_errx(env, "TAS: mutex not appropriately aligned");
		return (EINVAL);
	}

#ifdef HAVE_SHARED_LATCHES
	if (F_ISSET(mutexp, DB_MUTEX_SHARED))
		atomic_init(&mutexp->sharecount, 0);
	else
#endif
	if (MUTEX_INIT(&mutexp->tas)) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "TAS: mutex initialize");
		return (__os_posix_err(ret));
	}
#ifdef HAVE_MUTEX_HYBRID
	if ((ret = __db_pthread_mutex_init(env,
	     mutex, flags | DB_MUTEX_SELF_BLOCK)) != 0)
		return (ret);
#endif
	return (0);
}

/*
 * __db_tas_mutex_lock_int
 *     Internal function to lock a mutex, or just try to lock it without waiting
 */
static inline int
__db_tas_mutex_lock_int(env, mutex, nowait)
	ENV *env;
	db_mutex_t mutex;
	int nowait;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	DB_THREAD_INFO *ip;
	u_int32_t nspins;
	int ret;
#ifndef HAVE_MUTEX_HYBRID
	u_long ms, max_ms;
#endif

	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	CHECK_MTX_THREAD(env, mutexp);

#ifdef HAVE_STATISTICS
	if (F_ISSET(mutexp, DB_MUTEX_LOCKED))
		++mutexp->mutex_set_wait;
	else
		++mutexp->mutex_set_nowait;
#endif

#ifndef HAVE_MUTEX_HYBRID
	/*
	 * Wait 1ms initially, up to 10ms for mutexes backing logical database
	 * locks, and up to 25 ms for mutual exclusion data structure mutexes.
	 * SR: #7675
	 */
	ms = 1;
	max_ms = F_ISSET(mutexp, DB_MUTEX_LOGICAL_LOCK) ? 10 : 25;
#endif

	 /*
	 * Only check the thread state once, by initializing the thread
	 * control block pointer to null.  If it is not the failchk
	 * thread, then ip will have a valid value subsequent times
	 * in the loop.
	 */
	ip = NULL;

loop:	/* Attempt to acquire the resource for N spins. */
	for (nspins =
	    mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
#ifdef HAVE_MUTEX_S390_CC_ASSEMBLY
		tsl_t zero;

		zero = 0;
#endif

		dbenv = env->dbenv;

#ifdef HAVE_MUTEX_HPPA_MSEM_INIT
	relock:
#endif
		/*
		 * Avoid interlocked instructions until they're likely to
		 * succeed by first checking whether it is held
		 */
		if (MUTEXP_IS_BUSY(mutexp) || !MUTEXP_ACQUIRE(mutexp)) {
			if (F_ISSET(dbenv, DB_ENV_FAILCHK) &&
			    ip == NULL && dbenv->is_alive(dbenv,
			    mutexp->pid, mutexp->tid, 0) == 0) {
				ret = __env_set_state(env, &ip, THREAD_VERIFY);
				if (ret != 0 ||
				    ip->dbth_state == THREAD_FAILCHK)
					return (DB_RUNRECOVERY);
			}
			if (nowait)
				return (DB_LOCK_NOTGRANTED);
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
			MUTEX_PAUSE
			continue;
		}

		MEMBAR_ENTER();

#ifdef HAVE_MUTEX_HPPA_MSEM_INIT
		/*
		 * HP semaphores are unlocked automatically when a holding
		 * process exits.  If the mutex appears to be locked
		 * (F_ISSET(DB_MUTEX_LOCKED)) but we got here, assume this
		 * has happened.  Set the pid and tid into the mutex and
		 * lock again.  (The default state of the mutexes used to
		 * block in __lock_get_internal is locked, so exiting with
		 * a locked mutex is reasonable behavior for a process that
		 * happened to initialize or use one of them.)
		 */
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);
			goto relock;
		}
		/*
		 * If we make it here, the mutex isn't locked, the diagnostic
		 * won't fire, and we were really unlocked by someone calling
		 * the DB mutex unlock function.
		 */
#endif
#ifdef DIAGNOSTIC
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			char buf[DB_THREADID_STRLEN];
			__db_errx(env,
			    "TAS lock failed: lock %d currently in use: ID: %s",
			    mutex, dbenv->thread_id_string(dbenv,
			    mutexp->pid, mutexp->tid, buf));
			return (__env_panic(env, EACCES));
		}
#endif
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);

#ifdef DIAGNOSTIC
		/*
		 * We want to switch threads as often as possible.  Yield
		 * every time we get a mutex to ensure contention.
		 */
		if (F_ISSET(dbenv, DB_ENV_YIELDCPU))
			__os_yield(env, 0, 0);
#endif
		return (0);
	}

	/* Wait for the lock to become available. */
#ifdef HAVE_MUTEX_HYBRID
	/*
	 * By yielding here we can get the other thread to give up the
	 * mutex before calling the more expensive library mutex call.
	 * Tests have shown this to be a big win when there is contention.
	 * With shared latches check the locked bit only after checking
	 * that no one has the latch in shared mode.
	 */
	__os_yield(env, 0, 0);
	if (!MUTEXP_IS_BUSY(mutexp))
		goto loop;
	if ((ret = __db_pthread_mutex_lock(env, mutex)) != 0)
		return (ret);
#else
	__os_yield(env, 0, ms * US_PER_MS);
	if ((ms <<= 1) > max_ms)
		ms = max_ms;
#endif

	/*
	 * We're spinning.  The environment might be hung, and somebody else
	 * has already recovered it.  The first thing recovery does is panic
	 * the environment.  Check to see if we're never going to get this
	 * mutex.
	 */
	PANIC_CHECK(env);

	goto loop;
}

/*
 * __db_tas_mutex_lock
 *	Lock on a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_tas_mutex_lock __P((ENV *, db_mutex_t));
 */
int
__db_tas_mutex_lock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_tas_mutex_lock_int(env, mutex, 0));
}

/*
 * __db_tas_mutex_trylock
 *	Try to exclusively lock a mutex without ever blocking - ever!
 *
 *	Returns 0 on success,
 *		DB_LOCK_NOTGRANTED on timeout
 *		Possibly DB_RUNRECOVERY if DB_ENV_FAILCHK or panic.
 *
 *	This will work for DB_MUTEX_SHARED, though it always tries
 *	for exclusive access.
 *
 * PUBLIC: int __db_tas_mutex_trylock __P((ENV *, db_mutex_t));
 */
int
__db_tas_mutex_trylock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_tas_mutex_lock_int(env, mutex, 1));
}

#if defined(HAVE_SHARED_LATCHES)
/*
 * __db_tas_mutex_readlock_int
 *    Internal function to get a shared lock on a latch, blocking if necessary.
 *
 */
static inline int
__db_tas_mutex_readlock_int(env, mutex, nowait)
	ENV *env;
	db_mutex_t mutex;
	int nowait;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	DB_THREAD_INFO *ip;
	int lock;
	u_int32_t nspins;
	int ret;
#ifndef HAVE_MUTEX_HYBRID
	u_long ms, max_ms;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	CHECK_MTX_THREAD(env, mutexp);

	DB_ASSERT(env, F_ISSET(mutexp, DB_MUTEX_SHARED));
#ifdef HAVE_STATISTICS
	if (F_ISSET(mutexp, DB_MUTEX_LOCKED))
		++mutexp->mutex_set_rd_wait;
	else
		++mutexp->mutex_set_rd_nowait;
#endif

#ifndef HAVE_MUTEX_HYBRID
	/*
	 * Wait 1ms initially, up to 10ms for mutexes backing logical database
	 * locks, and up to 25 ms for mutual exclusion data structure mutexes.
	 * SR: #7675
	 */
	ms = 1;
	max_ms = F_ISSET(mutexp, DB_MUTEX_LOGICAL_LOCK) ? 10 : 25;
#endif
	/*
	 * Only check the thread state once, by initializing the thread
	 * control block pointer to null.  If it is not the failchk
	 * thread, then ip will have a valid value subsequent times
	 * in the loop.
	 */
	ip = NULL;

loop:	/* Attempt to acquire the resource for N spins. */
	for (nspins =
	    mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		lock = atomic_read(&mutexp->sharecount);
		if (lock == MUTEX_SHARE_ISEXCLUSIVE ||
		    !atomic_compare_exchange(env,
			&mutexp->sharecount, lock, lock + 1)) {
			if (F_ISSET(dbenv, DB_ENV_FAILCHK) &&
			    ip == NULL && dbenv->is_alive(dbenv,
			    mutexp->pid, mutexp->tid, 0) == 0) {
				ret = __env_set_state(env, &ip, THREAD_VERIFY);
				if (ret != 0 ||
				    ip->dbth_state == THREAD_FAILCHK)
					return (DB_RUNRECOVERY);
			}
			if (nowait)
				return (DB_LOCK_NOTGRANTED);
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
			MUTEX_PAUSE
			continue;
		}

		MEMBAR_ENTER();
		/* For shared lactches the threadid is the last requestor's id.
		 */
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);

		return (0);
	}

	/* Wait for the lock to become available. */
#ifdef HAVE_MUTEX_HYBRID
	/*
	 * By yielding here we can get the other thread to give up the
	 * mutex before calling the more expensive library mutex call.
	 * Tests have shown this to be a big win when there is contention.
	 */
	__os_yield(env, 0, 0);
	if (atomic_read(&mutexp->sharecount) != MUTEX_SHARE_ISEXCLUSIVE)
		goto loop;
	if ((ret = __db_pthread_mutex_lock(env, mutex)) != 0)
		return (ret);
#else
	__os_yield(env, 0, ms * US_PER_MS);
	if ((ms <<= 1) > max_ms)
		ms = max_ms;
#endif

	/*
	 * We're spinning.  The environment might be hung, and somebody else
	 * has already recovered it.  The first thing recovery does is panic
	 * the environment.  Check to see if we're never going to get this
	 * mutex.
	 */
	PANIC_CHECK(env);

	goto loop;
}

/*
 * __db_tas_mutex_readlock
 *	Get a shared lock on a latch, waiting if necessary.
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_tas_mutex_readlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int
__db_tas_mutex_readlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_tas_mutex_readlock_int(env, mutex, 0));
}

/*
 * __db_tas_mutex_tryreadlock
 *	Try to get a shared lock on a latch; don't wait when busy.
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_tas_mutex_tryreadlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int
__db_tas_mutex_tryreadlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_tas_mutex_readlock_int(env, mutex, 1));
}
#endif

/*
 * __db_tas_mutex_unlock --
 *	Release a mutex.
 *
 * PUBLIC: int __db_tas_mutex_unlock __P((ENV *, db_mutex_t));
 *
 * Hybrid shared latch wakeup
 *	When an exclusive requester waits for the last shared holder to
 *	release, it increments mutexp->wait and pthread_cond_wait()'s. The
 *	last shared unlock calls __db_pthread_mutex_unlock() to wake it.
 */
int
__db_tas_mutex_unlock(env, mutex)
    ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
#ifdef HAVE_MUTEX_HYBRID
	int ret;
#ifdef MUTEX_DIAG
	int waiters;
#endif
#endif
#ifdef HAVE_SHARED_LATCHES
	int sharecount;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mutexp = MUTEXP_SET(mtxmgr, mutex);
#if defined(HAVE_MUTEX_HYBRID) && defined(MUTEX_DIAG)
	waiters = mutexp->wait;
#endif

#if defined(DIAGNOSTIC)
#if defined(HAVE_SHARED_LATCHES)
	if (F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		if (atomic_read(&mutexp->sharecount) == 0) {
			__db_errx(env, "shared unlock %d already unlocked",
			    mutex);
			return (__env_panic(env, EACCES));
		}
	} else
#endif
	if (!F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
		__db_errx(env, "unlock %d already unlocked", mutex);
		return (__env_panic(env, EACCES));
	}
#endif

#ifdef HAVE_SHARED_LATCHES
	if (F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		sharecount = atomic_read(&mutexp->sharecount);
		/*MUTEX_MEMBAR(mutexp->sharecount);*/		/* XXX why? */
		if (sharecount == MUTEX_SHARE_ISEXCLUSIVE) {
			F_CLR(mutexp, DB_MUTEX_LOCKED);
			/* Flush flag update before zeroing count */
			MEMBAR_EXIT();
			atomic_init(&mutexp->sharecount, 0);
		} else {
			DB_ASSERT(env, sharecount > 0);
			MEMBAR_EXIT();
			sharecount = atomic_dec(env, &mutexp->sharecount);
			DB_ASSERT(env, sharecount >= 0);
			if (sharecount > 0)
				return (0);
		}
	} else
#endif
	{
		F_CLR(mutexp, DB_MUTEX_LOCKED);
		MUTEX_UNSET(&mutexp->tas);
	}

#ifdef HAVE_MUTEX_HYBRID
#ifdef DIAGNOSTIC
	if (F_ISSET(dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
#endif

	/* Prevent the load of wait from being hoisted before MUTEX_UNSET */
	MUTEX_MEMBAR(mutexp->flags);
	if (mutexp->wait &&
	    (ret = __db_pthread_mutex_unlock(env, mutex)) != 0)
		    return (ret);

#ifdef MUTEX_DIAG
	if (mutexp->wait)
		printf("tas_unlock %d %x waiters! busy %x waiters %d/%d\n",
		    mutex, pthread_self(),
		    MUTEXP_BUSY_FIELD(mutexp), waiters, mutexp->wait);
#endif
#endif

	return (0);
}

/*
 * __db_tas_mutex_destroy --
 *	Destroy a mutex.
 *
 * PUBLIC: int __db_tas_mutex_destroy __P((ENV *, db_mutex_t));
 */
int
__db_tas_mutex_destroy(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
#ifdef HAVE_MUTEX_HYBRID
	int ret;
#endif

	if (!MUTEX_ON(env))
		return (0);

	mtxmgr = env->mutex_handle;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	MUTEX_DESTROY(&mutexp->tas);

#ifdef HAVE_MUTEX_HYBRID
	if ((ret = __db_pthread_mutex_destroy(env, mutex)) != 0)
		return (ret);
#endif

	COMPQUIET(mutexp, NULL);	/* MUTEX_DESTROY may not be defined. */
	return (0);
}
