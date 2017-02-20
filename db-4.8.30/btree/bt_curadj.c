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
#include "dbinc/btree.h"
#include "dbinc/mp.h"

static int __bam_opd_cursor __P((DB *, DBC *, db_pgno_t, u_int32_t, u_int32_t));

/*
 * Cursor adjustments are logged if they are for subtransactions.  This is
 * because it's possible for a subtransaction to adjust cursors which will
 * still be active after the subtransaction aborts, and so which must be
 * restored to their previous locations.  Cursors that can be both affected
 * by our cursor adjustments and active after our transaction aborts can
 * only be found in our parent transaction -- cursors in other transactions,
 * including other child transactions of our parent, must have conflicting
 * locker IDs, and so cannot be affected by adjustments in this transaction.
 */

/*
 * __bam_ca_delete --
 *	Update the cursors when items are deleted and when already deleted
 *	items are overwritten.  Return the number of relevant cursors found.
 *
 * PUBLIC: int __bam_ca_delete __P((DB *, db_pgno_t, u_int32_t, int, int *));
 */
int
__bam_ca_delete(dbp, pgno, indx, delete, countp)
	DB *dbp;
	db_pgno_t pgno;
	u_int32_t indx;
	int delete, *countp;
{
	BTREE_CURSOR *cp;
	DB *ldbp;
	DBC *dbc;
	ENV *env;
	int count;		/* !!!: Has to contain max number of cursors. */

	env = dbp->env;

	/*
	 * Adjust the cursors.  We have the page write locked, so the
	 * only other cursors that can be pointing at a page are
	 * those in the same thread of control.  Unfortunately, we don't
	 * know that they're using the same DB handle, so traverse
	 * all matching DB handles in the same ENV, then all cursors
	 * on each matching DB handle.
	 *
	 * Each cursor is single-threaded, so we only need to lock the
	 * list of DBs and then the list of cursors in each DB.
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (count = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			cp = (BTREE_CURSOR *)dbc->internal;
			if (cp->pgno == pgno && cp->indx == indx &&
			    !MVCC_SKIP_CURADJ(dbc, pgno)) {
				/*
				 * [#8032] This assert is checking
				 * for possible race conditions where we
				 * hold a cursor position without a lock.
				 * Unfortunately, there are paths in the
				 * Btree code that do not satisfy these
				 * conditions. None of them are known to
				 * be a problem, but this assert should
				 * be re-activated when the Btree stack
				 * code is re-written.
				DB_ASSERT(env, !STD_LOCKING(dbc) ||
				    cp->lock_mode != DB_LOCK_NG);
				 */
				if (delete) {
					F_SET(cp, C_DELETED);
					/*
					 * If we're deleting the item, we can't
					 * keep a streaming offset cached.
					 */
					cp->stream_start_pgno = PGNO_INVALID;
				} else
					F_CLR(cp, C_DELETED);

#ifdef HAVE_COMPRESSION
				/*
				 * We also set the C_COMPRESS_MODIFIED flag,
				 * which prompts the compression code to look
				 * for it's current entry again if it needs to.
				 *
				 * The flag isn't cleared, because the
				 * compression code still needs to do that even
				 * for an entry that becomes undeleted.
				 *
				 * This flag also needs to be set if an entry is
				 * updated, but since the compression code
				 * always deletes before an update, setting it
				 * here is sufficient.
				 */
				F_SET(cp, C_COMPRESS_MODIFIED);
#endif

				++count;
			}
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (countp != NULL)
		*countp = count;
	return (0);
}

/*
 * __ram_ca_delete --
 *	Return if any relevant cursors found.
 *
 * PUBLIC: int __ram_ca_delete __P((DB *, db_pgno_t, int *));
 */
