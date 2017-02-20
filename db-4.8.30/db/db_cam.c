/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000, 2010 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

static int __db_s_count __P((DB *));
static int __db_wrlock_err __P((ENV *));
static int __dbc_del_foreign __P((DBC *));
static int __dbc_del_oldskey __P((DB *, DBC *, DBT *, DBT *, DBT *));
static int __dbc_del_secondary __P((DBC *));
static int __dbc_pget_recno __P((DBC *, DBT *, DBT *, u_int32_t));
static inline int __dbc_put_append __P((DBC *,
		DBT *, DBT *, u_int32_t *, u_int32_t));
static inline int __dbc_put_fixed_len __P((DBC *, DBT *, DBT *));
static inline int __dbc_put_partial __P((DBC *,
		DBT *, DBT *, DBT *, DBT *, u_int32_t *, u_int32_t));
static int __dbc_put_primary __P((DBC *, DBT *, DBT *, u_int32_t));
static inline int __dbc_put_resolve_key __P((DBC *,
		DBT *, DBT *, u_int32_t *, u_int32_t));
static inline int __dbc_put_secondaries __P((DBC *,
		DBT *, DBT *, DBT *, int, DBT *, u_int32_t *));

#define	CDB_LOCKING_INIT(env, dbc)					\
	/*								\
	 * If we are running CDB, this had better be either a write	\
	 * cursor or an immediate writer.  If it's a regular writer,	\
	 * that means we have an IWRITE lock and we need to upgrade	\
	 * it to a write lock.						\
	 */								\
	if (CDB_LOCKING(env)) {						\
		if (!F_ISSET(dbc, DBC_WRITECURSOR | DBC_WRITER))	\
			return (__db_wrlock_err(env));			\
									\
		if (F_ISSET(dbc, DBC_WRITECURSOR) &&			\
		    (ret = __lock_get(env,				\
		    (dbc)->locker, DB_LOCK_UPGRADE, &(dbc)->lock_dbt,	\
		    DB_LOCK_WRITE, &(dbc)->mylock)) != 0)		\
			return (ret);					\
	}
#define	CDB_LOCKING_DONE(env, dbc)					\
	/* Release the upgraded lock. */				\
	if (F_ISSET(dbc, DBC_WRITECURSOR))				\
		(void)__lock_downgrade(					\
		    env, &(dbc)->mylock, DB_LOCK_IWRITE, 0);

#define	SET_READ_LOCKING_FLAGS(dbc, var) do {				\
	var = 0;							\
	if (!F_ISSET(dbc, DBC_READ_COMMITTED | DBC_READ_UNCOMMITTED)) {	\
		if (LF_ISSET(DB_READ_COMMITTED))			\
			var = DBC_READ_COMMITTED | DBC_WAS_READ_COMMITTED; \
		if (LF_ISSET(DB_READ_UNCOMMITTED))			\
			var = DBC_READ_UNCOMMITTED;			\
	}								\
	LF_CLR(DB_READ_COMMITTED | DB_READ_UNCOMMITTED);		\
} while (0)

/*
 * __dbc_close --
 *	DBC->close.
 *
 * PUBLIC: int __dbc_close __P((DBC *));
 */
int
__dbc_close(dbc)
	DBC *dbc;
{
	DB *dbp;
	DBC *opd;
	DBC_INTERNAL *cp;
	DB_TXN *txn;
	ENV *env;
	int ret, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;
	cp = dbc->internal;
	opd = cp->opd;
	ret = 0;

	/*
	 * Remove the cursor(s) from the active queue.  We may be closing two
	 * cursors at once here, a top-level one and a lower-level, off-page
	 * duplicate one.  The access-method specific cursor close routine must
	 * close both of them in a single call.
	 *
	 * !!!
	 * Cursors must be removed from the active queue before calling the
	 * access specific cursor close routine, btree depends on having that
	 * order of operations.
	 */
	MUTEX_LOCK(env, dbp->mutex);

	if (opd != NULL) {
		DB_ASSERT(env, F_ISSET(opd, DBC_ACTIVE));
		F_CLR(opd, DBC_ACTIVE);
		TAILQ_REMOVE(&dbp->active_queue, opd, links);
	}
	DB_ASSERT(env, F_ISSET(dbc, DBC_ACTIVE));
	F_CLR(dbc, DBC_ACTIVE);
	TAILQ_REMOVE(&dbp->active_queue, dbc, links);

	MUTEX_UNLOCK(env, dbp->mutex);

	/* Call the access specific cursor close routine. */
	if ((t_ret =
	    dbc->am_close(dbc, PGNO_INVALID, NULL)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Release the lock after calling the access method specific close
	 * routine, a Btree cursor may have had pending deletes.
	 */
	if (CDB_LOCKING(env)) {
		/*
		 * Also, be sure not to free anything if mylock.off is
		 * INVALID;  in some cases, such as idup'ed read cursors
		 * and secondary update cursors, a cursor in a CDB
		 * environment may not have a lock at all.
		 */
		if ((t_ret = __LPUT(dbc, dbc->mylock)) != 0 && ret == 0)
			ret = t_ret;

		/* For safety's sake, since this is going on the free queue. */
		memset(&dbc->mylock, 0, sizeof(dbc->mylock));
		if (opd != NULL)
			memset(&opd->mylock, 0, sizeof(opd->mylock));
	}

	if ((txn = dbc->txn) != NULL)
		txn->cursors--;

	/* Move the cursor(s) to the free queue. */
	MUTEX_LOCK(env, dbp->mutex);
	if (opd != NULL) {
		if (txn != NULL)
			txn->cursors--;
		TAILQ_INSERT_TAIL(&dbp->free_queue, opd, links);
		opd = NULL;
	}
	TAILQ_INSERT_TAIL(&dbp->free_queue, dbc, links);
	MUTEX_UNLOCK(env, dbp->mutex);

	if (txn != NULL && F_ISSET(txn, TXN_PRIVATE) && txn->cursors == 0 &&
	    (t_ret = __txn_commit(txn, 0)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __dbc_destroy --
 *	Destroy the cursor, called after DBC->close.
 *
 * PUBLIC: int __dbc_destroy __P((DBC *));
 */
int
__dbc_destroy(dbc)
	DBC *dbc;
{
	DB *dbp;
	ENV *env;
	int ret, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;

	/* Remove the cursor from the free queue. */
	MUTEX_LOCK(env, dbp->mutex);
	TAILQ_REMOVE(&dbp->free_queue, dbc, links);
	MUTEX_UNLOCK(env, dbp->mutex);

	/* Free up allocated memory. */
	if (dbc->my_rskey.data != NULL)
		__os_free(env, dbc->my_rskey.data);
	if (dbc->my_rkey.data != NULL)
		__os_free(env, dbc->my_rkey.data);
	if (dbc->my_rdata.data != NULL)
		__os_free(env, dbc->my_rdata.data);

	/* Call the access specific cursor destroy routine. */
	ret = dbc->am_destroy == NULL ? 0 : dbc->am_destroy(dbc);

	/*
	 * Release the lock id for this cursor.
	 */
	if (LOCKING_ON(env) &&
	    F_ISSET(dbc, DBC_OWN_LID) &&
	    (t_ret = __lock_id_free(env, dbc->lref)) != 0 && ret == 0)
		ret = t_ret;

	__os_free(env, dbc);

	return (ret);
}

/*
 * __dbc_cmp --
 *	Compare the position of two cursors. Return whether two cursors are
 *	pointing to the same key/data pair.
 *
 * result == 0  if both cursors refer to the same item.
 * result == 1  otherwise
 *
 * PUBLIC: int __dbc_cmp __P((DBC *, DBC *, int *));
 */
int
__dbc_cmp(dbc, other_dbc, result)
	DBC *dbc, *other_dbc;
	int *result;
{
	DBC *curr_dbc, *curr_odbc;
	DBC_INTERNAL *dbc_int, *odbc_int;
	ENV *env;
	int ret;

	env = dbc->env;
	ret = 0;

#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbc->dbp)) {
		dbc = ((PART_CURSOR *)dbc->internal)->sub_cursor;
		other_dbc = ((PART_CURSOR *)other_dbc->internal)->sub_cursor;
	}
	/* Both cursors must still be valid. */
	if (dbc == NULL || other_dbc == NULL) {
		__db_errx(env,
"Both cursors must be initialized before calling DBC->cmp.");
		return (EINVAL);
	}

	if (dbc->dbp != other_dbc->dbp) {
		*result = 1;
		return (0);
	}
#endif

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbc->dbp))
		return (__bamc_compress_cmp(dbc, other_dbc, result));
#endif

	curr_dbc = dbc;
	curr_odbc = other_dbc;
	dbc_int = dbc->internal;
	odbc_int = other_dbc->internal;

	/* Both cursors must be on valid positions. */
	if (dbc_int->pgno == PGNO_INVALID || odbc_int->pgno == PGNO_INVALID) {
		__db_errx(env,
"Both cursors must be initialized before calling DBC->cmp.");
		return (EINVAL);
	}

	/*
	 * Use a loop since cursors can be nested. Off page duplicate
	 * sets can only be nested one level deep, so it is safe to use a
	 * while (true) loop.
	 */
	while (1) {
		if (dbc_int->pgno == odbc_int->pgno &&
		    dbc_int->indx == odbc_int->indx) {
			/*
			 * If one cursor is sitting on an off page duplicate
			 * set, the other will be pointing to the same set. Be
			 * careful, and check  anyway.
			 */
			if (dbc_int->opd != NULL && odbc_int->opd != NULL) {
				curr_dbc = dbc_int->opd;
				curr_odbc = odbc_int->opd;
				dbc_int = dbc_int->opd->internal;
				odbc_int= odbc_int->opd->internal;
				continue;
			} else if (dbc_int->opd == NULL &&
			    odbc_int->opd == NULL)
				*result = 0;
			else {
				__db_errx(env,
		"DBCursor->cmp mismatched off page duplicate cursor pointers.");
				return (EINVAL);
			}

			switch (curr_dbc->dbtype) {
			case DB_HASH:
				/*
				 * Make sure that on-page duplicate data
				 * indexes match, and that the deleted
				 * flags are consistent.
				 */
				ret = __hamc_cmp(curr_dbc, curr_odbc, result);
				break;
			case DB_BTREE:
			case DB_RECNO:
				/*
				 * Check for consisted deleted flags on btree
				 * specific cursors.
				 */
				ret = __bamc_cmp(curr_dbc, curr_odbc, result);
				break;
			default:
				/* NO-OP break out. */
				break;
			}
		} else
			*result = 1;
		return (ret);
	}
	/* NOTREACHED. */
	return (ret);
}

/*
 * __dbc_count --
 *	Return a count of duplicate data items.
 *
 * PUBLIC: int __dbc_count __P((DBC *, db_recno_t *));
 */
int
__dbc_count(dbc, recnop)
	DBC *dbc;
	db_recno_t *recnop;
{
	ENV *env;
	int ret;

	env = dbc->env;

#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbc->dbp))
		dbc = ((PART_CURSOR *)dbc->internal)->sub_cursor;
#endif
	/*
	 * Cursor Cleanup Note:
	 * All of the cursors passed to the underlying access methods by this
	 * routine are not duplicated and will not be cleaned up on return.
	 * So, pages/locks that the cursor references must be resolved by the
	 * underlying functions.
	 */
	switch (dbc->dbtype) {
	case DB_QUEUE:
	case DB_RECNO:
		*recnop = 1;
		break;
	case DB_HASH:
		if (dbc->internal->opd == NULL) {
			if ((ret = __hamc_count(dbc, recnop)) != 0)
				return (ret);
			break;
		}
		/* FALLTHROUGH */
	case DB_BTREE:
#ifdef HAVE_COMPRESSION
		if (DB_IS_COMPRESSED(dbc->dbp))
			return (__bamc_compress_count(dbc, recnop));
#endif
		if ((ret = __bamc_count(dbc, recnop)) != 0)
			return (ret);
		break;
	case DB_UNKNOWN:
	default:
		return (__db_unknown_type(env, "__dbc_count", dbc->dbtype));
	}
	return (0);
}

/*
 * __dbc_del --
 *	DBC->del.
 *
 * PUBLIC: int __dbc_del __P((DBC *, u_int32_t));
 */
int
__dbc_del(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	DB *dbp;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	env = dbp->env;

	CDB_LOCKING_INIT(env, dbc);

	/*
	 * If we're a secondary index, and DB_UPDATE_SECONDARY isn't set
	 * (which it only is if we're being called from a primary update),
	 * then we need to call through to the primary and delete the item.
	 *
	 * Note that this will delete the current item;  we don't need to
	 * delete it ourselves as well, so we can just goto done.
	 */
	if (flags != DB_UPDATE_SECONDARY && F_ISSET(dbp, DB_AM_SECONDARY)) {
		ret = __dbc_del_secondary(dbc);
		goto done;
	}

	/*
	 * If we are a foreign db, go through and check any foreign key
	 * constraints first, which will make rolling back changes on an abort
	 * simpler.
	 */
	if (LIST_FIRST(&dbp->f_primaries) != NULL &&
	    (ret = __dbc_del_foreign(dbc)) != 0)
		goto done;

	/*
	 * If we are a primary and have secondary indices, go through
	 * and delete any secondary keys that point at the current record.
	 */
	if (DB_IS_PRIMARY(dbp) &&
	    (ret = __dbc_del_primary(dbc)) != 0)
		goto done;

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbp))
		ret = __bamc_compress_del(dbc, flags);
	else
#endif
		ret = __dbc_idel(dbc, flags);

done:	CDB_LOCKING_DONE(env, dbc);

	return (ret);
}

/*
 * __dbc_del --
 *	Implemenation of DBC->del.
 *
 * PUBLIC: int __dbc_idel __P((DBC *, u_int32_t));
 */
int
__dbc_idel(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	DB *dbp;
	DBC *opd;
	int ret, t_ret;

	COMPQUIET(flags, 0);

	dbp = dbc->dbp;

	/*
	 * Cursor Cleanup Note:
	 * All of the cursors passed to the underlying access methods by this
	 * routine are not duplicated and will not be cleaned up on return.
	 * So, pages/locks that the cursor references must be resolved by the
	 * underlying functions.
	 */

	/*
	 * Off-page duplicate trees are locked in the primary tree, that is,
	 * we acquire a write lock in the primary tree and no locks in the
	 * off-page dup tree.  If the del operation is done in an off-page
	 * duplicate tree, call the primary cursor's upgrade routine first.
	 */
	opd = dbc->internal->opd;
	if (opd == NULL)
		ret = dbc->am_del(dbc, flags);
	else if ((ret = dbc->am_writelock(dbc)) == 0)
		ret = opd->am_del(opd, flags);

	/*
	 * If this was an update that is supporting dirty reads
	 * then we may have just swapped our read for a write lock
	 * which is held by the surviving cursor.  We need
	 * to explicitly downgrade this lock.  The closed cursor
	 * may only have had a read lock.
	 */
	if (F_ISSET(dbp, DB_AM_READ_UNCOMMITTED) &&
	    dbc->internal->lock_mode == DB_LOCK_WRITE) {
		if ((t_ret =
		    __TLPUT(dbc, dbc->internal->lock)) != 0 && ret == 0)
			ret = t_ret;
		if (t_ret == 0)
			dbc->internal->lock_mode = DB_LOCK_WWRITE;
		if (dbc->internal->page != NULL && (t_ret =
		    __memp_shared(dbp->mpf, dbc->internal->page)) != 0 &&
		    ret == 0)
			ret = t_ret;
	}

	return (ret);
}

