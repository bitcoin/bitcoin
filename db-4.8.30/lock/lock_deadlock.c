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
#include "dbinc/txn.h"

#define	ISSET_MAP(M, N)	((M)[(N) / 32] & (1 << ((N) % 32)))

#define	CLEAR_MAP(M, N) {						\
	u_int32_t __i;							\
	for (__i = 0; __i < (N); __i++)					\
		(M)[__i] = 0;						\
}

#define	SET_MAP(M, B)	((M)[(B) / 32] |= (1 << ((B) % 32)))
#define	CLR_MAP(M, B)	((M)[(B) / 32] &= ~((u_int)1 << ((B) % 32)))

#define	OR_MAP(D, S, N)	{						\
	u_int32_t __i;							\
	for (__i = 0; __i < (N); __i++)					\
		D[__i] |= S[__i];					\
}
#define	BAD_KILLID	0xffffffff

typedef struct {
	int		valid;
	int		self_wait;
	int		in_abort;
	u_int32_t	count;
	u_int32_t	id;
	roff_t		last_lock;
	roff_t		last_obj;
	u_int32_t	last_ndx;
	u_int32_t	last_locker_id;
	db_pgno_t	pgno;
} locker_info;

static int __dd_abort __P((ENV *, locker_info *, int *));
static int __dd_build __P((ENV *, u_int32_t,
	    u_int32_t **, u_int32_t *, u_int32_t *, locker_info **, int*));
static int __dd_find __P((ENV *,
	    u_int32_t *, locker_info *, u_int32_t, u_int32_t, u_int32_t ***));
static int __dd_isolder __P((u_int32_t, u_int32_t, u_int32_t, u_int32_t));
static int __dd_verify __P((locker_info *, u_int32_t *, u_int32_t *,
	    u_int32_t *, u_int32_t, u_int32_t, u_int32_t));

#ifdef DIAGNOSTIC
static void __dd_debug
	    __P((ENV *, locker_info *, u_int32_t *, u_int32_t, u_int32_t));
#endif

/*
 * __lock_detect_pp --
 *	ENV->lock_detect pre/post processing.
 *
 * PUBLIC: int __lock_detect_pp __P((DB_ENV *, u_int32_t, u_int32_t, int *));
 */
int
__lock_detect_pp(dbenv, flags, atype, rejectp)
	DB_ENV *dbenv;
	u_int32_t flags, atype;
	int *rejectp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lk_handle, "DB_ENV->lock_detect", DB_INIT_LOCK);

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB_ENV->lock_detect", flags, 0)) != 0)
		return (ret);
	switch (atype) {
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
	    "DB_ENV->lock_detect: unknown deadlock detection mode specified");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__lock_detect(env, atype, rejectp)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __lock_detect --
 *	ENV->lock_detect.
 *
 * PUBLIC: int __lock_detect __P((ENV *, u_int32_t, int *));
 */