int
__ram_ca_delete(dbp, root_pgno, foundp)
	DB *dbp;
	db_pgno_t root_pgno;
	int *foundp;
{
	DB *ldbp;
	DBC *dbc;
	ENV *env;
	int found;

	env = dbp->env;

	/*
	 * Review the cursors.  See the comment in __bam_ca_delete().
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    found == 0 && ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links)
			if (dbc->internal->root == root_pgno &&
			    !MVCC_SKIP_CURADJ(dbc, root_pgno)) {
				found = 1;
				break;
			}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	*foundp = found;
	return (0);
}

/*
 * __bam_ca_di --
 *	Adjust the cursors during a delete or insert.
 *
 * PUBLIC: int __bam_ca_di __P((DBC *, db_pgno_t, u_int32_t, int));
 */
int
__bam_ca_di(my_dbc, pgno, indx, adjust)
	DBC *my_dbc;
	db_pgno_t pgno;
	u_int32_t indx;
	int adjust;
{
	DB *dbp, *ldbp;
	DBC *dbc;
	DBC_INTERNAL *cp;
	DB_LSN lsn;
	DB_TXN *my_txn;
	ENV *env;
	int found, ret;

	dbp = my_dbc->dbp;
	env = dbp->env;

	my_txn = IS_SUBTRANSACTION(my_dbc->txn) ? my_dbc->txn : NULL;

	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			if (dbc->dbtype == DB_RECNO)
				continue;
			cp = dbc->internal;
			if (cp->pgno == pgno && cp->indx >= indx &&
			    (dbc == my_dbc || !MVCC_SKIP_CURADJ(dbc, pgno))) {
				/* Cursor indices should never be negative. */
				DB_ASSERT(env, cp->indx != 0 || adjust > 0);
				/* [#8032]
				DB_ASSERT(env, !STD_LOCKING(dbc) ||
				    cp->lock_mode != DB_LOCK_NG);
				*/
				cp->indx += adjust;
				if (my_txn != NULL && dbc->txn != my_txn)
					found = 1;
			}
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (found != 0 && DBC_LOGGING(my_dbc)) {
		if ((ret = __bam_curadj_log(dbp, my_dbc->txn, &lsn, 0,
		    DB_CA_DI, pgno, 0, 0, (u_int32_t)adjust, indx, 0)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __bam_opd_cursor -- create a new opd cursor.
 */
static int
__bam_opd_cursor(dbp, dbc, first, tpgno, ti)
	DB *dbp;
	DBC *dbc;
	db_pgno_t tpgno;
	u_int32_t first, ti;
{
	BTREE_CURSOR *cp, *orig_cp;
	DBC *dbc_nopd;
	int ret;

	orig_cp = (BTREE_CURSOR *)dbc->internal;
	dbc_nopd = NULL;

	/*
	 * Allocate a new cursor and create the stack.  If duplicates
	 * are sorted, we've just created an off-page duplicate Btree.
	 * If duplicates aren't sorted, we've just created a Recno tree.
	 *
	 * Note that in order to get here at all, there shouldn't be
	 * an old off-page dup cursor--to augment the checking dbc_newopd
	 * will do, assert this.
	 */
	DB_ASSERT(dbp->env, orig_cp->opd == NULL);
	if ((ret = __dbc_newopd(dbc, tpgno, orig_cp->opd, &dbc_nopd)) != 0)
		return (ret);

	cp = (BTREE_CURSOR *)dbc_nopd->internal;
	cp->pgno = tpgno;
	cp->indx = ti;

	if (dbp->dup_compare == NULL) {
		/*
		 * Converting to off-page Recno trees is tricky.  The
		 * record number for the cursor is the index + 1 (to
		 * convert to 1-based record numbers).
		 */
		cp->recno = ti + 1;
	}

	/*
	 * Transfer the deleted flag from the top-level cursor to the
	 * created one.
	 */
	if (F_ISSET(orig_cp, C_DELETED)) {
		F_SET(cp, C_DELETED);
		F_CLR(orig_cp, C_DELETED);
	}

	/* Stack the cursors and reset the initial cursor's index. */
	orig_cp->opd = dbc_nopd;
	orig_cp->indx = first;
	return (0);
}

/*
 * __bam_ca_dup --
 *	Adjust the cursors when moving items from a leaf page to a duplicates
 *	page.
 *
 * PUBLIC: int __bam_ca_dup __P((DBC *,
 * PUBLIC:    u_int32_t, db_pgno_t, u_int32_t, db_pgno_t, u_int32_t));
 */
int
__bam_ca_dup(my_dbc, first, fpgno, fi, tpgno, ti)
	DBC *my_dbc;
	db_pgno_t fpgno, tpgno;
	u_int32_t first, fi, ti;
{
	BTREE_CURSOR *orig_cp;
	DB *dbp, *ldbp;
	DBC *dbc;
	DB_LSN lsn;
	DB_TXN *my_txn;
	ENV *env;
	int found, ret, t_ret;

	dbp = my_dbc->dbp;
	env = dbp->env;
	my_txn = IS_SUBTRANSACTION(my_dbc->txn) ? my_dbc->txn : NULL;
	ret = 0;

	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
loop:		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			/* Find cursors pointing to this record. */
			orig_cp = (BTREE_CURSOR *)dbc->internal;
			if (orig_cp->pgno != fpgno || orig_cp->indx != fi ||
			    MVCC_SKIP_CURADJ(dbc, fpgno))
				continue;

			/*
			 * Since we rescan the list see if this is already
			 * converted.
			 */
			if (orig_cp->opd != NULL)
				continue;

			MUTEX_UNLOCK(env, dbp->mutex);
			/* [#8032]
			DB_ASSERT(env, !STD_LOCKING(dbc) ||
			    orig_cp->lock_mode != DB_LOCK_NG);
			*/
			if ((ret = __bam_opd_cursor(dbp,
			    dbc, first, tpgno, ti)) != 0)
				goto err;
			if (my_txn != NULL && dbc->txn != my_txn)
				found = 1;
			/* We released the mutex to get a cursor, start over. */
			goto loop;
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
err:	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (found != 0 && DBC_LOGGING(my_dbc)) {
		if ((t_ret = __bam_curadj_log(dbp, my_dbc->txn,
		    &lsn, 0, DB_CA_DUP, fpgno, tpgno, 0, first, fi, ti)) != 0 &&
		    ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __bam_ca_undodup --
 *	Adjust the cursors when returning items to a leaf page
 *	from a duplicate page.
 *	Called only during undo processing.
 *
 * PUBLIC: int __bam_ca_undodup __P((DB *,
 * PUBLIC:    u_int32_t, db_pgno_t, u_int32_t, u_int32_t));
 */
int
__bam_ca_undodup(dbp, first, fpgno, fi, ti)
	DB *dbp;
	db_pgno_t fpgno;
	u_int32_t first, fi, ti;
{
	BTREE_CURSOR *orig_cp;
	DB *ldbp;
	DBC *dbc;
	ENV *env;
	int ret;

	env = dbp->env;
	ret = 0;

	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
loop:		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			orig_cp = (BTREE_CURSOR *)dbc->internal;

			/*
			 * A note on the orig_cp->opd != NULL requirement here:
			 * it's possible that there's a cursor that refers to
			 * the same duplicate set, but which has no opd cursor,
			 * because it refers to a different item and we took
			 * care of it while processing a previous record.
			 */
			if (orig_cp->pgno != fpgno ||
			    orig_cp->indx != first ||
			    orig_cp->opd == NULL || ((BTREE_CURSOR *)
			    orig_cp->opd->internal)->indx != ti ||
			    MVCC_SKIP_CURADJ(dbc, fpgno))
				continue;
			MUTEX_UNLOCK(env, dbp->mutex);
			if ((ret = __dbc_close(orig_cp->opd)) != 0)
				goto err;
			orig_cp->opd = NULL;
			orig_cp->indx = fi;
			/*
			 * We released the mutex to free a cursor,
			 * start over.
			 */
			goto loop;
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
err:	MUTEX_UNLOCK(env, env->mtx_dblist);

	return (ret);
}

/*
 * __bam_ca_rsplit --
 *	Adjust the cursors when doing reverse splits.
 *
 * PUBLIC: int __bam_ca_rsplit __P((DBC *, db_pgno_t, db_pgno_t));
 */
int
__bam_ca_rsplit(my_dbc, fpgno, tpgno)
	DBC* my_dbc;
	db_pgno_t fpgno, tpgno;
{
	DB *dbp, *ldbp;
	DBC *dbc;
	DB_LSN lsn;
	DB_TXN *my_txn;
	ENV *env;
	int found, ret;

	dbp = my_dbc->dbp;
	env = dbp->env;
	my_txn = IS_SUBTRANSACTION(my_dbc->txn) ? my_dbc->txn : NULL;

	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			if (dbc->dbtype == DB_RECNO)
				continue;
			if (dbc->internal->pgno == fpgno &&
			    !MVCC_SKIP_CURADJ(dbc, fpgno)) {
				dbc->internal->pgno = tpgno;
				/* [#8032]
				DB_ASSERT(env, !STD_LOCKING(dbc) ||
				    dbc->internal->lock_mode != DB_LOCK_NG);
				*/
				if (my_txn != NULL && dbc->txn != my_txn)
					found = 1;
			}
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (found != 0 && DBC_LOGGING(my_dbc)) {
		if ((ret = __bam_curadj_log(dbp, my_dbc->txn,
		    &lsn, 0, DB_CA_RSPLIT, fpgno, tpgno, 0, 0, 0, 0)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * __bam_ca_split --
 *	Adjust the cursors when splitting a page.
 *
 * PUBLIC: int __bam_ca_split __P((DBC *,
 * PUBLIC:    db_pgno_t, db_pgno_t, db_pgno_t, u_int32_t, int));
 */
int
__bam_ca_split(my_dbc, ppgno, lpgno, rpgno, split_indx, cleft)
	DBC *my_dbc;
	db_pgno_t ppgno, lpgno, rpgno;
	u_int32_t split_indx;
	int cleft;
{
	DB *dbp, *ldbp;
	DBC *dbc;
	DBC_INTERNAL *cp;
	DB_LSN lsn;
	DB_TXN *my_txn;
	ENV *env;
	int found, ret;

	dbp = my_dbc->dbp;
	env = dbp->env;
	my_txn = IS_SUBTRANSACTION(my_dbc->txn) ? my_dbc->txn : NULL;

	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 *
	 * If splitting the page that a cursor was on, the cursor has to be
	 * adjusted to point to the same record as before the split.  Most
	 * of the time we don't adjust pointers to the left page, because
	 * we're going to copy its contents back over the original page.  If
	 * the cursor is on the right page, it is decremented by the number of
	 * records split to the left page.
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			if (dbc->dbtype == DB_RECNO)
				continue;
			cp = dbc->internal;
			if (cp->pgno == ppgno &&
			    !MVCC_SKIP_CURADJ(dbc, ppgno)) {
				/* [#8032]
				DB_ASSERT(env, !STD_LOCKING(dbc) ||
				    cp->lock_mode != DB_LOCK_NG);
				*/
				if (my_txn != NULL && dbc->txn != my_txn)
					found = 1;
				if (cp->indx < split_indx) {
					if (cleft)
						cp->pgno = lpgno;
				} else {
					cp->pgno = rpgno;
					cp->indx -= split_indx;
				}
			}
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (found != 0 && DBC_LOGGING(my_dbc)) {
		if ((ret = __bam_curadj_log(dbp,
		    my_dbc->txn, &lsn, 0, DB_CA_SPLIT, ppgno, rpgno,
		    cleft ? lpgno : PGNO_INVALID, 0, split_indx, 0)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __bam_ca_undosplit --
 *	Adjust the cursors when undoing a split of a page.
 *	If we grew a level we will execute this for both the
 *	left and the right pages.
 *	Called only during undo processing.
 *
 * PUBLIC: int __bam_ca_undosplit __P((DB *,
 * PUBLIC:    db_pgno_t, db_pgno_t, db_pgno_t, u_int32_t));
 */
int
__bam_ca_undosplit(dbp, frompgno, topgno, lpgno, split_indx)
	DB *dbp;
	db_pgno_t frompgno, topgno, lpgno;
	u_int32_t split_indx;
{
	DB *ldbp;
	DBC *dbc;
	DBC_INTERNAL *cp;
	ENV *env;

	env = dbp->env;

	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 *
	 * When backing out a split, we move the cursor back
	 * to the original offset and bump it by the split_indx.
	 */
	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			if (dbc->dbtype == DB_RECNO)
				continue;
			cp = dbc->internal;
			if (cp->pgno == topgno &&
			    !MVCC_SKIP_CURADJ(dbc, topgno)) {
				cp->pgno = frompgno;
				cp->indx += split_indx;
			} else if (cp->pgno == lpgno &&
			    !MVCC_SKIP_CURADJ(dbc, lpgno))
				cp->pgno = frompgno;
		}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	return (0);
}
