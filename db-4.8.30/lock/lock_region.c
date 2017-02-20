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

static int  __lock_region_init __P((ENV *, DB_LOCKTAB *));
static size_t
	    __lock_region_size __P((ENV *));

/*
 * The conflict arrays are set up such that the row is the lock you are
 * holding and the column is the lock that is desired.
 */
#define	DB_LOCK_RIW_N	9
static const u_int8_t db_riw_conflicts[] = {
/*         N   R   W   WT  IW  IR  RIW DR  WW */
/*   N */  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*   R */  0,  0,  1,  0,  1,  0,  1,  0,  1,
/*   W */  0,  1,  1,  1,  1,  1,  1,  1,  1,
/*  WT */  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  IW */  0,  1,  1,  0,  0,  0,  0,  1,  1,
/*  IR */  0,  0,  1,  0,  0,  0,  0,  0,  1,
/* RIW */  0,  1,  1,  0,  0,  0,  0,  1,  1,
/*  DR */  0,  0,  1,  0,  1,  0,  1,  0,  0,
/*  WW */  0,  1,  1,  0,  1,  1,  1,  0,  1
};

/*
 * This conflict array is used for concurrent db access (CDB).  It uses
 * the same locks as the db_riw_conflicts array, but adds an IW mode to
 * be used for write cursors.
 */
#define	DB_LOCK_CDB_N	5
static const u_int8_t db_cdb_conflicts[] = {
	/*		N	R	W	WT	IW */
	/*   N */	0,	0,	0,	0,	0,
	/*   R */	0,	0,	1,	0,	0,
	/*   W */	0,	1,	1,	1,	1,
	/*  WT */	0,	0,	0,	0,	0,
	/*  IW */	0,	0,	1,	0,	1
};

/*
 * __lock_open --
 *	Internal version of lock_open: only called from ENV->open.
 *
 * PUBLIC: int __lock_open __P((ENV *, int));
 */
int
__lock_open(env, create_ok)
	ENV *env;
	int create_ok;
{
	DB_ENV *dbenv;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	size_t size;
	int region_locked, ret;

	dbenv = env->dbenv;
	region_locked = 0;

	/* Create the lock table structure. */
	if ((ret = __os_calloc(env, 1, sizeof(DB_LOCKTAB), &lt)) != 0)
		return (ret);
	lt->env = env;

	/* Join/create the lock region. */
	lt->reginfo.env = env;
	lt->reginfo.type = REGION_TYPE_LOCK;
	lt->reginfo.id = INVALID_REGION_ID;
	lt->reginfo.flags = REGION_JOIN_OK;
	if (create_ok)
		F_SET(&lt->reginfo, REGION_CREATE_OK);

	/* Make sure there is at least one object and lock per partition. */
	if (dbenv->lk_max_objects < dbenv->lk_partitions)
		dbenv->lk_max_objects = dbenv->lk_partitions;
	if (dbenv->lk_max < dbenv->lk_partitions)
		dbenv->lk_max = dbenv->lk_partitions;
	size = __lock_region_size(env);
	if ((ret = __env_region_attach(env, &lt->reginfo, size)) != 0)
		goto err;

	/* If we created the region, initialize it. */
	if (F_ISSET(&lt->reginfo, REGION_CREATE))
		if ((ret = __lock_region_init(env, lt)) != 0)
			goto err;

	/* Set the local addresses. */
	region = lt->reginfo.primary =
	    R_ADDR(&lt->reginfo, lt->reginfo.rp->primary);

	/* Set remaining pointers into region. */
	lt->conflicts = R_ADDR(&lt->reginfo, region->conf_off);
	lt->obj_tab = R_ADDR(&lt->reginfo, region->obj_off);
#ifdef HAVE_STATISTICS
	lt->obj_stat = R_ADDR(&lt->reginfo, region->stat_off);
#endif
	lt->part_array = R_ADDR(&lt->reginfo, region->part_off);
	lt->locker_tab = R_ADDR(&lt->reginfo, region->locker_off);

	env->lk_handle = lt;

	LOCK_REGION_LOCK(env);
	region_locked = 1;

	if (dbenv->lk_detect != DB_LOCK_NORUN) {
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
		    dbenv->lk_detect != DB_LOCK_DEFAULT &&
		    region->detect != dbenv->lk_detect) {
			__db_errx(env,
		    "lock_open: incompatible deadlock detector mode");
			ret = EINVAL;
			goto err;
		}
		if (region->detect == DB_LOCK_NORUN)
			region->detect = dbenv->lk_detect;
	}

	/*
	 * A process joining the region may have reset the lock and transaction
	 * timeouts.
	 */
	if (dbenv->lk_timeout != 0)
		region->lk_timeout = dbenv->lk_timeout;
	if (dbenv->tx_timeout != 0)
		region->tx_timeout = dbenv->tx_timeout;

	LOCK_REGION_UNLOCK(env);
	region_locked = 0;

	return (0);