int
__lock_detect(env, atype, rejectp)
	ENV *env;
	u_int32_t atype;
	int *rejectp;
{
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	db_timespec now;
	locker_info *idmap;
	u_int32_t *bitmap, *copymap, **deadp, **deadlist, *tmpmap;
	u_int32_t i, cid, keeper, killid, limit, nalloc, nlockers;
	u_int32_t lock_max, txn_max;
	int ret, status;

	/*
	 * If this environment is a replication client, then we must use the
	 * MINWRITE detection discipline.
	 */
	if (IS_REP_CLIENT(env))
		atype = DB_LOCK_MINWRITE;

	copymap = tmpmap = NULL;
	deadlist = NULL;

	lt = env->lk_handle;
	if (rejectp != NULL)
		*rejectp = 0;

	/* Check if a detector run is necessary. */

	/* Make a pass only if auto-detect would run. */
	region = lt->reginfo.primary;

	timespecclear(&now);
	if (region->need_dd == 0 &&
	     (!timespecisset(&region->next_timeout) ||
	     !__lock_expired(env, &now, &region->next_timeout))) {
		return (0);
	}
	if (region->need_dd == 0)
		atype = DB_LOCK_EXPIRE;

	/* Reset need_dd, so we know we've run the detector. */
	region->need_dd = 0;

	/* Build the waits-for bitmap. */
	ret = __dd_build(env,
	    atype, &bitmap, &nlockers, &nalloc, &idmap, rejectp);
	lock_max = region->stat.st_cur_maxid;
	if (ret != 0 || atype == DB_LOCK_EXPIRE)
		return (ret);

	/* If there are no lockers, there are no deadlocks. */
	if (nlockers == 0)
		return (0);

#ifdef DIAGNOSTIC
	if (FLD_ISSET(env->dbenv->verbose, DB_VERB_WAITSFOR))
		__dd_debug(env, idmap, bitmap, nlockers, nalloc);
#endif

	/* Now duplicate the bitmaps so we can verify deadlock participants. */
	if ((ret = __os_calloc(env, (size_t)nlockers,
	    sizeof(u_int32_t) * nalloc, &copymap)) != 0)
		goto err;
	memcpy(copymap, bitmap, nlockers * sizeof(u_int32_t) * nalloc);

	if ((ret = __os_calloc(env, sizeof(u_int32_t), nalloc, &tmpmap)) != 0)
		goto err;

	/* Find a deadlock. */
	if ((ret =
	    __dd_find(env, bitmap, idmap, nlockers, nalloc, &deadlist)) != 0)
		return (ret);

	/*
	 * We need the cur_maxid from the txn region as well.  In order
	 * to avoid tricky synchronization between the lock and txn
	 * regions, we simply unlock the lock region and then lock the
	 * txn region.  This introduces a small window during which the
	 * transaction system could then wrap.  We're willing to return
	 * the wrong answer for "oldest" or "youngest" in those rare
	 * circumstances.
	 */
	if (TXN_ON(env)) {
		TXN_SYSTEM_LOCK(env);
		txn_max = ((DB_TXNREGION *)
		    env->tx_handle->reginfo.primary)->cur_maxid;
		TXN_SYSTEM_UNLOCK(env);
	} else
		txn_max = TXN_MAXIMUM;

	killid = BAD_KILLID;
	for (deadp = deadlist; *deadp != NULL; deadp++) {
		if (rejectp != NULL)
			++*rejectp;
		killid = (u_int32_t)(*deadp - bitmap) / nalloc;
		limit = killid;

		/*
		 * There are cases in which our general algorithm will
		 * fail.  Returning 1 from verify indicates that the
		 * particular locker is not only involved in a deadlock,
		 * but that killing him will allow others to make forward
		 * progress.  Unfortunately, there are cases where we need
		 * to abort someone, but killing them will not necessarily
		 * ensure forward progress (imagine N readers all trying to
		 * acquire a write lock).
		 * killid is only set to lockers that pass the db_verify test.
		 * keeper will hold the best candidate even if it does
		 * not pass db_verify.  Once we fill in killid then we do
		 * not need a keeper, but we keep updating it anyway.
		 */

		keeper = idmap[killid].in_abort == 0 ? killid : BAD_KILLID;
		if (keeper == BAD_KILLID ||
		    __dd_verify(idmap, *deadp,
		    tmpmap, copymap, nlockers, nalloc, keeper) == 0)
			killid = BAD_KILLID;

		if (killid != BAD_KILLID &&
		    (atype == DB_LOCK_DEFAULT || atype == DB_LOCK_RANDOM))
			goto dokill;

		/*
		 * Start with the id that we know is deadlocked, then examine
		 * all other set bits and see if any are a better candidate
		 * for abortion and they are genuinely part of the deadlock.
		 * The definition of "best":
		 *	MAXLOCKS: maximum count
		 *	MAXWRITE: maximum write count
		 *	MINLOCKS: minimum count
		 *	MINWRITE: minimum write count
		 *	OLDEST: smallest id
		 *	YOUNGEST: largest id
		 */
		for (i = (limit + 1) % nlockers;
		    i != limit;
		    i = (i + 1) % nlockers) {
			if (!ISSET_MAP(*deadp, i) || idmap[i].in_abort)
				continue;

			/*
			 * Determine if we have a verified candidate
			 * in killid, if not then compare with the
			 * non-verified candidate in keeper.
			 */
			if (killid == BAD_KILLID) {
				if (keeper == BAD_KILLID)
					goto use_next;
				else
					cid = keeper;
			} else
				cid = killid;

			switch (atype) {
			case DB_LOCK_OLDEST:
				if (__dd_isolder(idmap[cid].id,
				    idmap[i].id, lock_max, txn_max))
					continue;
				break;
			case DB_LOCK_YOUNGEST:
				if (__dd_isolder(idmap[i].id,
				    idmap[cid].id, lock_max, txn_max))
					continue;
				break;
			case DB_LOCK_MAXLOCKS:
				if (idmap[i].count < idmap[cid].count)
					continue;
				break;
			case DB_LOCK_MAXWRITE:
				if (idmap[i].count < idmap[cid].count)
					continue;
				break;
			case DB_LOCK_MINLOCKS:
			case DB_LOCK_MINWRITE:
				if (idmap[i].count > idmap[cid].count)
					continue;
				break;
			case DB_LOCK_DEFAULT:
			case DB_LOCK_RANDOM:
				goto dokill;

			default:
				killid = BAD_KILLID;
				ret = EINVAL;
				goto dokill;
			}

use_next:		keeper = i;
			if (__dd_verify(idmap, *deadp,
			    tmpmap, copymap, nlockers, nalloc, i))
				killid = i;
		}

dokill:		if (killid == BAD_KILLID) {
			if (keeper == BAD_KILLID)
				continue;
			else {
				/*
				 * Removing a single locker will not
				 * break the deadlock, signal to run
				 * detection again.
				 */
				region->need_dd = 1;
				killid = keeper;
			}
		}

		/* Kill the locker with lockid idmap[killid]. */
		if ((ret = __dd_abort(env, &idmap[killid], &status)) != 0)
			break;

		/*
		 * It's possible that the lock was already aborted; this isn't
		 * necessarily a problem, so do not treat it as an error. If
		 * the txn was aborting and deadlocked trying to upgrade
		 * a was_write lock, the detector should be run again or
		 * the deadlock might persist.
		 */
		if (status != 0) {
			if (status != DB_ALREADY_ABORTED)
				__db_errx(env,
				    "warning: unable to abort locker %lx",
				    (u_long)idmap[killid].id);
			else 
				region->need_dd = 1;
		} else if (FLD_ISSET(env->dbenv->verbose, DB_VERB_DEADLOCK))
			__db_msg(env,
			    "Aborting locker %lx", (u_long)idmap[killid].id);
	}
err:	if(copymap != NULL)
		__os_free(env, copymap);
	if (deadlist != NULL)
		__os_free(env, deadlist);
	if(tmpmap != NULL)
		__os_free(env, tmpmap);
	__os_free(env, bitmap);
	__os_free(env, idmap);

	return (ret);
}

