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
#include "dbinc/lock.h"
#include "dbinc/mp.h"

static int  __bam_bulk __P((DBC *, DBT *, u_int32_t));
static int  __bamc_close __P((DBC *, db_pgno_t, int *));
static int  __bamc_del __P((DBC *, u_int32_t));
static int  __bamc_destroy __P((DBC *));
static int  __bamc_get __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
static int  __bamc_getstack __P((DBC *));
static int  __bamc_next __P((DBC *, int, int));
static int  __bamc_physdel __P((DBC *));
static int  __bamc_prev __P((DBC *));
static int  __bamc_put __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
static int  __bamc_search __P((DBC *,
		db_pgno_t, const DBT *, u_int32_t, int *));
static int  __bamc_writelock __P((DBC *));
static int  __bam_getboth_finddatum __P((DBC *, DBT *, u_int32_t));
static int  __bam_getbothc __P((DBC *, DBT *));
static int  __bam_get_prev __P((DBC *));
static int  __bam_isopd __P((DBC *, db_pgno_t *));
#ifdef HAVE_COMPRESSION
static int  __bam_getlte __P((DBC *, DBT *, DBT *));
#endif

/*
 * Acquire a new page/lock.  If we hold a page/lock, discard the page, and
 * lock-couple the lock.
 *
 * !!!
 * We have to handle both where we have a lock to lock-couple and where we
 * don't -- we don't duplicate locks when we duplicate cursors if we are
 * running in a transaction environment as there's no point if locks are
 * never discarded.  This means that the cursor may or may not hold a lock.
 * In the case where we are descending the tree we always want to unlock
 * the held interior page so we use ACQUIRE_COUPLE.
 */
#undef	ACQUIRE
#define	ACQUIRE(dbc, mode, lpgno, lock, fpgno, pagep, flags, ret) do {	\
	DB_MPOOLFILE *__mpf = (dbc)->dbp->mpf;				\
	if ((pagep) != NULL) {						\
		ret = __memp_fput(__mpf,				\
		     (dbc)->thread_info, pagep, dbc->priority);		\
		pagep = NULL;						\
	} else								\
		ret = 0;						\
	if ((ret) == 0 && STD_LOCKING(dbc))				\
		ret = __db_lget(					\
		    dbc, LCK_COUPLE, lpgno, mode, flags, &(lock));	\
	if ((ret) == 0)							\
		ret = __memp_fget(__mpf, &(fpgno),			\
		    (dbc)->thread_info, (dbc)->txn, 0, &(pagep));	\
} while (0)

/* Acquire a new page/lock for a cursor. */
#undef	ACQUIRE_CUR
#define	ACQUIRE_CUR(dbc, mode, p, flags, ret) do {			\
	BTREE_CURSOR *__cp = (BTREE_CURSOR *)(dbc)->internal;		\
	if (p != __cp->pgno)						\
		__cp->pgno = PGNO_INVALID;				\
	ACQUIRE(dbc, mode, p, __cp->lock, p, __cp->page, flags, ret);	\
	if ((ret) == 0) {						\
		__cp->pgno = p;						\
		__cp->lock_mode = (mode);				\
	}								\
} while (0)

/*
 * Acquire a write lock if we don't already have one.
 *
 * !!!
 * See ACQUIRE macro on why we handle cursors that don't have locks.
 */
#undef	ACQUIRE_WRITE_LOCK
#define	ACQUIRE_WRITE_LOCK(dbc, ret) do {				\
	BTREE_CURSOR *__cp = (BTREE_CURSOR *)(dbc)->internal;		\
	DB_MPOOLFILE *__mpf = (dbc)->dbp->mpf;				\
	int __get_page = 0;						\
	ret = 0;							\
	if (STD_LOCKING(dbc) &&	__cp->lock_mode != DB_LOCK_WRITE) {	\
		if (__cp->page != NULL) {				\
			(ret) = __memp_fput(__mpf, (dbc)->thread_info,	\
			    __cp->page, (dbc)->priority);		\
			__cp->page = NULL;				\
			__get_page = 1;					\
			if ((ret) !=0)					\
				break;					\
		}							\
		if (((ret) = __db_lget((dbc),				\
		    LOCK_ISSET(__cp->lock) ? LCK_COUPLE : 0,		\
		    __cp->pgno, DB_LOCK_WRITE, 0, &__cp->lock)) != 0)	\
			break;						\
		__cp->lock_mode = DB_LOCK_WRITE;			\
		if (__get_page == 0)					\
			break;						\
		(ret) = __memp_fget(__mpf, &__cp->pgno,		\
		    (dbc)->thread_info,					\
		    (dbc)->txn, DB_MPOOL_DIRTY, &__cp->page);		\
	}								\
} while (0)

/* Discard the current page/lock for a cursor. */
#undef	DISCARD_CUR
#define	DISCARD_CUR(dbc, ret) do {					\
	BTREE_CURSOR *__cp = (BTREE_CURSOR *)(dbc)->internal;		\
	DB_MPOOLFILE *__mpf = (dbc)->dbp->mpf;				\
	int __t_ret;							\
	if ((__cp->page) != NULL) {					\
		__t_ret = __memp_fput(__mpf,				\
		     (dbc)->thread_info, __cp->page, dbc->priority);\
		__cp->page = NULL;					\
	} else								\
		__t_ret = 0;						\
	if (__t_ret != 0 && (ret) == 0)					\
		ret = __t_ret;						\
	__t_ret = __TLPUT((dbc), __cp->lock);				\
	if (__t_ret != 0 && (ret) == 0)					\
		ret = __t_ret;						\
	if ((ret) == 0 && !LOCK_ISSET(__cp->lock))			\
		__cp->lock_mode = DB_LOCK_NG;				\
	__cp->stream_start_pgno = PGNO_INVALID;				\
} while (0)

/* If on-page item is a deleted record. */
#undef	IS_DELETED
#define	IS_DELETED(dbp, page, indx)					\
	B_DISSET(GET_BKEYDATA(dbp, page,				\
	    (indx) + (TYPE(page) == P_LBTREE ? O_INDX : 0))->type)
#undef	IS_CUR_DELETED
#define	IS_CUR_DELETED(dbc)						\
	IS_DELETED((dbc)->dbp, (dbc)->internal->page, (dbc)->internal->indx)

/*
 * Test to see if two cursors could point to duplicates of the same key.
 * In the case of off-page duplicates they are they same, as the cursors
 * will be in the same off-page duplicate tree.  In the case of on-page
 * duplicates, the key index offsets must be the same.  For the last test,
 * as the original cursor may not have a valid page pointer, we use the
 * current cursor's.
 */
#undef	IS_DUPLICATE
#define	IS_DUPLICATE(dbc, i1, i2)					\
	    (P_INP((dbc)->dbp,((PAGE *)(dbc)->internal->page))[i1] ==	\
	     P_INP((dbc)->dbp,((PAGE *)(dbc)->internal->page))[i2])
#undef	IS_CUR_DUPLICATE
#define	IS_CUR_DUPLICATE(dbc, orig_pgno, orig_indx)			\
	(F_ISSET(dbc, DBC_OPD) ||					\
	    (orig_pgno == (dbc)->internal->pgno &&			\
	    IS_DUPLICATE(dbc, (dbc)->internal->indx, orig_indx)))

/*
 * __bamc_init --
 *	Initialize the access private portion of a cursor
 *
 * PUBLIC: int __bamc_init __P((DBC *, DBTYPE));
 */
int
__bamc_init(dbc, dbtype)
	DBC *dbc;
	DBTYPE dbtype;
{
	ENV *env;
	int ret;
#ifdef HAVE_COMPRESSION
	BTREE_CURSOR *cp;
#endif

	env = dbc->env;

	/* Allocate/initialize the internal structure. */
	if (dbc->internal == NULL) {
		if ((ret = __os_calloc(
		    env, 1, sizeof(BTREE_CURSOR), &dbc->internal)) != 0)
			return (ret);

#ifdef HAVE_COMPRESSION
		cp = (BTREE_CURSOR*)dbc->internal;
		cp->compressed.flags = DB_DBT_USERMEM;
		cp->key1.flags = DB_DBT_USERMEM;
		cp->key2.flags = DB_DBT_USERMEM;
		cp->data1.flags = DB_DBT_USERMEM;
		cp->data2.flags = DB_DBT_USERMEM;
		cp->del_key.flags = DB_DBT_USERMEM;
		cp->del_data.flags = DB_DBT_USERMEM;
#endif
	}

	/* Initialize methods. */
	dbc->close = dbc->c_close = __dbc_close_pp;
	dbc->cmp = __dbc_cmp_pp;
	dbc->count = dbc->c_count = __dbc_count_pp;
	dbc->del = dbc->c_del = __dbc_del_pp;
	dbc->dup = dbc->c_dup = __dbc_dup_pp;
	dbc->get = dbc->c_get = __dbc_get_pp;
	dbc->pget = dbc->c_pget = __dbc_pget_pp;
	dbc->put = dbc->c_put = __dbc_put_pp;
	if (dbtype == DB_BTREE) {
		dbc->am_bulk = __bam_bulk;
		dbc->am_close = __bamc_close;
		dbc->am_del = __bamc_del;
		dbc->am_destroy = __bamc_destroy;
		dbc->am_get = __bamc_get;
		dbc->am_put = __bamc_put;
		dbc->am_writelock = __bamc_writelock;
	} else {
		dbc->am_bulk = __bam_bulk;
		dbc->am_close = __bamc_close;
		dbc->am_del = __ramc_del;
		dbc->am_destroy = __bamc_destroy;
		dbc->am_get = __ramc_get;
		dbc->am_put = __ramc_put;
		dbc->am_writelock = __bamc_writelock;
	}

	return (0);
}

/*
 * __bamc_refresh
 *	Set things up properly for cursor re-use.
 *
 * PUBLIC: int __bamc_refresh __P((DBC *));
 */
int
__bamc_refresh(dbc)
	DBC *dbc;
{
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;

	dbp = dbc->dbp;
	t = dbp->bt_internal;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * If our caller set the root page number, it's because the root was
	 * known.  This is always the case for off page dup cursors.  Else,
	 * pull it out of our internal information.
	 */
	if (cp->root == PGNO_INVALID)
		cp->root = t->bt_root;

	LOCK_INIT(cp->lock);
	cp->lock_mode = DB_LOCK_NG;

	if (cp->sp == NULL) {
		cp->sp = cp->stack;
		cp->esp = cp->stack + sizeof(cp->stack) / sizeof(cp->stack[0]);
	}
	BT_STK_CLR(cp);

#ifdef HAVE_COMPRESSION
	/* Initialize compression */
	cp->prevKey = 0;
	cp->prevData = 0;
	cp->currentKey = 0;
	cp->currentData = 0;
	cp->compcursor = 0;
	cp->compend = 0;
	cp->prevcursor = 0;
	cp->prev2cursor = 0;
#endif

	/*
	 * The btree leaf page data structures require that two key/data pairs
	 * (or four items) fit on a page, but other than that there's no fixed
	 * requirement.  The btree off-page duplicates only require two items,
	 * to be exact, but requiring four for them as well seems reasonable.
	 *
	 * Recno uses the btree bt_ovflsize value -- it's close enough.
	 */
	cp->ovflsize = B_MINKEY_TO_OVFLSIZE(
	    dbp,  F_ISSET(dbc, DBC_OPD) ? 2 : t->bt_minkey, dbp->pgsize);

	cp->recno = RECNO_OOB;
	cp->order = INVALID_ORDER;
	cp->flags = 0;

	/* Initialize for record numbers. */
	if (F_ISSET(dbc, DBC_OPD) ||
	    dbc->dbtype == DB_RECNO || F_ISSET(dbp, DB_AM_RECNUM)) {
		F_SET(cp, C_RECNUM);

		/*
		 * All btrees that support record numbers, optionally standard
		 * recno trees, and all off-page duplicate recno trees have
		 * mutable record numbers.
		 */
		if ((F_ISSET(dbc, DBC_OPD) && dbc->dbtype == DB_RECNO) ||
		    F_ISSET(dbp, DB_AM_RECNUM | DB_AM_RENUMBER))
			F_SET(cp, C_RENUMBER);
	}

	return (0);
}

/*
 * __bamc_close --
 *	Close down the cursor.
 */
