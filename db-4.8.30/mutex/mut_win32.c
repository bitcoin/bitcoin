/*
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#define	LOAD_ACTUAL_MUTEX_CODE
#include "db_int.h"

#include "dbinc/atomic.h"
/*
 * This is where we load in the actual test-and-set mutex code.
 */
#include "dbinc/mutex_int.h"

/* We don't want to run this code even in "ordinary" diagnostic mode. */
#undef MUTEX_DIAG

/*
 * Common code to get an event handle.  This is executed whenever a mutex
 * blocks, or when unlocking a mutex that a thread is waiting on.  We can't
 * keep these handles around, since the mutex structure is in shared memory,
 * and each process gets its own handle value.
 *
 * We pass security attributes so that the created event is accessible by all
 * users, in case a Windows service is sharing an environment with a local
 * process run as a different user.
 */
static _TCHAR hex_digits[] = _T("0123456789abcdef");
static SECURITY_DESCRIPTOR null_sd;
static SECURITY_ATTRIBUTES all_sa;
static int security_initialized = 0;

static __inline int get_handle(env, mutexp, eventp)
	ENV *env;
	DB_MUTEX *mutexp;
	HANDLE *eventp;
{
	_TCHAR idbuf[] = _T("db.m00000000");
	_TCHAR *p = idbuf + 12;
	int ret = 0;
	u_int32_t id;

	for (id = (mutexp)->id; id != 0; id >>= 4)
		*--p = hex_digits[id & 0xf];

#ifndef DB_WINCE
	if (!security_initialized) {
		InitializeSecurityDescriptor(&null_sd,
		    SECURITY_DESCRIPTOR_REVISION);
		SetSecurityDescriptorDacl(&null_sd, TRUE, 0, FALSE);
		all_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		all_sa.bInheritHandle = FALSE;
		all_sa.lpSecurityDescriptor = &null_sd;
		security_initialized = 1;
	}
#endif

	if ((*eventp = CreateEvent(&all_sa, FALSE, FALSE, idbuf)) == NULL) {
		ret = __os_get_syserr();
		__db_syserr(env, ret, "Win32 create event failed");
	}

	return (ret);
}

/*
 * __db_win32_mutex_lock_int
 *	Internal function to lock a win32 mutex
 *
 *	If the wait paramter is 0, this function will return DB_LOCK_NOTGRANTED
 *	rather than wait.
 *
 */
static __inline int
__db_win32_mutex_lock_int(env, mutex, wait)
	ENV *env;
	db_mutex_t mutex;
	int wait;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	DB_THREAD_INFO *ip;
	HANDLE event;
	u_int32_t nspins;
	int ms, ret;
#ifdef MUTEX_DIAG
	LARGE_INTEGER now;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	CHECK_MTX_THREAD(env, mutexp);

	/*
	 * See WINCE_ATOMIC_MAGIC definition for details.
	 * Use sharecount, because the value just needs to be a db_atomic_t
	 * memory mapped onto the same page as those being Interlocked*.
	 */
	WINCE_ATOMIC_MAGIC(&mutexp->sharecount);

	event = NULL;
	ms = 50;
	ret = 0;

	/*
	 * Only check the thread state once, by initializing the thread
	 * control block pointer to null.  If it is not the failchk
	 * thread, then ip will have a valid value subsequent times
	 * in the loop.
	 */
	ip = NULL;

loop:	/* Attempt to acquire the mutex mutex_tas_spins times, if waiting. */
	for (nspins =
	    mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		/*
		 * We can avoid the (expensive) interlocked instructions if
		 * the mutex is already busy.
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
			if (!wait)
				return (DB_LOCK_NOTGRANTED);
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause before retrying. [#6975]
			 */
			MUTEX_PAUSE
			continue;
		}

#ifdef DIAGNOSTIC
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			char buf[DB_THREADID_STRLEN];
			__db_errx(env,
			    "Win32 lock failed: mutex already locked by %s",
			     dbenv->thread_id_string(dbenv,
			     mutexp->pid, mutexp->tid, buf));
			return (__env_panic(env, EACCES));
		}
#endif
		F_SET(mutexp, DB_MUTEX_LOCKED);
		dbenv->thread_id(dbenv, &mutexp->pid, &mutexp->tid);

#ifdef HAVE_STATISTICS
		if (event == NULL)
			++mutexp->mutex_set_nowait;
		else
			++mutexp->mutex_set_wait;