#ifdef HAVE_COMPRESSION
/*
 * __dbc_bulk_del --
 *	Bulk del for a cursor.
 *
 *	Only implemented for compressed BTrees. In this file in order to
 *	use the CDB_LOCKING_* macros.
 *
 * PUBLIC: #ifdef HAVE_COMPRESSION
 * PUBLIC: int __dbc_bulk_del __P((DBC *, DBT *, u_int32_t));
 * PUBLIC: #endif
 */
int
__dbc_bulk_del(dbc, key, flags)
	DBC *dbc;
	DBT *key;
	u_int32_t flags;
{
	ENV *env;
	int ret;

	env = dbc->env;

	DB_ASSERT(env, DB_IS_COMPRESSED(dbc->dbp));

	CDB_LOCKING_INIT(env, dbc);

	ret = __bamc_compress_bulk_del(dbc, key, flags);

	CDB_LOCKING_DONE(env, dbc);

	return (ret);
}
#endif

/*
 * __dbc_dup --
 *	Duplicate a cursor
 *
 * PUBLIC: int __dbc_dup __P((DBC *, DBC **, u_int32_t));
 */
int
__dbc_dup(dbc_orig, dbcp, flags)
	DBC *dbc_orig;
	DBC **dbcp;
	u_int32_t flags;
{
	DBC *dbc_n, *dbc_nopd;
	int ret;

	dbc_n = dbc_nopd = NULL;

	/* Allocate a new cursor and initialize it. */
	if ((ret = __dbc_idup(dbc_orig, &dbc_n, flags)) != 0)
		goto err;
	*dbcp = dbc_n;

	/*
	 * If the cursor references an off-page duplicate tree, allocate a
	 * new cursor for that tree and initialize it.
	 */
	if (dbc_orig->internal->opd != NULL) {
		if ((ret =
		   __dbc_idup(dbc_orig->internal->opd, &dbc_nopd, flags)) != 0)
			goto err;
		dbc_n->internal->opd = dbc_nopd;
		dbc_nopd->internal->pdbc = dbc_n;
	}
	return (0);

err:	if (dbc_n != NULL)
		(void)__dbc_close(dbc_n);
	if (dbc_nopd != NULL)
		(void)__dbc_close(dbc_nopd);

	return (ret);
}

/*
 * __dbc_idup --
 *	Internal version of __dbc_dup.
 *
 * PUBLIC: int __dbc_idup __P((DBC *, DBC **, u_int32_t));
 */
int
__dbc_idup(dbc_orig, dbcp, flags)
	DBC *dbc_orig, **dbcp;
	u_int32_t flags;
{
	DB *dbp;
	DBC *dbc_n;
	DBC_INTERNAL *int_n, *int_orig;
	ENV *env;
	int ret;

	dbp = dbc_orig->dbp;
	dbc_n = *dbcp;
	env = dbp->env;

	if ((ret = __db_cursor_int(dbp, dbc_orig->thread_info,
	    dbc_orig->txn, dbc_orig->dbtype, dbc_orig->internal->root,
	    F_ISSET(dbc_orig, DBC_OPD) | DBC_DUPLICATE,
	    dbc_orig->locker, &dbc_n)) != 0)
		return (ret);

	/* Position the cursor if requested, acquiring the necessary locks. */
	if (LF_ISSET(DB_POSITION)) {
		int_n = dbc_n->internal;
		int_orig = dbc_orig->internal;

		dbc_n->flags |= dbc_orig->flags & ~DBC_OWN_LID;

		int_n->indx = int_orig->indx;
		int_n->pgno = int_orig->pgno;
		int_n->root = int_orig->root;
		int_n->lock_mode = int_orig->lock_mode;

		int_n->stream_start_pgno = int_orig->stream_start_pgno;
		int_n->stream_off = int_orig->stream_off;
		int_n->stream_curr_pgno = int_orig->stream_curr_pgno;

		switch (dbc_orig->dbtype) {
		case DB_QUEUE:
			if ((ret = __qamc_dup(dbc_orig, dbc_n)) != 0)
				goto err;
			break;
		case DB_BTREE:
		case DB_RECNO:
			if ((ret = __bamc_dup(dbc_orig, dbc_n, flags)) != 0)
				goto err;
			break;
		case DB_HASH:
			if ((ret = __hamc_dup(dbc_orig, dbc_n)) != 0)
				goto err;
			break;
		case DB_UNKNOWN:
		default:
			ret = __db_unknown_type(env,
			    "__dbc_idup", dbc_orig->dbtype);
			goto err;
		}
	} else if (F_ISSET(dbc_orig, DBC_BULK)) {
		/*
		 * For bulk cursors, remember what page were on, even if we
		 * don't know that the next operation will be nearby.
		 */
		dbc_n->internal->pgno = dbc_orig->internal->pgno;
	}

	/* Copy the locking flags to the new cursor. */
	F_SET(dbc_n, F_ISSET(dbc_orig, DBC_BULK |
	    DBC_READ_COMMITTED | DBC_READ_UNCOMMITTED | DBC_WRITECURSOR));

	/*
	 * If we're in CDB and this isn't an offpage dup cursor, then
	 * we need to get a lock for the duplicated cursor.
	 */
	if (CDB_LOCKING(env) && !F_ISSET(dbc_n, DBC_OPD) &&
	    (ret = __lock_get(env, dbc_n->locker, 0,
	    &dbc_n->lock_dbt, F_ISSET(dbc_orig, DBC_WRITECURSOR) ?
	    DB_LOCK_IWRITE : DB_LOCK_READ, &dbc_n->mylock)) != 0)
		goto err;

	dbc_n->priority = dbc_orig->priority;
	dbc_n->internal->pdbc = dbc_orig->internal->pdbc;
	*dbcp = dbc_n;
	return (0);

err:	(void)__dbc_close(dbc_n);
	return (ret);
}

/*
 * __dbc_newopd --
 *	Create a new off-page duplicate cursor.
 *
 * PUBLIC: int __dbc_newopd __P((DBC *, db_pgno_t, DBC *, DBC **));
 */
int
__dbc_newopd(dbc_parent, root, oldopd, dbcp)
	DBC *dbc_parent;
	db_pgno_t root;
	DBC *oldopd;
	DBC **dbcp;
{
	DB *dbp;
	DBC *opd;
	DBTYPE dbtype;
	int ret;

	dbp = dbc_parent->dbp;
	dbtype = (dbp->dup_compare == NULL) ? DB_RECNO : DB_BTREE;

	/*
	 * On failure, we want to default to returning the old off-page dup
	 * cursor, if any;  our caller can't be left with a dangling pointer
	 * to a freed cursor.  On error the only allowable behavior is to
	 * close the cursor (and the old OPD cursor it in turn points to), so
	 * this should be safe.
	 */
	*dbcp = oldopd;

	if ((ret = __db_cursor_int(dbp, dbc_parent->thread_info,
	    dbc_parent->txn,
	    dbtype, root, DBC_OPD, dbc_parent->locker, &opd)) != 0)
		return (ret);

	opd->priority = dbc_parent->priority;
	opd->internal->pdbc = dbc_parent;
	*dbcp = opd;

	/*
	 * Check to see if we already have an off-page dup cursor that we've
	 * passed in.  If we do, close it.  It'd be nice to use it again
	 * if it's a cursor belonging to the right tree, but if we're doing
	 * a cursor-relative operation this might not be safe, so for now
	 * we'll take the easy way out and always close and reopen.
	 *
	 * Note that under no circumstances do we want to close the old
	 * cursor without returning a valid new one;  we don't want to
	 * leave the main cursor in our caller with a non-NULL pointer
	 * to a freed off-page dup cursor.
	 */
	if (oldopd != NULL && (ret = __dbc_close(oldopd)) != 0)
		return (ret);

	return (0);
}

/*
 * __dbc_get --
 *	Get using a cursor.
 *
 * PUBLIC: int __dbc_get __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_get(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
#ifdef HAVE_PARTITION
	if (F_ISSET(dbc, DBC_PARTITIONED))
		return (__partc_get(dbc, key, data, flags));
#endif

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbc->dbp))
		return (__bamc_compress_get(dbc, key, data, flags));
#endif

	return (__dbc_iget(dbc, key, data, flags));
}

/*
 * __dbc_iget --
 *	Implementation of get using a cursor.
 *
 * PUBLIC: int __dbc_iget __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_iget(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp;
	DBC *ddbc, *dbc_n, *opd;
	DBC_INTERNAL *cp, *cp_n;
	DB_MPOOLFILE *mpf;
	ENV *env;
	db_pgno_t pgno;
	db_indx_t indx_off;
	u_int32_t multi, orig_ulen, tmp_flags, tmp_read_locking, tmp_rmw;
	u_int8_t type;
	int key_small, ret, t_ret;

	COMPQUIET(orig_ulen, 0);

	key_small = 0;

	/*
	 * Cursor Cleanup Note:
	 * All of the cursors passed to the underlying access methods by this
	 * routine are duplicated cursors.  On return, any referenced pages
	 * will be discarded, and, if the cursor is not intended to be used
	 * again, the close function will be called.  So, pages/locks that
	 * the cursor references do not need to be resolved by the underlying
	 * functions.
	 */
	dbp = dbc->dbp;
	env = dbp->env;
	mpf = dbp->mpf;
	dbc_n = NULL;
	opd = NULL;

	/* Clear OR'd in additional bits so we can check for flag equality. */
	tmp_rmw = LF_ISSET(DB_RMW);
	LF_CLR(DB_RMW);

	SET_READ_LOCKING_FLAGS(dbc, tmp_read_locking);

	multi = LF_ISSET(DB_MULTIPLE|DB_MULTIPLE_KEY);
	LF_CLR(DB_MULTIPLE|DB_MULTIPLE_KEY);

	/*
	 * Return a cursor's record number.  It has nothing to do with the
	 * cursor get code except that it was put into the interface.
	 */
	if (flags == DB_GET_RECNO) {
		if (tmp_rmw)
			F_SET(dbc, DBC_RMW);
		F_SET(dbc, tmp_read_locking);
		ret = __bamc_rget(dbc, data);
		if (tmp_rmw)
			F_CLR(dbc, DBC_RMW);
		/* Clear the temp flags, but leave WAS_READ_COMMITTED. */
		F_CLR(dbc, tmp_read_locking & ~DBC_WAS_READ_COMMITTED);
		return (ret);
	}

	if (flags == DB_CONSUME || flags == DB_CONSUME_WAIT)
		CDB_LOCKING_INIT(env, dbc);

	/* Don't return the key or data if it was passed to us. */
	if (!DB_RETURNS_A_KEY(dbp, flags))
		F_SET(key, DB_DBT_ISSET);
	if (flags == DB_GET_BOTH &&
	    (dbp->dup_compare == NULL || dbp->dup_compare == __bam_defcmp))
		F_SET(data, DB_DBT_ISSET);

	/*
	 * If we have an off-page duplicates cursor, and the operation applies
	 * to it, perform the operation.  Duplicate the cursor and call the
	 * underlying function.
	 *
	 * Off-page duplicate trees are locked in the primary tree, that is,
	 * we acquire a write lock in the primary tree and no locks in the
	 * off-page dup tree.  If the DB_RMW flag was specified and the get
	 * operation is done in an off-page duplicate tree, call the primary
	 * cursor's upgrade routine first.
	 */
	cp = dbc->internal;
	if (cp->opd != NULL &&
	    (flags == DB_CURRENT || flags == DB_GET_BOTHC ||
	    flags == DB_NEXT || flags == DB_NEXT_DUP ||
	    flags == DB_PREV || flags == DB_PREV_DUP)) {
		if (tmp_rmw && (ret = dbc->am_writelock(dbc)) != 0)
			goto err;
		if (F_ISSET(dbc, DBC_TRANSIENT))
			opd = cp->opd;
		else if ((ret = __dbc_idup(cp->opd, &opd, DB_POSITION)) != 0)
			goto err;

		if ((ret = opd->am_get(opd, key, data, flags, NULL)) == 0)
			goto done;
		/*
		 * Another cursor may have deleted all of the off-page
		 * duplicates, so for operations that are moving a cursor, we
		 * need to skip the empty tree and retry on the parent cursor.
		 */
		if (ret == DB_NOTFOUND &&
		    (flags == DB_PREV || flags == DB_NEXT)) {
			ret = __dbc_close(opd);
			opd = NULL;
			if (F_ISSET(dbc, DBC_TRANSIENT))
				cp->opd = NULL;
		}
		if (ret != 0)
			goto err;
	} else if (cp->opd != NULL && F_ISSET(dbc, DBC_TRANSIENT)) {
		if ((ret = __dbc_close(cp->opd)) != 0)
			goto err;
		cp->opd = NULL;
	}

	/*
	 * Perform an operation on the main cursor.  Duplicate the cursor,
	 * upgrade the lock as required, and call the underlying function.
	 */
	switch (flags) {
	case DB_CURRENT:
	case DB_GET_BOTHC:
	case DB_NEXT:
	case DB_NEXT_DUP:
	case DB_NEXT_NODUP:
	case DB_PREV:
	case DB_PREV_DUP:
	case DB_PREV_NODUP:
		tmp_flags = DB_POSITION;
		break;
	default:
		tmp_flags = 0;
		break;
	}

	/*
	 * If this cursor is going to be closed immediately, we don't
	 * need to take precautions to clean it up on error.
	 */
	if (F_ISSET(dbc, DBC_TRANSIENT | DBC_PARTITIONED))
		dbc_n = dbc;
	else {
		ret = __dbc_idup(dbc, &dbc_n, tmp_flags);

		if (ret != 0)
			goto err;
		COPY_RET_MEM(dbc, dbc_n);
	}

	if (tmp_rmw)
		F_SET(dbc_n, DBC_RMW);
	F_SET(dbc_n, tmp_read_locking);

	switch (multi) {
	case DB_MULTIPLE:
		F_SET(dbc_n, DBC_MULTIPLE);
		break;
	case DB_MULTIPLE_KEY:
		F_SET(dbc_n, DBC_MULTIPLE_KEY);
		break;
	case DB_MULTIPLE | DB_MULTIPLE_KEY:
		F_SET(dbc_n, DBC_MULTIPLE|DBC_MULTIPLE_KEY);
		break;
	case 0:
	default:
		break;
	}