static int
__bamc_close(dbc, root_pgno, rmroot)
	DBC *dbc;
	db_pgno_t root_pgno;
	int *rmroot;
{
	BTREE_CURSOR *cp, *cp_opd, *cp_c;
	DB *dbp;
	DBC *dbc_opd, *dbc_c;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	int cdb_lock, count, ret;

	dbp = dbc->dbp;
	env = dbp->env;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	cp_opd = (dbc_opd = cp->opd) == NULL ?
	    NULL : (BTREE_CURSOR *)dbc_opd->internal;
	cdb_lock = ret = 0;

	/*
	 * There are 3 ways this function is called:
	 *
	 * 1. Closing a primary cursor: we get called with a pointer to a
	 *    primary cursor that has a NULL opd field.  This happens when
	 *    closing a btree/recno database cursor without an associated
	 *    off-page duplicate tree.
	 *
	 * 2. Closing a primary and an off-page duplicate cursor stack: we
	 *    get called with a pointer to the primary cursor which has a
	 *    non-NULL opd field.  This happens when closing a btree cursor
	 *    into database with an associated off-page btree/recno duplicate
	 *    tree. (It can't be a primary recno database, recno databases
	 *    don't support duplicates.)
	 *
	 * 3. Closing an off-page duplicate cursor stack: we get called with
	 *    a pointer to the off-page duplicate cursor.  This happens when
	 *    closing a non-btree database that has an associated off-page
	 *    btree/recno duplicate tree or for a btree database when the
	 *    opd tree is not empty (root_pgno == PGNO_INVALID).
	 *
	 * If either the primary or off-page duplicate cursor deleted a btree
	 * key/data pair, check to see if the item is still referenced by a
	 * different cursor.  If it is, confirm that cursor's delete flag is
	 * set and leave it to that cursor to do the delete.
	 *
	 * NB: The test for == 0 below is correct.  Our caller already removed
	 * our cursor argument from the active queue, we won't find it when we
	 * search the queue in __bam_ca_delete().
	 * NB: It can't be true that both the primary and off-page duplicate
	 * cursors have deleted a btree key/data pair.  Either the primary
	 * cursor may have deleted an item and there's no off-page duplicate
	 * cursor, or there's an off-page duplicate cursor and it may have
	 * deleted an item.
	 *
	 * Primary recno databases aren't an issue here.  Recno keys are either
	 * deleted immediately or never deleted, and do not have to be handled
	 * here.
	 *
	 * Off-page duplicate recno databases are an issue here, cases #2 and
	 * #3 above can both be off-page recno databases.  The problem is the
	 * same as the final problem for off-page duplicate btree databases.
	 * If we no longer need the off-page duplicate tree, we want to remove
	 * it.  For off-page duplicate btrees, we are done with the tree when
	 * we delete the last item it contains, i.e., there can be no further
	 * references to it when it's empty.  For off-page duplicate recnos,
	 * we remove items from the tree as the application calls the remove
	 * function, so we are done with the tree when we close the last cursor
	 * that references it.
	 *
	 * We optionally take the root page number from our caller.  If the
	 * primary database is a btree, we can get it ourselves because dbc
	 * is the primary cursor.  If the primary database is not a btree,
	 * the problem is that we may be dealing with a stack of pages.  The
	 * cursor we're using to do the delete points at the bottom of that
	 * stack and we need the top of the stack.
	 */
	if (F_ISSET(cp, C_DELETED)) {
		dbc_c = dbc;
		switch (dbc->dbtype) {
		case DB_BTREE:				/* Case #1, #3. */
			if ((ret = __bam_ca_delete(
			    dbp, cp->pgno, cp->indx, 1, &count)) != 0)
				goto err;
			if (count == 0)
				goto lock;
			goto done;
		case DB_RECNO:
			if (!F_ISSET(dbc, DBC_OPD))	/* Case #1. */
				goto done;
							/* Case #3. */
			if ((ret = __ram_ca_delete(dbp, cp->root, &count)) != 0)
				goto err;
			if (count == 0)
				goto lock;
			goto done;
		case DB_HASH:
		case DB_QUEUE:
		case DB_UNKNOWN:
		default:
			ret = __db_unknown_type(
			    env, "DbCursor.close", dbc->dbtype);
			goto err;
		}
	}

	if (dbc_opd == NULL)
		goto done;

	if (F_ISSET(cp_opd, C_DELETED)) {		/* Case #2. */
		/*
		 * We will not have been provided a root page number.  Acquire
		 * one from the primary database.
		 */
		if ((h = cp->page) == NULL && (ret = __memp_fget(mpf, &cp->pgno,
		     dbc->thread_info, dbc->txn, 0, &h)) != 0)
			goto err;
		root_pgno = GET_BOVERFLOW(dbp, h, cp->indx + O_INDX)->pgno;
		if ((ret = __memp_fput(mpf,
		     dbc->thread_info, h, dbc->priority)) != 0)
			goto err;
		cp->page = NULL;

		dbc_c = dbc_opd;
		switch (dbc_opd->dbtype) {
		case DB_BTREE:
			if ((ret = __bam_ca_delete(
			    dbp, cp_opd->pgno, cp_opd->indx, 1, &count)) != 0)
				goto err;
			if (count == 0)
				goto lock;
			goto done;
		case DB_RECNO:
			if ((ret =
			    __ram_ca_delete(dbp, cp_opd->root, &count)) != 0)
				goto err;
			if (count == 0)
				goto lock;
			goto done;
		case DB_HASH:
		case DB_QUEUE:
		case DB_UNKNOWN:
		default:
			ret = __db_unknown_type(
			   env, "DbCursor.close", dbc->dbtype);
			goto err;
		}
	}
	goto done;

lock:	cp_c = (BTREE_CURSOR *)dbc_c->internal;

	/*
	 * If this is CDB, upgrade the lock if necessary.  While we acquired
	 * the write lock to logically delete the record, we released it when
	 * we returned from that call, and so may not be holding a write lock
	 * at the moment.
	 */
	if (CDB_LOCKING(env)) {
		if (F_ISSET(dbc, DBC_WRITECURSOR)) {
			if ((ret = __lock_get(env,
			    dbc->locker, DB_LOCK_UPGRADE, &dbc->lock_dbt,
			    DB_LOCK_WRITE, &dbc->mylock)) != 0)
				goto err;
			cdb_lock = 1;
		}
		goto delete;
	}

	/*
	 * The variable dbc_c has been initialized to reference the cursor in
	 * which we're going to do the delete.  Initialize the cursor's lock
	 * structures as necessary.
	 *
	 * First, we may not need to acquire any locks.  If we're in case #3,
	 * that is, the primary database isn't a btree database, our caller
	 * is responsible for acquiring any necessary locks before calling us.
	 */
	if (F_ISSET(dbc, DBC_OPD))
		goto delete;

	/*
	 * Otherwise, acquire a write lock on the primary database's page.
	 *
	 * Lock the primary database page, regardless of whether we're deleting
	 * an item on a primary database page or an off-page duplicates page.
	 *
	 * If the cursor that did the initial logical deletion (and had a write
	 * lock) is not the same cursor doing the physical deletion (which may
	 * have only ever had a read lock on the item), we need to upgrade to a
	 * write lock.  The confusion comes as follows:
	 *
	 *	C1	created, acquires item read lock
	 *	C2	dup C1, create C2, also has item read lock.
	 *	C1	acquire write lock, delete item
	 *	C1	close
	 *	C2	close, needs a write lock to physically delete item.
	 *
	 * If we're in a TXN, we know that C2 will be able to acquire the write
	 * lock, because no locker other than the one shared by C1 and C2 can
	 * acquire a write lock -- the original write lock C1 acquired was never
	 * discarded.
	 *
	 * If we're not in a TXN, it's nastier.  Other cursors might acquire
	 * read locks on the item after C1 closed, discarding its write lock,
	 * and such locks would prevent C2 from acquiring a read lock.  That's
	 * OK, though, we'll simply wait until we can acquire a write lock, or
	 * we'll deadlock.  (Which better not happen, since we're not in a TXN.)
	 *
	 * There are similar scenarios with dirty reads, where the cursor may
	 * have downgraded its write lock to a was-write lock.
	 */
	if (STD_LOCKING(dbc))
		if ((ret = __db_lget(dbc,
		    LCK_COUPLE, cp->pgno, DB_LOCK_WRITE, 0, &cp->lock)) != 0)
			goto err;

delete:	/*
	 * If the delete occurred in a Btree, we're going to look at the page
	 * to see if the item has to be physically deleted.  Otherwise, we do
	 * not need the actual page (and it may not even exist, it might have
	 * been truncated from the file after an allocation aborted).
	 *
	 * Delete the on-page physical item referenced by the cursor.
	 */
	if (dbc_c->dbtype == DB_BTREE) {
		if ((ret = __memp_fget(mpf, &cp_c->pgno, dbc->thread_info,
		    dbc->txn, DB_MPOOL_DIRTY, &cp_c->page)) != 0)
			goto err;
		if ((ret = __bamc_physdel(dbc_c)) != 0)
			goto err;
	}

	/*
	 * If we're not working in an off-page duplicate tree, then we're
	 * done.
	 */
	if (!F_ISSET(dbc_c, DBC_OPD) || root_pgno == PGNO_INVALID)
		goto done;

	/*
	 * We may have just deleted the last element in the off-page duplicate
	 * tree, and closed the last cursor in the tree.  For an off-page btree
	 * there are no other cursors in the tree by definition, if the tree is
	 * empty.  For an off-page recno we know we have closed the last cursor
	 * in the tree because the __ram_ca_delete call above returned 0 only
	 * in that case.  So, if the off-page duplicate tree is empty at this
	 * point, we want to remove it.
	 */
	if (((h = dbc_c->internal->page) == NULL || h->pgno != root_pgno) &&
	    (ret = __memp_fget(mpf,
	    &root_pgno, dbc->thread_info, dbc->txn, 0, &h)) != 0)
		goto err;
	if (NUM_ENT(h) == 0) {
		if (h != dbc_c->internal->page)
			DISCARD_CUR(dbc_c, ret);
		else
			dbc_c->internal->page = NULL;
		if (ret != 0)
			goto err;
		if ((ret = __db_free(dbc, h)) != 0)
			goto err;
	} else {
		if (h != dbc_c->internal->page && (ret = __memp_fput(mpf,
		     dbc->thread_info, h, dbc->priority)) != 0)
			goto err;
		goto done;
	}

	/*
	 * When removing the tree, we have to do one of two things.  If this is
	 * case #2, that is, the primary tree is a btree, delete the key that's
	 * associated with the tree from the btree leaf page.  We know we are
	 * the only reference to it and we already have the correct lock.  We
	 * detect this case because the cursor that was passed to us references
	 * an off-page duplicate cursor.
	 *
	 * If this is case #3, that is, the primary tree isn't a btree, pass
	 * the information back to our caller, it's their job to do cleanup on
	 * the primary page.
	 */
	if (dbc_opd != NULL) {
		if ((ret = __memp_fget(mpf, &cp->pgno, dbc->thread_info,
		    dbc->txn, DB_MPOOL_DIRTY, &cp->page)) != 0)
			goto err;
		if ((ret = __bamc_physdel(dbc)) != 0)
			goto err;
	} else
		*rmroot = 1;
err:
done:	/*
	 * Discard the page references and locks, and confirm that the stack
	 * has been emptied.
	 */
	if (dbc_opd != NULL)
		DISCARD_CUR(dbc_opd, ret);
	DISCARD_CUR(dbc, ret);

	/* Downgrade any CDB lock we acquired. */
	if (cdb_lock)
		(void)__lock_downgrade(env, &dbc->mylock, DB_LOCK_IWRITE, 0);

	return (ret);
}

/*
 * __bamc_cmp --
 *	Compare two btree cursors for equality.
 *
 * This function is only called with two cursors that point to the same item.
 * It only distinguishes cursors pointing to deleted and undeleted items at
 * the same location.
 *
 * PUBLIC: int __bamc_cmp __P((DBC *, DBC *, int *));
 */
int
__bamc_cmp(dbc, other_dbc, result)
	DBC *dbc, *other_dbc;
	int *result;
{
	ENV *env;
	BTREE_CURSOR *bcp, *obcp;

	env = dbc->env;
	bcp = (BTREE_CURSOR *)dbc->internal;
	obcp = (BTREE_CURSOR *)other_dbc->internal;

	DB_ASSERT (env, bcp->pgno == obcp->pgno);
	DB_ASSERT (env, bcp->indx == obcp->indx);

	/* Check to see if both cursors have the same deleted flag. */
	*result =
	    ((F_ISSET(bcp, C_DELETED)) == F_ISSET(obcp, C_DELETED)) ? 0 : 1;
	return (0);
}