err:	if (lt->reginfo.addr != NULL) {
		if (region_locked)
			LOCK_REGION_UNLOCK(env);
		(void)__env_region_detach(env, &lt->reginfo, 0);
	}
	env->lk_handle = NULL;

	__os_free(env, lt);
	return (ret);
}

/*
 * __lock_region_init --
 *	Initialize the lock region.
 */
static int
__lock_region_init(env, lt)
	ENV *env;
	DB_LOCKTAB *lt;
{
	const u_int8_t *lk_conflicts;
	struct __db_lock *lp;
	DB_ENV *dbenv;
	DB_LOCKER *lidp;
	DB_LOCKOBJ *op;
	DB_LOCKREGION *region;
	DB_LOCKPART *part;
	u_int32_t extra_locks, extra_objects, i, j, max;
	u_int8_t *addr;
	int lk_modes, ret;

	dbenv = env->dbenv;

	if ((ret = __env_alloc(&lt->reginfo,
	    sizeof(DB_LOCKREGION), &lt->reginfo.primary)) != 0)
		goto mem_err;
	lt->reginfo.rp->primary = R_OFFSET(&lt->reginfo, lt->reginfo.primary);
	region = lt->reginfo.primary;
	memset(region, 0, sizeof(*region));

	if ((ret = __mutex_alloc(
	    env, MTX_LOCK_REGION, 0, &region->mtx_region)) != 0)
		return (ret);

	/* Select a conflict matrix if none specified. */
	if (dbenv->lk_modes == 0)
		if (CDB_LOCKING(env)) {
			lk_modes = DB_LOCK_CDB_N;
			lk_conflicts = db_cdb_conflicts;
		} else {
			lk_modes = DB_LOCK_RIW_N;
			lk_conflicts = db_riw_conflicts;
		}
	else {
		lk_modes = dbenv->lk_modes;
		lk_conflicts = dbenv->lk_conflicts;
	}

	region->need_dd = 0;
	timespecclear(&region->next_timeout);
	region->detect = DB_LOCK_NORUN;
	region->lk_timeout = dbenv->lk_timeout;
	region->tx_timeout = dbenv->tx_timeout;
	region->locker_t_size = __db_tablesize(dbenv->lk_max_lockers);
	region->object_t_size = __db_tablesize(dbenv->lk_max_objects);
	region->part_t_size = dbenv->lk_partitions;
	region->lock_id = 0;
	region->cur_maxid = DB_LOCK_MAXID;
	region->nmodes = lk_modes;
	memset(&region->stat, 0, sizeof(region->stat));
	region->stat.st_maxlocks = dbenv->lk_max;
	region->stat.st_maxlockers = dbenv->lk_max_lockers;
	region->stat.st_maxobjects = dbenv->lk_max_objects;
	region->stat.st_partitions = dbenv->lk_partitions;

	/* Allocate room for the conflict matrix and initialize it. */
	if ((ret = __env_alloc(
	    &lt->reginfo, (size_t)(lk_modes * lk_modes), &addr)) != 0)
		goto mem_err;
	memcpy(addr, lk_conflicts, (size_t)(lk_modes * lk_modes));
	region->conf_off = R_OFFSET(&lt->reginfo, addr);

	/* Allocate room for the object hash table and initialize it. */
	if ((ret = __env_alloc(&lt->reginfo,
	    region->object_t_size * sizeof(DB_HASHTAB), &addr)) != 0)
		goto mem_err;
	__db_hashinit(addr, region->object_t_size);
	region->obj_off = R_OFFSET(&lt->reginfo, addr);

	/* Allocate room for the object hash stats table and initialize it. */
	if ((ret = __env_alloc(&lt->reginfo,
	    region->object_t_size * sizeof(DB_LOCK_HSTAT), &addr)) != 0)
		goto mem_err;
	memset(addr, 0, region->object_t_size * sizeof(DB_LOCK_HSTAT));
	region->stat_off = R_OFFSET(&lt->reginfo, addr);

	/* Allocate room for the partition table and initialize its mutexes. */
	if ((ret = __env_alloc(&lt->reginfo,
	    region->part_t_size * sizeof(DB_LOCKPART), &part)) != 0)
		goto mem_err;
	memset(part, 0, region->part_t_size * sizeof(DB_LOCKPART));
	region->part_off = R_OFFSET(&lt->reginfo, part);
	for (i = 0; i < region->part_t_size; i++) {
		if ((ret = __mutex_alloc(
		    env, MTX_LOCK_REGION, 0, &part[i].mtx_part)) != 0)
			return (ret);
	}
	if ((ret = __mutex_alloc(
	    env, MTX_LOCK_REGION, 0, &region->mtx_dd)) != 0)
		return (ret);

	if ((ret = __mutex_alloc(
	    env, MTX_LOCK_REGION, 0, &region->mtx_lockers)) != 0)
		return (ret);

	/* Allocate room for the locker hash table and initialize it. */
	if ((ret = __env_alloc(&lt->reginfo,
	    region->locker_t_size * sizeof(DB_HASHTAB), &addr)) != 0)
		goto mem_err;
	__db_hashinit(addr, region->locker_t_size);
	region->locker_off = R_OFFSET(&lt->reginfo, addr);

	SH_TAILQ_INIT(&region->dd_objs);

	/*
	 * If the locks and objects don't divide evenly, spread them around.
	 */
	extra_locks = region->stat.st_maxlocks -
	    ((region->stat.st_maxlocks / region->part_t_size) *
	    region->part_t_size);
	extra_objects = region->stat.st_maxobjects -
	    ((region->stat.st_maxobjects / region->part_t_size) *
	    region->part_t_size);
	for (j = 0; j < region->part_t_size; j++) {
		/* Initialize locks onto a free list. */
		SH_TAILQ_INIT(&part[j].free_locks);
		max = region->stat.st_maxlocks / region->part_t_size;
		if (extra_locks > 0) {
			max++;
			extra_locks--;
		}
		for (i = 0; i < max; ++i) {
			if ((ret = __env_alloc(&lt->reginfo,
			    sizeof(struct __db_lock), &lp)) != 0)
				goto mem_err;
			lp->mtx_lock = MUTEX_INVALID;
			lp->gen = 0;
			lp->status = DB_LSTAT_FREE;
			SH_TAILQ_INSERT_HEAD(
			    &part[j].free_locks, lp, links, __db_lock);
		}
		/* Initialize objects onto a free list.  */
		max = region->stat.st_maxobjects / region->part_t_size;
		if (extra_objects > 0) {
			max++;
			extra_objects--;
		}
		SH_TAILQ_INIT(&part[j].free_objs);
		for (i = 0; i < max; ++i) {
			if ((ret = __env_alloc(&lt->reginfo,
			    sizeof(DB_LOCKOBJ), &op)) != 0)
				goto mem_err;
			SH_TAILQ_INSERT_HEAD(
			    &part[j].free_objs, op, links, __db_lockobj);
			op->generation = 0;
		}
	}

	/* Initialize lockers onto a free list.  */
	SH_TAILQ_INIT(&region->lockers);
	SH_TAILQ_INIT(&region->free_lockers);
	for (i = 0; i < region->stat.st_maxlockers; ++i) {
		if ((ret =
		    __env_alloc(&lt->reginfo, sizeof(DB_LOCKER), &lidp)) != 0) {
mem_err:		__db_errx(env,
			    "unable to allocate memory for the lock table");
			return (ret);
		}
		SH_TAILQ_INSERT_HEAD(
		    &region->free_lockers, lidp, links, __db_locker);
	}

	lt->reginfo.mtx_alloc = region->mtx_region;
	return (0);
}