retry:	pgno = PGNO_INVALID;
	ret = dbc_n->am_get(dbc_n, key, data, flags, &pgno);
	if (tmp_rmw)
		F_CLR(dbc_n, DBC_RMW);
	/*
	 * Clear the temporary locking flags in the new cursor.  The user's
	 * (old) cursor needs to have the WAS_READ_COMMITTED flag because this
	 * is used on the next call on that cursor.
	 */
	F_CLR(dbc_n, tmp_read_locking);
	F_SET(dbc, tmp_read_locking & DBC_WAS_READ_COMMITTED);
	F_CLR(dbc_n, DBC_MULTIPLE|DBC_MULTIPLE_KEY);
	if (ret != 0)
		goto err;

	cp_n = dbc_n->internal;

	/*
	 * We may be referencing a new off-page duplicates tree.  Acquire
	 * a new cursor and call the underlying function.
	 */
	if (pgno != PGNO_INVALID) {
		if ((ret = __dbc_newopd(dbc,
		    pgno, cp_n->opd, &cp_n->opd)) != 0)
			goto err;

		switch (flags) {
		case DB_FIRST:
		case DB_NEXT:
		case DB_NEXT_NODUP:
		case DB_SET:
		case DB_SET_RECNO:
		case DB_SET_RANGE:
			tmp_flags = DB_FIRST;
			break;
		case DB_LAST:
		case DB_PREV:
		case DB_PREV_NODUP:
			tmp_flags = DB_LAST;
			break;
		case DB_GET_BOTH:
		case DB_GET_BOTHC:
		case DB_GET_BOTH_RANGE:
			tmp_flags = flags;
			break;
		default:
			ret = __db_unknown_flag(env, "__dbc_get", flags);
			goto err;
		}
		ret = cp_n->opd->am_get(cp_n->opd, key, data, tmp_flags, NULL);
		/*
		 * Another cursor may have deleted all of the off-page
		 * duplicates, so for operations that are moving a cursor, we
		 * need to skip the empty tree and retry on the parent cursor.
		 */
		if (ret == DB_NOTFOUND) {
			switch (flags) {
			case DB_FIRST:
			case DB_NEXT:
			case DB_NEXT_NODUP:
				flags = DB_NEXT;
				break;
			case DB_LAST:
			case DB_PREV:
			case DB_PREV_NODUP:
				flags = DB_PREV;
				break;
			default:
				goto err;
			}

			ret = __dbc_close(cp_n->opd);
			cp_n->opd = NULL;
			if (ret == 0)
				goto retry;
		}
		if (ret != 0)
			goto err;
	}

done:	/*
	 * Return a key/data item.  The only exception is that we don't return
	 * a key if the user already gave us one, that is, if the DB_SET flag
	 * was set.  The DB_SET flag is necessary.  In a Btree, the user's key
	 * doesn't have to be the same as the key stored the tree, depending on
	 * the magic performed by the comparison function.  As we may not have
	 * done any key-oriented operation here, the page reference may not be
	 * valid.  Fill it in as necessary.  We don't have to worry about any
	 * locks, the cursor must already be holding appropriate locks.
	 *
	 * XXX
	 * If not a Btree and DB_SET_RANGE is set, we shouldn't return a key
	 * either, should we?
	 */
	cp_n = dbc_n == NULL ? dbc->internal : dbc_n->internal;
	if (!F_ISSET(key, DB_DBT_ISSET)) {
		if (cp_n->page == NULL && (ret = __memp_fget(mpf, &cp_n->pgno,
		    dbc->thread_info, dbc->txn, 0, &cp_n->page)) != 0)
			goto err;

		if ((ret = __db_ret(dbc, cp_n->page, cp_n->indx, key,
		    &dbc->rkey->data, &dbc->rkey->ulen)) != 0) {
			/*
			 * If the key DBT is too small, we still want to return
			 * the size of the data.  Otherwise applications are
			 * forced to check each one with a separate call.  We
			 * don't want to copy the data, so we set the ulen to
			 * zero before calling __db_ret.
			 */
			if (ret == DB_BUFFER_SMALL &&
			    F_ISSET(data, DB_DBT_USERMEM)) {
				key_small = 1;
				orig_ulen = data->ulen;
				data->ulen = 0;
			} else
				goto err;
		}
	}
	if (multi != 0 && dbc->am_bulk != NULL) {
		/*
		 * Even if fetching from the OPD cursor we need a duplicate
		 * primary cursor if we are going after multiple keys.
		 */
		if (dbc_n == NULL) {
			/*
			 * Non-"_KEY" DB_MULTIPLE doesn't move the main cursor,
			 * so it's safe to just use dbc, unless the cursor
			 * has an open off-page duplicate cursor whose state
			 * might need to be preserved.
			 */
			if ((!(multi & DB_MULTIPLE_KEY) &&
			    dbc->internal->opd == NULL) ||
			    F_ISSET(dbc, DBC_TRANSIENT | DBC_PARTITIONED))
				dbc_n = dbc;
			else {
				if ((ret = __dbc_idup(dbc,
				    &dbc_n, DB_POSITION)) != 0)
					goto err;
				if ((ret = dbc_n->am_get(dbc_n,
				    key, data, DB_CURRENT, &pgno)) != 0)
					goto err;
			}
			cp_n = dbc_n->internal;
		}

		/*
		 * If opd is set then we dupped the opd that we came in with.
		 * When we return we may have a new opd if we went to another
		 * key.
		 */
		if (opd != NULL) {
			DB_ASSERT(env, cp_n->opd == NULL);
			cp_n->opd = opd;
			opd = NULL;
		}

		/*
		 * Bulk get doesn't use __db_retcopy, so data.size won't
		 * get set up unless there is an error.  Assume success
		 * here.  This is the only call to am_bulk, and it avoids
		 * setting it exactly the same everywhere.  If we have an
		 * DB_BUFFER_SMALL error, it'll get overwritten with the
		 * needed value.
		 */
		data->size = data->ulen;
		ret = dbc_n->am_bulk(dbc_n, data, flags | multi);
	} else if (!F_ISSET(data, DB_DBT_ISSET)) {
		ddbc = opd != NULL ? opd :
		    cp_n->opd != NULL ? cp_n->opd : dbc_n;
		cp = ddbc->internal;
		if (cp->page == NULL &&
		    (ret = __memp_fget(mpf, &cp->pgno,
			 dbc->thread_info, ddbc->txn, 0, &cp->page)) != 0)
			goto err;

		type = TYPE(cp->page);
		indx_off = ((type == P_LBTREE ||
		    type == P_HASH || type == P_HASH_UNSORTED) ? O_INDX : 0);
		ret = __db_ret(ddbc, cp->page, cp->indx + indx_off,
		    data, &dbc->rdata->data, &dbc->rdata->ulen);
	}

err:	/* Don't pass DB_DBT_ISSET back to application level, error or no. */
	F_CLR(key, DB_DBT_ISSET);
	F_CLR(data, DB_DBT_ISSET);

	/* Cleanup and cursor resolution. */
	if (opd != NULL) {
		/*
		 * To support dirty reads we must reget the write lock
		 * if we have just stepped off a deleted record.
		 * Since the OPD cursor does not know anything
		 * about the referencing page or cursor we need
		 * to peek at the OPD cursor and get the lock here.
		 */
		if (F_ISSET(dbp, DB_AM_READ_UNCOMMITTED) &&
		     F_ISSET((BTREE_CURSOR *)
		     dbc->internal->opd->internal, C_DELETED))
			if ((t_ret =
			    dbc->am_writelock(dbc)) != 0 && ret == 0)
				ret = t_ret;
		if ((t_ret = __dbc_cleanup(
		    dbc->internal->opd, opd, ret)) != 0 && ret == 0)
			ret = t_ret;
	}

	if (key_small) {
		data->ulen = orig_ulen;
		if (ret == 0)
			ret = DB_BUFFER_SMALL;
	}

	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 &&
	    (ret == 0 || ret == DB_BUFFER_SMALL))
		ret = t_ret;

	if (flags == DB_CONSUME || flags == DB_CONSUME_WAIT)
		CDB_LOCKING_DONE(env, dbc);
	return (ret);
}

/* Internal flags shared by the dbc_put functions. */
#define	DBC_PUT_RMW		0x001
#define	DBC_PUT_NODEL		0x002
#define	DBC_PUT_HAVEREC		0x004

/*
 * __dbc_put_resolve_key --
 *	Get the current key and data so that we can correctly update the
 *	secondary and foreign databases.
 */
static inline int
__dbc_put_resolve_key(dbc, oldkey, olddata, put_statep, flags)
	DBC *dbc;
	DBT *oldkey, *olddata;
	u_int32_t flags, *put_statep;
{
	DB *dbp;
	ENV *env;
	int ret, rmw;

	dbp = dbc->dbp;
	env = dbp->env;
	rmw = FLD_ISSET(*put_statep, DBC_PUT_RMW) ? DB_RMW : 0;

	DB_ASSERT(env, flags == DB_CURRENT);
	COMPQUIET(flags, 0);

	/*
	 * This is safe to do on the cursor we already have;
	 * error or no, it won't move.
	 *
	 * We use DB_RMW for all of these gets because we'll be
	 * writing soon enough in the "normal" put code.  In
	 * transactional databases we'll hold those write locks
	 * even if we close the cursor we're reading with.
	 *
	 * The DB_KEYEMPTY return needs special handling -- if the
	 * cursor is on a deleted key, we return DB_NOTFOUND.
	 */
	memset(oldkey, 0, sizeof(DBT));
	if ((ret = __dbc_get(dbc, oldkey, olddata, rmw | DB_CURRENT)) != 0)
		return (ret == DB_KEYEMPTY ? DB_NOTFOUND : ret);

	/* Record that we've looked for the old record. */
	FLD_SET(*put_statep, DBC_PUT_HAVEREC);
	return (0);
}

/*
 * __dbc_put_append --
 *	Handle an append to a primary.
 */