/*
 * __bamc_destroy --
 *	Close a single cursor -- internal version.
 */
static int
__bamc_destroy(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	ENV *env;

	cp = (BTREE_CURSOR *)dbc->internal;
	env = dbc->env;

	/* Discard the structures. */
	if (cp->sp != cp->stack)
		__os_free(env, cp->sp);

#ifdef HAVE_COMPRESSION
	/* Free the memory used for compression */
	__os_free(env, cp->compressed.data);
	__os_free(env, cp->key1.data);
	__os_free(env, cp->key2.data);
	__os_free(env, cp->data1.data);
	__os_free(env, cp->data2.data);
	__os_free(env, cp->del_key.data);
	__os_free(env, cp->del_data.data);
#endif

	__os_free(env, cp);

	return (0);
}

/*
 * __bamc_count --
 *	Return a count of on and off-page duplicates.
 *
 * PUBLIC: int __bamc_count __P((DBC *, db_recno_t *));
 */
int
__bamc_count(dbc, recnop)
	DBC *dbc;
	db_recno_t *recnop;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_MPOOLFILE *mpf;
	db_indx_t indx, top;
	db_recno_t recno;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Called with the top-level cursor that may reference an off-page
	 * duplicates tree.  We don't have to acquire any new locks, we have
	 * to have a read lock to even get here.
	 */
	if (cp->opd == NULL) {
		/*
		 * On-page duplicates, get the page and count.
		 */
		DB_ASSERT(dbp->env, cp->page == NULL);
		if ((ret = __memp_fget(mpf, &cp->pgno,
		    dbc->thread_info, dbc->txn, 0, &cp->page)) != 0)
			return (ret);

		/*
		 * Move back to the beginning of the set of duplicates and
		 * then count forward.
		 */
		for (indx = cp->indx;; indx -= P_INDX)
			if (indx == 0 ||
			    !IS_DUPLICATE(dbc, indx, indx - P_INDX))
				break;
		for (recno = 0,
		    top = NUM_ENT(cp->page) - P_INDX;; indx += P_INDX) {
			if (!IS_DELETED(dbp, cp->page, indx))
				++recno;
			if (indx == top ||
			    !IS_DUPLICATE(dbc, indx, indx + P_INDX))
				break;
		}
	} else {
		/*
		 * Off-page duplicates tree, get the root page of the off-page
		 * duplicate tree.
		 */
		if ((ret = __memp_fget(mpf, &cp->opd->internal->root,
		    dbc->thread_info, dbc->txn, 0, &cp->page)) != 0)
			return (ret);

		/*
		 * If the page is an internal page use the page's count as it's
		 * up-to-date and reflects the status of cursors in the tree.
		 * If the page is a leaf page for unsorted duplicates, use the
		 * page's count as cursors don't mark items deleted on the page
		 * and wait, cursor delete items immediately.
		 * If the page is a leaf page for sorted duplicates, there may
		 * be cursors on the page marking deleted items -- count.
		 */
		if (TYPE(cp->page) == P_LDUP)
			for (recno = 0, indx = 0,
			    top = NUM_ENT(cp->page) - O_INDX;; indx += O_INDX) {
				if (!IS_DELETED(dbp, cp->page, indx))
					++recno;
				if (indx == top)
					break;
			}
		else
			recno = RE_NREC(cp->page);
	}

	*recnop = recno;

	ret = __memp_fput(mpf, dbc->thread_info, cp->page, dbc->priority);
	cp->page = NULL;

	return (ret);
}

/*
 * __bamc_del --
 *	Delete using a cursor.
 */