#endif
		if (event != NULL) {
			CloseHandle(event);
			InterlockedDecrement(&mutexp->nwaiters);
#ifdef MUTEX_DIAG
			if (ret != WAIT_OBJECT_0) {
				QueryPerformanceCounter(&now);
				printf("[%I64d]: Lost signal on mutex %p, "
				    "id %d, ms %d\n",
				    now.QuadPart, mutexp, mutexp->id, ms);
			}
#endif
		}

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

	/*
	 * Yield the processor; wait 50 ms initially, up to 1 second.  This
	 * loop is needed to work around a race where the signal from the
	 * unlocking thread gets lost.  We start at 50 ms because it's unlikely
	 * to happen often and we want to avoid wasting CPU.
	 */
	if (event == NULL) {
#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&now);
		printf("[%I64d]: Waiting on mutex %p, id %d\n",
		    now.QuadPart, mutexp, mutexp->id);
#endif
		InterlockedIncrement(&mutexp->nwaiters);
		if ((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;
	}
	if ((ret = WaitForSingleObject(event, ms)) == WAIT_FAILED) {
		ret = __os_get_syserr();
		goto err;
	}
	if ((ms <<= 1) > MS_PER_SEC)
		ms = MS_PER_SEC;

	PANIC_CHECK(env);
	goto loop;

err:	__db_syserr(env, ret, "Win32 lock failed");
	return (__env_panic(env, __os_posix_err(ret)));
}

/*
 * __db_win32_mutex_init --
 *	Initialize a Win32 mutex.
 *
 * PUBLIC: int __db_win32_mutex_init __P((ENV *, db_mutex_t, u_int32_t));
 */
int
__db_win32_mutex_init(env, mutex, flags)
	ENV *env;
	db_mutex_t mutex;
	u_int32_t flags;
{
	DB_MUTEX *mutexp;

	mutexp = MUTEXP_SET(env->mutex_handle, mutex);
	mutexp->id = ((getpid() & 0xffff) << 16) ^ P_TO_UINT32(mutexp);
	F_SET(mutexp, flags);

	return (0);
}

/*
 * __db_win32_mutex_lock
 *	Lock on a mutex, blocking if necessary.
 *
 * PUBLIC: int __db_win32_mutex_lock __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_lock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_win32_mutex_lock_int(env, mutex, 1));
}

/*
 * __db_win32_mutex_trylock
 *	Try to lock a mutex, returning without waiting if it is busy
 *
 * PUBLIC: int __db_win32_mutex_trylock __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_trylock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_win32_mutex_lock_int(env, mutex, 0));
}

#if defined(HAVE_SHARED_LATCHES)
/*
 * __db_win32_mutex_readlock_int
 *	Try to lock a mutex, possibly waiting if requested and necessary.
 */
int
__db_win32_mutex_readlock_int(env, mutex, nowait)
	ENV *env;
	db_mutex_t mutex;
	int nowait;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	DB_MUTEXREGION *mtxregion;
	HANDLE event;
	u_int32_t nspins;
	int ms, ret;
	long exch_ret, mtx_val;
#ifdef MUTEX_DIAG
	LARGE_INTEGER now;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mtxregion = mtxmgr->reginfo.primary;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

	CHECK_MTX_THREAD(env, mutexp);

	/*
	 * See WINCE_ATOMIC_MAGIC definition for details.
	 * Use sharecount, because the value just needs to be a db_atomic_t
	 * memory mapped onto the same page as those being Interlocked*.
	 */
	WINCE_ATOMIC_MAGIC(&mutexp->sharecount);

	event = NULL;
	ms = 50;
	ret = 0;
	/*
	 * This needs to be initialized, since if mutexp->tas
	 * is write locked on the first pass, it needs a value.
	 */
	exch_ret = 0;

loop:	/* Attempt to acquire the resource for N spins. */
	for (nspins =
	    mtxregion->stat.st_mutex_tas_spins; nspins > 0; --nspins) {
		/*
		 * We can avoid the (expensive) interlocked instructions if
		 * the mutex is already "set".
		 */
retry:		mtx_val = atomic_read(&mutexp->sharecount);
		if (mtx_val == MUTEX_SHARE_ISEXCLUSIVE) {
			if (nowait)
				return (DB_LOCK_NOTGRANTED);

			continue;
		} else if (!atomic_compare_exchange(env, &mutexp->sharecount,
		    mtx_val, mtx_val + 1)) {
			/*
			 * Some systems (notably those with newer Intel CPUs)
			 * need a small pause here. [#6975]
			 */
			MUTEX_PAUSE
			goto retry;
		}

#ifdef HAVE_STATISTICS
		if (event == NULL)
			++mutexp->mutex_set_rd_nowait;
		else
			++mutexp->mutex_set_rd_wait;
#endif
		if (event != NULL) {
			CloseHandle(event);
			InterlockedDecrement(&mutexp->nwaiters);
#ifdef MUTEX_DIAG
			if (ret != WAIT_OBJECT_0) {
				QueryPerformanceCounter(&now);
				printf("[%I64d]: Lost signal on mutex %p, "
				    "id %d, ms %d\n",
				    now.QuadPart, mutexp, mutexp->id, ms);
			}
#endif
		}

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

	/*
	 * Yield the processor; wait 50 ms initially, up to 1 second.  This
	 * loop is needed to work around a race where the signal from the
	 * unlocking thread gets lost.  We start at 50 ms because it's unlikely
	 * to happen often and we want to avoid wasting CPU.
	 */
	if (event == NULL) {
#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&now);
		printf("[%I64d]: Waiting on mutex %p, id %d\n",
		    now.QuadPart, mutexp, mutexp->id);
#endif
		InterlockedIncrement(&mutexp->nwaiters);
		if ((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;
	}
	if ((ret = WaitForSingleObject(event, ms)) == WAIT_FAILED) {
		ret = __os_get_syserr();
		goto err;
	}
	if ((ms <<= 1) > MS_PER_SEC)
		ms = MS_PER_SEC;

	PANIC_CHECK(env);
	goto loop;

err:	__db_syserr(env, ret, "Win32 read lock failed");
	return (__env_panic(env, __os_posix_err(ret)));
}

/*
 * __db_win32_mutex_readlock
 *	Get a shared lock on a latch
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_win32_mutex_readlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int
__db_win32_mutex_readlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_win32_mutex_readlock_int(env, mutex, 0));
}

/*
 * __db_win32_mutex_tryreadlock
 *	Try to a shared lock on a latch
 *
 * PUBLIC: #if defined(HAVE_SHARED_LATCHES)
 * PUBLIC: int __db_win32_mutex_tryreadlock __P((ENV *, db_mutex_t));
 * PUBLIC: #endif
 */
int
__db_win32_mutex_tryreadlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (__db_win32_mutex_readlock_int(env, mutex, 1));
}
#endif

/*
 * __db_win32_mutex_unlock --
 *	Release a mutex.
 *
 * PUBLIC: int __db_win32_mutex_unlock __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_unlock(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	DB_MUTEXMGR *mtxmgr;
	HANDLE event;
	int ret;
#ifdef MUTEX_DIAG
	LARGE_INTEGER now;
#endif
	dbenv = env->dbenv;

	if (!MUTEX_ON(env) || F_ISSET(dbenv, DB_ENV_NOLOCKING))
		return (0);

	mtxmgr = env->mutex_handle;
	mutexp = MUTEXP_SET(mtxmgr, mutex);

#ifdef DIAGNOSTIC
	if (!MUTEXP_IS_BUSY(mutexp) || !(F_ISSET(mutexp, DB_MUTEX_SHARED) ||
	    F_ISSET(mutexp, DB_MUTEX_LOCKED))) {
		__db_errx(env,
		 "Win32 unlock failed: lock already unlocked: mutex %d busy %d",
		    mutex, MUTEXP_BUSY_FIELD(mutexp));
		return (__env_panic(env, EACCES));
	}
#endif
	/*
	 * If we have a shared latch, and a read lock (DB_MUTEX_LOCKED is only
	 * set for write locks), then decrement the latch. If the readlock is
	 * still held by other threads, just return. Otherwise go ahead and
	 * notify any waiting threads.
	 */
#ifdef HAVE_SHARED_LATCHES
	if (F_ISSET(mutexp, DB_MUTEX_SHARED)) {
		if (F_ISSET(mutexp, DB_MUTEX_LOCKED)) {
			F_CLR(mutexp, DB_MUTEX_LOCKED);
			if ((ret = InterlockedExchange(
			    (interlocked_val)(&atomic_read(
			    &mutexp->sharecount)), 0)) !=
			    MUTEX_SHARE_ISEXCLUSIVE) {
				ret = DB_RUNRECOVERY;
				goto err;
			}
		} else if (InterlockedDecrement(
		    (interlocked_val)(&atomic_read(&mutexp->sharecount))) > 0)
			return (0);
	} else
#endif
	{
		F_CLR(mutexp, DB_MUTEX_LOCKED);
		MUTEX_UNSET(&mutexp->tas);
	}

	if (mutexp->nwaiters > 0) {
		if ((ret = get_handle(env, mutexp, &event)) != 0)
			goto err;

#ifdef MUTEX_DIAG
		QueryPerformanceCounter(&now);
		printf("[%I64d]: Signalling mutex %p, id %d\n",
		    now.QuadPart, mutexp, mutexp->id);
#endif
		if (!PulseEvent(event)) {
			ret = __os_get_syserr();
			CloseHandle(event);
			goto err;
		}

		CloseHandle(event);
	}

	return (0);

err:	__db_syserr(env, ret, "Win32 unlock failed");
	return (__env_panic(env, __os_posix_err(ret)));
}

/*
 * __db_win32_mutex_destroy --
 *	Destroy a mutex.
 *
 * PUBLIC: int __db_win32_mutex_destroy __P((ENV *, db_mutex_t));
 */
int
__db_win32_mutex_destroy(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	return (0);
}