/*
 * ========================================================================
 * Utilities
 */

#define	DD_INVALID_ID	((u_int32_t) -1)

/*
 * __dd_build --
 *	Build the lock dependency bit maps.
 * Notes on syncronization:  
 *	LOCK_SYSTEM_LOCK is used to hold objects locked when we have
 *		a single partition.
 *	LOCK_LOCKERS is held while we are walking the lockers list and
 *		to single thread the use of lockerp->dd_id.
 *	LOCK_DD protects the DD list of objects.
 */

static int
__dd_build(env, atype, bmp, nlockers, allocp, idmap, rejectp)
	ENV *env;
	u_int32_t atype, **bmp, *nlockers, *allocp;
	locker_info **idmap;
	int *rejectp;
{
	struct __db_lock *lp;
	DB_LOCKER *lip, *lockerp, *child;
	DB_LOCKOBJ *op, *lo, *np;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	locker_info *id_array;
	db_timespec now, min_timeout;
	u_int32_t *bitmap, count, dd;
	u_int32_t *entryp, gen, id, indx, ndx, nentries, *tmpmap;
	u_int8_t *pptr;
	int is_first, ret;

	COMPQUIET(indx, 0);
	lt = env->lk_handle;
	region = lt->reginfo.primary;
	timespecclear(&now);
	timespecclear(&min_timeout);

	/*
	 * While we always check for expired timeouts, if we are called with
	 * DB_LOCK_EXPIRE, then we are only checking for timeouts (i.e., not
	 * doing deadlock detection at all).  If we aren't doing real deadlock
	 * detection, then we can skip a significant, amount of the processing.
	 * In particular we do not build the conflict array and our caller
	 * needs to expect this.
	 */
	LOCK_SYSTEM_LOCK(lt, region);
	if (atype == DB_LOCK_EXPIRE) {
skip:		LOCK_DD(env, region);
		op = SH_TAILQ_FIRST(&region->dd_objs, __db_lockobj);
		for (; op != NULL; op = np) {
			indx = op->indx;
			gen = op->generation;
			UNLOCK_DD(env, region);
			OBJECT_LOCK_NDX(lt, region, indx);
			if (op->generation != gen) {
				OBJECT_UNLOCK(lt, region, indx);
				goto skip;
			}
			SH_TAILQ_FOREACH(lp, &op->waiters, links, __db_lock) {
				lockerp = (DB_LOCKER *)
				    R_ADDR(&lt->reginfo, lp->holder);
				if (lp->status == DB_LSTAT_WAITING) {
					if (__lock_expired(env,
					    &now, &lockerp->lk_expire)) {
						lp->status = DB_LSTAT_EXPIRED;
						MUTEX_UNLOCK(
						    env, lp->mtx_lock);
						if (rejectp != NULL)
							++*rejectp;
						continue;
					}
					if (timespecisset(
					    &lockerp->lk_expire) &&
					    (!timespecisset(&min_timeout) ||
					    timespeccmp(&min_timeout,
					    &lockerp->lk_expire, >)))
						min_timeout =
						    lockerp->lk_expire;
				}
			}
			LOCK_DD(env, region);
			np = SH_TAILQ_NEXT(op, dd_links, __db_lockobj);
			OBJECT_UNLOCK(lt, region, indx);
		}
		UNLOCK_DD(env, region);
		LOCK_SYSTEM_UNLOCK(lt, region);
		goto done;
	}

	/*
	 * Allocate after locking the region
	 * to make sure the structures are large enough.
	 */
	LOCK_LOCKERS(env, region);
	count = region->nlockers;
	if (count == 0) {
		UNLOCK_LOCKERS(env, region);
		*nlockers = 0;
		return (0);
	}

	if (FLD_ISSET(env->dbenv->verbose, DB_VERB_DEADLOCK))
		__db_msg(env, "%lu lockers", (u_long)count);

	nentries = (u_int32_t)DB_ALIGN(count, 32) / 32;

	/* Allocate enough space for a count by count bitmap matrix. */
	if ((ret = __os_calloc(env, (size_t)count,
	    sizeof(u_int32_t) * nentries, &bitmap)) != 0) {
		UNLOCK_LOCKERS(env, region);
		return (ret);
	}

	if ((ret = __os_calloc(env,
	    sizeof(u_int32_t), nentries, &tmpmap)) != 0) {
		UNLOCK_LOCKERS(env, region);
		__os_free(env, bitmap);
		return (ret);
	}

	if ((ret = __os_calloc(env,
	    (size_t)count, sizeof(locker_info), &id_array)) != 0) {
		UNLOCK_LOCKERS(env, region);
		__os_free(env, bitmap);
		__os_free(env, tmpmap);
		return (ret);
	}

	/*
	 * First we go through and assign each locker a deadlock detector id.
	 */
	id = 0;
	SH_TAILQ_FOREACH(lip, &region->lockers, ulinks, __db_locker) {
		if (lip->master_locker == INVALID_ROFF) {
			DB_ASSERT(env, id < count);
			lip->dd_id = id++;
			id_array[lip->dd_id].id = lip->id;
			switch (atype) {
			case DB_LOCK_MINLOCKS:
			case DB_LOCK_MAXLOCKS:
				id_array[lip->dd_id].count = lip->nlocks;
				break;
			case DB_LOCK_MINWRITE:
			case DB_LOCK_MAXWRITE:
				id_array[lip->dd_id].count = lip->nwrites;
				break;
			default:
				break;
			}
		} else
			lip->dd_id = DD_INVALID_ID;

	}

	/*
	 * We only need consider objects that have waiters, so we use
	 * the list of objects with waiters (dd_objs) instead of traversing
	 * the entire hash table.  For each object, we traverse the waiters
	 * list and add an entry in the waitsfor matrix for each waiter/holder
	 * combination. We don't want to lock from the DD mutex to the
	 * hash mutex, so we drop deadlock mutex  and get the hash mutex.  Then
	 * check to see if the object has changed.  Once we have the object
	 * locked then locks cannot be remove and lockers cannot go away.
	 */
	if (0) {
		/* If an object has changed state, start over. */
again:		memset(bitmap, 0, count * sizeof(u_int32_t) * nentries);
	}
	LOCK_DD(env, region);
	op = SH_TAILQ_FIRST(&region->dd_objs, __db_lockobj);
	for (; op != NULL; op = np) {
		indx = op->indx;
		gen = op->generation;
		UNLOCK_DD(env, region);

		OBJECT_LOCK_NDX(lt, region, indx);
		if (gen != op->generation) {
			OBJECT_UNLOCK(lt, region, indx);
			goto again;
		}

		/*
		 * First we go through and create a bit map that
		 * represents all the holders of this object.
		 */

		CLEAR_MAP(tmpmap, nentries);
		SH_TAILQ_FOREACH(lp, &op->holders, links, __db_lock) {
			lockerp = (DB_LOCKER *)R_ADDR(&lt->reginfo, lp->holder);

			if (lockerp->dd_id == DD_INVALID_ID) {
				/*
				 * If the locker was not here when we started,
				 * then it was not deadlocked at that time.
				 */
				if (lockerp->master_locker == INVALID_ROFF)
					continue;
				dd = ((DB_LOCKER *)R_ADDR(&lt->reginfo,
				    lockerp->master_locker))->dd_id;
				if (dd == DD_INVALID_ID)
					continue;
				lockerp->dd_id = dd;
				switch (atype) {
				case DB_LOCK_MINLOCKS:
				case DB_LOCK_MAXLOCKS:
					id_array[dd].count += lockerp->nlocks;
					break;
				case DB_LOCK_MINWRITE:
				case DB_LOCK_MAXWRITE:
					id_array[dd].count += lockerp->nwrites;
					break;
				default:
					break;
				}

			} else
				dd = lockerp->dd_id;
			id_array[dd].valid = 1;

			/*
			 * If the holder has already been aborted, then
			 * we should ignore it for now.
			 */
			if (lp->status == DB_LSTAT_HELD)
				SET_MAP(tmpmap, dd);
		}

		/*
		 * Next, for each waiter, we set its row in the matrix
		 * equal to the map of holders we set up above.
		 */
		for (is_first = 1,
		    lp = SH_TAILQ_FIRST(&op->waiters, __db_lock);
		    lp != NULL;
		    is_first = 0,
		    lp = SH_TAILQ_NEXT(lp, links, __db_lock)) {
			lockerp = (DB_LOCKER *)R_ADDR(&lt->reginfo, lp->holder);
			if (lp->status == DB_LSTAT_WAITING) {
				if (__lock_expired(env,
				    &now, &lockerp->lk_expire)) {
					lp->status = DB_LSTAT_EXPIRED;
					MUTEX_UNLOCK(env, lp->mtx_lock);
					if (rejectp != NULL)
						++*rejectp;
					continue;
				}
				if (timespecisset(&lockerp->lk_expire) &&
				    (!timespecisset(&min_timeout) ||
				    timespeccmp(
				    &min_timeout, &lockerp->lk_expire, >)))
					min_timeout = lockerp->lk_expire;
			}

			if (lockerp->dd_id == DD_INVALID_ID) {
				dd = ((DB_LOCKER *)R_ADDR(&lt->reginfo,
				    lockerp->master_locker))->dd_id;
				lockerp->dd_id = dd;
				switch (atype) {
				case DB_LOCK_MINLOCKS:
				case DB_LOCK_MAXLOCKS:
					id_array[dd].count += lockerp->nlocks;
					break;
				case DB_LOCK_MINWRITE:
				case DB_LOCK_MAXWRITE:
					id_array[dd].count += lockerp->nwrites;
					break;
				default:
					break;
				}
			} else
				dd = lockerp->dd_id;
			id_array[dd].valid = 1;

			/*
			 * If the transaction is pending abortion, then
			 * ignore it on this iteration.
			 */
			if (lp->status != DB_LSTAT_WAITING)
				continue;

			entryp = bitmap + (nentries * dd);
			OR_MAP(entryp, tmpmap, nentries);
			/*
			 * If this is the first waiter on the queue,
			 * then we remove the waitsfor relationship
			 * with oneself.  However, if it's anywhere
			 * else on the queue, then we have to keep
			 * it and we have an automatic deadlock.
			 */
			if (is_first) {
				if (ISSET_MAP(entryp, dd))
					id_array[dd].self_wait = 1;
				CLR_MAP(entryp, dd);
			}
		}
		LOCK_DD(env, region);
		np = SH_TAILQ_NEXT(op, dd_links, __db_lockobj);
		OBJECT_UNLOCK(lt, region, indx);
	}
	UNLOCK_DD(env, region);

	/*
	 * Now for each locker, record its last lock and set abort status.
	 * We need to look at the heldby list carefully.  We have the LOCKERS
	 * locked so they cannot go away.  The lock at the head of the
	 * list can be removed by locking the object it points at.
	 * Since lock memory is not freed if we get a lock we can look
	 * at it safely but SH_LIST_FIRST is not atomic, so we check that
	 * the list has not gone empty during that macro. We check abort
	 * status after building the bit maps so that we will not detect
	 * a blocked transaction without noting that it is already aborting.
	 */
	for (id = 0; id < count; id++) {
		if (!id_array[id].valid)
			continue;
		if ((ret = __lock_getlocker_int(lt,
		    id_array[id].id, 0, &lockerp)) != 0 || lockerp == NULL)
			continue;

		/*
		 * If this is a master transaction, try to
		 * find one of its children's locks first,
		 * as they are probably more recent.
		 */
		child = SH_LIST_FIRST(&lockerp->child_locker, __db_locker);
		if (child != NULL) {
			do {
c_retry:			lp = SH_LIST_FIRST(&child->heldby, __db_lock);
				if (SH_LIST_EMPTY(&child->heldby) || lp == NULL)
					goto c_next;

				if (F_ISSET(child, DB_LOCKER_INABORT))
					id_array[id].in_abort = 1;
				ndx = lp->indx;
				OBJECT_LOCK_NDX(lt, region, ndx);
				if (lp != SH_LIST_FIRST(
				    &child->heldby, __db_lock) ||
				    ndx != lp->indx) {
					OBJECT_UNLOCK(lt, region, ndx);
					goto c_retry;
				}

				if (lp != NULL &&
				    lp->status == DB_LSTAT_WAITING) {
					id_array[id].last_locker_id = child->id;
					goto get_lock;
				} else {
					OBJECT_UNLOCK(lt, region, ndx);
				}
c_next:				child = SH_LIST_NEXT(
				    child, child_link, __db_locker);
			} while (child != NULL);
		}

l_retry:	lp = SH_LIST_FIRST(&lockerp->heldby, __db_lock);
		if (!SH_LIST_EMPTY(&lockerp->heldby) && lp != NULL) {
			ndx = lp->indx;
			OBJECT_LOCK_NDX(lt, region, ndx);
			if (lp != SH_LIST_FIRST(&lockerp->heldby, __db_lock) ||
			    lp->indx != ndx) {
				OBJECT_UNLOCK(lt, region, ndx);
				goto l_retry;
			}
			id_array[id].last_locker_id = lockerp->id;
get_lock:		id_array[id].last_lock = R_OFFSET(&lt->reginfo, lp);
			id_array[id].last_obj = lp->obj;
			lo = (DB_LOCKOBJ *)((u_int8_t *)lp + lp->obj);
			id_array[id].last_ndx = lo->indx;
			pptr = SH_DBT_PTR(&lo->lockobj);
			if (lo->lockobj.size >= sizeof(db_pgno_t))
				memcpy(&id_array[id].pgno,
				    pptr, sizeof(db_pgno_t));
			else
				id_array[id].pgno = 0;
			OBJECT_UNLOCK(lt, region, ndx);
		}
		if (F_ISSET(lockerp, DB_LOCKER_INABORT))
			id_array[id].in_abort = 1;
	}
	UNLOCK_LOCKERS(env, region);
	LOCK_SYSTEM_UNLOCK(lt, region);

	/*
	 * Now we can release everything except the bitmap matrix that we
	 * created.
	 */
	*nlockers = id;
	*idmap = id_array;
	*bmp = bitmap;
	*allocp = nentries;
	__os_free(env, tmpmap);
done:	if (timespecisset(&region->next_timeout))
		region->next_timeout = min_timeout;
	return (0);
}