/*
 * __lock_env_refresh --
 *	Clean up after the lock system on a close or failed open.
 *
 * PUBLIC: int __lock_env_refresh __P((ENV *));
 */
int
__lock_env_refresh(env)
	ENV *env;
{
	struct __db_lock *lp;
	DB_LOCKER *locker;
	DB_LOCKOBJ *lockobj;
	DB_LOCKREGION *lr;
	DB_LOCKTAB *lt;
	REGINFO *reginfo;
	u_int32_t j;
	int ret;

	lt = env->lk_handle;
	reginfo = &lt->reginfo;
	lr = reginfo->primary;

	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if (F_ISSET(env, ENV_PRIVATE)) {
		reginfo->mtx_alloc = MUTEX_INVALID;
		/* Discard the conflict matrix. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, lr->conf_off));

		/* Discard the object hash table. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, lr->obj_off));

		/* Discard the locker hash table. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, lr->locker_off));

		/* Discard the object hash stat table. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, lr->stat_off));

		for (j = 0; j < lr->part_t_size; j++) {
			/* Discard locks. */
			while ((lp = SH_TAILQ_FIRST(
			    &FREE_LOCKS(lt, j), __db_lock)) != NULL) {
				SH_TAILQ_REMOVE(&FREE_LOCKS(lt, j),
				     lp, links, __db_lock);
				__env_alloc_free(reginfo, lp);
			}

			/* Discard objects. */
			while ((lockobj = SH_TAILQ_FIRST(
			    &FREE_OBJS(lt, j), __db_lockobj)) != NULL) {
				SH_TAILQ_REMOVE(&FREE_OBJS(lt, j),
				     lockobj, links, __db_lockobj);
				__env_alloc_free(reginfo, lockobj);
			}
		}

		/* Discard the object partition array. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, lr->part_off));

		/* Discard lockers. */
		while ((locker =
		    SH_TAILQ_FIRST(&lr->free_lockers, __db_locker)) != NULL) {
			SH_TAILQ_REMOVE(
			    &lr->free_lockers, locker, links, __db_locker);
			__env_alloc_free(reginfo, locker);
		}
	}

	/* Detach from the region. */
	ret = __env_region_detach(env, reginfo, 0);

	/* Discard DB_LOCKTAB. */
	__os_free(env, lt);
	env->lk_handle = NULL;

	return (ret);
}