static inline int
__dbc_put_append(dbc, key, data, put_statep, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags, *put_statep;
{
	DB *dbp;
	ENV *env;
	DBC *dbc_n;
	DBT tdata;
	int ret, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;
	ret = 0;
	dbc_n = NULL;

	DB_ASSERT(env, flags == DB_APPEND);
	COMPQUIET(flags, 0);

	/*
	 * With DB_APPEND, we need to do the insert to populate the key value.
	 * So we swap the 'normal' order of updating secondary / verifying
	 * foreign databases and inserting.
	 *
	 * If there is an append callback, the value stored in data->data may
	 * be replaced and then freed.  To avoid passing a freed pointer back
	 * to the user, just operate on a copy of the data DBT.
	 */
	tdata = *data;

	/*
	 * If this cursor is going to be closed immediately, we don't
	 * need to take precautions to clean it up on error.
	 */
	if (F_ISSET(dbc, DBC_TRANSIENT))
		dbc_n = dbc;
	else if ((ret = __dbc_idup(dbc, &dbc_n, 0)) != 0)
		goto err;

	/*
	 * Append isn't a normal put operation;  call the appropriate access
	 * method's append function.
	 */
	switch (dbp->type) {
	case DB_QUEUE:
		if ((ret = __qam_append(dbc_n, key, &tdata)) != 0)
			goto err;
		break;
	case DB_RECNO:
		if ((ret = __ram_append(dbc_n, key, &tdata)) != 0)
			goto err;
		break;
	default:
		/* The interface should prevent this. */
		DB_ASSERT(env,
		    dbp->type == DB_QUEUE || dbp->type == DB_RECNO);

		ret = __db_ferr(env, "DBC->put", 0);
		goto err;
	}

	/*
	 * The append callback, if one exists, may have allocated a new
	 * tdata.data buffer.  If so, free it.
	 */
	FREE_IF_NEEDED(env, &tdata);

	/*
	 * The key value may have been generated by the above operation, but
	 * not set in the data buffer. Make sure it is there so that secondary
	 * updates can complete.
	 */
	if ((ret = __dbt_usercopy(env, key)) != 0)
		goto err;

	/* An append cannot be replacing an existing item. */
	FLD_SET(*put_statep, DBC_PUT_NODEL);

err:	if (dbc_n != NULL &&
	    (t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __dbc_put_partial --
 *	Ensure that the data item we are using is complete and correct.
 *      Otherwise we could break the secondary constraints.
 */
static inline int
__dbc_put_partial(dbc, pkey, data, orig_data, out_data, put_statep, flags)
	DBC *dbc;
	DBT *pkey, *data, *orig_data, *out_data;
	u_int32_t *put_statep, flags;
{
	DB *dbp;
	DBC *pdbc;
	ENV *env;
	int ret, rmw, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;
	ret = t_ret = 0;
	rmw = FLD_ISSET(*put_statep, DBC_PUT_RMW) ? DB_RMW : 0;

	if (!FLD_ISSET(*put_statep, DBC_PUT_HAVEREC) &&
	    !FLD_ISSET(*put_statep, DBC_PUT_NODEL)) {
		/*
		 * We're going to have to search the tree for the
		 * specified key.  Dup a cursor (so we have the same
		 * locking info) and do a c_get.
		 */
		if ((ret = __dbc_idup(dbc, &pdbc, 0)) != 0)
			return (ret);

		/*
		 * When doing a put with DB_CURRENT, partial data items have
		 * already been resolved.
		 */
		DB_ASSERT(env, flags != DB_CURRENT);

		F_SET(pkey, DB_DBT_ISSET);
		ret = __dbc_get(pdbc, pkey, orig_data, rmw | DB_SET);
		if (ret == DB_KEYEMPTY || ret == DB_NOTFOUND) {
			FLD_SET(*put_statep, DBC_PUT_NODEL);
			ret = 0;
		}
		if ((t_ret = __dbc_close(pdbc)) != 0)
			ret = t_ret;
		if (ret != 0)
			return (ret);

		FLD_SET(*put_statep, DBC_PUT_HAVEREC);
	}

	COMPQUIET(flags, 0);

	/*
	 * Now build the new datum from orig_data and the partial data
	 * we were given.  It's okay to do this if no record was
	 * returned above: a partial put on an empty record is allowed,
	 * if a little strange.  The data is zero-padded.
	 */
	return (__db_buildpartial(dbp, orig_data, data, out_data));
}

/*
 * __dbc_put_fixed_len --
 *      Handle padding for fixed-length records.
 */
static inline int
__dbc_put_fixed_len(dbc, data, out_data)
	DBC *dbc;
	DBT *data, *out_data;
{
	DB *dbp;
	ENV *env;
	int re_pad, ret;
	u_int32_t re_len, size;

	dbp = dbc->dbp;
	env = dbp->env;
	ret = 0;

	/*
	 * Handle fixed-length records.  If the primary database has
	 * fixed-length records, we need to pad out the datum before
	 * we pass it into the callback function;  we always index the
	 * "real" record.
	 */
	if (dbp->type == DB_QUEUE) {
		re_len = ((QUEUE *)dbp->q_internal)->re_len;
		re_pad = ((QUEUE *)dbp->q_internal)->re_pad;
	} else {
		re_len = ((BTREE *)dbp->bt_internal)->re_len;
		re_pad = ((BTREE *)dbp->bt_internal)->re_pad;
	}

	size = data->size;
	if (size > re_len) {
		ret = __db_rec_toobig(env, size, re_len);
		return (ret);
	} else if (size < re_len) {
		/*
		 * If we're not doing a partial put, copy data->data into
		 * out_data->data, then pad out out_data->data. This overrides
		 * the assignment made above, which is used in the more common
		 * case when padding is not needed.
		 *
		 * If we're doing a partial put, the data we want are already
		 * in out_data.data; we just need to pad.
		 */
		if (F_ISSET(data, DB_DBT_PARTIAL)) {
		       if ((ret = __os_realloc(
			    env, re_len, &out_data->data)) != 0)
				return (ret);
		       /*
			* In the partial case, we have built the item into
			* out_data already using __db_buildpartial. Just need
			* to pad from the end of out_data, not from data->size.
			*/
		       size = out_data->size;
		} else {
			if ((ret = __os_malloc(
			    env, re_len, &out_data->data)) != 0)
				return (ret);
			memcpy(out_data->data, data->data, size);
		}
		memset((u_int8_t *)out_data->data + size, re_pad,
		    re_len - size);
		out_data->size = re_len;
	}

	return (ret);
}

/*
 * __dbc_put_secondaries --
 *	Insert the secondary keys, and validate the foreign key constraints.
 */
static inline int
__dbc_put_secondaries(dbc,
    pkey, data, orig_data, s_count, s_keys_buf, put_statep)
	DBC *dbc;
	DBT *pkey, *data, *orig_data, *s_keys_buf;
	int s_count;
	u_int32_t *put_statep;
{
	DB *dbp, *sdbp;
	DBC *fdbc, *sdbc;
	DBT fdata, oldpkey, *skeyp, temppkey, tempskey, *tskeyp;
	ENV *env;
	int cmp, ret, rmw, t_ret;
	u_int32_t nskey;

	dbp = dbc->dbp;
	env = dbp->env;
	fdbc = sdbc = NULL;
	sdbp = NULL;
	ret = t_ret = 0;
	rmw = FLD_ISSET(*put_statep, DBC_PUT_RMW) ? DB_RMW : 0;

	/*
	 * Loop through the secondaries.  (Step 3.)
	 *
	 * Note that __db_s_first and __db_s_next will take care of
	 * thread-locking and refcounting issues.
	 */
	for (ret = __db_s_first(dbp, &sdbp), skeyp = s_keys_buf;
	    sdbp != NULL && ret == 0;
	    ret = __db_s_next(&sdbp, dbc->txn), ++skeyp) {
		DB_ASSERT(env, skeyp - s_keys_buf < s_count);
		/*
		 * Don't process this secondary if the key is immutable and we
		 * know that the old record exists.  This optimization can't be
		 * used if we have not checked for the old record yet.
		 */
		if (FLD_ISSET(*put_statep, DBC_PUT_HAVEREC) &&
		    !FLD_ISSET(*put_statep, DBC_PUT_NODEL) &&
		    FLD_ISSET(sdbp->s_assoc_flags, DB_ASSOC_IMMUTABLE_KEY))
			continue;

		/*
		 * Call the callback for this secondary, to get the
		 * appropriate secondary key.
		 */
		if ((ret = sdbp->s_callback(sdbp,
		    pkey, data, skeyp)) != 0) {
			/* Not indexing is equivalent to an empty key set. */
			if (ret == DB_DONOTINDEX) {
				F_SET(skeyp, DB_DBT_MULTIPLE);
				skeyp->size = 0;
				ret = 0;
			} else
				goto err;
		}

		if (sdbp->s_foreign != NULL &&
		    (ret = __db_cursor_int(sdbp->s_foreign,
		    dbc->thread_info, dbc->txn, sdbp->s_foreign->type,
		    PGNO_INVALID, 0, dbc->locker, &fdbc)) != 0)
			goto err;

		/*
		 * Mark the secondary key DBT(s) as set -- that is, the
		 * callback returned at least one secondary key.
		 *
		 * Also, if this secondary index is associated with a foreign
		 * database, check that the foreign db contains the key(s) to
		 * maintain referential integrity.  Set flags in fdata to avoid
		 * mem copying, we just need to know existence.  We need to do
		 * this check before setting DB_DBT_ISSET, otherwise __dbc_get
		 * will overwrite the flag values.
		 */
		if (F_ISSET(skeyp, DB_DBT_MULTIPLE)) {
#ifdef DIAGNOSTIC
			__db_check_skeyset(sdbp, skeyp);
#endif
			for (tskeyp = (DBT *)skeyp->data, nskey = skeyp->size;
			     nskey > 0; nskey--, tskeyp++) {
				if (fdbc != NULL) {
					memset(&fdata, 0, sizeof(DBT));
					F_SET(&fdata,
					    DB_DBT_PARTIAL | DB_DBT_USERMEM);
					if ((ret = __dbc_get(
					    fdbc, tskeyp, &fdata,
					    DB_SET | rmw)) == DB_NOTFOUND ||
					    ret == DB_KEYEMPTY) {
						ret = DB_FOREIGN_CONFLICT;
						break;
					}
				}
				F_SET(tskeyp, DB_DBT_ISSET);
			}
			tskeyp = (DBT *)skeyp->data;
			nskey = skeyp->size;
		} else {
			if (fdbc != NULL) {
				memset(&fdata, 0, sizeof(DBT));
				F_SET(&fdata, DB_DBT_PARTIAL | DB_DBT_USERMEM);
				if ((ret = __dbc_get(fdbc, skeyp, &fdata,
				    DB_SET | rmw)) == DB_NOTFOUND ||
				    ret == DB_KEYEMPTY)
					ret = DB_FOREIGN_CONFLICT;
			}
			F_SET(skeyp, DB_DBT_ISSET);
			tskeyp = skeyp;
			nskey = 1;
		}
		if (fdbc != NULL && (t_ret = __dbc_close(fdbc)) != 0 &&
		    ret == 0)
			ret = t_ret;
		fdbc = NULL;
		if (ret != 0)
			goto err;

		/*
		 * If we have the old record, we can generate and remove any
		 * old secondary key(s) now.  We can also skip the secondary
		 * put if there is no change.
		 */
		if (FLD_ISSET(*put_statep, DBC_PUT_HAVEREC)) {
			if ((ret = __dbc_del_oldskey(sdbp, dbc,
			    skeyp, pkey, orig_data)) == DB_KEYEXIST)
				continue;
			else if (ret != 0)
				goto err;
		}
		if (nskey == 0)
			continue;

		/*
		 * Open a cursor in this secondary.
		 *
		 * Use the same locker ID as our primary cursor, so that
		 * we're guaranteed that the locks don't conflict (e.g. in CDB
		 * or if we're subdatabases that share and want to lock a
		 * metadata page).
		 */
		if ((ret = __db_cursor_int(sdbp, dbc->thread_info, dbc->txn,
		    sdbp->type, PGNO_INVALID, 0, dbc->locker, &sdbc)) != 0)
			goto err;

		/*
		 * If we're in CDB, updates will fail since the new cursor
		 * isn't a writer.  However, we hold the WRITE lock in the
		 * primary and will for as long as our new cursor lasts,
		 * and the primary and secondary share a lock file ID,
		 * so it's safe to consider this a WRITER.  The close
		 * routine won't try to put anything because we don't
		 * really have a lock.
		 */
		if (CDB_LOCKING(env)) {
			DB_ASSERT(env, sdbc->mylock.off == LOCK_INVALID);
			F_SET(sdbc, DBC_WRITER);
		}

		/*
		 * Swap the primary key to the byte order of this secondary, if
		 * necessary.  By doing this now, we can compare directly
		 * against the data already in the secondary without having to
		 * swap it after reading.
		 */
		SWAP_IF_NEEDED(sdbp, pkey);

		for (; nskey > 0 && ret == 0; nskey--, tskeyp++) {
			/* Skip this key if it is already in the database. */
			if (!F_ISSET(tskeyp, DB_DBT_ISSET))
				continue;

			/*
			 * There are three cases here--
			 * 1) The secondary supports sorted duplicates.
			 *	If we attempt to put a secondary/primary pair
			 *	that already exists, that's a duplicate
			 *	duplicate, and c_put will return DB_KEYEXIST
			 *	(see __db_duperr).  This will leave us with
			 *	exactly one copy of the secondary/primary pair,
			 *	and this is just right--we'll avoid deleting it
			 *	later, as the old and new secondaries will
			 *	match (since the old secondary is the dup dup
			 *	that's already there).
			 * 2) The secondary supports duplicates, but they're not
			 *	sorted.  We need to avoid putting a duplicate
			 *	duplicate, because the matching old and new
			 *	secondaries will prevent us from deleting
			 *	anything and we'll wind up with two secondary
			 *	records that point to the same primary key.  Do
			 *	a c_get(DB_GET_BOTH);  only do the put if the
			 *	secondary doesn't exist.
			 * 3) The secondary doesn't support duplicates at all.
			 *	In this case, secondary keys must be unique;
			 *	if another primary key already exists for this
			 *	secondary key, we have to either overwrite it
			 *	or not put this one, and in either case we've
			 *	corrupted the secondary index.  Do a
			 *	c_get(DB_SET).  If the secondary/primary pair
			 *	already exists, do nothing;  if the secondary
			 *	exists with a different primary, return an
			 *	error;  and if the secondary does not exist,
			 *	put it.
			 */
			if (!F_ISSET(sdbp, DB_AM_DUP)) {
				/* Case 3. */
				memset(&oldpkey, 0, sizeof(DBT));
				F_SET(&oldpkey, DB_DBT_MALLOC);
				ret = __dbc_get(sdbc,
				    tskeyp, &oldpkey, rmw | DB_SET);
				if (ret == 0) {
					cmp = __bam_defcmp(sdbp,
					    &oldpkey, pkey);
					__os_ufree(env, oldpkey.data);
					/*
					 * If the secondary key is unchanged,
					 * skip the put and go on to the next
					 * one.
					 */
					if (cmp == 0)
						continue;

					__db_errx(env, "%s%s",
			    "Put results in a non-unique secondary key in an ",
			    "index not configured to support duplicates");
					ret = EINVAL;
				}
				if (ret != DB_NOTFOUND && ret != DB_KEYEMPTY)
					break;
			} else if (!F_ISSET(sdbp, DB_AM_DUPSORT)) {
				/* Case 2. */
				DB_INIT_DBT(tempskey,
				    tskeyp->data, tskeyp->size);
				DB_INIT_DBT(temppkey,
				    pkey->data, pkey->size);
				ret = __dbc_get(sdbc, &tempskey, &temppkey,
				    rmw | DB_GET_BOTH);
				if (ret != DB_NOTFOUND && ret != DB_KEYEMPTY)
					break;
			}

			ret = __dbc_put(sdbc, tskeyp, pkey,
			    DB_UPDATE_SECONDARY);

			/*
			 * We don't know yet whether this was a put-overwrite
			 * that in fact changed nothing.  If it was, we may get
			 * DB_KEYEXIST.  This is not an error.
			 */
			if (ret == DB_KEYEXIST)
				ret = 0;
		}

		/* Make sure the primary key is back in native byte-order. */
		SWAP_IF_NEEDED(sdbp, pkey);

		if ((t_ret = __dbc_close(sdbc)) != 0 && ret == 0)
			ret = t_ret;

		if (ret != 0)
			goto err;

		/*
		 * Mark that we have a key for this secondary so we can check
		 * it later before deleting the old one.  We can't set it
		 * earlier or it would be cleared in the calls above.
		 */
		F_SET(skeyp, DB_DBT_ISSET);
	}
err:	if (sdbp != NULL &&
	    (t_ret = __db_s_done(sdbp, dbc->txn)) != 0 && ret == 0)
		ret = t_ret;
	COMPQUIET(s_count, 0);
	return (ret);
}

static int
__dbc_put_primary(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp, *sdbp;
	DBC *dbc_n, *pdbc;
	DBT oldkey, olddata, newdata;
	DBT *all_skeys, *skeyp, *tskeyp;
	ENV *env;
	int ret, t_ret, s_count;
	u_int32_t nskey, put_state, rmw;

	dbp = dbc->dbp;
	env = dbp->env;
	ret = t_ret = s_count = 0;
	put_state = 0;
	sdbp = NULL;
	pdbc = dbc_n = NULL;
	all_skeys = NULL;
	memset(&newdata, 0, sizeof(DBT));
	memset(&olddata, 0, sizeof(DBT));

	/*
	 * We do multiple cursor operations in some cases and subsequently
	 * access the data DBT information.  Set DB_DBT_MALLOC so we don't risk
	 * modification of the data between our uses of it.
	 */
	F_SET(&olddata, DB_DBT_MALLOC);

	/*
	 * We have at least one secondary which we may need to update.
	 *
	 * There is a rather vile locking issue here.  Secondary gets
	 * will always involve acquiring a read lock in the secondary,
	 * then acquiring a read lock in the primary.  Ideally, we
	 * would likewise perform puts by updating all the secondaries
	 * first, then doing the actual put in the primary, to avoid
	 * deadlock (since having multiple threads doing secondary
	 * gets and puts simultaneously is probably a common case).
	 *
	 * However, if this put is a put-overwrite--and we have no way to
	 * tell in advance whether it will be--we may need to delete
	 * an outdated secondary key.  In order to find that old
	 * secondary key, we need to get the record we're overwriting,
	 * before we overwrite it.
	 *
	 * (XXX: It would be nice to avoid this extra get, and have the
	 * underlying put routines somehow pass us the old record
	 * since they need to traverse the tree anyway.  I'm saving
	 * this optimization for later, as it's a lot of work, and it
	 * would be hard to fit into this locking paradigm anyway.)
	 *
	 * The simple thing to do would be to go get the old record before
	 * we do anything else.  Unfortunately, though, doing so would
	 * violate our "secondary, then primary" lock acquisition
	 * ordering--even in the common case where no old primary record
	 * exists, we'll still acquire and keep a lock on the page where
	 * we're about to do the primary insert.
	 *
	 * To get around this, we do the following gyrations, which
	 * hopefully solve this problem in the common case:
	 *
	 * 1) If this is a c_put(DB_CURRENT), go ahead and get the
	 *    old record.  We already hold the lock on this page in
	 *    the primary, so no harm done, and we'll need the primary
	 *    key (which we weren't passed in this case) to do any
	 *    secondary puts anyway.
	 *    If this is a put(DB_APPEND), then we need to insert the item,
	 *    so that we can know the key value. So go ahead and insert. In
	 *    the case of a put(DB_APPEND) without secondaries it is
	 *    implemented in the __db_put method as an optimization.
	 *
	 * 2) If we're doing a partial put, we need to perform the
	 *    get on the primary key right away, since we don't have
	 *    the whole datum that the secondary key is based on.
	 *    We may also need to pad out the record if the primary
	 *    has a fixed record length.
	 *
	 * 3) Loop through the secondary indices, putting into each a
	 *    new secondary key that corresponds to the new record.
	 *
	 * 4) If we haven't done so in (1) or (2), get the old primary
	 *    key/data pair.  If one does not exist--the common case--we're
	 *    done with secondary indices, and can go straight on to the
	 *    primary put.
	 *
	 * 5) If we do have an old primary key/data pair, however, we need
	 *    to loop through all the secondaries a second time and delete
	 *    the old secondary in each.
	 */
	s_count = __db_s_count(dbp);
	if ((ret = __os_calloc(env,
	    (u_int)s_count, sizeof(DBT), &all_skeys)) != 0)
		goto err;

	/*
	 * Primary indices can't have duplicates, so only DB_APPEND,
	 * DB_CURRENT, DB_KEYFIRST, and DB_KEYLAST make any sense.  Other flags
	 * should have been caught by the checking routine, but
	 * add a sprinkling of paranoia.
	 */
	DB_ASSERT(env, flags == DB_APPEND || flags == DB_CURRENT ||
	      flags == DB_KEYFIRST || flags == DB_KEYLAST ||
	      flags == DB_NOOVERWRITE || flags == DB_OVERWRITE_DUP);

	/*
	 * We'll want to use DB_RMW in a few places, but it's only legal
	 * when locking is on.
	 */
	rmw = STD_LOCKING(dbc) ? DB_RMW : 0;
	if (rmw)
		FLD_SET(put_state, DBC_PUT_RMW);

	/* Resolve the primary key if required (Step 1). */
	if (flags == DB_CURRENT) {
		if ((ret = __dbc_put_resolve_key(dbc,
		    &oldkey, &olddata, &put_state, flags)) != 0)
			goto err;
		key = &oldkey;
	} else if (flags == DB_APPEND) {
		if ((ret = __dbc_put_append(dbc,
		    key, data, &put_state, flags)) != 0)
			goto err;
	}

	/*
	 * PUT_NOOVERWRITE with secondaries is a troublesome case. We need
	 * to check that the insert will work prior to making any changes
	 * to secondaries. Try to work within the locking constraints outlined
	 * above.
	 *
	 * This is DB->put (DB_NOOVERWRITE). DBC->put(DB_NODUPDATA) is not
	 * relevant since it is only valid on DBs that support duplicates,
	 * which primaries with secondaries can't have.
	 */
	if (flags == DB_NOOVERWRITE) {
		/* Don't bother retrieving the data. */
		F_SET(key, DB_DBT_ISSET);
		olddata.dlen = 0;
		olddata.flags = DB_DBT_PARTIAL | DB_DBT_USERMEM;
		if (__dbc_get(dbc, key, &olddata, DB_SET) != DB_NOTFOUND) {
			ret = DB_KEYEXIST;
			goto done;
		}
	}

	/*
	 * Check for partial puts using DB_DBT_PARTIAL (Step 2).
	 */
	if (F_ISSET(data, DB_DBT_PARTIAL)) {
		if ((ret = __dbc_put_partial(dbc,
		    key, data, &olddata, &newdata, &put_state, flags)) != 0)
			goto err;
	} else {
		newdata = *data;
	}

	/*
	 * Check for partial puts, with fixed length record databases (Step 2).
	 */
	if ((dbp->type == DB_RECNO && F_ISSET(dbp, DB_AM_FIXEDLEN)) ||
	    (dbp->type == DB_QUEUE)) {
		if ((ret = __dbc_put_fixed_len(dbc, data, &newdata)) != 0)
			goto err;
	}

	/* Validate any foreign databases, and update secondaries. (Step 3). */
	if ((ret = __dbc_put_secondaries(dbc, key, &newdata,
	    &olddata, s_count, all_skeys, &put_state))
	    != 0)
		goto err;
	/*
	 * If we've already got the old primary key/data pair, the secondary
	 * updates are already done.
	 */
	if (FLD_ISSET(put_state, DBC_PUT_HAVEREC))
		goto done;

	/*
	 * If still necessary, go get the old primary key/data.  (Step 4.)
	 *
	 * See the comments in step 2.  This is real familiar.
	 */
	if ((ret = __dbc_idup(dbc, &pdbc, 0)) != 0)
		goto err;
	DB_ASSERT(env, flags != DB_CURRENT);
	F_SET(key, DB_DBT_ISSET);
	ret = __dbc_get(pdbc, key, &olddata, rmw | DB_SET);
	if (ret == DB_KEYEMPTY || ret == DB_NOTFOUND) {
		FLD_SET(put_state, DBC_PUT_NODEL);
		ret = 0;
	}
	if ((t_ret = __dbc_close(pdbc)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		goto err;

	/*
	 * Check whether we do in fact have an old record we may need to
	 * delete.  (Step 5).
	 */
	if (FLD_ISSET(put_state, DBC_PUT_NODEL))
		goto done;

	for (ret = __db_s_first(dbp, &sdbp), skeyp = all_skeys;
	    sdbp != NULL && ret == 0;
	    ret = __db_s_next(&sdbp, dbc->txn), skeyp++) {
		DB_ASSERT(env, skeyp - all_skeys < s_count);
		/*
		 * Don't process this secondary if the key is immutable.  We
		 * know that the old record exists, so this optimization can
		 * always be used.
		 */
		if (FLD_ISSET(sdbp->s_assoc_flags, DB_ASSOC_IMMUTABLE_KEY))
			continue;

		if ((ret = __dbc_del_oldskey(sdbp, dbc,
		    skeyp, key, &olddata)) != 0 && ret != DB_KEYEXIST)
			goto err;
	}
	if (ret != 0)
		goto err;

done:
err:
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 && ret == 0)
		ret = t_ret;

	/* If newdata or olddata were used, free their buffers. */
	if (newdata.data != NULL && newdata.data != data->data)
		__os_free(env, newdata.data);
	if (olddata.data != NULL)
		__os_ufree(env, olddata.data);

	CDB_LOCKING_DONE(env, dbc);

	if (sdbp != NULL &&
	    (t_ret = __db_s_done(sdbp, dbc->txn)) != 0 && ret == 0)
		ret = t_ret;

	for (skeyp = all_skeys; skeyp - all_skeys < s_count; skeyp++) {
		if (F_ISSET(skeyp, DB_DBT_MULTIPLE)) {
			for (nskey = skeyp->size, tskeyp = (DBT *)skeyp->data;
			    nskey > 0;
			    nskey--, tskeyp++)
				FREE_IF_NEEDED(env, tskeyp);
		}
		FREE_IF_NEEDED(env, skeyp);
	}
	if (all_skeys != NULL)
		__os_free(env, all_skeys);
	return (ret);
}

/*
 * __dbc_put --
 *	Put using a cursor.
 *
 * PUBLIC: int __dbc_put __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_put(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp;
	int ret;

	dbp = dbc->dbp;
	ret = 0;

	/*
	 * Putting to secondary indices is forbidden;  when we need to
	 * internally update one, we're called with a private flag,
	 * DB_UPDATE_SECONDARY, which does the right thing but won't return an
	 * error during flag checking.
	 *
	 * As a convenience, many places that want the default DB_KEYLAST
	 * behavior call DBC->put with flags == 0.  Protect lower-level code
	 * here by translating that.
	 *
	 * Lastly, the DB_OVERWRITE_DUP flag is equivalent to DB_KEYLAST unless
	 * there are sorted duplicates.  Limit the number of places that need
	 * to test for it explicitly.
	 */
	if (flags == DB_UPDATE_SECONDARY || flags == 0 ||
	    (flags == DB_OVERWRITE_DUP && !F_ISSET(dbp, DB_AM_DUPSORT)))
		flags = DB_KEYLAST;

	CDB_LOCKING_INIT(dbc->env, dbc);

	/*
	 * Check to see if we are a primary and have secondary indices.
	 * If we are not, we save ourselves a good bit of trouble and
	 * just skip to the "normal" put.
	 */
	if (DB_IS_PRIMARY(dbp) &&
	    ((ret = __dbc_put_primary(dbc, key, data, flags)) != 0))
		return (ret);

	/*
	 * If this is an append operation, the insert was done prior to the
	 * secondary updates, so we are finished.
	 */
	if (flags == DB_APPEND)
		return (ret);

#ifdef HAVE_COMPRESSION
	if (DB_IS_COMPRESSED(dbp))
		return (__bamc_compress_put(dbc, key, data, flags));
#endif

	return (__dbc_iput(dbc, key, data, flags));
}

/*
 * __dbc_iput --
 *	Implementation of put using a cursor.
 *
 * PUBLIC: int __dbc_iput __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_iput(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DBC *dbc_n, *oldopd, *opd;
	db_pgno_t pgno;
	int ret, t_ret;
	u_int32_t tmp_flags;

	/*
	 * Cursor Cleanup Note:
	 * All of the cursors passed to the underlying access methods by this
	 * routine are duplicated cursors.  On return, any referenced pages
	 * will be discarded, and, if the cursor is not intended to be used
	 * again, the close function will be called.  So, pages/locks that
	 * the cursor references do not need to be resolved by the underlying
	 * functions.
	 */
	dbc_n = NULL;
	ret = t_ret = 0;

	/*
	 * If we have an off-page duplicates cursor, and the operation applies
	 * to it, perform the operation.  Duplicate the cursor and call the
	 * underlying function.
	 *
	 * Off-page duplicate trees are locked in the primary tree, that is,
	 * we acquire a write lock in the primary tree and no locks in the
	 * off-page dup tree.  If the put operation is done in an off-page
	 * duplicate tree, call the primary cursor's upgrade routine first.
	 */
	if (dbc->internal->opd != NULL &&
	    (flags == DB_AFTER || flags == DB_BEFORE || flags == DB_CURRENT)) {
		/*
		 * A special case for hash off-page duplicates.  Hash doesn't
		 * support (and is documented not to support) put operations
		 * relative to a cursor which references an already deleted
		 * item.  For consistency, apply the same criteria to off-page
		 * duplicates as well.
		 */
		if (dbc->dbtype == DB_HASH && F_ISSET(
		    ((BTREE_CURSOR *)(dbc->internal->opd->internal)),
		    C_DELETED)) {
			ret = DB_NOTFOUND;
			goto err;
		}

		if ((ret = dbc->am_writelock(dbc)) != 0 ||
		    (ret = __dbc_dup(dbc, &dbc_n, DB_POSITION)) != 0)
			goto err;
		opd = dbc_n->internal->opd;
		if ((ret = opd->am_put(
		    opd, key, data, flags, NULL)) != 0)
			goto err;
		goto done;
	}

	/*
	 * Perform an operation on the main cursor.  Duplicate the cursor,
	 * and call the underlying function.
	 */
	if (flags == DB_AFTER || flags == DB_BEFORE || flags == DB_CURRENT)
		tmp_flags = DB_POSITION;
	else
		tmp_flags = 0;

	/*
	 * If this cursor is going to be closed immediately, we don't
	 * need to take precautions to clean it up on error.
	 */
	if (F_ISSET(dbc, DBC_TRANSIENT | DBC_PARTITIONED))
		dbc_n = dbc;
	else if ((ret = __dbc_idup(dbc, &dbc_n, tmp_flags)) != 0)
		goto err;

	pgno = PGNO_INVALID;
	if ((ret = dbc_n->am_put(dbc_n, key, data, flags, &pgno)) != 0)
		goto err;

	/*
	 * We may be referencing a new off-page duplicates tree.  Acquire
	 * a new cursor and call the underlying function.
	 */
	if (pgno != PGNO_INVALID) {
		oldopd = dbc_n->internal->opd;
		if ((ret = __dbc_newopd(dbc, pgno, oldopd, &opd)) != 0) {
			dbc_n->internal->opd = opd;
			goto err;
		}

		dbc_n->internal->opd = opd;
		opd->internal->pdbc = dbc_n;

		if (flags == DB_NOOVERWRITE)
			flags = DB_KEYLAST;
		if ((ret = opd->am_put(
		    opd, key, data, flags, NULL)) != 0)
			goto err;
	}

done:
err:	/* Cleanup and cursor resolution. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __dbc_del_oldskey --
 *	Delete an old secondary key, if necessary.
 *	Returns DB_KEYEXIST if the new and old keys match..
 */
static int
__dbc_del_oldskey(sdbp, dbc, skey, pkey, olddata)
	DB *sdbp;
	DBC *dbc;
	DBT *skey, *pkey, *olddata;
{
	DB *dbp;
	DBC *sdbc;
	DBT *toldskeyp, *tskeyp;
	DBT oldskey, temppkey, tempskey;
	ENV *env;
	int ret, t_ret;
	u_int32_t i, noldskey, nsame, nskey, rmw;

	sdbc = NULL;
	dbp = sdbp->s_primary;
	env = dbp->env;
	nsame = 0;
	rmw = STD_LOCKING(dbc) ? DB_RMW : 0;

	/*
	 * Get the old secondary key.
	 */
	memset(&oldskey, 0, sizeof(DBT));
	if ((ret = sdbp->s_callback(sdbp, pkey, olddata, &oldskey)) != 0) {
		if (ret == DB_DONOTINDEX ||
		    (F_ISSET(&oldskey, DB_DBT_MULTIPLE) && oldskey.size == 0))
			/* There's no old key to delete. */
			ret = 0;
		return (ret);
	}

	if (F_ISSET(&oldskey, DB_DBT_MULTIPLE)) {
#ifdef DIAGNOSTIC
		__db_check_skeyset(sdbp, &oldskey);
#endif
		toldskeyp = (DBT *)oldskey.data;
		noldskey = oldskey.size;
	} else {
		toldskeyp = &oldskey;
		noldskey = 1;
	}

	if (F_ISSET(skey, DB_DBT_MULTIPLE)) {
		nskey = skey->size;
		skey = (DBT *)skey->data;
	} else
		nskey = F_ISSET(skey, DB_DBT_ISSET) ? 1 : 0;

	for (; noldskey > 0 && ret == 0; noldskey--, toldskeyp++) {
		/*
		 * Check whether this old secondary key is also a new key
		 * before we delete it.  Note that bt_compare is (and must be)
		 * set no matter what access method we're in.
		 */
		for (i = 0, tskeyp = skey; i < nskey; i++, tskeyp++)
			if (((BTREE *)sdbp->bt_internal)->bt_compare(sdbp,
			    toldskeyp, tskeyp) == 0) {
				nsame++;
				F_CLR(tskeyp, DB_DBT_ISSET);
				break;
			}

		if (i < nskey) {
			FREE_IF_NEEDED(env, toldskeyp);
			continue;
		}

		if (sdbc == NULL) {
			if ((ret = __db_cursor_int(sdbp,
			    dbc->thread_info, dbc->txn, sdbp->type,
			    PGNO_INVALID, 0, dbc->locker, &sdbc)) != 0)
				goto err;
			if (CDB_LOCKING(env)) {
				DB_ASSERT(env,
				    sdbc->mylock.off == LOCK_INVALID);
				F_SET(sdbc, DBC_WRITER);
			}
		}

		/*
		 * Don't let c_get(DB_GET_BOTH) stomp on our data.  Use
		 * temporary DBTs instead.
		 */
		SWAP_IF_NEEDED(sdbp, pkey);
		DB_INIT_DBT(temppkey, pkey->data, pkey->size);
		DB_INIT_DBT(tempskey, toldskeyp->data, toldskeyp->size);
		if ((ret = __dbc_get(sdbc,
		    &tempskey, &temppkey, rmw | DB_GET_BOTH)) == 0)
			ret = __dbc_del(sdbc, DB_UPDATE_SECONDARY);
		else if (ret == DB_NOTFOUND)
			ret = __db_secondary_corrupt(dbp);
		SWAP_IF_NEEDED(sdbp, pkey);
		FREE_IF_NEEDED(env, toldskeyp);
	}

err:	for (; noldskey > 0; noldskey--, toldskeyp++)
		FREE_IF_NEEDED(env, toldskeyp);
	FREE_IF_NEEDED(env, &oldskey);
	if (sdbc != NULL && (t_ret = __dbc_close(sdbc)) != 0 && ret == 0)
		ret = t_ret;
	if (ret == 0 && nsame == nskey)
		return (DB_KEYEXIST);
	return (ret);
}

/*
 * __db_duperr()
 *	Error message: we don't currently support sorted duplicate duplicates.
 * PUBLIC: int __db_duperr __P((DB *, u_int32_t));
 */
int
__db_duperr(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	/*
	 * If we run into this error while updating a secondary index,
	 * don't yell--there's no clean way to pass DB_NODUPDATA in along
	 * with DB_UPDATE_SECONDARY, but we may run into this problem
	 * in a normal, non-error course of events.
	 *
	 * !!!
	 * If and when we ever permit duplicate duplicates in sorted-dup
	 * databases, we need to either change the secondary index code
	 * to check for dup dups, or we need to maintain the implicit
	 * "DB_NODUPDATA" behavior for databases with DB_AM_SECONDARY set.
	 */
	if (flags != DB_NODUPDATA && !F_ISSET(dbp, DB_AM_SECONDARY))
		__db_errx(dbp->env,
		    "Duplicate data items are not supported with sorted data");
	return (DB_KEYEXIST);
}

/*
 * __dbc_cleanup --
 *	Clean up duplicate cursors.
 *
 * PUBLIC: int __dbc_cleanup __P((DBC *, DBC *, int));
 */
int
__dbc_cleanup(dbc, dbc_n, failed)
	DBC *dbc, *dbc_n;
	int failed;
{
	DB *dbp;
	DBC *opd;
	DBC_INTERNAL *internal;
	DB_MPOOLFILE *mpf;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	internal = dbc->internal;
	ret = 0;

	/* Discard any pages we're holding. */
	if (internal->page != NULL) {
		if ((t_ret = __memp_fput(mpf, dbc->thread_info,
		     internal->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		internal->page = NULL;
	}
	opd = internal->opd;
	if (opd != NULL && opd->internal->page != NULL) {
		if ((t_ret = __memp_fput(mpf, dbc->thread_info,
		    opd->internal->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		opd->internal->page = NULL;
	}

	/*
	 * If dbc_n is NULL, there's no internal cursor swapping to be done
	 * and no dbc_n to close--we probably did the entire operation on an
	 * offpage duplicate cursor.  Just return.
	 *
	 * If dbc and dbc_n are the same, we're either inside a DB->{put/get}
	 * operation, and as an optimization we performed the operation on
	 * the main cursor rather than on a duplicated one, or we're in a
	 * bulk get that can't have moved the cursor (DB_MULTIPLE with the
	 * initial c_get operation on an off-page dup cursor).  Just
	 * return--either we know we didn't move the cursor, or we're going
	 * to close it before we return to application code, so we're sure
	 * not to visibly violate the "cursor stays put on error" rule.
	 */
	if (dbc_n == NULL || dbc == dbc_n)
		return (ret);

	if (dbc_n->internal->page != NULL) {
		if ((t_ret = __memp_fput(mpf, dbc->thread_info,
		    dbc_n->internal->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		dbc_n->internal->page = NULL;
	}
	opd = dbc_n->internal->opd;
	if (opd != NULL && opd->internal->page != NULL) {
		if ((t_ret = __memp_fput(mpf, dbc->thread_info,
		     opd->internal->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		opd->internal->page = NULL;
	}

	/*
	 * If we didn't fail before entering this routine or just now when
	 * freeing pages, swap the interesting contents of the old and new
	 * cursors.
	 */
	if (!failed && ret == 0) {
		if (opd != NULL)
			opd->internal->pdbc = dbc;
		if (internal->opd != NULL)
			internal->opd->internal->pdbc = dbc_n;
		dbc->internal = dbc_n->internal;
		dbc_n->internal = internal;
	}

	/*
	 * Close the cursor we don't care about anymore.  The close can fail,
	 * but we only expect DB_LOCK_DEADLOCK failures.  This violates our
	 * "the cursor is unchanged on error" semantics, but since all you can
	 * do with a DB_LOCK_DEADLOCK failure is close the cursor, I believe
	 * that's OK.
	 *
	 * XXX
	 * There's no way to recover from failure to close the old cursor.
	 * All we can do is move to the new position and return an error.
	 *
	 * XXX
	 * We might want to consider adding a flag to the cursor, so that any
	 * subsequent operations other than close just return an error?
	 */
	if ((t_ret = __dbc_close(dbc_n)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * If this was an update that is supporting dirty reads
	 * then we may have just swapped our read for a write lock
	 * which is held by the surviving cursor.  We need
	 * to explicitly downgrade this lock.  The closed cursor
	 * may only have had a read lock.
	 */
	if (F_ISSET(dbp, DB_AM_READ_UNCOMMITTED) &&
	    dbc->internal->lock_mode == DB_LOCK_WRITE) {
		if ((t_ret =
		    __TLPUT(dbc, dbc->internal->lock)) != 0 && ret == 0)
			ret = t_ret;
		if (t_ret == 0)
			dbc->internal->lock_mode = DB_LOCK_WWRITE;
		if (dbc->internal->page != NULL && (t_ret =
		    __memp_shared(dbp->mpf, dbc->internal->page)) != 0 &&
		    ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __dbc_secondary_get_pp --
 *	This wrapper function for DBC->pget() is the DBC->get() function
 *	for a secondary index cursor.
 *
 * PUBLIC: int __dbc_secondary_get_pp __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_secondary_get_pp(dbc, skey, data, flags)
	DBC *dbc;
	DBT *skey, *data;
	u_int32_t flags;
{
	DB_ASSERT(dbc->env, F_ISSET(dbc->dbp, DB_AM_SECONDARY));
	return (__dbc_pget_pp(dbc, skey, NULL, data, flags));
}

/*
 * __dbc_pget --
 *	Get a primary key/data pair through a secondary index.
 *
 * PUBLIC: int __dbc_pget __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_pget(dbc, skey, pkey, data, flags)
	DBC *dbc;
	DBT *skey, *pkey, *data;
	u_int32_t flags;
{
	DB *pdbp, *sdbp;
	DBC *dbc_n, *pdbc;
	DBT nullpkey;
	u_int32_t save_pkey_flags, tmp_flags, tmp_read_locking, tmp_rmw;
	int pkeymalloc, ret, t_ret;

	sdbp = dbc->dbp;
	pdbp = sdbp->s_primary;
	dbc_n = NULL;
	pkeymalloc = t_ret = 0;

	/*
	 * The challenging part of this function is getting the behavior
	 * right for all the various permutations of DBT flags.  The
	 * next several blocks handle the various cases we need to
	 * deal with specially.
	 */

	/*
	 * We may be called with a NULL pkey argument, if we've been
	 * wrapped by a 2-DBT get call.  If so, we need to use our
	 * own DBT.
	 */
	if (pkey == NULL) {
		memset(&nullpkey, 0, sizeof(DBT));
		pkey = &nullpkey;
	}

	/* Clear OR'd in additional bits so we can check for flag equality. */
	tmp_rmw = LF_ISSET(DB_RMW);
	LF_CLR(DB_RMW);

	SET_READ_LOCKING_FLAGS(dbc, tmp_read_locking);
	/*
	 * DB_GET_RECNO is a special case, because we're interested not in
	 * the primary key/data pair, but rather in the primary's record
	 * number.
	 */
	if (flags == DB_GET_RECNO) {
		if (tmp_rmw)
			F_SET(dbc, DBC_RMW);
		F_SET(dbc, tmp_read_locking);
		ret = __dbc_pget_recno(dbc, pkey, data, flags);
		if (tmp_rmw)
			F_CLR(dbc, DBC_RMW);
		/* Clear the temp flags, but leave WAS_READ_COMMITTED. */
		F_CLR(dbc, tmp_read_locking & ~DBC_WAS_READ_COMMITTED);
		return (ret);
	}

	/*
	 * If the DBTs we've been passed don't have any of the
	 * user-specified memory management flags set, we want to make sure
	 * we return values using the DBTs dbc->rskey, dbc->rkey, and
	 * dbc->rdata, respectively.
	 *
	 * There are two tricky aspects to this:  first, we need to pass
	 * skey and pkey *in* to the initial c_get on the secondary key,
	 * since either or both may be looked at by it (depending on the
	 * get flag).  Second, we must not use a normal DB->get call
	 * on the secondary, even though that's what we want to accomplish,
	 * because the DB handle may be free-threaded.  Instead,
	 * we open a cursor, then take steps to ensure that we actually use
	 * the rkey/rdata from the *secondary* cursor.
	 *
	 * We accomplish all this by passing in the DBTs we started out
	 * with to the c_get, but swapping the contents of rskey and rkey,
	 * respectively, into rkey and rdata;  __db_ret will treat them like
	 * the normal key/data pair in a c_get call, and will realloc them as
	 * need be (this is "step 1").  Then, for "step 2", we swap back
	 * rskey/rkey/rdata to normal, and do a get on the primary with the
	 * secondary dbc appointed as the owner of the returned-data memory.
	 *
	 * Note that in step 2, we copy the flags field in case we need to
	 * pass down a DB_DBT_PARTIAL or other flag that is compatible with
	 * letting DB do the memory management.
	 */

	/*
	 * It is correct, though slightly sick, to attempt a partial get of a
	 * primary key.  However, if we do so here, we'll never find the
	 * primary record;  clear the DB_DBT_PARTIAL field of pkey just for the
	 * duration of the next call.
	 */
	save_pkey_flags = pkey->flags;
	F_CLR(pkey, DB_DBT_PARTIAL);

	/*
	 * Now we can go ahead with the meat of this call.  First, get the
	 * primary key from the secondary index.  (What exactly we get depends
	 * on the flags, but the underlying cursor get will take care of the
	 * dirty work.)  Duplicate the cursor, in case the later get on the
	 * primary fails.
	 */
	switch (flags) {
	case DB_CURRENT:
	case DB_GET_BOTHC:
	case DB_NEXT:
	case DB_NEXT_DUP:
	case DB_NEXT_NODUP:
	case DB_PREV:
	case DB_PREV_DUP:
	case DB_PREV_NODUP:
		tmp_flags = DB_POSITION;
		break;
	default:
		tmp_flags = 0;
		break;
	}

	if (F_ISSET(dbc, DBC_PARTITIONED | DBC_TRANSIENT))
		dbc_n = dbc;
	else if ((ret = __dbc_dup(dbc, &dbc_n, tmp_flags)) != 0)
		return (ret);

	F_SET(dbc_n, DBC_TRANSIENT);

	if (tmp_rmw)
		F_SET(dbc_n, DBC_RMW);
	F_SET(dbc_n, tmp_read_locking);

	/*
	 * If we've been handed a primary key, it will be in native byte order,
	 * so we need to swap it before reading from the secondary.
	 */
	if (flags == DB_GET_BOTH || flags == DB_GET_BOTHC ||
	    flags == DB_GET_BOTH_RANGE)
		SWAP_IF_NEEDED(sdbp, pkey);

retry:	/* Step 1. */
	dbc_n->rdata = dbc->rkey;
	dbc_n->rkey = dbc->rskey;
	ret = __dbc_get(dbc_n, skey, pkey, flags);
	/* Restore pkey's flags in case we stomped the PARTIAL flag. */
	pkey->flags = save_pkey_flags;

	/*
	 * We need to swap the primary key to native byte order if we read it
	 * successfully, or if we swapped it on entry above.  We can't return
	 * with the application's data modified.
	 */
	if (ret == 0 || flags == DB_GET_BOTH || flags == DB_GET_BOTHC ||
	    flags == DB_GET_BOTH_RANGE)
		SWAP_IF_NEEDED(sdbp, pkey);

	if (ret != 0)
		goto err;

	/*
	 * Now we're ready for "step 2".  If either or both of pkey and data do
	 * not have memory management flags set--that is, if DB is managing
	 * their memory--we need to swap around the rkey/rdata structures so
	 * that we don't wind up trying to use memory managed by the primary
	 * database cursor, which we'll close before we return.
	 *
	 * !!!
	 * If you're carefully following the bouncing ball, you'll note that in
	 * the DB-managed case, the buffer hanging off of pkey is the same as
	 * dbc->rkey->data.  This is just fine;  we may well realloc and stomp
	 * on it when we return, if we're doing a DB_GET_BOTH and need to
	 * return a different partial or key (depending on the comparison
	 * function), but this is safe.
	 *
	 * !!!
	 * We need to use __db_cursor_int here rather than simply calling
	 * pdbp->cursor, because otherwise, if we're in CDB, we'll allocate a
	 * new locker ID and leave ourselves open to deadlocks.  (Even though
	 * we're only acquiring read locks, we'll still block if there are any
	 * waiters.)
	 */
	if ((ret = __db_cursor_int(pdbp, dbc->thread_info,
	    dbc->txn, pdbp->type, PGNO_INVALID, 0, dbc->locker, &pdbc)) != 0)
		goto err;

	F_SET(pdbc, tmp_read_locking |
	     F_ISSET(dbc, DBC_READ_UNCOMMITTED | DBC_READ_COMMITTED | DBC_RMW));

	/*
	 * We're about to use pkey a second time.  If DB_DBT_MALLOC is set on
	 * it, we'll leak the memory we allocated the first time.  Thus, set
	 * DB_DBT_REALLOC instead so that we reuse that memory instead of
	 * leaking it.
	 *
	 * Alternatively, if the application is handling copying for pkey, we
	 * need to take a copy now.  The copy will be freed on exit from
	 * __dbc_pget_pp (and we must be coming through there if DB_DBT_USERCOPY
	 * is set).  In the case of DB_GET_BOTH_RANGE, the pkey supplied by
	 * the application has already been copied in but the value may have
	 * changed in the search.  In that case, free the original copy and get
	 * a new one.
	 *
	 * !!!
	 * This assumes that the user must always specify a compatible realloc
	 * function if a malloc function is specified.  I think this is a
	 * reasonable requirement.
	 */
	if (F_ISSET(pkey, DB_DBT_MALLOC)) {
		F_CLR(pkey, DB_DBT_MALLOC);
		F_SET(pkey, DB_DBT_REALLOC);
		pkeymalloc = 1;
	} else if (F_ISSET(pkey, DB_DBT_USERCOPY)) {
		if (flags == DB_GET_BOTH_RANGE)
			__dbt_userfree(sdbp->env, NULL, pkey, NULL);
		if ((ret = __dbt_usercopy(sdbp->env, pkey)) != 0)
			goto err;
	}

	/*
	 * Do the actual get.  Set DBC_TRANSIENT since we don't care about
	 * preserving the position on error, and it's faster.  SET_RET_MEM so
	 * that the secondary DBC owns any returned-data memory.
	 */
	F_SET(pdbc, DBC_TRANSIENT);
	SET_RET_MEM(pdbc, dbc);
	ret = __dbc_get(pdbc, pkey, data, DB_SET);

	/*
	 * If the item wasn't found in the primary, this is a bug; our
	 * secondary has somehow gotten corrupted, and contains elements that
	 * don't correspond to anything in the primary.  Complain.
	 */

	/* Now close the primary cursor. */
	if ((t_ret = __dbc_close(pdbc)) != 0 && ret == 0)
		ret = t_ret;

	else if (ret == DB_NOTFOUND) {
		if (!F_ISSET(pdbc, DBC_READ_UNCOMMITTED))
			ret = __db_secondary_corrupt(pdbp);
		else switch (flags) {
		case DB_GET_BOTHC:
		case DB_NEXT:
		case DB_NEXT_DUP:
		case DB_NEXT_NODUP:
		case DB_PREV:
		case DB_PREV_DUP:
		case DB_PREV_NODUP:
			goto retry;
		default:
			break;
		}
	}

err:	/* Cleanup and cursor resolution. */
	if ((t_ret = __dbc_cleanup(dbc, dbc_n, ret)) != 0 && ret == 0)
		ret = t_ret;
	if (pkeymalloc) {
		/*
		 * If pkey had a MALLOC flag, we need to restore it; otherwise,
		 * if the user frees the buffer but reuses the DBT without
		 * NULL'ing its data field or changing the flags, we may drop
		 * core.
		 */
		F_CLR(pkey, DB_DBT_REALLOC);
		F_SET(pkey, DB_DBT_MALLOC);
	}

	return (ret);
}

/*
 * __dbc_pget_recno --
 *	Perform a DB_GET_RECNO c_pget on a secondary index.  Returns
 * the secondary's record number in the pkey field and the primary's
 * in the data field.
 */
static int
__dbc_pget_recno(sdbc, pkey, data, flags)
	DBC *sdbc;
	DBT *pkey, *data;
	u_int32_t flags;
{
	DB *pdbp, *sdbp;
	DBC *pdbc;
	DBT discardme, primary_key;
	ENV *env;
	db_recno_t oob;
	u_int32_t rmw;
	int ret, t_ret;

	sdbp = sdbc->dbp;
	pdbp = sdbp->s_primary;
	env = sdbp->env;
	pdbc = NULL;
	ret = t_ret = 0;

	rmw = LF_ISSET(DB_RMW);

	memset(&discardme, 0, sizeof(DBT));
	F_SET(&discardme, DB_DBT_USERMEM | DB_DBT_PARTIAL);

	oob = RECNO_OOB;

	/*
	 * If the primary is an rbtree, we want its record number, whether
	 * or not the secondary is one too.  Fetch the recno into "data".
	 *
	 * If it's not an rbtree, return RECNO_OOB in "data".
	 */
	if (F_ISSET(pdbp, DB_AM_RECNUM)) {
		/*
		 * Get the primary key, so we can find the record number
		 * in the primary. (We're uninterested in the secondary key.)
		 */
		memset(&primary_key, 0, sizeof(DBT));
		F_SET(&primary_key, DB_DBT_MALLOC);
		if ((ret = __dbc_get(sdbc,
		    &discardme, &primary_key, rmw | DB_CURRENT)) != 0)
			return (ret);

		/*
		 * Open a cursor on the primary, set it to the right record,
		 * and fetch its recno into "data".
		 *
		 * (See __dbc_pget for comments on the use of __db_cursor_int.)
		 *
		 * SET_RET_MEM so that the secondary DBC owns any returned-data
		 * memory.
		 */
		if ((ret = __db_cursor_int(pdbp, sdbc->thread_info, sdbc->txn,
		    pdbp->type, PGNO_INVALID, 0, sdbc->locker, &pdbc)) != 0)
			goto perr;
		SET_RET_MEM(pdbc, sdbc);
		if ((ret = __dbc_get(pdbc,
		    &primary_key, &discardme, rmw | DB_SET)) != 0)
			goto perr;

		ret = __dbc_get(pdbc, &discardme, data, rmw | DB_GET_RECNO);

perr:		__os_ufree(env, primary_key.data);
		if (pdbc != NULL &&
		    (t_ret = __dbc_close(pdbc)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			return (ret);
	} else if ((ret = __db_retcopy(env, data, &oob,
		    sizeof(oob), &sdbc->rkey->data, &sdbc->rkey->ulen)) != 0)
			return (ret);

	/*
	 * If the secondary is an rbtree, we want its record number, whether
	 * or not the primary is one too.  Fetch the recno into "pkey".
	 *
	 * If it's not an rbtree, return RECNO_OOB in "pkey".
	 */
	if (F_ISSET(sdbp, DB_AM_RECNUM))
		return (__dbc_get(sdbc, &discardme, pkey, flags));
	else
		return (__db_retcopy(env, pkey, &oob,
		    sizeof(oob), &sdbc->rdata->data, &sdbc->rdata->ulen));
}

/*
 * __db_wrlock_err -- do not have a write lock.
 */
static int
__db_wrlock_err(env)
	ENV *env;
{
	__db_errx(env, "Write attempted on read-only cursor");
	return (EPERM);
}

/*
 * __dbc_del_secondary --
 *	Perform a delete operation on a secondary index:  call through
 *	to the primary and delete the primary record that this record
 *	points to.
 *
 *	Note that deleting the primary record will call c_del on all
 *	the secondaries, including this one;  thus, it is not necessary
 *	to execute both this function and an actual delete.
 */
static int
__dbc_del_secondary(dbc)
	DBC *dbc;
{
	DB *pdbp;
	DBC *pdbc;
	DBT skey, pkey;
	ENV *env;
	int ret, t_ret;
	u_int32_t rmw;

	pdbp = dbc->dbp->s_primary;
	env = pdbp->env;
	rmw = STD_LOCKING(dbc) ? DB_RMW : 0;

	/*
	 * Get the current item that we're pointing at.
	 * We don't actually care about the secondary key, just
	 * the primary.
	 */
	memset(&skey, 0, sizeof(DBT));
	memset(&pkey, 0, sizeof(DBT));
	F_SET(&skey, DB_DBT_PARTIAL | DB_DBT_USERMEM);
	if ((ret = __dbc_get(dbc, &skey, &pkey, DB_CURRENT)) != 0)
		return (ret);

	SWAP_IF_NEEDED(dbc->dbp, &pkey);

	/*
	 * Create a cursor on the primary with our locker ID,
	 * so that when it calls back, we don't conflict.
	 *
	 * We create a cursor explicitly because there's no
	 * way to specify the same locker ID if we're using
	 * locking but not transactions if we use the DB->del
	 * interface.  This shouldn't be any less efficient
	 * anyway.
	 */
	if ((ret = __db_cursor_int(pdbp, dbc->thread_info, dbc->txn,
	    pdbp->type, PGNO_INVALID, 0, dbc->locker, &pdbc)) != 0)
		return (ret);

	/*
	 * See comment in __dbc_put--if we're in CDB,
	 * we already hold the locks we need, and we need to flag
	 * the cursor as a WRITER so we don't run into errors
	 * when we try to delete.
	 */
	if (CDB_LOCKING(env)) {
		DB_ASSERT(env, pdbc->mylock.off == LOCK_INVALID);
		F_SET(pdbc, DBC_WRITER);
	}

	/*
	 * Set the new cursor to the correct primary key.  Then
	 * delete it.  We don't really care about the datum;
	 * just reuse our skey DBT.
	 *
	 * If the primary get returns DB_NOTFOUND, something is amiss--
	 * every record in the secondary should correspond to some record
	 * in the primary.
	 */
	if ((ret = __dbc_get(pdbc, &pkey, &skey, DB_SET | rmw)) == 0)
		ret = __dbc_del(pdbc, 0);
	else if (ret == DB_NOTFOUND)
		ret = __db_secondary_corrupt(pdbp);

	if ((t_ret = __dbc_close(pdbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __dbc_del_primary --
 *	Perform a delete operation on a primary index.  Loop through
 *	all the secondary indices which correspond to this primary
 *	database, and delete any secondary keys that point at the current
 *	record.
 *
 * PUBLIC: int __dbc_del_primary __P((DBC *));
 */
int
__dbc_del_primary(dbc)
	DBC *dbc;
{
	DB *dbp, *sdbp;
	DBC *sdbc;
	DBT *tskeyp;
	DBT data, pkey, skey, temppkey, tempskey;
	ENV *env;
	u_int32_t nskey, rmw;
	int ret, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;
	sdbp = NULL;
	rmw = STD_LOCKING(dbc) ? DB_RMW : 0;

	/*
	 * If we're called at all, we have at least one secondary.
	 * (Unfortunately, we can't assert this without grabbing the mutex.)
	 * Get the current record so that we can construct appropriate
	 * secondary keys as needed.
	 */
	memset(&pkey, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	if ((ret = __dbc_get(dbc, &pkey, &data, DB_CURRENT)) != 0)
		return (ret);

	memset(&skey, 0, sizeof(DBT));
	for (ret = __db_s_first(dbp, &sdbp);
	    sdbp != NULL && ret == 0;
	    ret = __db_s_next(&sdbp, dbc->txn)) {
		/*
		 * Get the secondary key for this secondary and the current
		 * item.
		 */
		if ((ret = sdbp->s_callback(sdbp, &pkey, &data, &skey)) != 0) {
			/* Not indexing is equivalent to an empty key set. */
			if (ret == DB_DONOTINDEX) {
				F_SET(&skey, DB_DBT_MULTIPLE);
				skey.size = 0;
			} else /* We had a substantive error.  Bail. */
				goto err;
		}

#ifdef DIAGNOSTIC
		if (F_ISSET(&skey, DB_DBT_MULTIPLE))
			__db_check_skeyset(sdbp, &skey);
#endif

		if (F_ISSET(&skey, DB_DBT_MULTIPLE)) {
			tskeyp = (DBT *)skey.data;
			nskey = skey.size;
			if (nskey == 0)
				continue;
		} else {
			tskeyp = &skey;
			nskey = 1;
		}

		/* Open a secondary cursor. */
		if ((ret = __db_cursor_int(sdbp,
		    dbc->thread_info, dbc->txn, sdbp->type,
		    PGNO_INVALID, 0, dbc->locker, &sdbc)) != 0)
			goto err;
		/* See comment above and in __dbc_put. */
		if (CDB_LOCKING(env)) {
			DB_ASSERT(env, sdbc->mylock.off == LOCK_INVALID);
			F_SET(sdbc, DBC_WRITER);
		}

		for (; nskey > 0; nskey--, tskeyp++) {
			/*
			 * Set the secondary cursor to the appropriate item.
			 * Delete it.
			 *
			 * We want to use DB_RMW if locking is on; it's only
			 * legal then, though.
			 *
			 * !!!
			 * Don't stomp on any callback-allocated buffer in skey
			 * when we do a c_get(DB_GET_BOTH); use a temp DBT
			 * instead.  Similarly, don't allow pkey to be
			 * invalidated when the cursor is closed.
			 */
			DB_INIT_DBT(tempskey, tskeyp->data, tskeyp->size);
			SWAP_IF_NEEDED(sdbp, &pkey);
			DB_INIT_DBT(temppkey, pkey.data, pkey.size);
			if ((ret = __dbc_get(sdbc, &tempskey, &temppkey,
			    DB_GET_BOTH | rmw)) == 0)
				ret = __dbc_del(sdbc, DB_UPDATE_SECONDARY);
			else if (ret == DB_NOTFOUND)
				ret = __db_secondary_corrupt(dbp);
			SWAP_IF_NEEDED(sdbp, &pkey);
			FREE_IF_NEEDED(env, tskeyp);
		}

		if ((t_ret = __dbc_close(sdbc)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;

		/*
		 * In the common case where there is a single secondary key, we
		 * will have freed any application-allocated data in skey
		 * already.  In the multiple key case, we need to free it here.
		 * It is safe to do this twice as the macro resets the data
		 * field.
		 */
		FREE_IF_NEEDED(env, &skey);
	}

err:	if (sdbp != NULL &&
	    (t_ret = __db_s_done(sdbp, dbc->txn)) != 0 && ret == 0)
		ret = t_ret;
	FREE_IF_NEEDED(env, &skey);
	return (ret);
}

/*
 * __dbc_del_foreign --
 *	Apply the foreign database constraints for a particular foreign
 *	database when an item is being deleted (dbc points at item being deleted
 *	in the foreign database.)
 *
 *      Delete happens in dbp, check for occurrences of key in pdpb.
 *      Terminology:
 *        Foreign db = Where delete occurs (dbp).
 *        Secondary db = Where references to dbp occur (sdbp, a secondary)
 *        Primary db = sdbp's primary database, references to dbp are secondary
 *                      keys here
 *        Foreign Key = Key being deleted in dbp (fkey)
 *        Primary Key = Key of the corresponding entry in sdbp's primary (pkey).
 */
static int
__dbc_del_foreign(dbc)
	DBC *dbc;
{
	DB_FOREIGN_INFO *f_info;
	DB *dbp, *pdbp, *sdbp;
	DBC *pdbc, *sdbc;
	DBT data, fkey, pkey;
	ENV *env;
	u_int32_t flags, rmw;
	int changed, ret, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;

	memset(&fkey, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));
	if ((ret = __dbc_get(dbc, &fkey, &data, DB_CURRENT)) != 0)
		return (ret);

	LIST_FOREACH(f_info, &(dbp->f_primaries), f_links) {
		sdbp = f_info->dbp;
		pdbp = sdbp->s_primary;
		flags = f_info->flags;

		rmw = (STD_LOCKING(dbc) &&
		    !LF_ISSET(DB_FOREIGN_ABORT)) ? DB_RMW : 0;

		/*
		 * Handle CDB locking.  Some of this is copied from
		 * __dbc_del_primary, but a bit more acrobatics are required.
		 * If we're not going to abort, then we need to get a write
		 * cursor.  If CDB_ALLDB is set, then only one write cursor is
		 * allowed and we hold it, so we fudge things and promote the
		 * cursor on the other DBs manually, it won't cause a problem.
		 * If CDB_ALLDB is not set, then we go through the usual route
		 * to make sure we block as necessary.  If there are any open
		 * read cursors on sdbp, the delete or put call later will
		 * block.
		 *
		 * If NULLIFY is set, we'll need a cursor on the primary to
		 * update it with the nullified data.  Because primary and
		 * secondary dbs share a lock file ID in CDB, we open a cursor
		 * on the secondary and then get another writeable cursor on the
		 * primary via __db_cursor_int to avoid deadlocking.
		 */
		sdbc = pdbc = NULL;
		if (!LF_ISSET(DB_FOREIGN_ABORT) && CDB_LOCKING(env) &&
		    !F_ISSET(env->dbenv, DB_ENV_CDB_ALLDB)) {
			ret = __db_cursor(sdbp,
			    dbc->thread_info, dbc->txn, &sdbc, DB_WRITECURSOR);
			if (LF_ISSET(DB_FOREIGN_NULLIFY) && ret == 0) {
				ret = __db_cursor_int(pdbp,
				    dbc->thread_info, dbc->txn, pdbp->type,
				    PGNO_INVALID, 0, dbc->locker, &pdbc);
				F_SET(pdbc, DBC_WRITER);
			}
		} else {
			ret = __db_cursor_int(sdbp, dbc->thread_info, dbc->txn,
			    sdbp->type, PGNO_INVALID, 0, dbc->locker, &sdbc);
			if (LF_ISSET(DB_FOREIGN_NULLIFY) && ret == 0)
				ret = __db_cursor_int(pdbp, dbc->thread_info,
				    dbc->txn, pdbp->type, PGNO_INVALID, 0,
				    dbc->locker, &pdbc);
			}
		if (ret != 0) {
			if (sdbc != NULL)
				(void)__dbc_close(sdbc);
			return (ret);
		}
		if (CDB_LOCKING(env) && F_ISSET(env->dbenv, DB_ENV_CDB_ALLDB)) {
			DB_ASSERT(env, sdbc->mylock.off == LOCK_INVALID);
			F_SET(sdbc, DBC_WRITER);
			if (LF_ISSET(DB_FOREIGN_NULLIFY) && pdbc != NULL) {
				DB_ASSERT(env,
				    pdbc->mylock.off == LOCK_INVALID);
				F_SET(pdbc, DBC_WRITER);
			}
		}

		/*
		 * There are three actions possible when a foreign database has
		 * items corresponding to a deleted item:
		 * DB_FOREIGN_ABORT - The delete operation should be aborted.
		 * DB_FOREIGN_CASCADE - All corresponding foreign items should
		 *    be deleted.
		 * DB_FOREIGN_NULLIFY - A callback needs to be made, allowing
		 *    the application to modify the data DBT from the
		 *    associated database.  If the callback makes a
		 *    modification, the updated item needs to replace the
		 *    original item in the foreign db
		 */
		memset(&pkey, 0, sizeof(DBT));
		memset(&data, 0, sizeof(DBT));
		ret = __dbc_pget(sdbc, &fkey, &pkey, &data, DB_SET|rmw);

		if (ret == DB_NOTFOUND) {
			/* No entry means no constraint */
			ret = __dbc_close(sdbc);
			if (LF_ISSET(DB_FOREIGN_NULLIFY) &&
			    (t_ret = __dbc_close(pdbc)) != 0)
				ret = t_ret;
			if (ret != 0)
				return (ret);
			continue;
		} else if (ret != 0) {
			/* Just return the error code from the pget */
			(void)__dbc_close(sdbc);
			if (LF_ISSET(DB_FOREIGN_NULLIFY))
				(void)__dbc_close(pdbc);
			return (ret);
		} else if (LF_ISSET(DB_FOREIGN_ABORT)) {
			/* If the record exists and ABORT is set, we're done */
			if ((ret = __dbc_close(sdbc)) != 0)
				return (ret);
			return (DB_FOREIGN_CONFLICT);
		}

		/*
		 * There were matching items in the primary DB, and the action
		 * is either DB_FOREIGN_CASCADE or DB_FOREIGN_NULLIFY.
		 */
		while (ret == 0) {
			if (LF_ISSET(DB_FOREIGN_CASCADE)) {
				/*
				 * Don't use the DB_UPDATE_SECONDARY flag,
				 * since we want the delete to cascade into the
				 * secondary's primary.
				 */
				if ((ret = __dbc_del(sdbc, 0)) != 0) {
					__db_err(env, ret,
	    "Attempt to execute cascading delete in a foreign index failed");
					break;
				}
			} else if (LF_ISSET(DB_FOREIGN_NULLIFY)) {
				changed = 0;
				if ((ret = f_info->callback(sdbp,
				    &pkey, &data, &fkey, &changed)) != 0) {
					__db_err(env, ret,
				    "Foreign database application callback");
					break;
				}

				/*
				 * If the user callback modified the DBT and
				 * a put on the primary failed.
				 */
				if (changed && (ret = __dbc_put(pdbc,
				    &pkey, &data, DB_KEYFIRST)) != 0) {
					__db_err(env, ret,
  "Attempt to overwrite item in foreign database with nullified value failed");
					break;
				}
			}
			/* retrieve the next matching item from the prim. db */
			memset(&pkey, 0, sizeof(DBT));
			memset(&data, 0, sizeof(DBT));
			ret = __dbc_pget(sdbc,
			    &fkey, &pkey, &data, DB_NEXT_DUP|rmw);
		}

		if (ret == DB_NOTFOUND)
			ret = 0;
		if ((t_ret = __dbc_close(sdbc)) != 0 && ret == 0)
			ret = t_ret;
		if (LF_ISSET(DB_FOREIGN_NULLIFY) &&
		    (t_ret = __dbc_close(pdbc)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			return (ret);
	}

	return (ret);
}

/*
 * __db_s_first --
 *	Get the first secondary, if any are present, from the primary.
 *
 * PUBLIC: int __db_s_first __P((DB *, DB **));
 */
int
__db_s_first(pdbp, sdbpp)
	DB *pdbp, **sdbpp;
{
	DB *sdbp;

	MUTEX_LOCK(pdbp->env, pdbp->mutex);
	sdbp = LIST_FIRST(&pdbp->s_secondaries);

	/* See __db_s_next. */
	if (sdbp != NULL)
		sdbp->s_refcnt++;
	MUTEX_UNLOCK(pdbp->env, pdbp->mutex);

	*sdbpp = sdbp;

	return (0);
}

/*
 * __db_s_next --
 *	Get the next secondary in the list.
 *
 * PUBLIC: int __db_s_next __P((DB **, DB_TXN *));
 */
int
__db_s_next(sdbpp, txn)
	DB **sdbpp;
	DB_TXN *txn;
{
	DB *sdbp, *pdbp, *closeme;
	ENV *env;
	int ret;

	/*
	 * Secondary indices are kept in a linked list, s_secondaries,
	 * off each primary DB handle.  If a primary is free-threaded,
	 * this list may only be traversed or modified while the primary's
	 * thread mutex is held.
	 *
	 * The tricky part is that we don't want to hold the thread mutex
	 * across the full set of secondary puts necessary for each primary
	 * put, or we'll wind up essentially single-threading all the puts
	 * to the handle;  the secondary puts will each take about as
	 * long as the primary does, and may require I/O.  So we instead
	 * hold the thread mutex only long enough to follow one link to the
	 * next secondary, and then we release it before performing the
	 * actual secondary put.
	 *
	 * The only danger here is that we might legitimately close a
	 * secondary index in one thread while another thread is performing
	 * a put and trying to update that same secondary index.  To
	 * prevent this from happening, we refcount the secondary handles.
	 * If close is called on a secondary index handle while we're putting
	 * to it, it won't really be closed--the refcount will simply drop,
	 * and we'll be responsible for closing it here.
	 */
	sdbp = *sdbpp;
	pdbp = sdbp->s_primary;
	env = pdbp->env;
	closeme = NULL;

	MUTEX_LOCK(env, pdbp->mutex);
	DB_ASSERT(env, sdbp->s_refcnt != 0);
	if (--sdbp->s_refcnt == 0) {
		LIST_REMOVE(sdbp, s_links);
		closeme = sdbp;
	}
	sdbp = LIST_NEXT(sdbp, s_links);
	if (sdbp != NULL)
		sdbp->s_refcnt++;
	MUTEX_UNLOCK(env, pdbp->mutex);

	*sdbpp = sdbp;

	/*
	 * closeme->close() is a wrapper;  call __db_close explicitly.
	 */
	if (closeme == NULL)
		ret = 0;
	else 
		ret = __db_close(closeme, txn, 0);

	return (ret);
}

/*
 * __db_s_done --
 *	Properly decrement the refcount on a secondary database handle we're
 *	using, without calling __db_s_next.
 *
 * PUBLIC: int __db_s_done __P((DB *, DB_TXN *));
 */
int
__db_s_done(sdbp, txn)
	DB *sdbp;
	DB_TXN *txn;
{
	DB *pdbp;
	ENV *env;
	int doclose, ret;

	pdbp = sdbp->s_primary;
	env = pdbp->env;
	doclose = 0;

	MUTEX_LOCK(env, pdbp->mutex);
	DB_ASSERT(env, sdbp->s_refcnt != 0);
	if (--sdbp->s_refcnt == 0) {
		LIST_REMOVE(sdbp, s_links);
		doclose = 1;
	}
	MUTEX_UNLOCK(env, pdbp->mutex);

	if (doclose == 0)
		ret = 0;
	else
		ret = __db_close(sdbp, txn, 0);
	return (ret);
}

/*
 * __db_s_count --
 *	Count the number of secondaries associated with a given primary.
 */
static int
__db_s_count(pdbp)
	DB *pdbp;
{
	DB *sdbp;
	ENV *env;
	int count;

	env = pdbp->env;
	count = 0;

	MUTEX_LOCK(env, pdbp->mutex);
	for (sdbp = LIST_FIRST(&pdbp->s_secondaries);
	    sdbp != NULL;
	    sdbp = LIST_NEXT(sdbp, s_links))
		++count;
	MUTEX_UNLOCK(env, pdbp->mutex);

	return (count);
}

/*
 * __db_buildpartial --
 *	Build the record that will result after a partial put is applied to
 *	an existing record.
 *
 *	This should probably be merged with __bam_build, but that requires
 *	a little trickery if we plan to keep the overflow-record optimization
 *	in that function.
 *
 * PUBLIC: int __db_buildpartial __P((DB *, DBT *, DBT *, DBT *));
 */
int
__db_buildpartial(dbp, oldrec, partial, newrec)
	DB *dbp;
	DBT *oldrec, *partial, *newrec;
{
	ENV *env;
	u_int32_t len, nbytes;
	u_int8_t *buf;
	int ret;

	env = dbp->env;

	DB_ASSERT(env, F_ISSET(partial, DB_DBT_PARTIAL));

	memset(newrec, 0, sizeof(DBT));

	nbytes = __db_partsize(oldrec->size, partial);
	newrec->size = nbytes;

	if ((ret = __os_malloc(env, nbytes, &buf)) != 0)
		return (ret);
	newrec->data = buf;

	/* Nul or pad out the buffer, for any part that isn't specified. */
	memset(buf,
	    F_ISSET(dbp, DB_AM_FIXEDLEN) ? ((BTREE *)dbp->bt_internal)->re_pad :
	    0, nbytes);

	/* Copy in any leading data from the original record. */
	memcpy(buf, oldrec->data,
	    partial->doff > oldrec->size ? oldrec->size : partial->doff);

	/* Copy the data from partial. */
	memcpy(buf + partial->doff, partial->data, partial->size);

	/* Copy any trailing data from the original record. */
	len = partial->doff + partial->dlen;
	if (oldrec->size > len)
		memcpy(buf + partial->doff + partial->size,
		    (u_int8_t *)oldrec->data + len, oldrec->size - len);

	return (0);
}

/*
 * __db_partsize --
 *	Given the number of bytes in an existing record and a DBT that
 *	is about to be partial-put, calculate the size of the record
 *	after the put.
 *
 *	This code is called from __bam_partsize.
 *
 * PUBLIC: u_int32_t __db_partsize __P((u_int32_t, DBT *));
 */
u_int32_t
__db_partsize(nbytes, data)
	u_int32_t nbytes;
	DBT *data;
{

	/*
	 * There are really two cases here:
	 *
	 * Case 1: We are replacing some bytes that do not exist (i.e., they
	 * are past the end of the record).  In this case the number of bytes
	 * we are replacing is irrelevant and all we care about is how many
	 * bytes we are going to add from offset.  So, the new record length
	 * is going to be the size of the new bytes (size) plus wherever those
	 * new bytes begin (doff).
	 *
	 * Case 2: All the bytes we are replacing exist.  Therefore, the new
	 * size is the oldsize (nbytes) minus the bytes we are replacing (dlen)
	 * plus the bytes we are adding (size).
	 */
	if (nbytes < data->doff + data->dlen)		/* Case 1 */
		return (data->doff + data->size);

	return (nbytes + data->size - data->dlen);	/* Case 2 */
}

#ifdef DIAGNOSTIC
/*
 * __db_check_skeyset --
 *	Diagnostic check that the application's callback returns a set of
 *	secondary keys without repeats.
 *
 * PUBLIC: #ifdef DIAGNOSTIC
 * PUBLIC: void __db_check_skeyset __P((DB *, DBT *));
 * PUBLIC: #endif
 */
void
__db_check_skeyset(sdbp, skeyp)
	DB *sdbp;
	DBT *skeyp;
{
	DBT *firstkey, *lastkey, *key1, *key2;
	ENV *env;

	env = sdbp->env;

	firstkey = (DBT *)skeyp->data;
	lastkey = firstkey + skeyp->size;
	for (key1 = firstkey; key1 < lastkey; key1++)
		for (key2 = key1 + 1; key2 < lastkey; key2++)
			DB_ASSERT(env,
			    ((BTREE *)sdbp->bt_internal)->bt_compare(sdbp,
			    key1, key2) != 0);
}
#endif