static int
__dd_find(env, bmp, idmap, nlockers, nalloc, deadp)
	ENV *env;
	u_int32_t *bmp, nlockers, nalloc;
	locker_info *idmap;
	u_int32_t ***deadp;
{
	u_int32_t i, j, k, *mymap, *tmpmap, **retp;
	u_int ndead, ndeadalloc;
	int ret;

#undef	INITIAL_DEAD_ALLOC
#define	INITIAL_DEAD_ALLOC	8

	ndeadalloc = INITIAL_DEAD_ALLOC;
	ndead = 0;
	if ((ret = __os_malloc(env,
	    ndeadalloc * sizeof(u_int32_t *), &retp)) != 0)
		return (ret);

	/*
	 * For each locker, OR in the bits from the lockers on which that
	 * locker is waiting.
	 */
	for (mymap = bmp, i = 0; i < nlockers; i++, mymap += nalloc) {
		if (!idmap[i].valid)
			continue;
		for (j = 0; j < nlockers; j++) {
			if (!ISSET_MAP(mymap, j))
				continue;

			/* Find the map for this bit. */
			tmpmap = bmp + (nalloc * j);
			OR_MAP(mymap, tmpmap, nalloc);
			if (!ISSET_MAP(mymap, i))
				continue;

			/* Make sure we leave room for NULL. */
			if (ndead + 2 >= ndeadalloc) {
				ndeadalloc <<= 1;
				/*
				 * If the alloc fails, then simply return the
				 * deadlocks that we already have.
				 */
				if (__os_realloc(env,
				    ndeadalloc * sizeof(u_int32_t *),
				    &retp) != 0) {
					retp[ndead] = NULL;
					*deadp = retp;
					return (0);
				}
			}
			retp[ndead++] = mymap;

			/* Mark all participants in this deadlock invalid. */
			for (k = 0; k < nlockers; k++)
				if (ISSET_MAP(mymap, k))
					idmap[k].valid = 0;
			break;
		}
	}
	retp[ndead] = NULL;
	*deadp = retp;
	return (0);
}

