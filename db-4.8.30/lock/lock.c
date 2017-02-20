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
#include "dbinc/log.h"

static int __lock_allocobj __P((DB_LOCKTAB *, u_int32_t));
static int __lock_alloclock __P((DB_LOCKTAB *, u_int32_t));
static int  __lock_freelock __P((DB_LOCKTAB *,
		struct __db_lock *, DB_LOCKER *, u_int32_t));
static int  __lock_getobj
		__P((DB_LOCKTAB *, const DBT *, u_int32_t, int, DB_LOCKOBJ **));
static int __lock_get_api __P((ENV *,
		u_int32_t, u_int32_t, const DBT *, db_lockmode_t, DB_LOCK *));
static int  __lock_inherit_locks __P ((DB_LOCKTAB *, DB_LOCKER *, u_int32_t));
static int  __lock_is_parent __P((DB_LOCKTAB *, roff_t, DB_LOCKER *));
static int  __lock_put_internal __P((DB_LOCKTAB *,
		struct __db_lock *, u_int32_t,  u_int32_t));
static int  __lock_put_nolock __P((ENV *, DB_LOCK *, int *, u_int32_t));
static int __lock_remove_waiter __P((DB_LOCKTAB *,
		DB_LOCKOBJ *, struct __db_lock *, db_status_t));
static int __lock_trade __P((ENV *, DB_LOCK *, DB_LOCKER *));
static int __lock_vec_api __P((ENV *,
		u_int32_t, u_int32_t,  DB_LOCKREQ *, int, DB_LOCKREQ **));

static const char __db_lock_invalid[] = "%s: Lock is no longer valid";
static const char __db_locker_invalid[] = "Locker is not valid";

/*
 * __lock_vec_pp --
 *	ENV->lock_vec pre/post processing.
 *
 * PUBLIC: int __lock_vec_pp __P((DB_ENV *,
 * PUBLIC:     u_int32_t, u_int32_t, DB_LOCKREQ *, int, DB_LOCKREQ **));
 */