static int
__bamc_del(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_MPOOLFILE *mpf;
	int count, ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	ret = 0;
	COMPQUIET(flags, 0);

	/* If the item was already deleted, return failure. */
	if (F_ISSET(cp, C_DELETED))
		return (DB_KEYEMPTY);

	/*
	 * This code is always called with a page lock but no page.
	 */
	DB_ASSERT(dbp->env, cp->page == NULL);

	/*
	 * We don't physically delete the record until the cursor moves, so
	 * we have to have a long-lived write lock on the page instead of a
	 * a long-lived read lock.  Note, we have to have a read lock to even
	 * get here.
	 *
	 * If we're maintaining record numbers, we lock the entire tree, else
	 * we lock the single page.
	 */
	if (F_ISSET(cp, C_RECNUM)) {
		if ((ret = __bamc_getstack(dbc)) != 0)
			goto err;
		cp->page = cp->csp->page;
	} else {
		ACQUIRE_CUR(dbc, DB_LOCK_WRITE, cp->pgno, 0, ret);
		if (ret != 0)
			goto err;
	}

	/* Mark the page dirty. */
	if ((ret = __memp_dirty(mpf,
	    &cp->page, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
		goto err;

	/* Log the change. */
	if (DBC_LOGGING(dbc)) {
		if ((ret = __bam_cdel_log(dbp, dbc->txn, &LSN(cp->page), 0,
		    PGNO(cp->page), &LSN(cp->page), cp->indx)) != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(LSN(cp->page));

	/* Set the intent-to-delete flag on the page. */
	if (TYPE(cp->page) == P_LBTREE)
		B_DSET(GET_BKEYDATA(dbp, cp->page, cp->indx + O_INDX)->type);
	else
		B_DSET(GET_BKEYDATA(dbp, cp->page, cp->indx)->type);

err:	/*
	 * If we've been successful so far and the tree has record numbers,
	 * adjust the record counts.  Either way, release acquired page(s).
	 */
	if (F_ISSET(cp, C_RECNUM)) {
		cp->csp->page = cp->page;
		if (ret == 0)
			ret = __bam_adjust(dbc, -1);
		(void)__bam_stkrel(dbc, 0);
	} else
		if (cp->page != NULL &&
		    (t_ret = __memp_fput(mpf, dbc->thread_info,
		    cp->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;

	cp->page = NULL;

	/*
	 * Update the cursors last, after all chance of recoverable failure
	 * is past.
	 */
	if (ret == 0)
		ret = __bam_ca_delete(dbp, cp->pgno, cp->indx, 1, &count);

	return (ret);
}

/*
 * __bamc_dup --
 *	Duplicate a btree cursor, such that the new one holds appropriate
 *	locks for the position of the original.
 *
 * PUBLIC: int __bamc_dup __P((DBC *, DBC *, u_int32_t));
 */
int
__bamc_dup(orig_dbc, new_dbc, flags)
	DBC *orig_dbc, *new_dbc;
	u_int32_t flags;
{
	BTREE_CURSOR *orig, *new;

	orig = (BTREE_CURSOR *)orig_dbc->internal;
	new = (BTREE_CURSOR *)new_dbc->internal;

	new->ovflsize = orig->ovflsize;
	new->recno = orig->recno;
	new->flags = orig->flags;

#ifdef HAVE_COMPRESSION
	/* Copy the compression state */
	return (__bamc_compress_dup(orig_dbc, new_dbc, flags));
#else
	COMPQUIET(flags, 0);

	return (0);
#endif
}

/*
 * __bamc_get --
 *	Get using a cursor (btree).
 */
static int
__bamc_get(dbc, key, data, flags, pgnop)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
	db_pgno_t *pgnop;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_MPOOLFILE *mpf;
	db_pgno_t orig_pgno;
	db_indx_t orig_indx;
	int exact, newopd, ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	orig_pgno = cp->pgno;
	orig_indx = cp->indx;

	newopd = 0;
	switch (flags) {
	case DB_CURRENT:
		/* It's not possible to return a deleted record. */
		if (F_ISSET(cp, C_DELETED)) {
			ret = DB_KEYEMPTY;
			goto err;
		}

		/*
		 * Acquire the current page.  We have at least a read-lock
		 * already.  The caller may have set DB_RMW asking for a
		 * write lock, but upgrading to a write lock has no better
		 * chance of succeeding now instead of later, so don't try.
		 */
		if ((ret = __memp_fget(mpf, &cp->pgno,
		    dbc->thread_info, dbc->txn, 0, &cp->page)) != 0)
			goto err;
		break;
	case DB_FIRST:
		newopd = 1;
		if ((ret = __bamc_search(dbc,
		     PGNO_INVALID, NULL, flags, &exact)) != 0)
			goto err;
		break;
	case DB_GET_BOTH:
	case DB_GET_BOTH_RANGE:
		/*
		 * There are two ways to get here based on DBcursor->get
		 * with the DB_GET_BOTH/DB_GET_BOTH_RANGE flags set:
		 *
		 * 1. Searching a sorted off-page duplicate tree: do a tree
		 * search.
		 *
		 * 2. Searching btree: do a tree search.  If it returns a
		 * reference to off-page duplicate tree, return immediately
		 * and let our caller deal with it.  If the search doesn't
		 * return a reference to off-page duplicate tree, continue
		 * with an on-page search.
		 */
		if (F_ISSET(dbc, DBC_OPD)) {
			if ((ret = __bamc_search(
			    dbc, PGNO_INVALID, data, flags, &exact)) != 0)
				goto err;
			if (flags == DB_GET_BOTH) {
				if (!exact) {
					ret = DB_NOTFOUND;
					goto err;
				}
				break;
			}

			/*
			 * We didn't require an exact match, so the search may
			 * may have returned an entry past the end of the page,
			 * or we may be referencing a deleted record.  If so,
			 * move to the next entry.
			 */
			if ((cp->indx == NUM_ENT(cp->page) ||
			    IS_CUR_DELETED(dbc)) &&
			    (ret = __bamc_next(dbc, 1, 0)) != 0)
				goto err;
		} else {
			if ((ret = __bamc_search(
			    dbc, PGNO_INVALID, key, flags, &exact)) != 0)
				return (ret);
			if (!exact) {
				ret = DB_NOTFOUND;
				goto err;
			}

			if (pgnop != NULL && __bam_isopd(dbc, pgnop)) {
				newopd = 1;
				break;
			}
			if ((ret =
			    __bam_getboth_finddatum(dbc, data, flags)) != 0)
				goto err;
		}
		break;
#ifdef HAVE_COMPRESSION
	case DB_SET_LTE:
		if ((ret = __bam_getlte(dbc, key, NULL)) != 0)
			goto err;
		break;
	case DB_GET_BOTH_LTE:
		if ((ret = __bam_getlte(dbc, key, data)) != 0)
			goto err;
		break;
#endif
	case DB_GET_BOTHC:
		if ((ret = __bam_getbothc(dbc, data)) != 0)
			goto err;
		break;
	case DB_LAST:
		newopd = 1;
		if ((ret = __bamc_search(dbc,
		     PGNO_INVALID, NULL, flags, &exact)) != 0)
			goto err;
		break;
	case DB_NEXT:
		newopd = 1;
		if (cp->pgno == PGNO_INVALID) {
			if ((ret = __bamc_search(dbc,
			     PGNO_INVALID, NULL, DB_FIRST, &exact)) != 0)
				goto err;
		} else
			if ((ret = __bamc_next(dbc, 1, 0)) != 0)
				goto err;
		break;
	case DB_NEXT_DUP:
		if ((ret = __bamc_next(dbc, 1, 0)) != 0)
			goto err;
		if (!IS_CUR_DUPLICATE(dbc, orig_pgno, orig_indx)) {
			ret = DB_NOTFOUND;
			goto err;
		}
		break;
	case DB_NEXT_NODUP:
		newopd = 1;
		if (cp->pgno == PGNO_INVALID) {
			if ((ret = __bamc_search(dbc,
			     PGNO_INVALID, NULL, DB_FIRST, &exact)) != 0)
				goto err;
		} else
			do {
				if ((ret = __bamc_next(dbc, 1, 0)) != 0)
					goto err;
			} while (IS_CUR_DUPLICATE(dbc, orig_pgno, orig_indx));
		break;
	case DB_PREV:
		newopd = 1;
		if (cp->pgno == PGNO_INVALID) {
			if ((ret = __bamc_search(dbc,
			     PGNO_INVALID, NULL, DB_LAST, &exact)) != 0)
				goto err;
		} else
			if ((ret = __bamc_prev(dbc)) != 0)
				goto err;
		break;
	case DB_PREV_DUP:
		if ((ret = __bamc_prev(dbc)) != 0)
			goto err;
		if (!IS_CUR_DUPLICATE(dbc, orig_pgno, orig_indx)) {
			ret = DB_NOTFOUND;
			goto err;
		}
		break;
	case DB_PREV_NODUP:
		newopd = 1;
		if (cp->pgno == PGNO_INVALID) {
			if ((ret = __bamc_search(dbc,
			     PGNO_INVALID, NULL, DB_LAST, &exact)) != 0)
				goto err;
		} else
			do {
				if ((ret = __bamc_prev(dbc)) != 0)
					goto err;
			} while (IS_CUR_DUPLICATE(dbc, orig_pgno, orig_indx));
		break;
	case DB_SET:
	case DB_SET_RECNO:
		newopd = 1;
		if ((ret = __bamc_search(dbc,
		    PGNO_INVALID, key, flags, &exact)) != 0)
			goto err;
		break;
	case DB_SET_RANGE:
		newopd = 1;
		if ((ret = __bamc_search(dbc,
		    PGNO_INVALID, key, flags, &exact)) != 0)
			goto err;

		/*
		 * As we didn't require an exact match, the search function
		 * may have returned an entry past the end of the page.  Or,
		 * we may be referencing a deleted record.  If so, move to
		 * the next entry.
		 */
		if (cp->indx == NUM_ENT(cp->page) || IS_CUR_DELETED(dbc))
			if ((ret = __bamc_next(dbc, 0, 0)) != 0)
				goto err;
		break;
	default:
		ret = __db_unknown_flag(dbp->env, "__bamc_get", flags);
		goto err;
	}

	/*
	 * We may have moved to an off-page duplicate tree.  Return that
	 * information to our caller.
	 */
	if (newopd && pgnop != NULL)
		(void)__bam_isopd(dbc, pgnop);

err:	/*
	 * Regardless of whether we were successful or not, if the cursor
	 * moved, clear the delete flag, DBcursor->get never references a
	 * deleted key, if it moved at all.
	 */
	if (F_ISSET(cp, C_DELETED) &&
	    (cp->pgno != orig_pgno || cp->indx != orig_indx))
		F_CLR(cp, C_DELETED);

	return (ret);
}

static int
__bam_get_prev(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	DBT key, data;
	db_pgno_t pgno;
	int ret;

	if ((ret = __bamc_prev(dbc)) != 0)
		return (ret);

	if (__bam_isopd(dbc, &pgno)) {
		cp = (BTREE_CURSOR *)dbc->internal;
		if ((ret = __dbc_newopd(dbc, pgno, cp->opd, &cp->opd)) != 0)
			return (ret);
		if ((ret = cp->opd->am_get(cp->opd,
		    &key, &data, DB_LAST, NULL)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __bam_bulk -- Return bulk data from a btree.
 */
static int
__bam_bulk(dbc, data, flags)
	DBC *dbc;
	DBT *data;
	u_int32_t flags;
{
	BKEYDATA *bk;
	BOVERFLOW *bo;
	BTREE_CURSOR *cp;
	PAGE *pg;
	db_indx_t *inp, indx, pg_keyoff;
	int32_t  *endp, key_off, *offp, *saveoffp;
	u_int8_t *dbuf, *dp, *np;
	u_int32_t key_size, pagesize, size, space;
	int adj, is_key, need_pg, next_key, no_dup, rec_key, ret;

	ret = 0;
	key_off = 0;
	size = 0;
	pagesize = dbc->dbp->pgsize;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * dp tracks the beginning of the page in the buffer.
	 * np is the next place to copy things into the buffer.
	 * dbuf always stays at the beginning of the buffer.
	 */
	dbuf = data->data;
	np = dp = dbuf;

	/* Keep track of space that is left.  There is a termination entry */
	space = data->ulen;
	space -= sizeof(*offp);

	/* Build the offset/size table from the end up. */
	endp = (int32_t *)((u_int8_t *)dbuf + data->ulen);
	endp--;
	offp = endp;

	key_size = 0;

	/*
	 * Distinguish between BTREE and RECNO.
	 * There are no keys in RECNO.  If MULTIPLE_KEY is specified
	 * then we return the record numbers.
	 * is_key indicates that multiple btree keys are returned.
	 * rec_key is set if we are returning record numbers.
	 * next_key is set if we are going after the next key rather than dup.
	 */
	if (dbc->dbtype == DB_BTREE) {
		is_key = LF_ISSET(DB_MULTIPLE_KEY) ? 1: 0;
		rec_key = 0;
		next_key = is_key && LF_ISSET(DB_OPFLAGS_MASK) != DB_NEXT_DUP;
		adj = 2;
	} else {
		is_key = 0;
		rec_key = LF_ISSET(DB_MULTIPLE_KEY) ? 1 : 0;
		next_key = LF_ISSET(DB_OPFLAGS_MASK) != DB_NEXT_DUP;
		adj = 1;
	}
	no_dup = LF_ISSET(DB_OPFLAGS_MASK) == DB_NEXT_NODUP;

next_pg:
	indx = cp->indx;
	pg = cp->page;

	inp = P_INP(dbc->dbp, pg);
	/* The current page is not yet in the buffer. */
	need_pg = 1;

	/*
	 * Keep track of the offset of the current key on the page.
	 * If we are returning keys, set it to 0 first so we force
	 * the copy of the key to the buffer.
	 */
	pg_keyoff = 0;
	if (is_key == 0)
		pg_keyoff = inp[indx];

	do {
		if (IS_DELETED(dbc->dbp, pg, indx)) {
			if (dbc->dbtype != DB_RECNO)
				continue;

			cp->recno++;
			/*
			 * If we are not returning recnos then we
			 * need to fill in every slot so the user
			 * can calculate the record numbers.
			 */
			if (rec_key != 0)
				continue;

			space -= 2 * sizeof(*offp);
			/* Check if space as underflowed. */
			if (space > data->ulen)
				goto back_up;

			/* Just mark the empty recno slots. */
			*offp-- = 0;
			*offp-- = 0;
			continue;
		}

		/*
		 * Check to see if we have a new key.
		 * If so, then see if we need to put the
		 * key on the page.  If its already there
		 * then we just point to it.
		 */
		if (is_key && pg_keyoff != inp[indx]) {
			bk = GET_BKEYDATA(dbc->dbp, pg, indx);
			if (B_TYPE(bk->type) == B_OVERFLOW) {
				bo = (BOVERFLOW *)bk;
				size = key_size = bo->tlen;
				if (key_size > space)
					goto get_key_space;
				if ((ret = __bam_bulk_overflow(dbc,
				    bo->tlen, bo->pgno, np)) != 0)
					return (ret);
				space -= key_size;
				key_off = (int32_t)(np - dbuf);
				np += key_size;
			} else {
				if (need_pg) {
					dp = np;
					size = pagesize - HOFFSET(pg);
					if (space < size) {
get_key_space:
						/* Nothing added, then error. */
						if (offp == endp) {
							data->size = (u_int32_t)
							    DB_ALIGN(size +
							    pagesize, 1024);
							return
							    (DB_BUFFER_SMALL);
						}
						/*
						 * We need to back up to the
						 * last record put into the
						 * buffer so that it is
						 * CURRENT.
						 */
						if (indx != 0)
							indx -= P_INDX;
						else {
							if ((ret =
							    __bam_get_prev(
							    dbc)) != 0)
								return (ret);
							indx = cp->indx;
							pg = cp->page;
						}
						break;
					}
					/*
					 * Move the data part of the page
					 * to the buffer.
					 */
					memcpy(dp,
					   (u_int8_t *)pg + HOFFSET(pg), size);
					need_pg = 0;
					space -= size;
					np += size;
				}
				key_size = bk->len;
				key_off = (int32_t)((inp[indx] - HOFFSET(pg))
				    + (dp - dbuf) + SSZA(BKEYDATA, data));
				pg_keyoff = inp[indx];
			}
		}

		/*
		 * Reserve space for the pointers and sizes.
		 * Either key/data pair or just for a data item.
		 */
		space -= (is_key ? 4 : 2) * sizeof(*offp);
		if (rec_key)
			space -= sizeof(*offp);

		/* Check to see if space has underflowed. */
		if (space > data->ulen)
			goto back_up;

		/*
		 * Determine if the next record is in the
		 * buffer already or if it needs to be copied in.
		 * If we have an off page dup, then copy as many
		 * as will fit into the buffer.
		 */
		bk = GET_BKEYDATA(dbc->dbp, pg, indx + adj - 1);
		if (B_TYPE(bk->type) == B_DUPLICATE) {
			bo = (BOVERFLOW *)bk;
			if (is_key) {
				*offp-- = (int32_t)key_off;
				*offp-- = (int32_t)key_size;
			}
			/*
			 * We pass the offset of the current key.
			 * On return we check to see if offp has
			 * moved to see if any data fit.
			 */
			saveoffp = offp;
			if ((ret = __bam_bulk_duplicates(dbc, bo->pgno,
			    dbuf, is_key ? offp + P_INDX : NULL,
			    &offp, &np, &space, no_dup)) != 0) {
				if (ret == DB_BUFFER_SMALL) {
					size = space;
					space = 0;
					/* If nothing was added, then error. */
					if (offp == saveoffp) {
						offp += 2;
						goto back_up;
					}
					goto get_space;
				}
				return (ret);
			}
		} else if (B_TYPE(bk->type) == B_OVERFLOW) {
			bo = (BOVERFLOW *)bk;
			size = bo->tlen;
			if (size > space)
				goto back_up;
			if ((ret =
			    __bam_bulk_overflow(dbc,
				bo->tlen, bo->pgno, np)) != 0)
				return (ret);
			space -= size;
			if (is_key) {
				*offp-- = (int32_t)key_off;
				*offp-- = (int32_t)key_size;
			} else if (rec_key)
				*offp-- = (int32_t)cp->recno;
			*offp-- = (int32_t)(np - dbuf);
			np += size;
			*offp-- = (int32_t)size;
		} else {
			if (need_pg) {
				dp = np;
				size = pagesize - HOFFSET(pg);
				if (space < size) {
back_up:
					/*
					 * Back up the index so that the
					 * last record in the buffer is CURRENT
					 */
					if (indx >= adj)
						indx -= adj;
					else {
						if ((ret =
						    __bam_get_prev(dbc)) != 0 &&
						    ret != DB_NOTFOUND)
							return (ret);
						indx = cp->indx;
						pg = cp->page;
					}
					if (dbc->dbtype == DB_RECNO)
						cp->recno--;
get_space:
					/*
					 * See if we put anything in the
					 * buffer or if we are doing a DBP->get
					 * did we get all of the data.
					 */
					if (offp >=
					    (is_key ? &endp[-1] : endp) ||
					    F_ISSET(dbc, DBC_FROM_DB_GET)) {
						data->size = (u_int32_t)
						    DB_ALIGN(size +
						    data->ulen - space, 1024);
						return (DB_BUFFER_SMALL);
					}
					break;
				}
				memcpy(dp, (u_int8_t *)pg + HOFFSET(pg), size);
				need_pg = 0;
				space -= size;
				np += size;
			}
			/*
			 * Add the offsets and sizes to the end of the buffer.
			 * First add the key info then the data info.
			 */
			if (is_key) {
				*offp-- = (int32_t)key_off;
				*offp-- = (int32_t)key_size;
			} else if (rec_key)
				*offp-- = (int32_t)cp->recno;
			*offp-- = (int32_t)((inp[indx + adj - 1] - HOFFSET(pg))
			    + (dp - dbuf) + SSZA(BKEYDATA, data));
			*offp-- = bk->len;
		}
		if (dbc->dbtype == DB_RECNO)
			cp->recno++;
		else if (no_dup) {
			while (indx + adj < NUM_ENT(pg) &&
			    pg_keyoff == inp[indx + adj])
				indx += adj;
		}
	/*
	 * Stop when we either run off the page or we move to the next key and
	 * we are not returning multiple keys.
	 */
	} while ((indx += adj) < NUM_ENT(pg) &&
	    (next_key || pg_keyoff == inp[indx]));

	/* If we are off the page then try to the next page. */
	if (ret == 0 && next_key && indx >= NUM_ENT(pg)) {
		cp->indx = indx;
		ret = __bamc_next(dbc, 0, 1);
		if (ret == 0)
			goto next_pg;
		if (ret != DB_NOTFOUND)
			return (ret);
	}

	/*
	 * If we did a DBP->get we must error if we did not return
	 * all the data for the current key because there is
	 * no way to know if we did not get it all, nor any
	 * interface to fetch the balance.
	 */

	if (ret == 0 && indx < pg->entries &&
	    F_ISSET(dbc, DBC_TRANSIENT) && pg_keyoff == inp[indx]) {
		data->size = (data->ulen - space) + size;
		return (DB_BUFFER_SMALL);
	}
	/*
	 * Must leave the index pointing at the last record fetched.
	 * If we are not fetching keys, we may have stepped to the
	 * next key.
	 */
	if (ret == DB_BUFFER_SMALL || next_key || pg_keyoff == inp[indx])
		cp->indx = indx;
	else
		cp->indx = indx - P_INDX;

	if (rec_key == 1)
		*offp = RECNO_OOB;
	else
		*offp = -1;
	return (0);
}

/*
 * __bam_bulk_overflow --
 *	Dump overflow record into the buffer.
 *	The space requirements have already been checked.
 * PUBLIC: int __bam_bulk_overflow
 * PUBLIC:    __P((DBC *, u_int32_t, db_pgno_t, u_int8_t *));
 */
int
__bam_bulk_overflow(dbc, len, pgno, dp)
	DBC *dbc;
	u_int32_t len;
	db_pgno_t pgno;
	u_int8_t *dp;
{
	DBT dbt;

	memset(&dbt, 0, sizeof(dbt));
	F_SET(&dbt, DB_DBT_USERMEM);
	dbt.ulen = len;
	dbt.data = (void *)dp;
	return (__db_goff(dbc, &dbt, len, pgno, NULL, NULL));
}

/*
 * __bam_bulk_duplicates --
 *	Put as many off page duplicates as will fit into the buffer.
 * This routine will adjust the cursor to reflect the position in
 * the overflow tree.
 * PUBLIC: int __bam_bulk_duplicates __P((DBC *,
 * PUBLIC:       db_pgno_t, u_int8_t *, int32_t *,
 * PUBLIC:	 int32_t **, u_int8_t **, u_int32_t *, int));
 */
int
__bam_bulk_duplicates(dbc, pgno, dbuf, keyoff, offpp, dpp, spacep, no_dup)
	DBC *dbc;
	db_pgno_t pgno;
	u_int8_t *dbuf;
	int32_t *keyoff, **offpp;
	u_int8_t **dpp;
	u_int32_t *spacep;
	int no_dup;
{
	BKEYDATA *bk;
	BOVERFLOW *bo;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBC *opd;
	DBT key, data;
	PAGE *pg;
	db_indx_t indx, *inp;
	int32_t *offp;
	u_int32_t pagesize, size, space;
	u_int8_t *dp, *np;
	int first, need_pg, ret, t_ret;

	ret = 0;

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;
	opd = cp->opd;

	if (opd == NULL) {
		if ((ret = __dbc_newopd(dbc, pgno, NULL, &opd)) != 0)
			return (ret);
		cp->opd = opd;
		if ((ret = opd->am_get(opd,
		    &key, &data, DB_FIRST, NULL)) != 0)
			goto close_opd;
	}

	pagesize = opd->dbp->pgsize;
	cp = (BTREE_CURSOR *)opd->internal;
	space = *spacep;
	/* Get current offset slot. */
	offp = *offpp;

	/*
	 * np is the next place to put data.
	 * dp is the beginning of the current page in the buffer.
	 */
	np = dp = *dpp;
	first = 1;
	indx = cp->indx;

	do {
		/* Fetch the current record.  No initial move. */
		if ((ret = __bamc_next(opd, 0, 0)) != 0)
			break;
		pg = cp->page;
		indx = cp->indx;
		inp = P_INP(dbp, pg);
		/* We need to copy the page to the buffer. */
		need_pg = 1;

		do {
			if (IS_DELETED(dbp, pg, indx))
				goto contin;
			bk = GET_BKEYDATA(dbp, pg, indx);
			space -= 2 * sizeof(*offp);
			/* Allocate space for key if needed. */
			if (first == 0 && keyoff != NULL)
				space -= 2 * sizeof(*offp);

			/* Did space underflow? */
			if (space > *spacep) {
				ret = DB_BUFFER_SMALL;
				if (first == 1) {
					/* Get the absolute value. */
					space = -(int32_t)space;
					space = *spacep + space;
					if (need_pg)
						space += pagesize - HOFFSET(pg);
				}
				break;
			}
			if (B_TYPE(bk->type) == B_OVERFLOW) {
				bo = (BOVERFLOW *)bk;
				size = bo->tlen;
				if (size > space) {
					ret = DB_BUFFER_SMALL;
					space = *spacep + size;
					break;
				}
				if (first == 0 && keyoff != NULL) {
					*offp-- = keyoff[0];
					*offp-- = keyoff[-1];
				}
				if ((ret = __bam_bulk_overflow(dbc,
				    bo->tlen, bo->pgno, np)) != 0)
					return (ret);
				space -= size;
				*offp-- = (int32_t)(np - dbuf);
				np += size;
			} else {
				if (need_pg) {
					dp = np;
					size = pagesize - HOFFSET(pg);
					if (space < size) {
						ret = DB_BUFFER_SMALL;
						/* Return space required. */
						space = *spacep + size;
						break;
					}
					memcpy(dp,
					    (u_int8_t *)pg + HOFFSET(pg), size);
					need_pg = 0;
					space -= size;
					np += size;
				}
				if (first == 0 && keyoff != NULL) {
					*offp-- = keyoff[0];
					*offp-- = keyoff[-1];
				}
				size = bk->len;
				*offp-- = (int32_t)((inp[indx] - HOFFSET(pg))
				    + (dp - dbuf) + SSZA(BKEYDATA, data));
			}
			*offp-- = (int32_t)size;
			first = 0;
			if (no_dup)
				break;
contin:
			indx++;
			if (opd->dbtype == DB_RECNO)
				cp->recno++;
		} while (indx < NUM_ENT(pg));
		if (no_dup)
			break;
		cp->indx = indx;

	} while (ret == 0);

	/* Return the updated information. */
	*spacep = space;
	*offpp = offp;
	*dpp = np;

	/*
	 * If we ran out of space back up the pointer.
	 * If we did not return any dups or reached the end, close the opd.
	 */
	if (ret == DB_BUFFER_SMALL) {
		if (opd->dbtype == DB_RECNO) {
			if (--cp->recno == 0)
				goto close_opd;
		} else if (indx != 0)
			cp->indx--;
		else {
			t_ret = __bamc_prev(opd);
			if (t_ret == DB_NOTFOUND)
				goto close_opd;
			if (t_ret != 0)
				ret = t_ret;
		}
	} else if (keyoff == NULL && ret == DB_NOTFOUND) {
		cp->indx--;
		if (opd->dbtype == DB_RECNO)
			--cp->recno;
	} else if (indx == 0 || ret == DB_NOTFOUND) {
close_opd:
		if (ret == DB_NOTFOUND)
			ret = 0;
		if ((t_ret = __dbc_close(opd)) != 0 && ret == 0)
			ret = t_ret;
		((BTREE_CURSOR *)dbc->internal)->opd = NULL;
	}
	if (ret == DB_NOTFOUND)
		ret = 0;

	return (ret);
}

/*
 * __bam_getbothc --
 *	Search for a matching data item on a join.
 */
static int
__bam_getbothc(dbc, data)
	DBC *dbc;
	DBT *data;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_MPOOLFILE *mpf;
	int cmp, exact, ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Acquire the current page.  We have at least a read-lock
	 * already.  The caller may have set DB_RMW asking for a
	 * write lock, but upgrading to a write lock has no better
	 * chance of succeeding now instead of later, so don't try.
	 */
	if ((ret = __memp_fget(mpf, &cp->pgno,
	     dbc->thread_info, dbc->txn, 0, &cp->page)) != 0)
		return (ret);

	/*
	 * An off-page duplicate cursor.  Search the remaining duplicates
	 * for one which matches (do a normal btree search, then verify
	 * that the retrieved record is greater than the original one).
	 */
	if (F_ISSET(dbc, DBC_OPD)) {
		/*
		 * Check to make sure the desired item comes strictly after
		 * the current position;  if it doesn't, return DB_NOTFOUND.
		 */
		if ((ret = __bam_cmp(dbc, data, cp->page, cp->indx,
		    dbp->dup_compare == NULL ? __bam_defcmp : dbp->dup_compare,
		    &cmp)) != 0)
			return (ret);

		if (cmp <= 0)
			return (DB_NOTFOUND);

		/* Discard the current page, we're going to do a full search. */
		if ((ret = __memp_fput(mpf,
		     dbc->thread_info, cp->page, dbc->priority)) != 0)
			return (ret);
		cp->page = NULL;

		return (__bamc_search(dbc,
		    PGNO_INVALID, data, DB_GET_BOTH, &exact));
	}

	/*
	 * We're doing a DBC->get(DB_GET_BOTHC) and we're already searching
	 * a set of on-page duplicates (either sorted or unsorted).  Continue
	 * a linear search from after the current position.
	 *
	 * (Note that we could have just finished a "set" of one duplicate,
	 * i.e. not a duplicate at all, but the following check will always
	 * return DB_NOTFOUND in this case, which is the desired behavior.)
	 */
	if (cp->indx + P_INDX >= NUM_ENT(cp->page) ||
	    !IS_DUPLICATE(dbc, cp->indx, cp->indx + P_INDX))
		return (DB_NOTFOUND);
	cp->indx += P_INDX;

	return (__bam_getboth_finddatum(dbc, data, DB_GET_BOTH));
}

#ifdef HAVE_COMPRESSION
/*
 * __bam_getlte --
 *	Search for the largest entry <= key/data - used by compression.
 *
 *	data == NULL indicates the DB_SET_LTE flag
 *	data != NULL indicates the DB_GET_BOTH_LTE flag
 *
 *	Only works for a primary cursor - not an OPD cursor. Handles the
 *	OPD manipulation as well - no need to return to the caller to
 *	perform more OPD movements.
 */
static int
__bam_getlte(dbc, key, data)
	DBC *dbc;
	DBT *key, *data;
{
	BTREE_CURSOR *cp, *ocp;
	DB *dbp;
	db_pgno_t pgno;
	int exact, ret;

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;

	/* Begin by searching for the key */
	ret = __bamc_search(dbc, PGNO_INVALID, key, DB_SET_RANGE, &exact);
	if (ret == DB_NOTFOUND)
		goto find_last;
	if (ret != 0)
		goto end;

	if (cp->indx == NUM_ENT(cp->page) || IS_CUR_DELETED(dbc)) {
		/*
		 * Move to the next entry if we're past the end of the
		 * page or on a deleted entry.
		 */
		ret = __bamc_next(dbc, 0, 0);
		if (ret == DB_NOTFOUND)
			goto find_last;
		if (ret != 0)
			goto end;

		/* Check if we're still on the correct key */
		if ((ret = __bam_cmp(dbc, key, cp->page, cp->indx,
		    ((BTREE*)dbp->bt_internal)->bt_compare, &exact)) != 0)
			goto end;
		exact = (exact == 0);
	}

	if (exact == 0) {
		ret = __bam_get_prev(dbc);
		goto end;
	}

	if (__bam_isopd(dbc, &pgno)) {
		/*
		 * We want to do unusual things with off-page duplicates, so
		 * deal with them here rather than returning to handle them.
		 */
		if ((ret = __dbc_newopd(dbc, pgno, cp->opd, &cp->opd)) != 0)
			goto end;

		/* Search for the correct duplicate */
		ret = __bamc_search(cp->opd, PGNO_INVALID, data,
			data == NULL ? DB_FIRST : DB_SET_RANGE, &exact);
		if (ret == DB_NOTFOUND)
			goto find_last_dup;
		if (ret != 0)
			goto end;

		ocp = (BTREE_CURSOR *)cp->opd->internal;
		if (ocp->indx == NUM_ENT(ocp->page) ||
		    IS_CUR_DELETED(cp->opd)) {
			/*
			 * Move to the next entry if we're past the end of the
			 * page or on a deleted entry.
			 */
			ret = __bamc_next(cp->opd, 0, 0);
			if (ret == DB_NOTFOUND)
				goto find_last_dup;
			if (ret != 0)
				goto end;

			if (data != NULL) {
				/* Check if we're still on the correct data */
				if ((ret = __bam_cmp(
					    dbc, data, ocp->page, ocp->indx,
					    dbp->dup_compare, &exact)) != 0)
					goto end;
				exact = (exact == 0);
			} else
				exact = 1;
		}

		if (exact == 0) {
			/* Move to the previous entry */
			ret = __bamc_prev(cp->opd);
			if (ret == DB_NOTFOUND) {
				if ((ret = __dbc_close(cp->opd)) != 0)
					goto end;
				cp->opd = NULL;
				ret = __bam_get_prev(dbc);
			}
		}
	} else if(data != NULL) {
		/*
		 * If we got an exact match with on-page duplicates, we need to
		 * search in them.
		 */
		ret = __bam_getboth_finddatum(dbc, data, DB_GET_BOTH_RANGE);
		if (ret == DB_NOTFOUND)
			exact = 0;
		else if (ret != 0)
			goto end;
		else {
			/* Check if we're still on the correct data */
			if ((ret = __bam_cmp(dbc, data, cp->page,
			    cp->indx + O_INDX, dbp->dup_compare, &exact)) != 0)
				goto end;
			exact = (exact == 0);
		}

		if (exact == 0) {
			ret = __bam_get_prev(dbc);
		}
	}

 end:
	return (ret);

 find_last:
	if ((ret = __bamc_search(
	    dbc, PGNO_INVALID, NULL, DB_LAST, &exact)) != 0)
		return (ret);

	if (__bam_isopd(dbc, &pgno)) {
		if ((ret = __dbc_newopd(dbc, pgno, cp->opd, &cp->opd)) != 0)
			return (ret);
 find_last_dup:
		if ((ret = __bamc_search(
		    cp->opd, PGNO_INVALID, NULL, DB_LAST, &exact)) != 0)
			return (ret);
	}

	return (ret);
}
#endif

/*
 * __bam_getboth_finddatum --
 *	Find a matching on-page data item.
 */
static int
__bam_getboth_finddatum(dbc, data, flags)
	DBC *dbc;
	DBT *data;
	u_int32_t flags;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	db_indx_t base, lim, top;
	int cmp, ret;

	COMPQUIET(cmp, 0);

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Called (sometimes indirectly) from DBC->get to search on-page data
	 * item(s) for a matching value.  If the original flag was DB_GET_BOTH
	 * or DB_GET_BOTH_RANGE, the cursor is set to the first undeleted data
	 * item for the key.  If the original flag was DB_GET_BOTHC, the cursor
	 * argument is set to the first data item we can potentially return.
	 * In both cases, there may or may not be additional duplicate data
	 * items to search.
	 *
	 * If the duplicates are not sorted, do a linear search.
	 */
	if (dbp->dup_compare == NULL) {
		for (;; cp->indx += P_INDX) {
			if (!IS_CUR_DELETED(dbc) &&
			    (ret = __bam_cmp(dbc, data, cp->page,
			    cp->indx + O_INDX, __bam_defcmp, &cmp)) != 0)
				return (ret);
			if (cmp == 0)
				return (0);

			if (cp->indx + P_INDX >= NUM_ENT(cp->page) ||
			    !IS_DUPLICATE(dbc, cp->indx, cp->indx + P_INDX))
				break;
		}
		return (DB_NOTFOUND);
	}

	/*
	 * If the duplicates are sorted, do a binary search.  The reason for
	 * this is that large pages and small key/data pairs result in large
	 * numbers of on-page duplicates before they get pushed off-page.
	 *
	 * Find the top and bottom of the duplicate set.  Binary search
	 * requires at least two items, don't loop if there's only one.
	 */
	for (base = top = cp->indx; top < NUM_ENT(cp->page); top += P_INDX)
		if (!IS_DUPLICATE(dbc, cp->indx, top))
			break;
	if (base == (top - P_INDX)) {
		if  ((ret = __bam_cmp(dbc, data, cp->page,
		    cp->indx + O_INDX, dbp->dup_compare, &cmp)) != 0)
			return (ret);
		if (cmp == 0 || (cmp < 0 && flags == DB_GET_BOTH_RANGE))
			return 0;
		cp->indx = top;
		return DB_NOTFOUND;
	}

	for (lim = (top - base) / (db_indx_t)P_INDX; lim != 0; lim >>= 1) {
		cp->indx = base + ((lim >> 1) * P_INDX);
		if ((ret = __bam_cmp(dbc, data, cp->page,
		    cp->indx + O_INDX, dbp->dup_compare, &cmp)) != 0)
			return (ret);
		if (cmp == 0) {
			/*
			 * XXX
			 * No duplicate duplicates in sorted duplicate sets,
			 * so there can be only one.
			 */
			if (!IS_CUR_DELETED(dbc))
				return (0);
			break;
		}
		if (cmp > 0) {
			base = cp->indx + P_INDX;
			--lim;
		}
	}

	/* No match found; if we're looking for an exact match, we're done. */
	if (flags == DB_GET_BOTH)
		return (DB_NOTFOUND);

	/*
	 * Base is the smallest index greater than the data item, may be zero
	 * or a last + O_INDX index, and may be deleted.  Find an undeleted
	 * item.
	 */
	cp->indx = base;
	while (cp->indx < top && IS_CUR_DELETED(dbc))
		cp->indx += P_INDX;
	return (cp->indx < top ? 0 : DB_NOTFOUND);
}

/*
 * __bamc_put --
 *	Put using a cursor.
 */
static int
__bamc_put(dbc, key, data, flags, pgnop)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
	db_pgno_t *pgnop;
{
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT dbt;
	DB_MPOOLFILE *mpf;
	db_pgno_t root_pgno;
	int cmp, exact, own, ret, stack;
	u_int32_t iiop;
	void *arg;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	root_pgno = cp->root;

split:	ret = stack = 0;
	switch (flags) {
	case DB_CURRENT:
		if (F_ISSET(cp, C_DELETED))
			return (DB_NOTFOUND);
		/* FALLTHROUGH */
	case DB_AFTER:
	case DB_BEFORE:
		iiop = flags;
		own = 1;

		/* Acquire the current page with a write lock. */
		ACQUIRE_WRITE_LOCK(dbc, ret);
		if (ret != 0)
			goto err;
		if (cp->page == NULL && (ret = __memp_fget(mpf, &cp->pgno,
		    dbc->thread_info, dbc->txn, 0, &cp->page)) != 0)
			goto err;
		break;
	case DB_KEYFIRST:
	case DB_KEYLAST:
	case DB_NODUPDATA:
	case DB_NOOVERWRITE:
	case DB_OVERWRITE_DUP:
		own = 0;
		/*
		 * Searching off-page, sorted duplicate tree: do a tree search
		 * for the correct item; __bamc_search returns the smallest
		 * slot greater than the key, use it.
		 *
		 * See comment below regarding where we can start the search.
		 */
		if (F_ISSET(dbc, DBC_OPD)) {
			if ((ret = __bamc_search(dbc,
			    F_ISSET(cp, C_RECNUM) ? cp->root : root_pgno,
			    data, flags, &exact)) != 0)
				goto err;
			stack = 1;

			/* Disallow "sorted" duplicate duplicates. */
			if (exact != 0) {
				if (flags == DB_OVERWRITE_DUP ||
				    IS_DELETED(dbp, cp->page, cp->indx)) {
					iiop = DB_CURRENT;
					break;
				}
				ret = __db_duperr(dbp, flags);
				goto err;
			}
			iiop = DB_BEFORE;
			break;
		}

		/*
		 * Searching a btree.
		 *
		 * If we've done a split, we can start the search from the
		 * parent of the split page, which __bam_split returned
		 * for us in root_pgno, unless we're in a Btree with record
		 * numbering.  In that case, we'll need the true root page
		 * in order to adjust the record count.
		 */
		if ((ret = __bamc_search(dbc,
		    F_ISSET(cp, C_RECNUM) ? cp->root : root_pgno, key,
		    flags == DB_KEYFIRST || dbp->dup_compare != NULL ?
		    DB_KEYFIRST : DB_KEYLAST, &exact)) != 0)
			goto err;
		stack = 1;

		/*
		 * If we don't have an exact match, __bamc_search returned
		 * the smallest slot greater than the key, use it.
		 */
		if (!exact) {
			iiop = DB_KEYFIRST;
			break;

		/*
		 * Check for NOOVERWRITE.  It is possible that there
		 * is a key with an empty duplicate page attached.
		 */
		} else if (flags == DB_NOOVERWRITE && !IS_CUR_DELETED(dbc)) {
			if (pgnop != NULL && __bam_isopd(dbc, pgnop))
				ret = __bam_opd_exists(dbc, *pgnop);
			else
				ret = DB_KEYEXIST;
			if (ret != 0)
				goto err;
		}

		/*
		 * If duplicates aren't supported, replace the current item.
		 */
		if (!F_ISSET(dbp, DB_AM_DUP)) {
			iiop = DB_CURRENT;
			break;
		}

		/*
		 * If we find a matching entry, it may be an off-page duplicate
		 * tree.  Return the page number to our caller, we need a new
		 * cursor.
		 */
		if (pgnop != NULL && __bam_isopd(dbc, pgnop))
			goto done;

		/* If the duplicates aren't sorted, move to the right slot. */
		if (dbp->dup_compare == NULL) {
			if (flags == DB_KEYFIRST)
				iiop = DB_BEFORE;
			else
				for (;; cp->indx += P_INDX)
					if (cp->indx + P_INDX >=
					    NUM_ENT(cp->page) ||
					    !IS_DUPLICATE(dbc, cp->indx,
					    cp->indx + P_INDX)) {
						iiop = DB_AFTER;
						break;
					}
			break;
		}

		/*
		 * We know that we're looking at the first of a set of sorted
		 * on-page duplicates.  Walk the list to find the right slot.
		 */
		for (;; cp->indx += P_INDX) {
			if ((ret = __bam_cmp(dbc, data, cp->page,
			    cp->indx + O_INDX, dbp->dup_compare, &cmp)) != 0)
				goto err;
			if (cmp < 0) {
				iiop = DB_BEFORE;
				break;
			}

			/* Disallow "sorted" duplicate duplicates. */
			if (cmp == 0) {
				if (flags == DB_OVERWRITE_DUP ||
				    IS_DELETED(dbp, cp->page, cp->indx)) {
					iiop = DB_CURRENT;
					break;
				}
				ret = __db_duperr(dbp, flags);
				goto err;
			}

			if (cp->indx + P_INDX >= NUM_ENT(cp->page) ||
			    P_INP(dbp, ((PAGE *)cp->page))[cp->indx] !=
			    P_INP(dbp, ((PAGE *)cp->page))[cp->indx + P_INDX]) {
				iiop = DB_AFTER;
				break;
			}
		}
		break;
	default:
		ret = __db_unknown_flag(dbp->env, "__bamc_put", flags);
		goto err;
	}

	switch (ret = __bam_iitem(dbc, key, data, iiop, 0)) {
	case 0:
		break;
	case DB_NEEDSPLIT:
		/*
		 * To split, we need a key for the page.  Either use the key
		 * argument or get a copy of the key from the page.
		 */
		if (flags == DB_AFTER ||
		    flags == DB_BEFORE || flags == DB_CURRENT) {
			memset(&dbt, 0, sizeof(DBT));
			if ((ret = __db_ret(dbc, cp->page, 0, &dbt,
			    &dbc->my_rkey.data, &dbc->my_rkey.ulen)) != 0)
				goto err;
			arg = &dbt;
		} else
			arg = F_ISSET(dbc, DBC_OPD) ? data : key;

		/*
		 * Discard any locks and pinned pages (the locks are discarded
		 * even if we're running with transactions, as they lock pages
		 * that we're sorry we ever acquired).  If stack is set and the
		 * cursor entries are valid, they point to the same entries as
		 * the stack, don't free them twice.
		 */
		if (stack)
			ret = __bam_stkrel(dbc, STK_CLRDBC | STK_NOLOCK);
		else
			DISCARD_CUR(dbc, ret);
		if (ret != 0)
			goto err;

		/*
		 * SR [#6059]
		 * If we do not own a lock on the page any more, then clear the
		 * cursor so we don't point at it.  Even if we call __bam_stkrel
		 * above we still may have entered the routine with the cursor
		 * positioned to a particular record.  This is in the case
		 * where C_RECNUM is set.
		 */
		if (own == 0) {
			cp->pgno = PGNO_INVALID;
			cp->indx = 0;
		}

		/* Split the tree. */
		if ((ret = __bam_split(dbc, arg, &root_pgno)) != 0)
			return (ret);

		goto split;
	default:
		goto err;
	}

err:
done:	/*
	 * If we inserted a key into the first or last slot of the tree,
	 * remember where it was so we can do it more quickly next time.
	 * If the tree has record numbers, we need a complete stack so
	 * that we can adjust the record counts, so skipping the tree search
	 * isn't possible.  For subdatabases we need to be careful that the
	 * page does not move from one db to another, so we track its LSN.
	 *
	 * If there are duplicates and we are inserting into the last slot,
	 * the cursor will point _to_ the last item, not after it, which
	 * is why we subtract P_INDX below.
	 */

	t = dbp->bt_internal;
	if (ret == 0 && TYPE(cp->page) == P_LBTREE &&
	    (flags == DB_KEYFIRST || flags == DB_KEYLAST) &&
	    !F_ISSET(cp, C_RECNUM) &&
	    (!F_ISSET(dbp, DB_AM_SUBDB) ||
	    (LOGGING_ON(dbp->env) && !F_ISSET(dbp, DB_AM_NOT_DURABLE))) &&
	    ((NEXT_PGNO(cp->page) == PGNO_INVALID &&
	    cp->indx >= NUM_ENT(cp->page) - P_INDX) ||
	    (PREV_PGNO(cp->page) == PGNO_INVALID && cp->indx == 0))) {
		t->bt_lpgno = cp->pgno;
		if (F_ISSET(dbp, DB_AM_SUBDB))
			t->bt_llsn = LSN(cp->page);
	} else
		t->bt_lpgno = PGNO_INVALID;
	/*
	 * Discard any pages pinned in the tree and their locks, except for
	 * the leaf page.  Note, the leaf page participated in any stack we
	 * acquired, and so we have to adjust the stack as necessary.  If
	 * there was only a single page on the stack, we don't have to free
	 * further stack pages.
	 */
	if (stack && BT_STK_POP(cp) != NULL)
		(void)__bam_stkrel(dbc, 0);

	/*
	 * Regardless of whether we were successful or not, clear the delete
	 * flag.  If we're successful, we either moved the cursor or the item
	 * is no longer deleted.  If we're not successful, then we're just a
	 * copy, no need to have the flag set.
	 *
	 * We may have instantiated off-page duplicate cursors during the put,
	 * so clear the deleted bit from the off-page duplicate cursor as well.
	 */
	F_CLR(cp, C_DELETED);
	if (cp->opd != NULL) {
		cp = (BTREE_CURSOR *)cp->opd->internal;
		F_CLR(cp, C_DELETED);
	}

	return (ret);
}

/*
 * __bamc_rget --
 *	Return the record number for a cursor.
 *
 * PUBLIC: int __bamc_rget __P((DBC *, DBT *));
 */
int
__bamc_rget(dbc, data)
	DBC *dbc;
	DBT *data;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT dbt;
	DB_MPOOLFILE *mpf;
	db_recno_t recno;
	int exact, ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Get the page with the current item on it.
	 * Get a copy of the key.
	 * Release the page, making sure we don't release it twice.
	 */
	if ((ret = __memp_fget(mpf, &cp->pgno,
	     dbc->thread_info, dbc->txn, 0, &cp->page)) != 0)
		return (ret);
	memset(&dbt, 0, sizeof(DBT));
	if ((ret = __db_ret(dbc, cp->page, cp->indx, &dbt,
	    &dbc->my_rkey.data, &dbc->my_rkey.ulen)) != 0)
		goto err;
	ret = __memp_fput(mpf, dbc->thread_info, cp->page, dbc->priority);
	cp->page = NULL;
	if (ret != 0)
		return (ret);

	if ((ret = __bam_search(dbc, PGNO_INVALID, &dbt,
	    F_ISSET(dbc, DBC_RMW) ? SR_FIND_WR : SR_FIND,
	    1, &recno, &exact)) != 0)
		goto err;

	ret = __db_retcopy(dbc->env, data,
	    &recno, sizeof(recno), &dbc->rdata->data, &dbc->rdata->ulen);

	/* Release the stack. */
err:	if ((t_ret = __bam_stkrel(dbc, 0)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __bamc_writelock --
 *	Upgrade the cursor to a write lock.
 */
static int
__bamc_writelock(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	int ret;

	cp = (BTREE_CURSOR *)dbc->internal;

	if (cp->lock_mode == DB_LOCK_WRITE)
		return (0);

	/*
	 * When writing to an off-page duplicate tree, we need to have the
	 * appropriate page in the primary tree locked.  The general DBC
	 * code calls us first with the primary cursor so we can acquire the
	 * appropriate lock.
	 */
	ACQUIRE_WRITE_LOCK(dbc, ret);
	return (ret);
}

/*
 * __bamc_next --
 *	Move to the next record.
 */
static int
__bamc_next(dbc, initial_move, deleted_okay)
	DBC *dbc;
	int initial_move, deleted_okay;
{
	BTREE_CURSOR *cp;
	db_indx_t adjust;
	db_lockmode_t lock_mode;
	db_pgno_t pgno;
	int ret;

	cp = (BTREE_CURSOR *)dbc->internal;
	ret = 0;

	/*
	 * We're either moving through a page of duplicates or a btree leaf
	 * page.
	 *
	 * !!!
	 * This code handles empty pages and pages with only deleted entries.
	 */
	if (F_ISSET(dbc, DBC_OPD)) {
		adjust = O_INDX;
		lock_mode = DB_LOCK_NG;
	} else {
		adjust = dbc->dbtype == DB_BTREE ? P_INDX : O_INDX;
		lock_mode =
		    F_ISSET(dbc, DBC_RMW) ? DB_LOCK_WRITE : DB_LOCK_READ;
	}
	if (cp->page == NULL) {
		ACQUIRE_CUR(dbc, lock_mode, cp->pgno, 0, ret);
		if (ret != 0)
			return (ret);
	}

	if (initial_move)
		cp->indx += adjust;

	for (;;) {
		/*
		 * If at the end of the page, move to a subsequent page.
		 *
		 * !!!
		 * Check for >= NUM_ENT.  If the original search landed us on
		 * NUM_ENT, we may have incremented indx before the test.
		 */
		if (cp->indx >= NUM_ENT(cp->page)) {
			if ((pgno = NEXT_PGNO(cp->page)) == PGNO_INVALID)
				return (DB_NOTFOUND);

			ACQUIRE_CUR(dbc, lock_mode, pgno, 0, ret);
			if (ret != 0)
				return (ret);
			cp->indx = 0;
			continue;
		}
		if (!deleted_okay && IS_CUR_DELETED(dbc)) {
			cp->indx += adjust;
			continue;
		}
		break;
	}
	return (0);
}

/*
 * __bamc_prev --
 *	Move to the previous record.
 */
static int
__bamc_prev(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	db_indx_t adjust;
	db_lockmode_t lock_mode;
	db_pgno_t pgno;
	int ret;

	cp = (BTREE_CURSOR *)dbc->internal;
	ret = 0;

	/*
	 * We're either moving through a page of duplicates or a btree leaf
	 * page.
	 *
	 * !!!
	 * This code handles empty pages and pages with only deleted entries.
	 */
	if (F_ISSET(dbc, DBC_OPD)) {
		adjust = O_INDX;
		lock_mode = DB_LOCK_NG;
	} else {
		adjust = dbc->dbtype == DB_BTREE ? P_INDX : O_INDX;
		lock_mode =
		    F_ISSET(dbc, DBC_RMW) ? DB_LOCK_WRITE : DB_LOCK_READ;
	}
	if (cp->page == NULL) {
		ACQUIRE_CUR(dbc, lock_mode, cp->pgno, 0, ret);
		if (ret != 0)
			return (ret);
	}

	for (;;) {
		/* If at the beginning of the page, move to a previous one. */
		if (cp->indx == 0) {
			if ((pgno =
			    PREV_PGNO(cp->page)) == PGNO_INVALID)
				return (DB_NOTFOUND);

			ACQUIRE_CUR(dbc, lock_mode, pgno, 0, ret);
			if (ret != 0)
				return (ret);

			if ((cp->indx = NUM_ENT(cp->page)) == 0)
				continue;
		}

		/* Ignore deleted records. */
		cp->indx -= adjust;
		if (IS_CUR_DELETED(dbc))
			continue;

		break;
	}
	return (0);
}

/*
 * __bamc_search --
 *	Move to a specified record.
 */
static int
__bamc_search(dbc, root_pgno, key, flags, exactp)
	DBC *dbc;
	db_pgno_t root_pgno;
	const DBT *key;
	u_int32_t flags;
	int *exactp;
{
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;
	PAGE *h;
	db_indx_t base, indx, *inp, lim;
	db_pgno_t bt_lpgno;
	db_recno_t recno;
	u_int32_t sflags;
	int bulk, cmp, ret, t_ret;

	COMPQUIET(cmp, 0);
	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;
	t = dbp->bt_internal;
	ret = 0;
	bulk = (F_ISSET(dbc, DBC_BULK) && cp->pgno != PGNO_INVALID);

	/*
	 * Find an entry in the database.  Discard any lock we currently hold,
	 * we're going to search the tree.
	 */
	DISCARD_CUR(dbc, ret);
	if (ret != 0)
		return (ret);

	switch (flags) {
	case DB_FIRST:
		sflags = (F_ISSET(dbc, DBC_RMW) ? SR_WRITE : SR_READ) | SR_MIN;
		goto search;
	case DB_LAST:
		sflags = (F_ISSET(dbc, DBC_RMW) ? SR_WRITE : SR_READ) | SR_MAX;
		goto search;
	case DB_SET_RECNO:
		if ((ret = __ram_getno(dbc, key, &recno, 0)) != 0)
			return (ret);
		sflags =
		    (F_ISSET(dbc, DBC_RMW) ?  SR_FIND_WR : SR_FIND) | SR_EXACT;
		if ((ret = __bam_rsearch(dbc, &recno, sflags, 1, exactp)) != 0)
			return (ret);
		goto done;
	case DB_SET:
	case DB_GET_BOTH:
		sflags =
		    (F_ISSET(dbc, DBC_RMW) ? SR_FIND_WR : SR_FIND) | SR_EXACT;
		if (bulk)
			break;
		goto search;
	case DB_GET_BOTH_RANGE:
		sflags = (F_ISSET(dbc, DBC_RMW) ? SR_FIND_WR : SR_FIND);
		goto search;
	case DB_SET_RANGE:
		sflags =
		    (F_ISSET(dbc, DBC_RMW) ? SR_WRITE : SR_READ) | SR_DUPFIRST;
		goto search;
	case DB_KEYFIRST:
	case DB_NOOVERWRITE:
		sflags = SR_KEYFIRST;
		break;
	case DB_KEYLAST:
	case DB_NODUPDATA:
	case DB_OVERWRITE_DUP:
		sflags = SR_KEYLAST;
		break;
	default:
		return (__db_unknown_flag(dbp->env, "__bamc_search", flags));
	}

	/*
	 * If the application has a history of inserting into the first or last
	 * pages of the database, we check those pages first to avoid doing a
	 * full search.  Similarly, if the cursor is configured as a bulk
	 * cursor, check whether this operation belongs on the same page as the
	 * last one.
	 */
	if (bulk)
		bt_lpgno = cp->pgno;
	else {
		if (F_ISSET(dbc, DBC_OPD))
			goto search;

		/*
		 * !!!
		 * We do not mutex protect the t->bt_lpgno field, which means
		 * that it can only be used in an advisory manner.  If we find
		 * page we can use, great.  If we don't, we don't care, we do
		 * it the slow way instead.  Regardless, copy it into a local
		 * variable, otherwise we might acquire a lock for a page and
		 * then read a different page because it changed underfoot.
		 */
		bt_lpgno = t->bt_lpgno;
	}

	/*
	 * If the tree has no history of insertion, do it the slow way.
	 */
	if (bt_lpgno == PGNO_INVALID)
		goto search;

	/*
	 * Lock and retrieve the page on which we last inserted.
	 *
	 * The page may not exist: if a transaction created the page
	 * and then aborted, the page might have been truncated from
	 * the end of the file.  We don't want to wait on the lock.
	 * The page may not even be relevant to this search.
	 */
	h = NULL;
	ACQUIRE_CUR(dbc, DB_LOCK_WRITE, bt_lpgno, DB_LOCK_NOWAIT, ret);
	if (ret != 0) {
		if (ret == DB_LOCK_DEADLOCK ||
		    ret == DB_LOCK_NOTGRANTED ||
		    ret == DB_PAGE_NOTFOUND)
			ret = 0;
		goto fast_miss;
	}

	h = cp->page;
	inp = P_INP(dbp, h);

	/*
	 * It's okay if the page type isn't right or it's empty, it
	 * just means that the world changed.
	 */
	if (TYPE(h) != P_LBTREE || NUM_ENT(h) == 0)
		goto fast_miss;

	/* Verify that this page cannot have moved to another db. */
	if (F_ISSET(dbp, DB_AM_SUBDB) &&
	    LOG_COMPARE(&t->bt_llsn, &LSN(h)) != 0)
		goto fast_miss;

	/*
	 * What we do here is test to see if we're at the beginning or
	 * end of the tree and if the new item sorts before/after the
	 * first/last page entry.  We only try to catch inserts into
	 * the middle of the tree for bulk cursors.
	 */
	if (h->next_pgno == PGNO_INVALID) {
		indx = NUM_ENT(h) - P_INDX;
		if ((ret = __bam_cmp(dbc, key, h, indx,
		    t->bt_compare, &cmp)) != 0)
			goto fast_miss;
		if (cmp > 0)
			indx += P_INDX;
		if (cmp >= 0)
			goto fast_hit;
	}
	if (h->prev_pgno == PGNO_INVALID) {
		indx = 0;
		if ((ret = __bam_cmp(dbc, key, h, indx,
		    t->bt_compare, &cmp)) != 0)
			goto fast_miss;
		if (cmp <= 0)
			goto fast_hit;
	}
	if (bulk) {
		DB_BINARY_SEARCH_FOR(base, lim, NUM_ENT(h), P_INDX) {
			DB_BINARY_SEARCH_INCR(indx, base, lim, P_INDX);
			if ((ret = __bam_cmp(dbc, key, h, indx,
			    t->bt_compare, &cmp)) != 0)
				goto fast_miss;

			if (cmp == 0)
				goto fast_hit;
			if (cmp > 0)
				DB_BINARY_SEARCH_SHIFT_BASE(indx, base,
				    lim, P_INDX);
		}
		/*
		 * No match found: base is the smallest index greater than
		 * the key and may be zero or NUM_ENT(h).
		 */
		indx = base;
		if (indx > 0 && indx < NUM_ENT(h)) {
			if (FLD_ISSET(sflags, SR_EXACT))
				return (DB_NOTFOUND);
			goto fast_hit;
		}
	}
	goto fast_miss;

fast_hit:
	if (cmp == 0) {
		/*
		 * Found a duplicate.  Deal with DB_KEYFIRST / DB_KEYLAST.
		 */
		if (FLD_ISSET(sflags, SR_DUPFIRST))
			while (indx > 0 && inp[indx - P_INDX] == inp[indx])
				indx -= P_INDX;
		else if (FLD_ISSET(sflags, SR_DUPLAST))
			while (indx < (db_indx_t)(NUM_ENT(h) - P_INDX) &&
			    inp[indx] == inp[indx + P_INDX])
				indx += P_INDX;
	}

	/* Set the exact match flag, we may have found a duplicate. */
	*exactp = (cmp == 0);

	/*
	 * Insert the entry in the stack.  (Our caller is likely to
	 * call __bam_stkrel() after our return.)
	 */
	BT_STK_CLR(cp);
	BT_STK_ENTER(dbp->env,
	    cp, h, indx, cp->lock, cp->lock_mode, ret);
	if (ret != 0)
		return (ret);
	goto done;

fast_miss:
	/*
	 * This was not the right page, so we do not need to retain
	 * the lock even in the presence of transactions.
	 *
	 * This is also an error path, so ret may have been set.
	 */
	DISCARD_CUR(dbc, ret);
	cp->pgno = PGNO_INVALID;
	if ((t_ret = __LPUT(dbc, cp->lock)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		return (ret);

search:
	if ((ret = __bam_search(dbc, root_pgno,
	    key, sflags, 1, NULL, exactp)) != 0)
		return (ret);

done:	/* Initialize the cursor from the stack. */
	cp->page = cp->csp->page;
	cp->pgno = cp->csp->page->pgno;
	cp->indx = cp->csp->indx;
	cp->lock = cp->csp->lock;
	cp->lock_mode = cp->csp->lock_mode;

	/* If on an empty page or a deleted record, move to the next one. */
	if (flags == DB_FIRST &&
	    (NUM_ENT(cp->page) == 0 || IS_CUR_DELETED(dbc)))
		if ((ret = __bamc_next(dbc, 0, 0)) != 0)
			return (ret);
	if (flags == DB_LAST &&
	    (NUM_ENT(cp->page) == 0 || IS_CUR_DELETED(dbc)))
		if ((ret = __bamc_prev(dbc)) != 0)
			return (ret);

	return (0);
}

/*
 * __bamc_physdel --
 *	Physically remove an item from the page.
 */
static int
__bamc_physdel(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT key;
	DB_LOCK next_lock, prev_lock;
	db_pgno_t pgno;
	int delete_page, empty_page, exact, ret;

	dbp = dbc->dbp;
	memset(&key, 0, sizeof(DBT));
	cp = (BTREE_CURSOR *)dbc->internal;
	delete_page = empty_page = ret = 0;
	LOCK_INIT(next_lock);
	LOCK_INIT(prev_lock);

	/* If the page is going to be emptied, consider deleting it. */
	delete_page = empty_page =
	    NUM_ENT(cp->page) == (TYPE(cp->page) == P_LBTREE ? 2 : 1);

	/*
	 * Check if the application turned off reverse splits.  Applications
	 * can't turn off reverse splits in off-page duplicate trees, that
	 * space will never be reused unless the exact same key is specified.
	 */
	if (delete_page &&
	    !F_ISSET(dbc, DBC_OPD) && F_ISSET(dbp, DB_AM_REVSPLITOFF))
		delete_page = 0;

	/*
	 * We never delete the last leaf page.  (Not really true -- we delete
	 * the last leaf page of off-page duplicate trees, but that's handled
	 * by our caller, not down here.)
	 */
	if (delete_page && cp->pgno == cp->root)
		delete_page = 0;

	/*
	 * To delete a leaf page other than an empty root page, we need a
	 * copy of a key from the page.  Use the 0th page index since it's
	 * the last key the page held.
	 *
	 * !!!
	 * Note that because __bamc_physdel is always called from a cursor
	 * close, it should be safe to use the cursor's own "my_rkey" memory
	 * to temporarily hold this key.  We shouldn't own any returned-data
	 * memory of interest--if we do, we're in trouble anyway.
	 */
	if (delete_page) {
		if ((ret = __db_ret(dbc, cp->page, 0, &key,
		    &dbc->my_rkey.data, &dbc->my_rkey.ulen)) != 0)
			return (ret);
	}

	/*
	 * Delete the items.  If page isn't empty, we adjust the cursors.
	 *
	 * !!!
	 * The following operations to delete a page may deadlock.  The easy
	 * scenario is if we're deleting an item because we're closing cursors
	 * because we've already deadlocked and want to call txn->abort.  If
	 * we fail due to deadlock, we'll leave a locked, possibly empty page
	 * in the tree, which won't be empty long because we'll undo the delete
	 * when we undo the transaction's modifications.
	 *
	 * !!!
	 * Delete the key item first, otherwise the on-page duplicate checks
	 * in __bam_ditem() won't work!
	 */
	if ((ret = __memp_dirty(dbp->mpf,
	    &cp->page, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
		return (ret);
	if (TYPE(cp->page) == P_LBTREE) {
		if ((ret = __bam_ditem(dbc, cp->page, cp->indx)) != 0)
			return (ret);
		if (!empty_page)
			if ((ret = __bam_ca_di(dbc,
			    PGNO(cp->page), cp->indx, -1)) != 0)
				return (ret);
	}
	if ((ret = __bam_ditem(dbc, cp->page, cp->indx)) != 0)
		return (ret);

	/* Clear the deleted flag, the item is gone. */
	F_CLR(cp, C_DELETED);

	if (!empty_page)
		if ((ret = __bam_ca_di(dbc, PGNO(cp->page), cp->indx, -1)) != 0)
			return (ret);

	/*
	 * Need to downgrade write locks here or non-txn locks will get stuck.
	 */
	if (F_ISSET(dbc->dbp, DB_AM_READ_UNCOMMITTED)) {
		if ((ret = __TLPUT(dbc, cp->lock)) != 0)
			return (ret);
		cp->lock_mode = DB_LOCK_WWRITE;
		if (cp->page != NULL &&
		    (ret = __memp_shared(dbp->mpf, cp->page)) != 0)
			return (ret);
	}
	/* If we're not going to try and delete the page, we're done. */
	if (!delete_page)
		return (0);

	/*
	 * Lock the previous and next pages before latching the parent
	 * sub tree.
	 */
	if (STD_LOCKING(dbc)) {
		if ((pgno = PREV_PGNO(cp->page)) != PGNO_INVALID &&
		    (ret = __db_lget(dbc,
		    0, pgno, DB_LOCK_WRITE, 0, &prev_lock)) != 0)
			return (ret);
		if ((pgno = NEXT_PGNO(cp->page)) != PGNO_INVALID &&
		    (ret = __db_lget(dbc,
		    0, pgno, DB_LOCK_WRITE, 0, &next_lock)) != 0) {
			(void)__TLPUT(dbc, next_lock);
			return (ret);
		}
	}
	DISCARD_CUR(dbc, ret);
	if (ret != 0)
		goto err;
	ret = __bam_search(dbc, PGNO_INVALID, &key, SR_DEL, 0, NULL, &exact);

	/*
	 * If everything worked, delete the stack, otherwise, release the
	 * stack and page locks without further damage.
	 */
	if (ret == 0)
		ret = __bam_dpages(dbc, 1, BTD_RELINK);
	else
		(void)__bam_stkrel(dbc, 0);

err:	(void)__TLPUT(dbc, prev_lock);
	(void)__TLPUT(dbc, next_lock);
	return (ret);
}

/*
 * __bamc_getstack --
 *	Acquire a full stack for a cursor.
 */
static int
__bamc_getstack(dbc)
	DBC *dbc;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT dbt;
	DB_MPOOLFILE *mpf;
	PAGE *h;
	int exact, ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Get the page with the current item on it.  The caller of this
	 * routine has to already hold a read lock on the page, so there
	 * is no additional lock to acquire.
	 */
	if ((ret = __memp_fget(mpf, &cp->pgno,
	     dbc->thread_info, dbc->txn, 0, &h)) != 0)
		return (ret);

	/* Get a copy of a key from the page. */
	memset(&dbt, 0, sizeof(DBT));
	ret = __db_ret(dbc, h, 0, &dbt,
	     &dbc->my_rkey.data, &dbc->my_rkey.ulen);
	if ((t_ret = __memp_fput(mpf,
	     dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		return (ret);

	/* Get a write-locked stack for the page. */
	exact = 0;
	ret = __bam_search(dbc, PGNO_INVALID,
	    &dbt, SR_KEYFIRST, 1, NULL, &exact);

	return (ret);
}

/*
 * __bam_isopd --
 *	Return if the cursor references an off-page duplicate tree via its
 *	page number.
 */
static int
__bam_isopd(dbc, pgnop)
	DBC *dbc;
	db_pgno_t *pgnop;
{
	BOVERFLOW *bo;

	if (TYPE(dbc->internal->page) != P_LBTREE)
		return (0);

	bo = GET_BOVERFLOW(dbc->dbp,
	    dbc->internal->page, dbc->internal->indx + O_INDX);
	if (B_TYPE(bo->type) == B_DUPLICATE) {
		*pgnop = bo->pgno;
		return (1);
	}
	return (0);
}

/*
 * __bam_opd_exists --
 *	Return if the current position has any data.
 * PUBLIC: int  __bam_opd_exists __P((DBC *, db_pgno_t));
 */
int
__bam_opd_exists(dbc, pgno)
	DBC *dbc;
	db_pgno_t pgno;
{
	PAGE *h;
	int ret;

	if ((ret = __memp_fget(dbc->dbp->mpf, &pgno,
	    dbc->thread_info, dbc->txn, 0, &h)) != 0)
		return (ret);

	/*
	 * We always collapse OPD trees so we only need to check
	 * the number of entries on the root.  If there is a non-empty
	 * tree then there will be duplicates.
	 */
	if (NUM_ENT(h) == 0)
		ret = 0;
	else
		ret = DB_KEYEXIST;

	(void)__memp_fput(dbc->dbp->mpf, dbc->thread_info, h, dbc->priority);

	return (ret);
}