static int
__dd_abort(env, info, statusp)
	ENV *env;
	locker_info *info;
	int *statusp;
{
	struct __db_lock *lockp;
	DB_LOCKER *lockerp;
	DB_LOCKOBJ *sh_obj;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	int ret;

	*statusp = 0;

	lt = env->lk_handle;
	region = lt->reginfo.primary;
	ret = 0;

	/* We must lock so this locker cannot go away while we abort it. */
	LOCK_SYSTEM_LOCK(lt, region);
	LOCK_LOCKERS(env, region);

	/*
	 * Get the locker.  If it's gone or was aborted while we were
	 * detecting, return that.
	 */
	if ((ret = __lock_getlocker_int(lt,
	    info->last_locker_id, 0, &lockerp)) != 0)
		goto err;
	if (lockerp == NULL || F_ISSET(lockerp, DB_LOCKER_INABORT)) {
		*statusp = DB_ALREADY_ABORTED;
		goto out;
	}

	/*
	 * Find the locker's last lock.  It is possible for this lock to have
	 * been freed, either though a timeout or another detector run.
	 * First lock the lock object so it is stable.
	 */

	OBJECT_LOCK_NDX(lt, region, info->last_ndx);
	if ((lockp = SH_LIST_FIRST(&lockerp->heldby, __db_lock)) == NULL) {
		*statusp = DB_ALREADY_ABORTED;
		goto done;
	}
	if (R_OFFSET(&lt->reginfo, lockp) != info->last_lock ||
	    lockp->holder != R_OFFSET(&lt->reginfo, lockerp) ||
	    F_ISSET(lockerp, DB_LOCKER_INABORT) ||
	    lockp->obj != info->last_obj || lockp->status != DB_LSTAT_WAITING) {
		*statusp = DB_ALREADY_ABORTED;
		goto done;
	}

	sh_obj = (DB_LOCKOBJ *)((u_int8_t *)lockp + lockp->obj);

	/* Abort lock, take it off list, and wake up this lock. */
	lockp->status = DB_LSTAT_ABORTED;
	SH_TAILQ_REMOVE(&sh_obj->waiters, lockp, links, __db_lock);

	/*
	 * Either the waiters list is now empty, in which case we remove
	 * it from dd_objs, or it is not empty, in which case we need to
	 * do promotion.
	 */
	if (SH_TAILQ_FIRST(&sh_obj->waiters, __db_lock) == NULL) {
		LOCK_DD(env, region);
		SH_TAILQ_REMOVE(&region->dd_objs,
		    sh_obj, dd_links, __db_lockobj);
		UNLOCK_DD(env, region);
	} else
		ret = __lock_promote(lt, sh_obj, NULL, 0);
	MUTEX_UNLOCK(env, lockp->mtx_lock);

	STAT(region->stat.st_ndeadlocks++);
done:	OBJECT_UNLOCK(lt, region, info->last_ndx);
err:
out:	UNLOCK_LOCKERS(env, region);
	LOCK_SYSTEM_UNLOCK(lt, region);
	return (ret);
}