int
__lock_vec_pp(dbenv, lid, flags, list, nlist, elistp)
	DB_ENV *dbenv;
	u_int32_t lid, flags;
	int nlist;
	DB_LOCKREQ *list, **elistp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lk_handle, "DB_ENV->lock_vec", DB_INIT_LOCK);

	/* Validate arguments. */
	if ((ret = __db_fchk(env,
	     "DB_ENV->lock_vec", flags, DB_LOCK_NOWAIT)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env,
	     (__lock_vec_api(env, lid, flags, list, nlist, elistp)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

static int
__lock_vec_api(env, lid, flags, list, nlist, elistp)
	ENV *env;
	u_int32_t lid, flags;
	int nlist;
	DB_LOCKREQ *list, **elistp;
{
	DB_LOCKER *sh_locker;
	int ret;

	if ((ret =
	    __lock_getlocker(env->lk_handle, lid, 0, &sh_locker)) == 0)
		ret = __lock_vec(env, sh_locker, flags, list, nlist, elistp);
	return (ret);
}

/*
 * __lock_vec --
 *	ENV->lock_vec.
 *
 *	Vector lock routine.  This function takes a set of operations
 *	and performs them all at once.  In addition, lock_vec provides
 *	functionality for lock inheritance, releasing all locks for a
 *	given locker (used during transaction commit/abort), releasing
 *	all locks on a given object, and generating debugging information.
 *
 * PUBLIC: int __lock_vec __P((ENV *,
 * PUBLIC:     DB_LOCKER *, u_int32_t, DB_LOCKREQ *, int, DB_LOCKREQ **));
 */
int
__lock_vec(env, sh_locker, flags, list, nlist, elistp)
	ENV *env;
	DB_LOCKER *sh_locker;
	u_int32_t flags;
	int nlist;
	DB_LOCKREQ *list, **elistp;
{
	struct __db_lock *lp, *next_lock;
	DB_LOCK lock; DB_LOCKOBJ *sh_obj;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	DBT *objlist, *np;
	u_int32_t ndx;
	int did_abort, i, ret, run_dd, upgrade, writes;

	/* Check if locks have been globally turned off. */
	if (F_ISSET(env->dbenv, DB_ENV_NOLOCKING))
		return (0);

	lt = env->lk_handle;
	region = lt->reginfo.primary;

	run_dd = 0;
	LOCK_SYSTEM_LOCK(lt, region);
	for (i = 0, ret = 0; i < nlist && ret == 0; i++)
		switch (list[i].op) {
		case DB_LOCK_GET_TIMEOUT:
			LF_SET(DB_LOCK_SET_TIMEOUT);
			/* FALLTHROUGH */
		case DB_LOCK_GET:
			if (IS_RECOVERING(env)) {
				LOCK_INIT(list[i].lock);
				break;
			}
			ret = __lock_get_internal(lt,
			    sh_locker, flags, list[i].obj,
			    list[i].mode, list[i].timeout, &list[i].lock);
			break;
		case DB_LOCK_INHERIT:
			ret = __lock_inherit_locks(lt, sh_locker, flags);
			break;
		case DB_LOCK_PUT:
			ret = __lock_put_nolock(env,
			    &list[i].lock, &run_dd, flags);
			break;
		case DB_LOCK_PUT_ALL:		/* Put all locks. */
		case DB_LOCK_PUT_READ:		/* Put read locks. */
		case DB_LOCK_UPGRADE_WRITE:
				/* Upgrade was_write and put read locks. */
			/*
			 * Since the locker may hold no
			 * locks (i.e., you could call abort before you've
			 * done any work), it's perfectly reasonable for there
			 * to be no locker; this is not an error.
			 */
			if (sh_locker == NULL)
				/*
				 * If ret is set, then we'll generate an
				 * error.  If it's not set, we have nothing
				 * to do.
				 */
				break;
			upgrade = 0;
			writes = 1;
			if (list[i].op == DB_LOCK_PUT_READ)
				writes = 0;
			else if (list[i].op == DB_LOCK_UPGRADE_WRITE) {
				if (F_ISSET(sh_locker, DB_LOCKER_DIRTY))
					upgrade = 1;
				writes = 0;
			}
			objlist = list[i].obj;
			if (objlist != NULL) {
				/*
				 * We know these should be ilocks,
				 * but they could be something else,
				 * so allocate room for the size too.
				 */
				objlist->size =
				     sh_locker->nwrites * sizeof(DBT);
				if ((ret = __os_malloc(env,
				     objlist->size, &objlist->data)) != 0)
					goto up_done;
				memset(objlist->data, 0, objlist->size);
				np = (DBT *) objlist->data;
			} else
				np = NULL;

			/* Now traverse the locks, releasing each one. */
			for (lp = SH_LIST_FIRST(&sh_locker->heldby, __db_lock);
			    lp != NULL; lp = next_lock) {
				sh_obj = (DB_LOCKOBJ *)
				    ((u_int8_t *)lp + lp->obj);
				next_lock = SH_LIST_NEXT(lp,
				    locker_links, __db_lock);
				if (writes == 1 ||
				    lp->mode == DB_LOCK_READ ||
				    lp->mode == DB_LOCK_READ_UNCOMMITTED) {
					SH_LIST_REMOVE(lp,
					    locker_links, __db_lock);
					sh_obj = (DB_LOCKOBJ *)
					    ((u_int8_t *)lp + lp->obj);
					ndx = sh_obj->indx;
					OBJECT_LOCK_NDX(lt, region, ndx);
					/*
					 * We are not letting lock_put_internal
					 * unlink the lock, so we'll have to
					 * update counts here.
					 */
					if (lp->status == DB_LSTAT_HELD) {
						DB_ASSERT(env,
						    sh_locker->nlocks != 0);
						sh_locker->nlocks--;
						if (IS_WRITELOCK(lp->mode))
							sh_locker->nwrites--;
					}
					ret = __lock_put_internal(lt, lp,
					    sh_obj->indx,
					    DB_LOCK_FREE | DB_LOCK_DOALL);
					OBJECT_UNLOCK(lt, region, ndx);
					if (ret != 0)
						break;
					continue;
				}
				if (objlist != NULL) {
					DB_ASSERT(env, (u_int8_t *)np <
					     (u_int8_t *)objlist->data +
					     objlist->size);
					np->data = SH_DBT_PTR(&sh_obj->lockobj);
					np->size = sh_obj->lockobj.size;
					np++;
				}
			}
			if (ret != 0)
				goto up_done;

			if (objlist != NULL)
				if ((ret = __lock_fix_list(env,
				     objlist, sh_locker->nwrites)) != 0)
					goto up_done;
			switch (list[i].op) {
			case DB_LOCK_UPGRADE_WRITE:
				/*
				 * Upgrade all WWRITE locks to WRITE so
				 * that we can abort a transaction which
				 * was supporting dirty readers.
				 */
				if (upgrade != 1)
					goto up_done;
				SH_LIST_FOREACH(lp, &sh_locker->heldby,
				    locker_links, __db_lock) {
					if (lp->mode != DB_LOCK_WWRITE)
						continue;
					lock.off = R_OFFSET(&lt->reginfo, lp);
					lock.gen = lp->gen;
					F_SET(sh_locker, DB_LOCKER_INABORT);
					if ((ret = __lock_get_internal(lt,
					    sh_locker, flags | DB_LOCK_UPGRADE,
					    NULL, DB_LOCK_WRITE, 0, &lock)) !=0)
						break;
				}
			up_done:
				/* FALLTHROUGH */
			case DB_LOCK_PUT_READ:
			case DB_LOCK_PUT_ALL:
				break;
			default:
				break;
			}
			break;
		case DB_LOCK_PUT_OBJ:
			/* Remove all the locks associated with an object. */
			OBJECT_LOCK(lt, region, list[i].obj, ndx);
			if ((ret = __lock_getobj(lt, list[i].obj,
			    ndx, 0, &sh_obj)) != 0 || sh_obj == NULL) {
				if (ret == 0)
					ret = EINVAL;
				OBJECT_UNLOCK(lt, region, ndx);
				break;
			}

			/*
			 * Go through both waiters and holders.  Don't bother
			 * to run promotion, because everyone is getting
			 * released.  The processes waiting will still get
			 * awakened as their waiters are released.
			 */
			for (lp = SH_TAILQ_FIRST(&sh_obj->waiters, __db_lock);
			    ret == 0 && lp != NULL;
			    lp = SH_TAILQ_FIRST(&sh_obj->waiters, __db_lock))
				ret = __lock_put_internal(lt, lp, ndx,
				    DB_LOCK_UNLINK |
				    DB_LOCK_NOPROMOTE | DB_LOCK_DOALL);

			/*
			 * On the last time around, the object will get
			 * reclaimed by __lock_put_internal, structure the
			 * loop carefully so we do not get bitten.
			 */
			for (lp = SH_TAILQ_FIRST(&sh_obj->holders, __db_lock);
			    ret == 0 && lp != NULL;
			    lp = next_lock) {
				next_lock = SH_TAILQ_NEXT(lp, links, __db_lock);
				ret = __lock_put_internal(lt, lp, ndx,
				    DB_LOCK_UNLINK |
				    DB_LOCK_NOPROMOTE | DB_LOCK_DOALL);
			}
			OBJECT_UNLOCK(lt, region, ndx);
			break;

		case DB_LOCK_TIMEOUT:
			ret = __lock_set_timeout_internal(env,
			    sh_locker, 0, DB_SET_TXN_NOW);
			break;

		case DB_LOCK_TRADE:
			/*
			 * INTERNAL USE ONLY.
			 * Change the holder of the lock described in
			 * list[i].lock to the locker-id specified by
			 * the locker parameter.
			 */
			/*
			 * You had better know what you're doing here.
			 * We are trading locker-id's on a lock to
			 * facilitate file locking on open DB handles.
			 * We do not do any conflict checking on this,
			 * so heaven help you if you use this flag under
			 * any other circumstances.
			 */
			ret = __lock_trade(env, &list[i].lock, sh_locker);
			break;
#if defined(DEBUG) && defined(HAVE_STATISTICS)
		case DB_LOCK_DUMP:
			if (sh_locker == NULL)
				break;

			SH_LIST_FOREACH(
			    lp, &sh_locker->heldby, locker_links, __db_lock)
				__lock_printlock(lt, NULL, lp, 1);
			break;
#endif
		default:
			__db_errx(env,
			    "Invalid lock operation: %d", list[i].op);
			ret = EINVAL;
			break;
		}

	if (ret == 0 && region->detect != DB_LOCK_NORUN &&
	     (region->need_dd || timespecisset(&region->next_timeout)))
		run_dd = 1;
	LOCK_SYSTEM_UNLOCK(lt, region);

	if (run_dd)
		(void)__lock_detect(env, region->detect, &did_abort);

	if (ret != 0 && elistp != NULL)
		*elistp = &list[i - 1];

	return (ret);
}

/*
 * __lock_get_pp --
 *	ENV->lock_get pre/post processing.
 *
 * PUBLIC: int __lock_get_pp __P((DB_ENV *,
 * PUBLIC:     u_int32_t, u_int32_t, DBT *, db_lockmode_t, DB_LOCK *));
 */
int
__lock_get_pp(dbenv, locker, flags, obj, lock_mode, lock)
	DB_ENV *dbenv;
	u_int32_t locker, flags;
	DBT *obj;
	db_lockmode_t lock_mode;
	DB_LOCK *lock;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lk_handle, "DB_ENV->lock_get", DB_INIT_LOCK);

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB_ENV->lock_get", flags,
	    DB_LOCK_NOWAIT | DB_LOCK_UPGRADE | DB_LOCK_SWITCH)) != 0)
		return (ret);

	if ((ret = __dbt_usercopy(env, obj)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env,
	     (__lock_get_api(env, locker, flags, obj, lock_mode, lock)),
	     0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

static int
__lock_get_api(env, locker, flags, obj, lock_mode, lock)
	ENV *env;
	u_int32_t locker, flags;
	const DBT *obj;
	db_lockmode_t lock_mode;
	DB_LOCK *lock;
{
	DB_LOCKER *sh_locker;
	DB_LOCKREGION *region;
	int ret;

	COMPQUIET(region, NULL);

	region = env->lk_handle->reginfo.primary;

	LOCK_SYSTEM_LOCK(env->lk_handle, region);
	LOCK_LOCKERS(env, region);
	ret = __lock_getlocker_int(env->lk_handle, locker, 0, &sh_locker);
	UNLOCK_LOCKERS(env, region);
	if (ret == 0)
		ret = __lock_get_internal(env->lk_handle,
		    sh_locker, flags, obj, lock_mode, 0, lock);
	LOCK_SYSTEM_UNLOCK(env->lk_handle, region);
	return (ret);
}

/*
 * __lock_get --
 *	ENV->lock_get.
 *
 * PUBLIC: int __lock_get __P((ENV *,
 * PUBLIC:     DB_LOCKER *, u_int32_t, const DBT *, db_lockmode_t, DB_LOCK *));
 */
int
__lock_get(env, locker, flags, obj, lock_mode, lock)
	ENV *env;
	DB_LOCKER *locker;
	u_int32_t flags;
	const DBT *obj;
	db_lockmode_t lock_mode;
	DB_LOCK *lock;
{
	DB_LOCKTAB *lt;
	int ret;

	lt = env->lk_handle;

	if (IS_RECOVERING(env)) {
		LOCK_INIT(*lock);
		return (0);
	}

	LOCK_SYSTEM_LOCK(lt, (DB_LOCKREGION *)lt->reginfo.primary);
	ret = __lock_get_internal(lt, locker, flags, obj, lock_mode, 0, lock);
	LOCK_SYSTEM_UNLOCK(lt, (DB_LOCKREGION *)lt->reginfo.primary);
	return (ret);
}
/*
 * __lock_alloclock -- allocate a lock from another partition.
 *	We assume we have the partition locked on entry and leave
 * it unlocked on success since we will have to retry the lock operation.
 * The mutex will be locked if we are out of space.
 */
static int
__lock_alloclock(lt, part_id)
	DB_LOCKTAB *lt;
	u_int32_t part_id;
{
	struct __db_lock *sh_lock;
	DB_LOCKPART *end_p, *cur_p;
	DB_LOCKREGION *region;
	int begin;

	region = lt->reginfo.primary;

	if (region->part_t_size == 1)
		goto err;

	begin = 0;
	sh_lock = NULL;
	cur_p = &lt->part_array[part_id];
	MUTEX_UNLOCK(lt->env, cur_p->mtx_part);
	end_p = &lt->part_array[region->part_t_size];
	/*
	 * Start looking at the next partition and wrap around.  If
	 * we get back to our partition then raise an error.
	 */
again:	for (cur_p++; sh_lock == NULL && cur_p < end_p; cur_p++) {
		MUTEX_LOCK(lt->env, cur_p->mtx_part);
		if ((sh_lock =
		    SH_TAILQ_FIRST(&cur_p->free_locks, __db_lock)) != NULL)
			SH_TAILQ_REMOVE(&cur_p->free_locks,
			    sh_lock, links, __db_lock);
		MUTEX_UNLOCK(lt->env, cur_p->mtx_part);
	}
	if (sh_lock != NULL) {
		cur_p = &lt->part_array[part_id];
		MUTEX_LOCK(lt->env, cur_p->mtx_part);
		SH_TAILQ_INSERT_HEAD(&cur_p->free_locks,
		    sh_lock, links, __db_lock);
		STAT(cur_p->part_stat.st_locksteals++);
		MUTEX_UNLOCK(lt->env, cur_p->mtx_part);
		return (0);
	}
	if (!begin) {
		begin = 1;
		cur_p = lt->part_array;
		end_p = &lt->part_array[part_id];
		goto again;
	}

	cur_p = &lt->part_array[part_id];
	MUTEX_LOCK(lt->env, cur_p->mtx_part);

err:	return (__lock_nomem(lt->env, "lock entries"));
}

/*
 * __lock_get_internal --
 *	All the work for lock_get (and for the GET option of lock_vec) is done
 *	inside of lock_get_internal.
 *
 * PUBLIC: int  __lock_get_internal __P((DB_LOCKTAB *, DB_LOCKER *, u_int32_t,
 * PUBLIC:     const DBT *, db_lockmode_t, db_timeout_t, DB_LOCK *));
 */
int
__lock_get_internal(lt, sh_locker, flags, obj, lock_mode, timeout, lock)
	DB_LOCKTAB *lt;
	DB_LOCKER *sh_locker;
	u_int32_t flags;
	const DBT *obj;
	db_lockmode_t lock_mode;
	db_timeout_t timeout;
	DB_LOCK *lock;
{
	struct __db_lock *newl, *lp;
	ENV *env;
	DB_LOCKOBJ *sh_obj;
	DB_LOCKREGION *region;
	DB_THREAD_INFO *ip;
	u_int32_t ndx, part_id;
	int did_abort, ihold, grant_dirty, no_dd, ret, t_ret;
	roff_t holder, sh_off;

	/*
	 * We decide what action to take based on what locks are already held
	 * and what locks are in the wait queue.
	 */
	enum {
		GRANT,		/* Grant the lock. */
		UPGRADE,	/* Upgrade the lock. */
		HEAD,		/* Wait at head of wait queue. */
		SECOND,		/* Wait as the second waiter. */
		TAIL		/* Wait at tail of the wait queue. */
	} action;

	env = lt->env;
	region = lt->reginfo.primary;

	/* Check if locks have been globally turned off. */
	if (F_ISSET(env->dbenv, DB_ENV_NOLOCKING))
		return (0);

	if (sh_locker == NULL) {
		__db_errx(env, "Locker does not exist");
		return (EINVAL);
	}

	no_dd = ret = 0;
	newl = NULL;
	sh_obj = NULL;

	/* Check that the lock mode is valid.  */
	if (lock_mode >= (db_lockmode_t)region->nmodes) {
		__db_errx(env, "DB_ENV->lock_get: invalid lock mode %lu",
		    (u_long)lock_mode);
		return (EINVAL);
	}

again:	if (obj == NULL) {
		DB_ASSERT(env, LOCK_ISSET(*lock));
		lp = R_ADDR(&lt->reginfo, lock->off);
		sh_obj = (DB_LOCKOBJ *)((u_int8_t *)lp + lp->obj);
		ndx = sh_obj->indx;
		OBJECT_LOCK_NDX(lt, region, ndx);
	} else {
		/* Allocate a shared memory new object. */
		OBJECT_LOCK(lt, region, obj, lock->ndx);
		ndx = lock->ndx;
		if ((ret = __lock_getobj(lt, obj, lock->ndx, 1, &sh_obj)) != 0)
			goto err;
	}

#ifdef HAVE_STATISTICS
	if (LF_ISSET(DB_LOCK_UPGRADE))
		lt->obj_stat[ndx].st_nupgrade++;
	else if (!LF_ISSET(DB_LOCK_SWITCH))
		lt->obj_stat[ndx].st_nrequests++;
#endif

	/*
	 * Figure out if we can grant this lock or if it should wait.
	 * By default, we can grant the new lock if it does not conflict with
	 * anyone on the holders list OR anyone on the waiters list.
	 * The reason that we don't grant if there's a conflict is that
	 * this can lead to starvation (a writer waiting on a popularly
	 * read item will never be granted).  The downside of this is that
	 * a waiting reader can prevent an upgrade from reader to writer,
	 * which is not uncommon.
	 *
	 * There are two exceptions to the no-conflict rule.  First, if
	 * a lock is held by the requesting locker AND the new lock does
	 * not conflict with any other holders, then we grant the lock.
	 * The most common place this happens is when the holder has a
	 * WRITE lock and a READ lock request comes in for the same locker.
	 * If we do not grant the read lock, then we guarantee deadlock.
	 * Second, dirty readers are granted if at all possible while
	 * avoiding starvation, see below.
	 *
	 * In case of conflict, we put the new lock on the end of the waiters
	 * list, unless we are upgrading or this is a dirty reader in which
	 * case the locker goes at or near the front of the list.
	 */
	ihold = 0;
	grant_dirty = 0;
	holder = 0;

	/*
	 * SWITCH is a special case, used by the queue access method
	 * when we want to get an entry which is past the end of the queue.
	 * We have a DB_READ_LOCK and need to switch it to DB_LOCK_WAIT and
	 * join the waiters queue.  This must be done as a single operation
	 * so that another locker cannot get in and fail to wake us up.
	 */
	if (LF_ISSET(DB_LOCK_SWITCH))
		lp = NULL;
	else
		lp = SH_TAILQ_FIRST(&sh_obj->holders, __db_lock);

	sh_off = R_OFFSET(&lt->reginfo, sh_locker);
	for (; lp != NULL; lp = SH_TAILQ_NEXT(lp, links, __db_lock)) {
		DB_ASSERT(env, lp->status != DB_LSTAT_FREE);
		if (sh_off == lp->holder) {
			if (lp->mode == lock_mode &&
			    lp->status == DB_LSTAT_HELD) {
				if (LF_ISSET(DB_LOCK_UPGRADE))
					goto upgrade;

				/*
				 * Lock is held, so we can increment the
				 * reference count and return this lock
				 * to the caller.  We do not count reference
				 * increments towards the locks held by
				 * the locker.
				 */
				lp->refcount++;
				lock->off = R_OFFSET(&lt->reginfo, lp);
				lock->gen = lp->gen;
				lock->mode = lp->mode;
				goto done;
			} else {
				ihold = 1;
			}
		} else if (__lock_is_parent(lt, lp->holder, sh_locker))
			ihold = 1;
		else if (CONFLICTS(lt, region, lp->mode, lock_mode))
			break;
		else if (lp->mode == DB_LOCK_READ ||
		     lp->mode == DB_LOCK_WWRITE) {
			grant_dirty = 1;
			holder = lp->holder;
		}
	}

	/*
	 * If there are conflicting holders we will have to wait.  If we
	 * already hold a lock on this object or are doing an upgrade or
	 * this is a dirty reader it goes to the head of the queue, everyone
	 * else to the back.
	 */
	if (lp != NULL) {
		if (ihold || LF_ISSET(DB_LOCK_UPGRADE) ||
		    lock_mode == DB_LOCK_READ_UNCOMMITTED)
			action = HEAD;
		else
			action = TAIL;
	} else {
		if (LF_ISSET(DB_LOCK_SWITCH))
			action = TAIL;
		else if (LF_ISSET(DB_LOCK_UPGRADE))
			action = UPGRADE;
		else  if (ihold)
			action = GRANT;
		else {
			/*
			 * Look for conflicting waiters.
			 */
			SH_TAILQ_FOREACH(
			    lp, &sh_obj->waiters, links, __db_lock)
				if (CONFLICTS(lt, region, lp->mode,
				     lock_mode) && sh_off != lp->holder)
					break;

			/*
			 * If there are no conflicting holders or waiters,
			 * then we grant. Normally when we wait, we
			 * wait at the end (TAIL).  However, the goal of
			 * DIRTY_READ locks to allow forward progress in the
			 * face of updating transactions, so we try to allow
			 * all DIRTY_READ requests to proceed as rapidly
			 * as possible, so long as we can prevent starvation.
			 *
			 * When determining how to queue a DIRTY_READ
			 * request:
			 *
			 *	1. If there is a waiting upgrading writer,
			 *	   then we enqueue the dirty reader BEHIND it
			 *	   (second in the queue).
			 *	2. Else, if the current holders are either
			 *	   READ or WWRITE, we grant
			 *	3. Else queue SECOND i.e., behind the first
			 *	   waiter.
			 *
			 * The end result is that dirty_readers get to run
			 * so long as other lockers are blocked.  Once
			 * there is a locker which is only waiting on
			 * dirty readers then they queue up behind that
			 * locker so that it gets to run.  In general
			 * this locker will be a WRITE which will shortly
			 * get downgraded to a WWRITE, permitting the
			 * DIRTY locks to be granted.
			 */
			if (lp == NULL)
				action = GRANT;
			else if (grant_dirty &&
			    lock_mode == DB_LOCK_READ_UNCOMMITTED) {
				/*
				 * An upgrade will be at the head of the
				 * queue.
				 */
				lp = SH_TAILQ_FIRST(
				     &sh_obj->waiters, __db_lock);
				if (lp->mode == DB_LOCK_WRITE &&
				     lp->holder == holder)
					action = SECOND;
				else
					action = GRANT;
			} else if (lock_mode == DB_LOCK_READ_UNCOMMITTED)
				action = SECOND;
			else
				action = TAIL;
		}
	}

	switch (action) {
	case HEAD:
	case TAIL:
	case SECOND:
	case GRANT:
		part_id = LOCK_PART(region, ndx);
		/* Allocate a new lock. */
		if ((newl = SH_TAILQ_FIRST(
		    &FREE_LOCKS(lt, part_id), __db_lock)) == NULL) {
			if ((ret = __lock_alloclock(lt, part_id)) != 0)
				goto err;
			/* Dropped the mutex start over. */
			goto again;
		}
		SH_TAILQ_REMOVE(
		    &FREE_LOCKS(lt, part_id), newl, links, __db_lock);

#ifdef HAVE_STATISTICS
		/*
		 * Keep track of the maximum number of locks allocated
		 * in each partition and the maximum number of locks
		 * used by any one bucket.
		 */
		if (++lt->obj_stat[ndx].st_nlocks >
		    lt->obj_stat[ndx].st_maxnlocks)
			lt->obj_stat[ndx].st_maxnlocks =
			    lt->obj_stat[ndx].st_nlocks;
		if (++lt->part_array[part_id].part_stat.st_nlocks >
		    lt->part_array[part_id].part_stat.st_maxnlocks)
			lt->part_array[part_id].part_stat.st_maxnlocks =
			    lt->part_array[part_id].part_stat.st_nlocks;
#endif

		newl->holder = R_OFFSET(&lt->reginfo, sh_locker);
		newl->refcount = 1;
		newl->mode = lock_mode;
		newl->obj = (roff_t)SH_PTR_TO_OFF(newl, sh_obj);
		newl->indx = sh_obj->indx;
		/*
		 * Now, insert the lock onto its locker's list.
		 * If the locker does not currently hold any locks,
		 * there's no reason to run a deadlock
		 * detector, save that information.
		 */
		no_dd = sh_locker->master_locker == INVALID_ROFF &&
		    SH_LIST_FIRST(
		    &sh_locker->child_locker, __db_locker) == NULL &&
		    SH_LIST_FIRST(&sh_locker->heldby, __db_lock) == NULL;

		SH_LIST_INSERT_HEAD(
		    &sh_locker->heldby, newl, locker_links, __db_lock);

		/*
		 * Allocate a mutex if we do not have a mutex backing the lock.
		 *
		 * Use the lock mutex to block the thread; lock the mutex
		 * when it is allocated so that we will block when we try
		 * to lock it again.  We will wake up when another thread
		 * grants the lock and releases the mutex.  We leave it
		 * locked for the next use of this lock object.
		 */
		if (newl->mtx_lock == MUTEX_INVALID) {
			if ((ret = __mutex_alloc(env, MTX_LOGICAL_LOCK,
			    DB_MUTEX_LOGICAL_LOCK | DB_MUTEX_SELF_BLOCK,
			    &newl->mtx_lock)) != 0)
				goto err;
			MUTEX_LOCK(env, newl->mtx_lock);
		}
		break;

	case UPGRADE:
upgrade:	lp = R_ADDR(&lt->reginfo, lock->off);
		if (IS_WRITELOCK(lock_mode) && !IS_WRITELOCK(lp->mode))
			sh_locker->nwrites++;
		lp->mode = lock_mode;
		goto done;
	}

	switch (action) {
	case UPGRADE:
		DB_ASSERT(env, 0);
		break;
	case GRANT:
		newl->status = DB_LSTAT_HELD;
		SH_TAILQ_INSERT_TAIL(&sh_obj->holders, newl, links);
		break;
	case HEAD:
	case TAIL:
	case SECOND:
		if (LF_ISSET(DB_LOCK_NOWAIT)) {
			ret = DB_LOCK_NOTGRANTED;
			STAT(region->stat.st_lock_nowait++);
			goto err;
		}
		if ((lp =
		    SH_TAILQ_FIRST(&sh_obj->waiters, __db_lock)) == NULL) {
			LOCK_DD(env, region);
			SH_TAILQ_INSERT_HEAD(&region->dd_objs,
				    sh_obj, dd_links, __db_lockobj);
			UNLOCK_DD(env, region);
		}
		switch (action) {
		case HEAD:
			SH_TAILQ_INSERT_HEAD(
			     &sh_obj->waiters, newl, links, __db_lock);
			break;
		case SECOND:
			SH_TAILQ_INSERT_AFTER(
			     &sh_obj->waiters, lp, newl, links, __db_lock);
			break;
		case TAIL:
			SH_TAILQ_INSERT_TAIL(&sh_obj->waiters, newl, links);
			break;
		default:
			DB_ASSERT(env, 0);
		}

		/*
		 * First check to see if this txn has expired.
		 * If not then see if the lock timeout is past
		 * the expiration of the txn, if it is, use
		 * the txn expiration time.  lk_expire is passed
		 * to avoid an extra call to get the time.
		 */
		if (__lock_expired(env,
		    &sh_locker->lk_expire, &sh_locker->tx_expire)) {
			newl->status = DB_LSTAT_EXPIRED;
			sh_locker->lk_expire = sh_locker->tx_expire;

			/* We are done. */
			goto expired;
		}

		/*
		 * If a timeout was specified in this call then it
		 * takes priority.  If a lock timeout has been specified
		 * for this transaction then use that, otherwise use
		 * the global timeout value.
		 */
		if (!LF_ISSET(DB_LOCK_SET_TIMEOUT)) {
			if (F_ISSET(sh_locker, DB_LOCKER_TIMEOUT))
				timeout = sh_locker->lk_timeout;
			else
				timeout = region->lk_timeout;
		}
		if (timeout != 0)
			__lock_expires(env, &sh_locker->lk_expire, timeout);
		else
			timespecclear(&sh_locker->lk_expire);

		if (timespecisset(&sh_locker->tx_expire) &&
			(timeout == 0 || __lock_expired(env,
			    &sh_locker->lk_expire, &sh_locker->tx_expire)))
				sh_locker->lk_expire = sh_locker->tx_expire;
		if (timespecisset(&sh_locker->lk_expire) &&
		    (!timespecisset(&region->next_timeout) ||
		    timespeccmp(
		    &region->next_timeout, &sh_locker->lk_expire, >)))
			region->next_timeout = sh_locker->lk_expire;

in_abort:	newl->status = DB_LSTAT_WAITING;
		STAT(lt->obj_stat[ndx].st_lock_wait++);
		/* We are about to block, deadlock detector must run. */
		region->need_dd = 1;

		OBJECT_UNLOCK(lt, region, sh_obj->indx);

		/* If we are switching drop the lock we had. */
		if (LF_ISSET(DB_LOCK_SWITCH) &&
		    (ret = __lock_put_nolock(env,
		    lock, &ihold, DB_LOCK_NOWAITERS)) != 0) {
			OBJECT_LOCK_NDX(lt, region, sh_obj->indx);
			(void)__lock_remove_waiter(
			    lt, sh_obj, newl, DB_LSTAT_FREE);
			goto err;
		}

		LOCK_SYSTEM_UNLOCK(lt, region);

		/*
		 * Before waiting, see if the deadlock detector should run.
		 */
		if (region->detect != DB_LOCK_NORUN && !no_dd)
			(void)__lock_detect(env, region->detect, &did_abort);

		ip = NULL;
		if (env->thr_hashtab != NULL &&
		     (ret = __env_set_state(env, &ip, THREAD_BLOCKED)) != 0) {
			LOCK_SYSTEM_LOCK(lt, region);
			OBJECT_LOCK_NDX(lt, region, ndx);
			goto err;
		}

		MUTEX_LOCK(env, newl->mtx_lock);
		if (ip != NULL)
			ip->dbth_state = THREAD_ACTIVE;

		LOCK_SYSTEM_LOCK(lt, region);
		OBJECT_LOCK_NDX(lt, region, ndx);

		/* Turn off lock timeout. */
		if (newl->status != DB_LSTAT_EXPIRED)
			timespecclear(&sh_locker->lk_expire);

		switch (newl->status) {
		case DB_LSTAT_ABORTED:
			/*
			 * If we raced with the deadlock detector and it
			 * mistakenly picked this tranaction to abort again
			 * ignore the abort and request the lock again.
			 */
			if (F_ISSET(sh_locker, DB_LOCKER_INABORT))
				goto in_abort;
			ret = DB_LOCK_DEADLOCK;
			goto err;
		case DB_LSTAT_EXPIRED:
expired:		ret = __lock_put_internal(lt, newl,
			    ndx, DB_LOCK_UNLINK | DB_LOCK_FREE);
			newl = NULL;
			if (ret != 0)
				goto err;
#ifdef HAVE_STATISTICS
			if (timespeccmp(
			    &sh_locker->lk_expire, &sh_locker->tx_expire, ==))
				lt->obj_stat[ndx].st_ntxntimeouts++;
			else
				lt->obj_stat[ndx].st_nlocktimeouts++;
#endif
			ret = DB_LOCK_NOTGRANTED;
			timespecclear(&sh_locker->lk_expire);
			goto err;
		case DB_LSTAT_PENDING:
			if (LF_ISSET(DB_LOCK_UPGRADE)) {
				/*
				 * The lock just granted got put on the holders
				 * list.  Since we're upgrading some other lock,
				 * we've got to remove it here.
				 */
				SH_TAILQ_REMOVE(
				    &sh_obj->holders, newl, links, __db_lock);
				/*
				 * Ensure the object is not believed to be on
				 * the object's lists, if we're traversing by
				 * locker.
				 */
				newl->links.stqe_prev = -1;
				goto upgrade;
			} else
				newl->status = DB_LSTAT_HELD;
			break;
		case DB_LSTAT_FREE:
		case DB_LSTAT_HELD:
		case DB_LSTAT_WAITING:
		default:
			__db_errx(env,
			    "Unexpected lock status: %d", (int)newl->status);
			ret = __env_panic(env, EINVAL);
			goto err;
		}
	}

	lock->off = R_OFFSET(&lt->reginfo, newl);
	lock->gen = newl->gen;
	lock->mode = newl->mode;
	sh_locker->nlocks++;
	if (IS_WRITELOCK(newl->mode)) {
		sh_locker->nwrites++;
		if (newl->mode == DB_LOCK_WWRITE)
			F_SET(sh_locker, DB_LOCKER_DIRTY);
	}

	OBJECT_UNLOCK(lt, region, ndx);
	return (0);

err:	if (!LF_ISSET(DB_LOCK_UPGRADE | DB_LOCK_SWITCH))
		LOCK_INIT(*lock);

done:	if (newl != NULL &&
	     (t_ret = __lock_freelock(lt, newl, sh_locker,
	     DB_LOCK_FREE | DB_LOCK_UNLINK)) != 0 && ret == 0)
		ret = t_ret;
	OBJECT_UNLOCK(lt, region, ndx);

	return (ret);
}

/*
 * __lock_put_pp --
 *	ENV->lock_put pre/post processing.
 *
 * PUBLIC: int  __lock_put_pp __P((DB_ENV *, DB_LOCK *));
 */
int
__lock_put_pp(dbenv, lock)
	DB_ENV *dbenv;
	DB_LOCK *lock;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lk_handle, "DB_LOCK->lock_put", DB_INIT_LOCK);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__lock_put(env, lock)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __lock_put --
 *
 * PUBLIC: int  __lock_put __P((ENV *, DB_LOCK *));
 *  Internal lock_put interface.
 */
int
__lock_put(env, lock)
	ENV *env;
	DB_LOCK *lock;
{
	DB_LOCKTAB *lt;
	int ret, run_dd;

	if (IS_RECOVERING(env))
		return (0);

	lt = env->lk_handle;

	LOCK_SYSTEM_LOCK(lt, (DB_LOCKREGION *)lt->reginfo.primary);
	ret = __lock_put_nolock(env, lock, &run_dd, 0);
	LOCK_SYSTEM_UNLOCK(lt, (DB_LOCKREGION *)lt->reginfo.primary);

	/*
	 * Only run the lock detector if put told us to AND we are running
	 * in auto-detect mode.  If we are not running in auto-detect, then
	 * a call to lock_detect here will 0 the need_dd bit, but will not
	 * actually abort anything.
	 */
	if (ret == 0 && run_dd)
		(void)__lock_detect(env,
		    ((DB_LOCKREGION *)lt->reginfo.primary)->detect, NULL);
	return (ret);
}

static int
__lock_put_nolock(env, lock, runp, flags)
	ENV *env;
	DB_LOCK *lock;
	int *runp;
	u_int32_t flags;
{
	struct __db_lock *lockp;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	int ret;

	/* Check if locks have been globally turned off. */
	if (F_ISSET(env->dbenv, DB_ENV_NOLOCKING))
		return (0);

	lt = env->lk_handle;
	region = lt->reginfo.primary;

	lockp = R_ADDR(&lt->reginfo, lock->off);
	if (lock->gen != lockp->gen) {
		__db_errx(env, __db_lock_invalid, "DB_LOCK->lock_put");
		LOCK_INIT(*lock);
		return (EINVAL);
	}

	OBJECT_LOCK_NDX(lt, region, lock->ndx);
	ret = __lock_put_internal(lt,
	    lockp, lock->ndx, flags | DB_LOCK_UNLINK | DB_LOCK_FREE);
	OBJECT_UNLOCK(lt, region, lock->ndx);

	LOCK_INIT(*lock);

	*runp = 0;
	if (ret == 0 && region->detect != DB_LOCK_NORUN &&
	     (region->need_dd || timespecisset(&region->next_timeout)))
		*runp = 1;

	return (ret);
}

/*
 * __lock_downgrade --
 *
 * Used to downgrade locks.  Currently this is used in three places: 1) by the
 * Concurrent Data Store product to downgrade write locks back to iwrite locks
 * and 2) to downgrade write-handle locks to read-handle locks at the end of
 * an open/create. 3) To downgrade write locks to was_write to support dirty
 * reads.
 *
 * PUBLIC: int __lock_downgrade __P((ENV *,
 * PUBLIC:     DB_LOCK *, db_lockmode_t, u_int32_t));
 */
int
__lock_downgrade(env, lock, new_mode, flags)
	ENV *env;
	DB_LOCK *lock;
	db_lockmode_t new_mode;
	u_int32_t flags;
{
	struct __db_lock *lockp;
	DB_LOCKER *sh_locker;
	DB_LOCKOBJ *obj;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	int ret;

	ret = 0;

	/* Check if locks have been globally turned off. */
	if (F_ISSET(env->dbenv, DB_ENV_NOLOCKING))
		return (0);

	lt = env->lk_handle;
	region = lt->reginfo.primary;

	LOCK_SYSTEM_LOCK(lt, region);

	lockp = R_ADDR(&lt->reginfo, lock->off);
	if (lock->gen != lockp->gen) {
		__db_errx(env, __db_lock_invalid, "lock_downgrade");
		ret = EINVAL;
		goto out;
	}

	sh_locker = R_ADDR(&lt->reginfo, lockp->holder);

	if (IS_WRITELOCK(lockp->mode) && !IS_WRITELOCK(new_mode))
		sh_locker->nwrites--;

	lockp->mode = new_mode;
	lock->mode = new_mode;

	/* Get the object associated with this lock. */
	obj = (DB_LOCKOBJ *)((u_int8_t *)lockp + lockp->obj);
	OBJECT_LOCK_NDX(lt, region, obj->indx);
	STAT(lt->obj_stat[obj->indx].st_ndowngrade++);
	ret = __lock_promote(lt, obj, NULL, LF_ISSET(DB_LOCK_NOWAITERS));
	OBJECT_UNLOCK(lt, region, obj->indx);

out:	LOCK_SYSTEM_UNLOCK(lt, region);
	return (ret);
}

/*
 * __lock_put_internal -- put a lock structure
 * We assume that we are called with the proper object locked.
 */
static int
__lock_put_internal(lt, lockp, obj_ndx, flags)
	DB_LOCKTAB *lt;
	struct __db_lock *lockp;
	u_int32_t obj_ndx, flags;
{
	DB_LOCKOBJ *sh_obj;
	DB_LOCKREGION *region;
	ENV *env;
	u_int32_t part_id;
	int ret, state_changed;

	COMPQUIET(env, NULL);
	env = lt->env;
	region = lt->reginfo.primary;
	ret = state_changed = 0;

	if (!OBJ_LINKS_VALID(lockp)) {
		/*
		 * Someone removed this lock while we were doing a release
		 * by locker id.  We are trying to free this lock, but it's
		 * already been done; all we need to do is return it to the
		 * free list.
		 */
		(void)__lock_freelock(lt, lockp, NULL, DB_LOCK_FREE);
		return (0);
	}

#ifdef HAVE_STATISTICS
	if (LF_ISSET(DB_LOCK_DOALL))
		lt->obj_stat[obj_ndx].st_nreleases += lockp->refcount;
	else
		lt->obj_stat[obj_ndx].st_nreleases++;
#endif

	if (!LF_ISSET(DB_LOCK_DOALL) && lockp->refcount > 1) {
		lockp->refcount--;
		return (0);
	}

	/* Increment generation number. */
	lockp->gen++;

	/* Get the object associated with this lock. */
	sh_obj = (DB_LOCKOBJ *)((u_int8_t *)lockp + lockp->obj);

	/*
	 * Remove this lock from its holders/waitlist.  Set its status
	 * to ABORTED.  It may get freed below, but if not then the
	 * waiter has been aborted (it will panic if the lock is
	 * free).
	 */
	if (lockp->status != DB_LSTAT_HELD &&
	    lockp->status != DB_LSTAT_PENDING) {
		if ((ret = __lock_remove_waiter(
		    lt, sh_obj, lockp, DB_LSTAT_ABORTED)) != 0)
			return (ret);
	} else {
		SH_TAILQ_REMOVE(&sh_obj->holders, lockp, links, __db_lock);
		lockp->links.stqe_prev = -1;
	}

	if (LF_ISSET(DB_LOCK_NOPROMOTE))
		state_changed = 0;
	else
		if ((ret = __lock_promote(lt, sh_obj, &state_changed,
		    LF_ISSET(DB_LOCK_NOWAITERS))) != 0)
			return (ret);

	/* Check if object should be reclaimed. */
	if (SH_TAILQ_FIRST(&sh_obj->holders, __db_lock) == NULL &&
	    SH_TAILQ_FIRST(&sh_obj->waiters, __db_lock) == NULL) {
		part_id = LOCK_PART(region, obj_ndx);
		SH_TAILQ_REMOVE(
		    &lt->obj_tab[obj_ndx], sh_obj, links, __db_lockobj);
		if (sh_obj->lockobj.size > sizeof(sh_obj->objdata)) {
			if (region->part_t_size != 1)
				LOCK_REGION_LOCK(env);
			__env_alloc_free(&lt->reginfo,
			    SH_DBT_PTR(&sh_obj->lockobj));
			if (region->part_t_size != 1)
				LOCK_REGION_UNLOCK(env);
		}
		SH_TAILQ_INSERT_HEAD(
		    &FREE_OBJS(lt, part_id), sh_obj, links, __db_lockobj);
		sh_obj->generation++;
		STAT(lt->part_array[part_id].part_stat.st_nobjects--);
		STAT(lt->obj_stat[obj_ndx].st_nobjects--);
		state_changed = 1;
	}

	/* Free lock. */
	if (LF_ISSET(DB_LOCK_UNLINK | DB_LOCK_FREE))
		ret = __lock_freelock(lt, lockp,
		     R_ADDR(&lt->reginfo, lockp->holder), flags);

	/*
	 * If we did not promote anyone; we need to run the deadlock
	 * detector again.
	 */
	if (state_changed == 0)
		region->need_dd = 1;

	return (ret);
}

/*
 * __lock_freelock --
 *	Free a lock.  Unlink it from its locker if necessary.
 * We must hold the object lock.
 *
 */
static int
__lock_freelock(lt, lockp, sh_locker, flags)
	DB_LOCKTAB *lt;
	struct __db_lock *lockp;
	DB_LOCKER *sh_locker;
	u_int32_t flags;
{
	DB_LOCKREGION *region;
	ENV *env;
	u_int32_t part_id;
	int ret;

	env = lt->env;
	region = lt->reginfo.primary;

	if (LF_ISSET(DB_LOCK_UNLINK)) {
		SH_LIST_REMOVE(lockp, locker_links, __db_lock);
		if (lockp->status == DB_LSTAT_HELD) {
			sh_locker->nlocks--;
			if (IS_WRITELOCK(lockp->mode))
				sh_locker->nwrites--;
		}
	}

	if (LF_ISSET(DB_LOCK_FREE)) {
		/*
		 * If the lock is not held we cannot be sure of its mutex
		 * state so we just destroy it and let it be re-created
		 * when needed.
		 */
		part_id = LOCK_PART(region, lockp->indx);
		if (lockp->mtx_lock != MUTEX_INVALID &&
		     lockp->status != DB_LSTAT_HELD &&
		     lockp->status != DB_LSTAT_EXPIRED &&
		     (ret = __mutex_free(env, &lockp->mtx_lock)) != 0)
			return (ret);
		lockp->status = DB_LSTAT_FREE;
		SH_TAILQ_INSERT_HEAD(&FREE_LOCKS(lt, part_id),
		     lockp, links, __db_lock);
		STAT(lt->part_array[part_id].part_stat.st_nlocks--);
		STAT(lt->obj_stat[lockp->indx].st_nlocks--);
	}

	return (0);
}

/*
 * __lock_allocobj -- allocate a object from another partition.
 *	We assume we have the partition locked on entry and leave
 * with the same partition locked on exit.
 */
static int
__lock_allocobj(lt, part_id)
	DB_LOCKTAB *lt;
	u_int32_t part_id;
{
	DB_LOCKOBJ *sh_obj;
	DB_LOCKPART *end_p, *cur_p;
	DB_LOCKREGION *region;
	int begin;

	region = lt->reginfo.primary;

	if (region->part_t_size == 1)
		goto err;

	begin = 0;
	sh_obj = NULL;
	cur_p = &lt->part_array[part_id];
	MUTEX_UNLOCK(lt->env, cur_p->mtx_part);
	end_p = &lt->part_array[region->part_t_size];
	/*
	 * Start looking at the next partition and wrap around.  If
	 * we get back to our partition then raise an error.
	 */
again:	for (cur_p++; sh_obj == NULL && cur_p < end_p; cur_p++) {
		MUTEX_LOCK(lt->env, cur_p->mtx_part);
		if ((sh_obj =
		    SH_TAILQ_FIRST(&cur_p->free_objs, __db_lockobj)) != NULL)
			SH_TAILQ_REMOVE(&cur_p->free_objs,
			    sh_obj, links, __db_lockobj);
		MUTEX_UNLOCK(lt->env, cur_p->mtx_part);
	}
	if (sh_obj != NULL) {
		cur_p = &lt->part_array[part_id];
		MUTEX_LOCK(lt->env, cur_p->mtx_part);
		SH_TAILQ_INSERT_HEAD(&cur_p->free_objs,
		    sh_obj, links, __db_lockobj);
		STAT(cur_p->part_stat.st_objectsteals++);
		return (0);
	}
	if (!begin) {
		begin = 1;
		cur_p = lt->part_array;
		end_p = &lt->part_array[part_id];
		goto again;
	}
	MUTEX_LOCK(lt->env, cur_p->mtx_part);

err:	return (__lock_nomem(lt->env, "object entries"));
}
/*
 * __lock_getobj --
 *	Get an object in the object hash table.  The create parameter
 * indicates if the object should be created if it doesn't exist in
 * the table.
 *
 * This must be called with the object bucket locked.
 */
static int
__lock_getobj(lt, obj, ndx, create, retp)
	DB_LOCKTAB *lt;
	const DBT *obj;
	u_int32_t ndx;
	int create;
	DB_LOCKOBJ **retp;
{
	DB_LOCKOBJ *sh_obj;
	DB_LOCKREGION *region;
	ENV *env;
	int ret;
	void *p;
	u_int32_t len, part_id;

	env = lt->env;
	region = lt->reginfo.primary;
	len = 0;

	/* Look up the object in the hash table. */
retry:	SH_TAILQ_FOREACH(sh_obj, &lt->obj_tab[ndx], links, __db_lockobj) {
		len++;
		if (obj->size == sh_obj->lockobj.size &&
		    memcmp(obj->data,
		    SH_DBT_PTR(&sh_obj->lockobj), obj->size) == 0)
			break;
	}

	/*
	 * If we found the object, then we can just return it.  If
	 * we didn't find the object, then we need to create it.
	 */
	if (sh_obj == NULL && create) {
		/* Create new object and then insert it into hash table. */
		part_id = LOCK_PART(region, ndx);
		if ((sh_obj = SH_TAILQ_FIRST(&FREE_OBJS(
		    lt, part_id), __db_lockobj)) == NULL) {
			if ((ret = __lock_allocobj(lt, part_id)) == 0)
				goto retry;
			goto err;
		}

		/*
		 * If we can fit this object in the structure, do so instead
		 * of alloc-ing space for it.
		 */
		if (obj->size <= sizeof(sh_obj->objdata))
			p = sh_obj->objdata;
		else {
			/*
			 * If we have only one partition, the region is locked.
			 */
			if (region->part_t_size != 1)
				LOCK_REGION_LOCK(env);
			if ((ret =
			    __env_alloc(&lt->reginfo, obj->size, &p)) != 0) {
				__db_errx(env,
				    "No space for lock object storage");
				if (region->part_t_size != 1)
					LOCK_REGION_UNLOCK(env);
				goto err;
			}
			if (region->part_t_size != 1)
				LOCK_REGION_UNLOCK(env);
		}

		memcpy(p, obj->data, obj->size);

		SH_TAILQ_REMOVE(&FREE_OBJS(
		    lt, part_id), sh_obj, links, __db_lockobj);
#ifdef HAVE_STATISTICS
		/*
		 * Keep track of both the max number of objects allocated
		 * per partition and the max number of objects used by
		 * this bucket.
		 */
		len++;
		if (++lt->obj_stat[ndx].st_nobjects >
		    lt->obj_stat[ndx].st_maxnobjects)
			lt->obj_stat[ndx].st_maxnobjects =
			    lt->obj_stat[ndx].st_nobjects;
		if (++lt->part_array[part_id].part_stat.st_nobjects >
		    lt->part_array[part_id].part_stat.st_maxnobjects)
			lt->part_array[part_id].part_stat.st_maxnobjects =
			    lt->part_array[part_id].part_stat.st_nobjects;
#endif

		sh_obj->indx = ndx;
		SH_TAILQ_INIT(&sh_obj->waiters);
		SH_TAILQ_INIT(&sh_obj->holders);
		sh_obj->lockobj.size = obj->size;
		sh_obj->lockobj.off =
		    (roff_t)SH_PTR_TO_OFF(&sh_obj->lockobj, p);
		SH_TAILQ_INSERT_HEAD(
		    &lt->obj_tab[ndx], sh_obj, links, __db_lockobj);
	}

#ifdef HAVE_STATISTICS
	if (len > lt->obj_stat[ndx].st_hash_len)
		lt->obj_stat[ndx].st_hash_len = len;
#endif

#ifdef HAVE_STATISTICS
	if (len > lt->obj_stat[ndx].st_hash_len)
		lt->obj_stat[ndx].st_hash_len = len;
#endif

	*retp = sh_obj;
	return (0);

err:	return (ret);
}

/*
 * __lock_is_parent --
 *	Given a locker and a transaction, return 1 if the locker is
 * an ancestor of the designated transaction.  This is used to determine
 * if we should grant locks that appear to conflict, but don't because
 * the lock is already held by an ancestor.
 */
static int
__lock_is_parent(lt, l_off, sh_locker)
	DB_LOCKTAB *lt;
	roff_t l_off;
	DB_LOCKER *sh_locker;
{
	DB_LOCKER *parent;

	parent = sh_locker;
	while (parent->parent_locker != INVALID_ROFF) {
		if (parent->parent_locker == l_off)
			return (1);
		parent = R_ADDR(&lt->reginfo, parent->parent_locker);
	}

	return (0);
}

/*
 * __lock_locker_is_parent --
 *	Determine if "locker" is an ancestor of "child".
 * *retp == 1 if so, 0 otherwise.
 *
 * PUBLIC: int __lock_locker_is_parent
 * PUBLIC:     __P((ENV *, DB_LOCKER *, DB_LOCKER *, int *));
 */
int
__lock_locker_is_parent(env, locker, child, retp)
	ENV *env;
	DB_LOCKER *locker;
	DB_LOCKER *child;
	int *retp;
{
	DB_LOCKTAB *lt;

	lt = env->lk_handle;

	/*
	 * The locker may not exist for this transaction, if not then it has
	 * no parents.
	 */
	if (locker == NULL)
		*retp = 0;
	else
		*retp = __lock_is_parent(lt,
		     R_OFFSET(&lt->reginfo, locker), child);
	return (0);
}

/*
 * __lock_inherit_locks --
 *	Called on child commit to merge child's locks with parent's.
 */
static int
__lock_inherit_locks(lt, sh_locker, flags)
	DB_LOCKTAB *lt;
	DB_LOCKER *sh_locker;
	u_int32_t flags;
{
	DB_LOCKER *sh_parent;
	DB_LOCKOBJ *obj;
	DB_LOCKREGION *region;
	ENV *env;
	int ret;
	struct __db_lock *hlp, *lp;
	roff_t poff;

	env = lt->env;
	region = lt->reginfo.primary;

	/*
	 * Get the committing locker and mark it as deleted.
	 * This allows us to traverse the locker links without
	 * worrying that someone else is deleting locks out
	 * from under us.  However, if the locker doesn't
	 * exist, that just means that the child holds no
	 * locks, so inheritance is easy!
	 */
	if (sh_locker == NULL) {
		__db_errx(env, __db_locker_invalid);
		return (EINVAL);
	}

	/* Make sure we are a child transaction. */
	if (sh_locker->parent_locker == INVALID_ROFF) {
		__db_errx(env, "Not a child transaction");
		return (EINVAL);
	}
	sh_parent = R_ADDR(&lt->reginfo, sh_locker->parent_locker);

	/*
	 * In order to make it possible for a parent to have
	 * many, many children who lock the same objects, and
	 * not require an inordinate number of locks, we try
	 * to merge the child's locks with its parent's.
	 */
	poff = R_OFFSET(&lt->reginfo, sh_parent);
	for (lp = SH_LIST_FIRST(&sh_locker->heldby, __db_lock);
	    lp != NULL;
	    lp = SH_LIST_FIRST(&sh_locker->heldby, __db_lock)) {
		SH_LIST_REMOVE(lp, locker_links, __db_lock);

		/* See if the parent already has a lock. */
		obj = (DB_LOCKOBJ *)((u_int8_t *)lp + lp->obj);
		OBJECT_LOCK_NDX(lt, region, obj->indx);
		SH_TAILQ_FOREACH(hlp, &obj->holders, links, __db_lock)
			if (hlp->holder == poff && lp->mode == hlp->mode)
				break;

		if (hlp != NULL) {
			/* Parent already holds lock. */
			hlp->refcount += lp->refcount;

			/* Remove lock from object list and free it. */
			DB_ASSERT(env, lp->status == DB_LSTAT_HELD);
			SH_TAILQ_REMOVE(&obj->holders, lp, links, __db_lock);
			(void)__lock_freelock(lt, lp, sh_locker, DB_LOCK_FREE);
		} else {
			/* Just move lock to parent chains. */
			SH_LIST_INSERT_HEAD(&sh_parent->heldby,
			    lp, locker_links, __db_lock);
			lp->holder = poff;
		}

		/*
		 * We may need to promote regardless of whether we simply
		 * moved the lock to the parent or changed the parent's
		 * reference count, because there might be a sibling waiting,
		 * who will now be allowed to make forward progress.
		 */
		ret =
		     __lock_promote(lt, obj, NULL, LF_ISSET(DB_LOCK_NOWAITERS));
		OBJECT_UNLOCK(lt, region, obj->indx);
		if (ret != 0)
			return (ret);
	}

	/* Transfer child counts to parent. */
	sh_parent->nlocks += sh_locker->nlocks;
	sh_parent->nwrites += sh_locker->nwrites;

	return (0);
}

/*
 * __lock_promote --
 *
 * Look through the waiters and holders lists and decide which (if any)
 * locks can be promoted.   Promote any that are eligible.
 *
 * PUBLIC: int __lock_promote
 * PUBLIC:    __P((DB_LOCKTAB *, DB_LOCKOBJ *, int *, u_int32_t));
 */
int
__lock_promote(lt, obj, state_changedp, flags)
	DB_LOCKTAB *lt;
	DB_LOCKOBJ *obj;
	int *state_changedp;
	u_int32_t flags;
{
	struct __db_lock *lp_w, *lp_h, *next_waiter;
	DB_LOCKREGION *region;
	int had_waiters, state_changed;

	region = lt->reginfo.primary;
	had_waiters = 0;

	/*
	 * We need to do lock promotion.  We also need to determine if we're
	 * going to need to run the deadlock detector again.  If we release
	 * locks, and there are waiters, but no one gets promoted, then we
	 * haven't fundamentally changed the lockmgr state, so we may still
	 * have a deadlock and we have to run again.  However, if there were
	 * no waiters, or we actually promoted someone, then we are OK and we
	 * don't have to run it immediately.
	 *
	 * During promotion, we look for state changes so we can return this
	 * information to the caller.
	 */

	for (lp_w = SH_TAILQ_FIRST(&obj->waiters, __db_lock),
	    state_changed = lp_w == NULL;
	    lp_w != NULL;
	    lp_w = next_waiter) {
		had_waiters = 1;
		next_waiter = SH_TAILQ_NEXT(lp_w, links, __db_lock);

		/* Waiter may have aborted or expired. */
		if (lp_w->status != DB_LSTAT_WAITING)
			continue;
		/* Are we switching locks? */
		if (LF_ISSET(DB_LOCK_NOWAITERS) && lp_w->mode == DB_LOCK_WAIT)
			continue;

		SH_TAILQ_FOREACH(lp_h, &obj->holders, links, __db_lock) {
			if (lp_h->holder != lp_w->holder &&
			    CONFLICTS(lt, region, lp_h->mode, lp_w->mode)) {
				if (!__lock_is_parent(lt, lp_h->holder,
				     R_ADDR(&lt->reginfo, lp_w->holder)))
					break;
			}
		}
		if (lp_h != NULL)	/* Found a conflict. */
			break;

		/* No conflict, promote the waiting lock. */
		SH_TAILQ_REMOVE(&obj->waiters, lp_w, links, __db_lock);
		lp_w->status = DB_LSTAT_PENDING;
		SH_TAILQ_INSERT_TAIL(&obj->holders, lp_w, links);

		/* Wake up waiter. */
		MUTEX_UNLOCK(lt->env, lp_w->mtx_lock);
		state_changed = 1;
	}

	/*
	 * If this object had waiters and doesn't any more, then we need
	 * to remove it from the dd_obj list.
	 */
	if (had_waiters && SH_TAILQ_FIRST(&obj->waiters, __db_lock) == NULL) {
		LOCK_DD(lt->env, region);
		/*
		 * Bump the generation when removing an object from the
		 * queue so that the deadlock detector will retry.
		 */
		obj->generation++;
		SH_TAILQ_REMOVE(&region->dd_objs, obj, dd_links, __db_lockobj);
		UNLOCK_DD(lt->env, region);
	}

	if (state_changedp != NULL)
		*state_changedp = state_changed;

	return (0);
}

/*
 * __lock_remove_waiter --
 *	Any lock on the waitlist has a process waiting for it.  Therefore,
 * we can't return the lock to the freelist immediately.  Instead, we can
 * remove the lock from the list of waiters, set the status field of the
 * lock, and then let the process waking up return the lock to the
 * free list.
 *
 * This must be called with the Object bucket locked.
 */
static int
__lock_remove_waiter(lt, sh_obj, lockp, status)
	DB_LOCKTAB *lt;
	DB_LOCKOBJ *sh_obj;
	struct __db_lock *lockp;
	db_status_t status;
{
	DB_LOCKREGION *region;
	int do_wakeup;

	region = lt->reginfo.primary;

	do_wakeup = lockp->status == DB_LSTAT_WAITING;

	SH_TAILQ_REMOVE(&sh_obj->waiters, lockp, links, __db_lock);
	lockp->links.stqe_prev = -1;
	lockp->status = status;
	if (SH_TAILQ_FIRST(&sh_obj->waiters, __db_lock) == NULL) {
		LOCK_DD(lt->env, region);
		sh_obj->generation++;
		SH_TAILQ_REMOVE(
		    &region->dd_objs,
		    sh_obj, dd_links, __db_lockobj);
		UNLOCK_DD(lt->env, region);
	}

	/*
	 * Wake whoever is waiting on this lock.
	 */
	if (do_wakeup)
		MUTEX_UNLOCK(lt->env, lockp->mtx_lock);

	return (0);
}

/*
 * __lock_trade --
 *
 * Trade locker ids on a lock.  This is used to reassign file locks from
 * a transactional locker id to a long-lived locker id.  This should be
 * called with the region mutex held.
 */
static int
__lock_trade(env, lock, new_locker)
	ENV *env;
	DB_LOCK *lock;
	DB_LOCKER *new_locker;
{
	struct __db_lock *lp;
	DB_LOCKTAB *lt;
	int ret;

	lt = env->lk_handle;
	lp = R_ADDR(&lt->reginfo, lock->off);

	/* If the lock is already released, simply return. */
	if (lp->gen != lock->gen)
		return (DB_NOTFOUND);

	if (new_locker == NULL) {
		__db_errx(env, "Locker does not exist");
		return (EINVAL);
	}

	/* Remove the lock from its current locker. */
	if ((ret = __lock_freelock(lt,
	    lp, R_ADDR(&lt->reginfo, lp->holder), DB_LOCK_UNLINK)) != 0)
		return (ret);

	/* Add lock to its new locker. */
	SH_LIST_INSERT_HEAD(&new_locker->heldby, lp, locker_links, __db_lock);
	new_locker->nlocks++;
	if (IS_WRITELOCK(lp->mode))
		new_locker->nwrites++;
	lp->holder = R_OFFSET(&lt->reginfo, new_locker);

	return (0);
}
