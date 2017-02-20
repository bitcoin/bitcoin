/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/db_am.h"

#ifdef HAVE_STATISTICS
static int  __lock_dump_locker
		__P((ENV *, DB_MSGBUF *, DB_LOCKTAB *, DB_LOCKER *));
static int  __lock_dump_object __P((DB_LOCKTAB *, DB_MSGBUF *, DB_LOCKOBJ *));
static int  __lock_print_all __P((ENV *, u_int32_t));
static int  __lock_print_stats __P((ENV *, u_int32_t));
static void __lock_print_header __P((ENV *));
static int  __lock_stat __P((ENV *, DB_LOCK_STAT **, u_int32_t));

/*
 * __lock_stat_pp --
 *	ENV->lock_stat pre/post processing.
 *
 * PUBLIC: int __lock_stat_pp __P((DB_ENV *, DB_LOCK_STAT **, u_int32_t));
 */
int
__lock_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_LOCK_STAT **statp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lk_handle, "DB_ENV->lock_stat", DB_INIT_LOCK);

	if ((ret = __db_fchk(env,
	    "DB_ENV->lock_stat", flags, DB_STAT_CLEAR)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__lock_stat(env, statp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __lock_stat --
 *	ENV->lock_stat.
 */
static int
__lock_stat(env, statp, flags)
	ENV *env;
	DB_LOCK_STAT **statp;
	u_int32_t flags;
{
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	DB_LOCK_STAT *stats, tmp;
	DB_LOCK_HSTAT htmp;
	DB_LOCK_PSTAT ptmp;
	int ret;
	u_int32_t i;
	uintmax_t tmp_wait, tmp_nowait;

	*statp = NULL;
	lt = env->lk_handle;

	if ((ret = __os_umalloc(env, sizeof(*stats), &stats)) != 0)
		return (ret);

	/* Copy out the global statistics. */
	LOCK_REGION_LOCK(env);

	region = lt->reginfo.primary;
	memcpy(stats, &region->stat, sizeof(*stats));
	stats->st_locktimeout = region->lk_timeout;
	stats->st_txntimeout = region->tx_timeout;
	stats->st_id = region->lock_id;
	stats->st_cur_maxid = region->cur_maxid;
	stats->st_nlockers = region->nlockers;
	stats->st_nmodes = region->nmodes;

	for (i = 0; i < region->object_t_size; i++) {
		stats->st_nrequests += lt->obj_stat[i].st_nrequests;
		stats->st_nreleases += lt->obj_stat[i].st_nreleases;
		stats->st_nupgrade += lt->obj_stat[i].st_nupgrade;
		stats->st_ndowngrade += lt->obj_stat[i].st_ndowngrade;
		stats->st_lock_wait += lt->obj_stat[i].st_lock_wait;
		stats->st_lock_nowait += lt->obj_stat[i].st_lock_nowait;
		stats->st_nlocktimeouts += lt->obj_stat[i].st_nlocktimeouts;
		stats->st_ntxntimeouts += lt->obj_stat[i].st_ntxntimeouts;
		if (stats->st_maxhlocks < lt->obj_stat[i].st_maxnlocks)
			stats->st_maxhlocks = lt->obj_stat[i].st_maxnlocks;
		if (stats->st_maxhobjects < lt->obj_stat[i].st_maxnobjects)
			stats->st_maxhobjects = lt->obj_stat[i].st_maxnobjects;
		if (stats->st_hash_len < lt->obj_stat[i].st_hash_len)
			stats->st_hash_len = lt->obj_stat[i].st_hash_len;
		if (LF_ISSET(DB_STAT_CLEAR)) {
			htmp = lt->obj_stat[i];
			memset(&lt->obj_stat[i], 0, sizeof(lt->obj_stat[i]));
			lt->obj_stat[i].st_nlocks = htmp.st_nlocks;
			lt->obj_stat[i].st_maxnlocks = htmp.st_nlocks;
			lt->obj_stat[i].st_nobjects = htmp.st_nobjects;
			lt->obj_stat[i].st_maxnobjects = htmp.st_nobjects;

		}
	}

	for (i = 0; i < region->part_t_size; i++) {
		stats->st_nlocks += lt->part_array[i].part_stat.st_nlocks;
		stats->st_maxnlocks +=
		     lt->part_array[i].part_stat.st_maxnlocks;
		stats->st_nobjects += lt->part_array[i].part_stat.st_nobjects;
		stats->st_maxnobjects +=
		    lt->part_array[i].part_stat.st_maxnobjects;
		stats->st_locksteals +=
		    lt->part_array[i].part_stat.st_locksteals;
		if (stats->st_maxlsteals <
		    lt->part_array[i].part_stat.st_locksteals)
			stats->st_maxlsteals =
			    lt->part_array[i].part_stat.st_locksteals;
		stats->st_objectsteals +=
		    lt->part_array[i].part_stat.st_objectsteals;
		if (stats->st_maxosteals <
		    lt->part_array[i].part_stat.st_objectsteals)
			stats->st_maxosteals =
			    lt->part_array[i].part_stat.st_objectsteals;
		__mutex_set_wait_info(env,
		     lt->part_array[i].mtx_part, &tmp_wait, &tmp_nowait);
		stats->st_part_nowait += tmp_nowait;
		stats->st_part_wait += tmp_wait;
		if (tmp_wait > stats->st_part_max_wait) {
			stats->st_part_max_nowait = tmp_nowait;
			stats->st_part_max_wait = tmp_wait;
		}

		if (LF_ISSET(DB_STAT_CLEAR)) {
			ptmp = lt->part_array[i].part_stat;
			memset(&lt->part_array[i].part_stat,
			    0, sizeof(lt->part_array[i].part_stat));
			lt->part_array[i].part_stat.st_nlocks =
			     ptmp.st_nlocks;
			lt->part_array[i].part_stat.st_maxnlocks =
			     ptmp.st_nlocks;
			lt->part_array[i].part_stat.st_nobjects =
			     ptmp.st_nobjects;
			lt->part_array[i].part_stat.st_maxnobjects =
			     ptmp.st_nobjects;
		}
	}

	__mutex_set_wait_info(env, region->mtx_region,
	    &stats->st_region_wait, &stats->st_region_nowait);
	__mutex_set_wait_info(env, region->mtx_dd,
	    &stats->st_objs_wait, &stats->st_objs_nowait);
	__mutex_set_wait_info(env, region->mtx_lockers,
	    &stats->st_lockers_wait, &stats->st_lockers_nowait);
	stats->st_regsize = lt->reginfo.rp->size;
	if (LF_ISSET(DB_STAT_CLEAR)) {
		tmp = region->stat;
		memset(&region->stat, 0, sizeof(region->stat));
		if (!LF_ISSET(DB_STAT_SUBSYSTEM)) {
			__mutex_clear(env, region->mtx_region);
			__mutex_clear(env, region->mtx_dd);
			__mutex_clear(env, region->mtx_lockers);
			for (i = 0; i < region->part_t_size; i++)
				__mutex_clear(env, lt->part_array[i].mtx_part);
		}

		region->stat.st_maxlocks = tmp.st_maxlocks;
		region->stat.st_maxlockers = tmp.st_maxlockers;
		region->stat.st_maxobjects = tmp.st_maxobjects;
		region->stat.st_nlocks =
		    region->stat.st_maxnlocks = tmp.st_nlocks;
		region->stat.st_maxnlockers = region->nlockers;
		region->stat.st_nobjects =
		    region->stat.st_maxnobjects = tmp.st_nobjects;
		region->stat.st_partitions = tmp.st_partitions;
	}

	LOCK_REGION_UNLOCK(env);

	*statp = stats;
	return (0);
}

/*
 * __lock_stat_print_pp --
 *	ENV->lock_stat_print pre/post processing.
 *
 * PUBLIC: int __lock_stat_print_pp __P((DB_ENV *, u_int32_t));
 */
int
__lock_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lk_handle, "DB_ENV->lock_stat_print", DB_INIT_LOCK);

#define	DB_STAT_LOCK_FLAGS						\
	(DB_STAT_ALL | DB_STAT_CLEAR | DB_STAT_LOCK_CONF |		\
	 DB_STAT_LOCK_LOCKERS |	DB_STAT_LOCK_OBJECTS | DB_STAT_LOCK_PARAMS)
	if ((ret = __db_fchk(env, "DB_ENV->lock_stat_print",
	    flags, DB_STAT_CLEAR | DB_STAT_LOCK_FLAGS)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__lock_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __lock_stat_print --
 *	ENV->lock_stat_print method.
 *
 * PUBLIC: int  __lock_stat_print __P((ENV *, u_int32_t));
 */
int
__lock_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	u_int32_t orig_flags;
	int ret;

	orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR | DB_STAT_SUBSYSTEM);
	if (flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __lock_print_stats(env, orig_flags);
		if (flags == 0 || ret != 0)
			return (ret);
	}

	if (LF_ISSET(DB_STAT_ALL | DB_STAT_LOCK_CONF | DB_STAT_LOCK_LOCKERS |
	    DB_STAT_LOCK_OBJECTS | DB_STAT_LOCK_PARAMS) &&
	    (ret = __lock_print_all(env, orig_flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __lock_print_stats --
 *	Display default lock region statistics.
 */
static int
__lock_print_stats(env, flags)
	ENV *env;
	u_int32_t flags;
{
	DB_LOCK_STAT *sp;
	int ret;

#ifdef LOCK_DIAGNOSTIC
	DB_LOCKTAB *lt;
	DB_LOCKREGION *region;
	u_int32_t i;
	u_int32_t wait, nowait;

	lt = env->lk_handle;
	region = lt->reginfo.primary;

	for (i = 0; i < region->object_t_size; i++) {
		if (lt->obj_stat[i].st_hash_len == 0)
			continue;
		__db_dl(env,
		    "Hash bucket", (u_long)i);
		__db_dl(env, "Partition", (u_long)LOCK_PART(region, i));
		__mutex_set_wait_info(env,
		    lt->part_array[LOCK_PART(region, i)].mtx_part,
		    &wait, &nowait);
		__db_dl_pct(env,
	    "The number of partition mutex requests that required waiting",
		    (u_long)wait, DB_PCT(wait, wait + nowait), NULL);
		__db_dl(env,
		    "Maximum hash bucket length",
		    (u_long)lt->obj_stat[i].st_hash_len);
		__db_dl(env,
		    "Total number of locks requested",
		    (u_long)lt->obj_stat[i].st_nrequests);
		__db_dl(env,
		    "Total number of locks released",
		    (u_long)lt->obj_stat[i].st_nreleases);
		__db_dl(env,
		    "Total number of locks upgraded",
		    (u_long)lt->obj_stat[i].st_nupgrade);
		__db_dl(env,
		    "Total number of locks downgraded",
		    (u_long)lt->obj_stat[i].st_ndowngrade);
		__db_dl(env,
	  "Lock requests not available due to conflicts, for which we waited",
		    (u_long)lt->obj_stat[i].st_lock_wait);
		__db_dl(env,
  "Lock requests not available due to conflicts, for which we did not wait",
		    (u_long)lt->obj_stat[i].st_lock_nowait);
		__db_dl(env, "Number of locks that have timed out",
		    (u_long)lt->obj_stat[i].st_nlocktimeouts);
		__db_dl(env, "Number of transactions that have timed out",
		    (u_long)lt->obj_stat[i].st_ntxntimeouts);
	}
#endif
	if ((ret = __lock_stat(env, &sp, flags)) != 0)
		return (ret);

	if (LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default locking region information:");
	__db_dl(env, "Last allocated locker ID", (u_long)sp->st_id);
	__db_msg(env, "%#lx\tCurrent maximum unused locker ID",
	    (u_long)sp->st_cur_maxid);
	__db_dl(env, "Number of lock modes", (u_long)sp->st_nmodes);
	__db_dl(env,
	    "Maximum number of locks possible", (u_long)sp->st_maxlocks);
	__db_dl(env,
	    "Maximum number of lockers possible", (u_long)sp->st_maxlockers);
	__db_dl(env, "Maximum number of lock objects possible",
	    (u_long)sp->st_maxobjects);
	__db_dl(env, "Number of lock object partitions",
	    (u_long)sp->st_partitions);
	__db_dl(env, "Number of current locks", (u_long)sp->st_nlocks);
	__db_dl(env, "Maximum number of locks at any one time",
	    (u_long)sp->st_maxnlocks);
	__db_dl(env, "Maximum number of locks in any one bucket",
	    (u_long)sp->st_maxhlocks);
	__db_dl(env, "Maximum number of locks stolen by for an empty partition",
	    (u_long)sp->st_locksteals);
	__db_dl(env, "Maximum number of locks stolen for any one partition",
	    (u_long)sp->st_maxlsteals);
	__db_dl(env, "Number of current lockers", (u_long)sp->st_nlockers);
	__db_dl(env, "Maximum number of lockers at any one time",
	    (u_long)sp->st_maxnlockers);
	__db_dl(env,
	    "Number of current lock objects", (u_long)sp->st_nobjects);
	__db_dl(env, "Maximum number of lock objects at any one time",
	    (u_long)sp->st_maxnobjects);
	__db_dl(env, "Maximum number of lock objects in any one bucket",
	    (u_long)sp->st_maxhobjects);
	__db_dl(env,
	    "Maximum number of objects stolen by for an empty partition",
	    (u_long)sp->st_objectsteals);
	__db_dl(env, "Maximum number of objects stolen for any one partition",
	    (u_long)sp->st_maxosteals);
	__db_dl(env,
	    "Total number of locks requested", (u_long)sp->st_nrequests);
	__db_dl(env,
	    "Total number of locks released", (u_long)sp->st_nreleases);
	__db_dl(env,
	    "Total number of locks upgraded", (u_long)sp->st_nupgrade);
	__db_dl(env,
	    "Total number of locks downgraded", (u_long)sp->st_ndowngrade);
	__db_dl(env,
	  "Lock requests not available due to conflicts, for which we waited",
	    (u_long)sp->st_lock_wait);
	__db_dl(env,
  "Lock requests not available due to conflicts, for which we did not wait",
	    (u_long)sp->st_lock_nowait);
	__db_dl(env, "Number of deadlocks", (u_long)sp->st_ndeadlocks);
	__db_dl(env, "Lock timeout value", (u_long)sp->st_locktimeout);
	__db_dl(env, "Number of locks that have timed out",
	    (u_long)sp->st_nlocktimeouts);
	__db_dl(env,
	    "Transaction timeout value", (u_long)sp->st_txntimeout);
	__db_dl(env, "Number of transactions that have timed out",
	    (u_long)sp->st_ntxntimeouts);

	__db_dlbytes(env, "The size of the lock region",
	    (u_long)0, (u_long)0, (u_long)sp->st_regsize);
	__db_dl_pct(env,
	    "The number of partition locks that required waiting",
	    (u_long)sp->st_part_wait, DB_PCT(
	    sp->st_part_wait, sp->st_part_wait + sp->st_part_nowait), NULL);
	__db_dl_pct(env,
    "The maximum number of times any partition lock was waited for",
	    (u_long)sp->st_part_max_wait, DB_PCT(sp->st_part_max_wait,
	    sp->st_part_max_wait + sp->st_part_max_nowait), NULL);
	__db_dl_pct(env,
	    "The number of object queue operations that required waiting",
	    (u_long)sp->st_objs_wait, DB_PCT(sp->st_objs_wait,
	    sp->st_objs_wait + sp->st_objs_nowait), NULL);
	__db_dl_pct(env,
	    "The number of locker allocations that required waiting",
	    (u_long)sp->st_lockers_wait, DB_PCT(sp->st_lockers_wait,
	    sp->st_lockers_wait + sp->st_lockers_nowait), NULL);
	__db_dl_pct(env,
	    "The number of region locks that required waiting",
	    (u_long)sp->st_region_wait, DB_PCT(sp->st_region_wait,
	    sp->st_region_wait + sp->st_region_nowait), NULL);
	__db_dl(env, "Maximum hash bucket length",
	    (u_long)sp->st_hash_len);

	__os_ufree(env, sp);

	return (0);
}

/*
 * __lock_print_all --
 *	Display debugging lock region statistics.
 */
static int
__lock_print_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	DB_LOCKER *lip;
	DB_LOCKOBJ *op;
	DB_LOCKREGION *lrp;
	DB_LOCKTAB *lt;
	DB_MSGBUF mb;
	int i, j;
	u_int32_t k;

	lt = env->lk_handle;
	lrp = lt->reginfo.primary;
	DB_MSGBUF_INIT(&mb);

	LOCK_REGION_LOCK(env);
	__db_print_reginfo(env, &lt->reginfo, "Lock", flags);

	if (LF_ISSET(DB_STAT_ALL | DB_STAT_LOCK_PARAMS)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Lock region parameters:");
		__mutex_print_debug_single(env,
		    "Lock region region mutex", lrp->mtx_region, flags);
		STAT_ULONG("locker table size", lrp->locker_t_size);
		STAT_ULONG("object table size", lrp->object_t_size);
		STAT_ULONG("obj_off", lrp->obj_off);
		STAT_ULONG("locker_off", lrp->locker_off);
		STAT_ULONG("need_dd", lrp->need_dd);
		if (timespecisset(&lrp->next_timeout)) {
#ifdef HAVE_STRFTIME
			time_t t = (time_t)lrp->next_timeout.tv_sec;
			char tbuf[64];
			if (strftime(tbuf, sizeof(tbuf),
			    "%m-%d-%H:%M:%S", localtime(&t)) != 0)
				__db_msg(env, "next_timeout: %s.%09lu",
				     tbuf, (u_long)lrp->next_timeout.tv_nsec);
			else
#endif
				__db_msg(env, "next_timeout: %lu.%09lu",
				     (u_long)lrp->next_timeout.tv_sec,
				     (u_long)lrp->next_timeout.tv_nsec);
		}
	}

	if (LF_ISSET(DB_STAT_ALL | DB_STAT_LOCK_CONF)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Lock conflict matrix:");
		for (i = 0; i < lrp->stat.st_nmodes; i++) {
			for (j = 0; j < lrp->stat.st_nmodes; j++)
				__db_msgadd(env, &mb, "%lu\t", (u_long)
				    lt->conflicts[i * lrp->stat.st_nmodes + j]);
			DB_MSGBUF_FLUSH(env, &mb);
		}
	}
	LOCK_REGION_UNLOCK(env);

	if (LF_ISSET(DB_STAT_ALL | DB_STAT_LOCK_LOCKERS)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Locks grouped by lockers:");
		__lock_print_header(env);
		LOCK_LOCKERS(env, lrp);
		for (k = 0; k < lrp->locker_t_size; k++)
			SH_TAILQ_FOREACH(
			    lip, &lt->locker_tab[k], links, __db_locker)
				(void)__lock_dump_locker(env, &mb, lt, lip);
		UNLOCK_LOCKERS(env, lrp);
	}

	if (LF_ISSET(DB_STAT_ALL | DB_STAT_LOCK_OBJECTS)) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Locks grouped by object:");
		__lock_print_header(env);
		for (k = 0; k < lrp->object_t_size; k++) {
			OBJECT_LOCK_NDX(lt, lrp, k);
			SH_TAILQ_FOREACH(
			    op, &lt->obj_tab[k], links, __db_lockobj) {
				(void)__lock_dump_object(lt, &mb, op);
				__db_msg(env, "%s", "");
			}
			OBJECT_UNLOCK(lt, lrp, k);
		}
	}

	return (0);
}

static int
__lock_dump_locker(env, mbp, lt, lip)
	ENV *env;
	DB_MSGBUF *mbp;
	DB_LOCKTAB *lt;
	DB_LOCKER *lip;
{
	DB_LOCKREGION *lrp;
	struct __db_lock *lp;
	char buf[DB_THREADID_STRLEN];
	u_int32_t ndx;

	lrp = lt->reginfo.primary;

	__db_msgadd(env,
	    mbp, "%8lx dd=%2ld locks held %-4d write locks %-4d pid/thread %s",
	    (u_long)lip->id, (long)lip->dd_id, lip->nlocks, lip->nwrites,
	    env->dbenv->thread_id_string(env->dbenv, lip->pid, lip->tid, buf));
	if (timespecisset(&lip->tx_expire)) {
#ifdef HAVE_STRFTIME
		time_t t = (time_t)lip->tx_expire.tv_sec;
		char tbuf[64];
		if (strftime(tbuf, sizeof(tbuf),
		    "%m-%d-%H:%M:%S", localtime(&t)) != 0)
			__db_msgadd(env, mbp, "expires %s.%09lu",
			    tbuf, (u_long)lip->tx_expire.tv_nsec);
		else
#endif
			__db_msgadd(env, mbp, "expires %lu.%09lu",
			    (u_long)lip->tx_expire.tv_sec,
			    (u_long)lip->tx_expire.tv_nsec);
	}
	if (F_ISSET(lip, DB_LOCKER_TIMEOUT))
		__db_msgadd(
		    env, mbp, " lk timeout %lu", (u_long)lip->lk_timeout);
	if (timespecisset(&lip->lk_expire)) {
#ifdef HAVE_STRFTIME
		time_t t = (time_t)lip->lk_expire.tv_sec;
		char tbuf[64];
		if (strftime(tbuf,
		    sizeof(tbuf), "%m-%d-%H:%M:%S", localtime(&t)) != 0)
			__db_msgadd(env, mbp, " lk expires %s.%09lu",
			    tbuf, (u_long)lip->lk_expire.tv_nsec);
		else
#endif
			__db_msgadd(env, mbp, " lk expires %lu.%09lu",
			    (u_long)lip->lk_expire.tv_sec,
			    (u_long)lip->lk_expire.tv_nsec);
	}
	DB_MSGBUF_FLUSH(env, mbp);

	/*
	 * We need some care here since the list may change while we
	 * look.
	 */
retry:	SH_LIST_FOREACH(lp, &lip->heldby, locker_links, __db_lock) {
		if (!SH_LIST_EMPTY(&lip->heldby) && lp != NULL) {
			ndx = lp->indx;
			OBJECT_LOCK_NDX(lt, lrp, ndx);
			if (lp->indx == ndx)
				__lock_printlock(lt, mbp, lp, 1);
			else {
				OBJECT_UNLOCK(lt, lrp, ndx);
				goto retry;
			}
			OBJECT_UNLOCK(lt, lrp, ndx);
		}
	}
	return (0);
}

static int
__lock_dump_object(lt, mbp, op)
	DB_LOCKTAB *lt;
	DB_MSGBUF *mbp;
	DB_LOCKOBJ *op;
{
	struct __db_lock *lp;

	SH_TAILQ_FOREACH(lp, &op->holders, links, __db_lock)
		__lock_printlock(lt, mbp, lp, 1);
	SH_TAILQ_FOREACH(lp, &op->waiters, links, __db_lock)
		__lock_printlock(lt, mbp, lp, 1);
	return (0);
}

/*
 * __lock_print_header --
 */
static void
__lock_print_header(env)
	ENV *env;
{
	__db_msg(env, "%-8s %-10s%-4s %-7s %s",
	    "Locker", "Mode",
	    "Count", "Status", "----------------- Object ---------------");
}

/*
 * __lock_printlock --
 *
 * PUBLIC: void __lock_printlock
 * PUBLIC:     __P((DB_LOCKTAB *, DB_MSGBUF *mbp, struct __db_lock *, int));
 */
void
__lock_printlock(lt, mbp, lp, ispgno)
	DB_LOCKTAB *lt;
	DB_MSGBUF *mbp;
	struct __db_lock *lp;
	int ispgno;
{
	DB_LOCKOBJ *lockobj;
	DB_MSGBUF mb;
	ENV *env;
	db_pgno_t pgno;
	u_int32_t *fidp, type;
	u_int8_t *ptr;
	char *fname, *dname, *p, namebuf[26];
	const char *mode, *status;

	env = lt->env;

	if (mbp == NULL) {
		DB_MSGBUF_INIT(&mb);
		mbp = &mb;
	}

	switch (lp->mode) {
	case DB_LOCK_IREAD:
		mode = "IREAD";
		break;
	case DB_LOCK_IWR:
		mode = "IWR";
		break;
	case DB_LOCK_IWRITE:
		mode = "IWRITE";
		break;
	case DB_LOCK_NG:
		mode = "NG";
		break;
	case DB_LOCK_READ:
		mode = "READ";
		break;
	case DB_LOCK_READ_UNCOMMITTED:
		mode = "READ_UNCOMMITTED";
		break;
	case DB_LOCK_WRITE:
		mode = "WRITE";
		break;
	case DB_LOCK_WWRITE:
		mode = "WAS_WRITE";
		break;
	case DB_LOCK_WAIT:
		mode = "WAIT";
		break;
	default:
		mode = "UNKNOWN";
		break;
	}
	switch (lp->status) {
	case DB_LSTAT_ABORTED:
		status = "ABORT";
		break;
	case DB_LSTAT_EXPIRED:
		status = "EXPIRED";
		break;
	case DB_LSTAT_FREE:
		status = "FREE";
		break;
	case DB_LSTAT_HELD:
		status = "HELD";
		break;
	case DB_LSTAT_PENDING:
		status = "PENDING";
		break;
	case DB_LSTAT_WAITING:
		status = "WAIT";
		break;
	default:
		status = "UNKNOWN";
		break;
	}
	__db_msgadd(env, mbp, "%8lx %-10s %4lu %-7s ",
	    (u_long)((DB_LOCKER *)R_ADDR(&lt->reginfo, lp->holder))->id,
	    mode, (u_long)lp->refcount, status);

	lockobj = (DB_LOCKOBJ *)((u_int8_t *)lp + lp->obj);
	ptr = SH_DBT_PTR(&lockobj->lockobj);
	if (ispgno && lockobj->lockobj.size == sizeof(struct __db_ilock)) {
		/* Assume this is a DBT lock. */
		memcpy(&pgno, ptr, sizeof(db_pgno_t));
		fidp = (u_int32_t *)(ptr + sizeof(db_pgno_t));
		type = *(u_int32_t *)(ptr + sizeof(db_pgno_t) + DB_FILE_ID_LEN);
		(void)__dbreg_get_name(
		    lt->env, (u_int8_t *)fidp, &fname, &dname);
		if (fname == NULL && dname == NULL)
			__db_msgadd(env, mbp, "(%lx %lx %lx %lx %lx) ",
			    (u_long)fidp[0], (u_long)fidp[1], (u_long)fidp[2],
			    (u_long)fidp[3], (u_long)fidp[4]);
		else {
			if (fname != NULL && dname != NULL) {
				(void)snprintf(namebuf, sizeof(namebuf),
				    "%14s:%-10s", fname, dname);
				p = namebuf;
			} else if (fname != NULL)
				p = fname;
			else
				p = dname;
			__db_msgadd(env, mbp, "%-25s ", p);
		}
		__db_msgadd(env, mbp, "%-7s %7lu",
			type == DB_PAGE_LOCK ? "page" :
			type == DB_RECORD_LOCK ? "record" : "handle",
			(u_long)pgno);
	} else {
		__db_msgadd(env, mbp, "0x%lx ",
		    (u_long)R_OFFSET(&lt->reginfo, lockobj));
		__db_prbytes(env, mbp, ptr, lockobj->lockobj.size);
	}
	DB_MSGBUF_FLUSH(env, mbp);
}

#else /* !HAVE_STATISTICS */

int
__lock_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_LOCK_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}

int
__lock_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}
#endif