#ifdef DIAGNOSTIC
static void
__dd_debug(env, idmap, bitmap, nlockers, nalloc)
	ENV *env;
	locker_info *idmap;
	u_int32_t *bitmap, nlockers, nalloc;
{
	DB_MSGBUF mb;
	u_int32_t i, j, *mymap;

	DB_MSGBUF_INIT(&mb);

	__db_msg(env, "Waitsfor array\nWaiter:\tWaiting on:");
	for (mymap = bitmap, i = 0; i < nlockers; i++, mymap += nalloc) {
		if (!idmap[i].valid)
			continue;

		__db_msgadd(env, &mb,				/* Waiter. */
		    "%lx/%lu:\t", (u_long)idmap[i].id, (u_long)idmap[i].pgno);
		for (j = 0; j < nlockers; j++)
			if (ISSET_MAP(mymap, j))
				__db_msgadd(env,
				    &mb, " %lx", (u_long)idmap[j].id);
		__db_msgadd(env, &mb, " %lu", (u_long)idmap[i].last_lock);
		DB_MSGBUF_FLUSH(env, &mb);
	}
}
#endif

/*
 * Given a bitmap that contains a deadlock, verify that the bit
 * specified in the which parameter indicates a transaction that
 * is actually deadlocked.  Return 1 if really deadlocked, 0 otherwise.
 * deadmap  --  the array that identified the deadlock.
 * tmpmap   --  a copy of the initial bitmaps from the dd_build phase.
 * origmap  --  a temporary bit map into which we can OR things.
 * nlockers --  the number of actual lockers under consideration.
 * nalloc   --  the number of words allocated for the bitmap.
 * which    --  the locker in question.
 */