/*
 * __lock_region_mutex_count --
 *	Return the number of mutexes the lock region will need.
 *
 * PUBLIC: u_int32_t __lock_region_mutex_count __P((ENV *));
 */
u_int32_t
__lock_region_mutex_count(env)
	ENV *env;
{
	DB_ENV *dbenv;

	dbenv = env->dbenv;

	return (dbenv->lk_max + dbenv->lk_partitions + 3);
}

/*
 * __lock_region_size --
 *	Return the region size.
 */
static size_t
__lock_region_size(env)
	ENV *env;
{
	DB_ENV *dbenv;
	size_t retval;

	dbenv = env->dbenv;

	/*
	 * Figure out how much space we're going to need.  This list should
	 * map one-to-one with the __env_alloc calls in __lock_region_init.
	 */
	retval = 0;
	retval += __env_alloc_size(sizeof(DB_LOCKREGION));
	retval += __env_alloc_size((size_t)(dbenv->lk_modes * dbenv->lk_modes));
	retval += __env_alloc_size(
	    __db_tablesize(dbenv->lk_max_objects) * (sizeof(DB_HASHTAB)));
	retval += __env_alloc_size(
	    __db_tablesize(dbenv->lk_max_lockers) * (sizeof(DB_HASHTAB)));
	retval += __env_alloc_size(
	    __db_tablesize(dbenv->lk_max_objects) * (sizeof(DB_LOCK_HSTAT)));
	retval +=
	    __env_alloc_size(dbenv->lk_partitions * (sizeof(DB_LOCKPART)));
	retval += __env_alloc_size(sizeof(struct __db_lock)) * dbenv->lk_max;
	retval += __env_alloc_size(sizeof(DB_LOCKOBJ)) * dbenv->lk_max_objects;
	retval += __env_alloc_size(sizeof(DB_LOCKER)) * dbenv->lk_max_lockers;

	/*
	 * Include 16 bytes of string space per lock.  DB doesn't use it
	 * because we pre-allocate lock space for DBTs in the structure.
	 */
	retval += __env_alloc_size(dbenv->lk_max * 16);

	/* And we keep getting this wrong, let's be generous. */
	retval += retval / 4;

	return (retval);
}