static int
__dd_verify(idmap, deadmap, tmpmap, origmap, nlockers, nalloc, which)
	locker_info *idmap;
	u_int32_t *deadmap, *tmpmap, *origmap;
	u_int32_t nlockers, nalloc, which;
{
	u_int32_t *tmap;
	u_int32_t j;
	int count;

	memset(tmpmap, 0, sizeof(u_int32_t) * nalloc);

	/*
	 * In order for "which" to be actively involved in
	 * the deadlock, removing him from the evaluation
	 * must remove the deadlock.  So, we OR together everyone
	 * except which; if all the participants still have their
	 * bits set, then the deadlock persists and which does
	 * not participate.  If the deadlock does not persist
	 * then "which" does participate.
	 */
	count = 0;
	for (j = 0; j < nlockers; j++) {
		if (!ISSET_MAP(deadmap, j) || j == which)
			continue;

		/* Find the map for this bit. */
		tmap = origmap + (nalloc * j);

		/*
		 * We special case the first waiter who is also a holder, so
		 * we don't automatically call that a deadlock.  However, if
		 * it really is a deadlock, we need the bit set now so that
		 * we treat the first waiter like other waiters.
		 */
		if (idmap[j].self_wait)
			SET_MAP(tmap, j);
		OR_MAP(tmpmap, tmap, nalloc);
		count++;
	}

	if (count == 1)
		return (1);

	/*
	 * Now check the resulting map and see whether
	 * all participants still have their bit set.
	 */
	for (j = 0; j < nlockers; j++) {
		if (!ISSET_MAP(deadmap, j) || j == which)
			continue;
		if (!ISSET_MAP(tmpmap, j))
			return (1);
	}
	return (0);
}

/*
 * __dd_isolder --
 *
 * Figure out the relative age of two lockers.  We make all lockers
 * older than all transactions, because that's how it's worked
 * historically (because lockers are lower ids).
 */
static int
__dd_isolder(a, b, lock_max, txn_max)
	u_int32_t	a, b;
	u_int32_t	lock_max, txn_max;
{
	u_int32_t max;

	/* Check for comparing lock-id and txnid. */
	if (a <= DB_LOCK_MAXID && b > DB_LOCK_MAXID)
		return (1);
	if (b <= DB_LOCK_MAXID && a > DB_LOCK_MAXID)
		return (0);

	/* In the same space; figure out which one. */
	max = txn_max;
	if (a <= DB_LOCK_MAXID)
		max = lock_max;

	/*
	 * We can't get a 100% correct ordering, because we don't know
	 * where the current interval started and if there were older
	 * lockers outside the interval.  We do the best we can.
	 */

	/*
	 * Check for a wrapped case with ids above max.
	 */
	if (a > max && b < max)
		return (1);
	if (b > max && a < max)
		return (0);

	return (a < b);
}
