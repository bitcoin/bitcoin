/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"

static int  __ram_add __P((DBC *, db_recno_t *, DBT *, u_int32_t, u_int32_t));
static int  __ram_source __P((DB *));
static int  __ram_sread __P((DBC *, db_recno_t));
static int  __ram_update __P((DBC *, db_recno_t, int));

/*
 * In recno, there are two meanings to the on-page "deleted" flag.  If we're
 * re-numbering records, it means the record was implicitly created.  We skip
 * over implicitly created records if doing a cursor "next" or "prev", and
 * return DB_KEYEMPTY if they're explicitly requested..  If not re-numbering
 * records, it means that the record was implicitly created, or was deleted.
 * We skip over implicitly created or deleted records if doing a cursor "next"
 * or "prev", and return DB_KEYEMPTY if they're explicitly requested.
 *
 * If we're re-numbering records, then we have to detect in the cursor that
 * a record was deleted, and adjust the cursor as necessary on the next get.
 * If we're not re-numbering records, then we can detect that a record has
 * been deleted by looking at the actual on-page record, so we completely
 * ignore the cursor's delete flag.  This is different from the B+tree code.
 * It also maintains whether the cursor references a deleted record in the
 * cursor, and it doesn't always check the on-page value.
 */
#define	CD_SET(cp) {							\
	if (F_ISSET(cp, C_RENUMBER))					\
		F_SET(cp, C_DELETED);					\
}
#define	CD_CLR(cp) {							\
	if (F_ISSET(cp, C_RENUMBER)) {					\
		F_CLR(cp, C_DELETED);					\
		cp->order = INVALID_ORDER;				\
	}								\
}
#define	CD_ISSET(cp)							\
	(F_ISSET(cp, C_RENUMBER) && F_ISSET(cp, C_DELETED) ? 1 : 0)

/*
 * Macros for comparing the ordering of two cursors.
 * cp1 comes before cp2 iff one of the following holds:
 *	cp1's recno is less than cp2's recno
 *	recnos are equal, both deleted, and cp1's order is less than cp2's
 *	recnos are equal, cp1 deleted, and cp2 not deleted
 */
#define	C_LESSTHAN(cp1, cp2)						\
    (((cp1)->recno < (cp2)->recno) ||					\
    (((cp1)->recno == (cp2)->recno) &&					\
    ((CD_ISSET((cp1)) && CD_ISSET((cp2)) && (cp1)->order < (cp2)->order) || \
    (CD_ISSET((cp1)) && !CD_ISSET((cp2))))))

/*
 * cp1 is equal to cp2 iff their recnos and delete flags are identical,
 * and if the delete flag is set their orders are also identical.
 */
#define	C_EQUAL(cp1, cp2)						\
    ((cp1)->recno == (cp2)->recno && CD_ISSET((cp1)) == CD_ISSET((cp2)) && \
    (!CD_ISSET((cp1)) || (cp1)->order == (cp2)->order))

/*
 * Do we need to log the current cursor adjustment?
 */
#define	CURADJ_LOG(dbc)							\
	(DBC_LOGGING((dbc)) && (dbc)->txn != NULL && (dbc)->txn->parent != NULL)

/*
 * After a search, copy the found page into the cursor, discarding any
 * currently held lock.
 */
#define	STACK_TO_CURSOR(cp, ret) {					\
	int __t_ret;							\
	(cp)->page = (cp)->csp->page;					\
	(cp)->pgno = (cp)->csp->page->pgno;				\
	(cp)->indx = (cp)->csp->indx;					\
	if ((__t_ret = __TLPUT(dbc, (cp)->lock)) != 0 && (ret) == 0)	\
		ret = __t_ret;						\
	(cp)->lock = (cp)->csp->lock;					\
	(cp)->lock_mode = (cp)->csp->lock_mode;				\
}

/*
 * __ram_open --
 *	Recno open function.
 *
 * PUBLIC: int __ram_open __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:      DB_TXN *, const char *, db_pgno_t, u_int32_t));
 */
int
__ram_open(dbp, ip, txn, name, base_pgno, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name;
	db_pgno_t base_pgno;
	u_int32_t flags;
{
	BTREE *t;
	DBC *dbc;
	int ret, t_ret;

	COMPQUIET(name, NULL);
	t = dbp->bt_internal;

	/* Start up the tree. */
	if ((ret = __bam_read_root(dbp, ip, txn, base_pgno, flags)) != 0)
		return (ret);

	/*
	 * If the user specified a source tree, open it and map it in.
	 *
	 * !!!
	 * We don't complain if the user specified transactions or threads.
	 * It's possible to make it work, but you'd better know what you're
	 * doing!
	 */
	if (t->re_source != NULL && (ret = __ram_source(dbp)) != 0)
		return (ret);

	/* If we're snapshotting an underlying source file, do it now. */
	if (F_ISSET(dbp, DB_AM_SNAPSHOT)) {
		/* Allocate a cursor. */
		if ((ret = __db_cursor(dbp, ip, NULL, &dbc, 0)) != 0)
			return (ret);

		/* Do the snapshot. */
		if ((ret = __ram_update(dbc,
		    DB_MAX_RECORDS, 0)) != 0 && ret == DB_NOTFOUND)
			ret = 0;

		/* Discard the cursor. */
		if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __ram_append --
 *	Recno append function.
 *
 * PUBLIC: int __ram_append __P((DBC *, DBT *, DBT *));
 */
int
__ram_append(dbc, key, data)
	DBC *dbc;
	DBT *key, *data;
{
	BTREE_CURSOR *cp;
	int ret;

	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Make sure we've read in all of the backing source file.  If
	 * we found the record or it simply didn't exist, add the
	 * user's record.
	 */
	ret = __ram_update(dbc, DB_MAX_RECORDS, 0);
	if (ret == 0 || ret == DB_NOTFOUND)
		ret = __ram_add(dbc, &cp->recno, data, DB_APPEND, 0);

	/* Return the record number. */
	if (ret == 0 && key != NULL)
		ret = __db_retcopy(dbc->env, key, &cp->recno,
		    sizeof(cp->recno), &dbc->rkey->data, &dbc->rkey->ulen);

	return (ret);
}

/*
 * __ramc_del --
 *	Recno DBC->del function.
 *
 * PUBLIC: int __ramc_del __P((DBC *, u_int32_t));
 */
int
__ramc_del(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	BKEYDATA bk;
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT hdr, data;
	DB_LOCK next_lock, prev_lock;
	DB_LSN lsn;
	db_pgno_t npgno, ppgno, save_npgno, save_ppgno;
	int exact, nc, ret, stack, t_ret;

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;
	t = dbp->bt_internal;
	stack = 0;
	save_npgno = save_ppgno = PGNO_INVALID;
	LOCK_INIT(next_lock);
	LOCK_INIT(prev_lock);
	COMPQUIET(flags, 0);

	/*
	 * The semantics of cursors during delete are as follows: in
	 * non-renumbering recnos, records are replaced with a marker
	 * containing a delete flag.  If the record referenced by this cursor
	 * has already been deleted, we will detect that as part of the delete
	 * operation, and fail.
	 *
	 * In renumbering recnos, cursors which represent deleted items
	 * are flagged with the C_DELETED flag, and it is an error to
	 * call c_del a second time without an intervening cursor motion.
	 */
	if (CD_ISSET(cp))
		return (DB_KEYEMPTY);

	/* Search the tree for the key; delete only deletes exact matches. */
retry:	if ((ret = __bam_rsearch(dbc, &cp->recno, SR_DELETE, 1, &exact)) != 0)
		goto err;
	if (!exact) {
		ret = DB_NOTFOUND;
		goto err;
	}
	stack = 1;

	/* Copy the page into the cursor. */
	STACK_TO_CURSOR(cp, ret);
	if (ret != 0)
		goto err;

	/*
	 * If re-numbering records, the on-page deleted flag can only mean
	 * that this record was implicitly created.  Applications aren't
	 * permitted to delete records they never created, return an error.
	 *
	 * If not re-numbering records, the on-page deleted flag means that
	 * this record was implicitly created, or, was deleted at some time.
	 * The former is an error because applications aren't permitted to
	 * delete records they never created, the latter is an error because
	 * if the record was "deleted", we could never have found it.
	 */
	if (B_DISSET(GET_BKEYDATA(dbp, cp->page, cp->indx)->type)) {
		ret = DB_KEYEMPTY;
		goto err;
	}

	if (F_ISSET(cp, C_RENUMBER)) {
		/* If we are going to drop the page, lock its neighbors. */
		if (STD_LOCKING(dbc) &&
		    NUM_ENT(cp->page) == 1 && PGNO(cp->page) != cp->root) {
			if ((npgno = NEXT_PGNO(cp->page)) != PGNO_INVALID)
				TRY_LOCK(dbc, npgno, save_npgno,
				    next_lock, DB_LOCK_WRITE, retry);
			if (ret != 0)
				goto err;
			if ((ppgno = PREV_PGNO(cp->page)) != PGNO_INVALID)
				TRY_LOCK(dbc, ppgno, save_ppgno,
				    prev_lock, DB_LOCK_WRITE, retry);
			if (ret != 0)
				goto err;
		}
		/* Delete the item, adjust the counts, adjust the cursors. */
		if ((ret = __bam_ditem(dbc, cp->page, cp->indx)) != 0)
			goto err;
		if ((ret = __bam_adjust(dbc, -1)) != 0)
			goto err;
		if ((ret = __ram_ca(dbc, CA_DELETE, &nc)) != 0)
			goto err;
		if (nc > 0 &&
		    CURADJ_LOG(dbc) && (ret = __bam_rcuradj_log(dbp, dbc->txn,
		    &lsn, 0, CA_DELETE, cp->root, cp->recno, cp->order)) != 0)
			goto err;

		/*
		 * If the page is empty, delete it.
		 *
		 * We never delete a root page.  First, root pages of primary
		 * databases never go away, recno or otherwise.  However, if
		 * it's the root page of an off-page duplicates database, then
		 * it can be deleted.   We don't delete it here because we have
		 * no way of telling the primary database page holder (e.g.,
		 * the hash access method) that its page element should cleaned
		 * up because the underlying tree is gone.  So, we keep the page
		 * around until the last cursor referencing the empty tree is
		 * are closed, and then clean it up.
		 */
		if (NUM_ENT(cp->page) == 0 && PGNO(cp->page) != cp->root) {
			/*
			 * We want to delete a single item out of the last page
			 * that we're not deleting.
			 */
			ret = __bam_dpages(dbc, 0, BTD_RELINK);

			/*
			 * Regardless of the return from __bam_dpages, it will
			 * discard our stack and pinned page.
			 */
			stack = 0;
			cp->page = NULL;
			LOCK_INIT(cp->lock);
			cp->lock_mode = DB_LOCK_NG;
		}
	} else {
		/* Use a delete/put pair to replace the record with a marker. */
		if ((ret = __bam_ditem(dbc, cp->page, cp->indx)) != 0)
			goto err;

		B_TSET_DELETED(bk.type, B_KEYDATA);
		bk.len = 0;
		DB_INIT_DBT(hdr, &bk, SSZA(BKEYDATA, data));
		DB_INIT_DBT(data, "", 0);
		if ((ret = __db_pitem(dbc,
		    cp->page, cp->indx, BKEYDATA_SIZE(0), &hdr, &data)) != 0)
			goto err;
	}

	t->re_modified = 1;

err:	if (stack && (t_ret = __bam_stkrel(dbc, STK_CLRDBC)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __TLPUT(dbc, next_lock)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __TLPUT(dbc, prev_lock)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __ramc_get --
 *	Recno DBC->get function.
 *
 * PUBLIC: int __ramc_get
 * PUBLIC:     __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
 */
int
__ramc_get(dbc, key, data, flags, pgnop)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
	db_pgno_t *pgnop;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	int cmp, exact, ret;

	COMPQUIET(pgnop, NULL);

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;

	LF_CLR(DB_MULTIPLE|DB_MULTIPLE_KEY);
retry:	switch (flags) {
	case DB_CURRENT:
		/*
		 * If we're using mutable records and the deleted flag is
		 * set, the cursor is pointing at a nonexistent record;
		 * return an error.
		 */
		if (CD_ISSET(cp))
			return (DB_KEYEMPTY);
		break;
	case DB_NEXT_DUP:
		/*
		 * If we're not in an off-page dup set, we know there's no
		 * next duplicate since recnos don't have them.  If we
		 * are in an off-page dup set, the next item assuredly is
		 * a dup, so we set flags to DB_NEXT and keep going.
		 */
		if (!F_ISSET(dbc, DBC_OPD))
			return (DB_NOTFOUND);
		/* FALLTHROUGH */
	case DB_NEXT_NODUP:
		/*
		 * Recno databases don't have duplicates, set flags to DB_NEXT
		 * and keep going.
		 */
		/* FALLTHROUGH */
	case DB_NEXT:
		flags = DB_NEXT;
		/*
		 * If record numbers are mutable: if we just deleted a record,
		 * we have to avoid incrementing the record number so that we
		 * return the right record by virtue of renumbering the tree.
		 */
		if (CD_ISSET(cp)) {
			/*
			 * Clear the flag, we've moved off the deleted record.
			 */
			CD_CLR(cp);
			break;
		}

		if (cp->recno != RECNO_OOB) {
			++cp->recno;
			break;
		}
		/* FALLTHROUGH */
	case DB_FIRST:
		flags = DB_NEXT;
		cp->recno = 1;
		break;
	case DB_PREV_DUP:
		/*
		 * If we're not in an off-page dup set, we know there's no
		 * previous duplicate since recnos don't have them.  If we
		 * are in an off-page dup set, the previous item assuredly
		 * is a dup, so we set flags to DB_PREV and keep going.
		 */
		if (!F_ISSET(dbc, DBC_OPD))
			return (DB_NOTFOUND);
		/* FALLTHROUGH */
	case DB_PREV_NODUP:
		/*
		 * Recno databases don't have duplicates, set flags to DB_PREV
		 * and keep going.
		 */
		/* FALLTHROUGH */
	case DB_PREV:
		flags = DB_PREV;
		if (cp->recno != RECNO_OOB) {
			if (cp->recno == 1) {
				ret = DB_NOTFOUND;
				goto err;
			}
			--cp->recno;
			break;
		}
		/* FALLTHROUGH */
	case DB_LAST:
		flags = DB_PREV;
		if (((ret = __ram_update(dbc,
		    DB_MAX_RECORDS, 0)) != 0) && ret != DB_NOTFOUND)
			goto err;
		if ((ret = __bam_nrecs(dbc, &cp->recno)) != 0)
			goto err;
		if (cp->recno == 0) {
			ret = DB_NOTFOUND;
			goto err;
		}
		break;
	case DB_GET_BOTHC:
		/*
		 * If we're doing a join and these are offpage dups,
		 * we want to keep searching forward from after the
		 * current cursor position.  Increment the recno by 1,
		 * then proceed as for a DB_SET.
		 *
		 * Otherwise, we know there are no additional matching
		 * data, as recnos don't have dups.  return DB_NOTFOUND.
		 */
		if (F_ISSET(dbc, DBC_OPD)) {
			cp->recno++;
			break;
		}
		ret = DB_NOTFOUND;
		goto err;
		/* NOTREACHED */
	case DB_GET_BOTH:
	case DB_GET_BOTH_RANGE:
		/*
		 * If we're searching a set of off-page dups, we start
		 * a new linear search from the first record.  Otherwise,
		 * we compare the single data item associated with the
		 * requested record for a match.
		 */
		if (F_ISSET(dbc, DBC_OPD)) {
			cp->recno = 1;
			break;
		}
		/* FALLTHROUGH */
	case DB_SET:
	case DB_SET_RANGE:
		if ((ret = __ram_getno(dbc, key, &cp->recno, 0)) != 0)
			goto err;
		break;
	default:
		ret = __db_unknown_flag(dbp->env, "__ramc_get", flags);
		goto err;
	}

	/*
	 * For DB_PREV, DB_LAST, DB_SET and DB_SET_RANGE, we have already
	 * called __ram_update() to make sure sufficient records have been
	 * read from the backing source file.  Do it now for DB_CURRENT (if
	 * the current record was deleted we may need more records from the
	 * backing file for a DB_CURRENT operation), DB_FIRST and DB_NEXT.
	 * (We don't have to test for flags == DB_FIRST, because the switch
	 * statement above re-set flags to DB_NEXT in that case.)
	 */
	if ((flags == DB_NEXT || flags == DB_CURRENT) && ((ret =
	    __ram_update(dbc, cp->recno, 0)) != 0) && ret != DB_NOTFOUND)
		goto err;

	for (;; ++cp->recno) {
		/* Search the tree for the record. */
		if ((ret = __bam_rsearch(dbc, &cp->recno,
		    F_ISSET(dbc, DBC_RMW) ? SR_FIND_WR : SR_FIND,
		    1, &exact)) != 0)
			goto err;
		if (!exact) {
			ret = DB_NOTFOUND;
			goto err;
		}

		/* Copy the page into the cursor. */
		STACK_TO_CURSOR(cp, ret);
		if (ret != 0)
			goto err;

		/*
		 * If re-numbering records, the on-page deleted flag means this
		 * record was implicitly created.  If not re-numbering records,
		 * the on-page deleted flag means this record was implicitly
		 * created, or, it was deleted at some time.  Regardless, we
		 * skip such records if doing cursor next/prev operations or
		 * walking through off-page duplicates, and fail if they were
		 * requested explicitly by the application.
		 */
		if (B_DISSET(GET_BKEYDATA(dbp, cp->page, cp->indx)->type))
			switch (flags) {
			case DB_NEXT:
			case DB_PREV:
				(void)__bam_stkrel(dbc, STK_CLRDBC);
				goto retry;
			case DB_GET_BOTH:
			case DB_GET_BOTH_RANGE:
				/*
				 * If we're an OPD tree, we don't care about
				 * matching a record number on a DB_GET_BOTH
				 * -- everything belongs to the same tree.  A
				 * normal recno should give up and return
				 * DB_NOTFOUND if the matching recno is deleted.
				 */
				if (F_ISSET(dbc, DBC_OPD)) {
					(void)__bam_stkrel(dbc, STK_CLRDBC);
					continue;
				}
				ret = DB_NOTFOUND;
				goto err;
			default:
				ret = DB_KEYEMPTY;
				goto err;
			}

		if (flags == DB_GET_BOTH ||
		    flags == DB_GET_BOTHC || flags == DB_GET_BOTH_RANGE) {
			if ((ret = __bam_cmp(dbc, data, cp->page, cp->indx,
			    __bam_defcmp, &cmp)) != 0)
				return (ret);
			if (cmp == 0)
				break;
			if (!F_ISSET(dbc, DBC_OPD)) {
				ret = DB_NOTFOUND;
				goto err;
			}
			(void)__bam_stkrel(dbc, STK_CLRDBC);
		} else
			break;
	}

	/* Return the key if the user didn't give us one. */
	if (!F_ISSET(dbc, DBC_OPD) && !F_ISSET(key, DB_DBT_ISSET)) {
		ret = __db_retcopy(dbp->env,
		    key, &cp->recno, sizeof(cp->recno),
		    &dbc->rkey->data, &dbc->rkey->ulen);
		F_SET(key, DB_DBT_ISSET);
	}

	/* The cursor was reset, no further delete adjustment is necessary. */
err:	CD_CLR(cp);

	return (ret);
}

/*
 * __ramc_put --
 *	Recno DBC->put function.
 *
 * PUBLIC: int __ramc_put __P((DBC *, DBT *, DBT *, u_int32_t, db_pgno_t *));
 */
int
__ramc_put(dbc, key, data, flags, pgnop)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
	db_pgno_t *pgnop;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_LSN lsn;
	ENV *env;
	u_int32_t iiflags;
	int exact, nc, ret, t_ret;
	void *arg;

	COMPQUIET(pgnop, NULL);

	dbp = dbc->dbp;
	env = dbp->env;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * DB_KEYFIRST and DB_KEYLAST mean different things if they're
	 * used in an off-page duplicate tree.  If we're an off-page
	 * duplicate tree, they really mean "put at the beginning of the
	 * tree" and "put at the end of the tree" respectively, so translate
	 * them to something else.
	 */
	if (F_ISSET(dbc, DBC_OPD))
		switch (flags) {
		case DB_KEYFIRST:
			cp->recno = 1;
			flags = DB_BEFORE;
			break;
		case DB_KEYLAST:
			if ((ret = __ram_add(dbc,
			    &cp->recno, data, DB_APPEND, 0)) != 0)
				return (ret);
			if (CURADJ_LOG(dbc) &&
			    (ret = __bam_rcuradj_log(dbp, dbc->txn, &lsn, 0,
			    CA_ICURRENT, cp->root, cp->recno, cp->order)) != 0)
				return (ret);
			return (0);
		default:
			break;
		}

	/*
	 * Handle normal DB_KEYFIRST/DB_KEYLAST;  for a recno, which has
	 * no duplicates, these are identical and mean "put the given
	 * datum at the given recno".
	 */
	if (flags == DB_KEYFIRST || flags == DB_KEYLAST ||
	    flags == DB_NOOVERWRITE || flags == DB_OVERWRITE_DUP) {
		ret = __ram_getno(dbc, key, &cp->recno, 1);
		if (ret == 0 || ret == DB_NOTFOUND)
			ret = __ram_add(dbc, &cp->recno, data, flags, 0);
		return (ret);
	}

	/*
	 * If we're putting with a cursor that's marked C_DELETED, we need to
	 * take special care;  the cursor doesn't "really" reference the item
	 * corresponding to its current recno, but instead is "between" that
	 * record and the current one.  Translate the actual insert into
	 * DB_BEFORE, and let the __ram_ca work out the gory details of what
	 * should wind up pointing where.
	 */
	if (CD_ISSET(cp))
		iiflags = DB_BEFORE;
	else
		iiflags = flags;

split:	if ((ret = __bam_rsearch(dbc, &cp->recno, SR_INSERT, 1, &exact)) != 0)
		goto err;
	/*
	 * An inexact match is okay;  it just means we're one record past the
	 * end, which is reasonable if we're marked deleted.
	 */
	DB_ASSERT(env, exact || CD_ISSET(cp));

	/* Copy the page into the cursor. */
	STACK_TO_CURSOR(cp, ret);
	if (ret != 0)
		goto err;

	ret = __bam_iitem(dbc, key, data, iiflags, 0);
	t_ret = __bam_stkrel(dbc, STK_CLRDBC);

	if (t_ret != 0 && (ret == 0 || ret == DB_NEEDSPLIT))
		ret = t_ret;
	else if (ret == DB_NEEDSPLIT) {
		arg = &cp->recno;
		if ((ret = __bam_split(dbc, arg, NULL)) != 0)
			goto err;
		goto split;
	}
	if (ret != 0)
		goto err;

	switch (flags) {			/* Adjust the cursors. */
	case DB_AFTER:
		if ((ret = __ram_ca(dbc, CA_IAFTER, &nc)) != 0)
			goto err;

		/*
		 * We only need to adjust this cursor forward if we truly added
		 * the item after the current recno, rather than remapping it
		 * to DB_BEFORE.
		 */
		if (iiflags == DB_AFTER)
			++cp->recno;

		/* Only log if __ram_ca found any relevant cursors. */
		if (nc > 0 && CURADJ_LOG(dbc) &&
		    (ret = __bam_rcuradj_log(dbp, dbc->txn, &lsn, 0, CA_IAFTER,
		    cp->root, cp->recno, cp->order)) != 0)
			goto err;
		break;
	case DB_BEFORE:
		if ((ret = __ram_ca(dbc, CA_IBEFORE, &nc)) != 0)
			goto err;
		--cp->recno;

		/* Only log if __ram_ca found any relevant cursors. */
		if (nc > 0 && CURADJ_LOG(dbc) &&
		    (ret = __bam_rcuradj_log(dbp, dbc->txn, &lsn, 0, CA_IBEFORE,
		    cp->root, cp->recno, cp->order)) != 0)
			goto err;
		break;
	case DB_CURRENT:
		/*
		 * We only need to do an adjustment if we actually
		 * added an item, which we only would have done if the
		 * cursor was marked deleted.
		 */
		if (!CD_ISSET(cp))
			break;

		/* Only log if __ram_ca found any relevant cursors. */
		if ((ret = __ram_ca(dbc, CA_ICURRENT, &nc)) != 0)
			goto err;
		if (nc > 0 && CURADJ_LOG(dbc) &&
		    (ret = __bam_rcuradj_log(dbp, dbc->txn, &lsn, 0,
		    CA_ICURRENT, cp->root, cp->recno, cp->order)) != 0)
			goto err;
		break;
	default:
		break;
	}

	/* Return the key if we've created a new record. */
	if (!F_ISSET(dbc, DBC_OPD) &&
	    (flags == DB_AFTER || flags == DB_BEFORE) && key != NULL)
		ret = __db_retcopy(env, key, &cp->recno,
		    sizeof(cp->recno), &dbc->rkey->data, &dbc->rkey->ulen);

	/* The cursor was reset, no further delete adjustment is necessary. */
err:	CD_CLR(cp);

	return (ret);
}

/*
 * __ram_ca --
 *	Adjust cursors.  Returns the number of relevant cursors.
 *
 * PUBLIC: int __ram_ca __P((DBC *, ca_recno_arg, int *));
 */
int
__ram_ca(dbc_arg, op, foundp)
	DBC *dbc_arg;
	ca_recno_arg op;
	int *foundp;
{
	BTREE_CURSOR *cp, *cp_arg;
	DB *dbp, *ldbp;
	DBC *dbc;
	ENV *env;
	db_recno_t recno;
	u_int32_t order;
	int adjusted, found;

	dbp = dbc_arg->dbp;
	env = dbp->env;
	cp_arg = (BTREE_CURSOR *)dbc_arg->internal;
	recno = cp_arg->recno;

	/*
	 * It only makes sense to adjust cursors if we're a renumbering
	 * recno;  we should only be called if this is one.
	 */
	DB_ASSERT(env, F_ISSET(cp_arg, C_RENUMBER));

	MUTEX_LOCK(env, env->mtx_dblist);
	/*
	 * Adjust the cursors.  See the comment in __bam_ca_delete().
	 *
	 * If we're doing a delete, we need to find the highest
	 * order of any cursor currently pointing at this item,
	 * so we can assign a higher order to the newly deleted
	 * cursor.  Unfortunately, this requires a second pass through
	 * the cursor list.
	 */
	if (op == CA_DELETE) {
		FIND_FIRST_DB_MATCH(env, dbp, ldbp);
		for (order = 1;
		    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
		    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
			MUTEX_LOCK(env, dbp->mutex);
			TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
				cp = (BTREE_CURSOR *)dbc->internal;
				if (cp_arg->root == cp->root &&
				    recno == cp->recno && CD_ISSET(cp) &&
				    order <= cp->order &&
				    !MVCC_SKIP_CURADJ(dbc, cp->root))
					order = cp->order + 1;
			}
			MUTEX_UNLOCK(env, dbp->mutex);
		}
	} else
		order = INVALID_ORDER;

	/* Now go through and do the actual adjustments. */
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links) {
			cp = (BTREE_CURSOR *)dbc->internal;
			if (cp_arg->root != cp->root ||
			    MVCC_SKIP_CURADJ(dbc, cp->root))
				continue;
			++found;
			adjusted = 0;
			switch (op) {
			case CA_DELETE:
				if (recno < cp->recno) {
					--cp->recno;
					/*
					 * If the adjustment made them equal,
					 * we have to merge the orders.
					 */
					if (recno == cp->recno && CD_ISSET(cp))
						cp->order += order;
				} else if (recno == cp->recno &&
				    !CD_ISSET(cp)) {
					CD_SET(cp);
					cp->order = order;
					/*
					 * If we're deleting the item, we can't
					 * keep a streaming offset cached.
					 */
					cp->stream_start_pgno = PGNO_INVALID;
				}
				break;
			case CA_IBEFORE:
				/*
				 * IBEFORE is just like IAFTER, except that we
				 * adjust cursors on the current record too.
				 */
				if (C_EQUAL(cp_arg, cp)) {
					++cp->recno;
					adjusted = 1;
				}
				goto iafter;
			case CA_ICURRENT:

				/*
				 * If the original cursor wasn't deleted, we
				 * just did a replacement and so there's no
				 * need to adjust anything--we shouldn't have
				 * gotten this far.  Otherwise, we behave
				 * much like an IAFTER, except that all
				 * cursors pointing to the current item get
				 * marked undeleted and point to the new
				 * item.
				 */
				DB_ASSERT(env, CD_ISSET(cp_arg));
				if (C_EQUAL(cp_arg, cp)) {
					CD_CLR(cp);
					break;
				}
				/* FALLTHROUGH */
			case CA_IAFTER:
iafter:				if (!adjusted && C_LESSTHAN(cp_arg, cp)) {
					++cp->recno;
					adjusted = 1;
				}
				if (recno == cp->recno && adjusted)
					/*
					 * If we've moved this cursor's recno,
					 * split its order number--i.e.,
					 * decrement it by enough so that
					 * the lowest cursor moved has order 1.
					 * cp_arg->order is the split point,
					 * so decrement by one less than that.
					 */
					cp->order -= (cp_arg->order - 1);
				break;
			}
		}
		MUTEX_UNLOCK(dbp->env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	if (foundp != NULL)
		*foundp = found;
	return (0);
}

/*
 * __ram_getno --
 *	Check the user's record number, and make sure we've seen it.
 *
 * PUBLIC: int __ram_getno __P((DBC *, const DBT *, db_recno_t *, int));
 */
int
__ram_getno(dbc, key, rep, can_create)
	DBC *dbc;
	const DBT *key;
	db_recno_t *rep;
	int can_create;
{
	DB *dbp;
	db_recno_t recno;

	dbp = dbc->dbp;

	/* If passed an empty DBT from Java, key->data may be NULL */
	if (key->size != sizeof(db_recno_t)) {
		__db_errx(dbp->env, "illegal record number size");
		return (EINVAL);
	}

	/* Check the user's record number. */
	if ((recno = *(db_recno_t *)key->data) == 0) {
		__db_errx(dbp->env, "illegal record number of 0");
		return (EINVAL);
	}
	if (rep != NULL)
		*rep = recno;

	/*
	 * Btree can neither create records nor read them in.  Recno can
	 * do both, see if we can find the record.
	 */
	return (dbc->dbtype == DB_RECNO ?
	    __ram_update(dbc, recno, can_create) : 0);
}

/*
 * __ram_update --
 *	Ensure the tree has records up to and including the specified one.
 */
static int
__ram_update(dbc, recno, can_create)
	DBC *dbc;
	db_recno_t recno;
	int can_create;
{
	BTREE *t;
	DB *dbp;
	DBT *rdata;
	db_recno_t nrecs;
	int ret;

	dbp = dbc->dbp;
	t = dbp->bt_internal;

	/*
	 * If we can't create records and we've read the entire backing input
	 * file, we're done.
	 */
	if (!can_create && t->re_eof)
		return (0);

	/*
	 * If we haven't seen this record yet, try to get it from the original
	 * file.
	 */
	if ((ret = __bam_nrecs(dbc, &nrecs)) != 0)
		return (ret);
	if (!t->re_eof && recno > nrecs) {
		if ((ret = __ram_sread(dbc, recno)) != 0 && ret != DB_NOTFOUND)
			return (ret);
		if ((ret = __bam_nrecs(dbc, &nrecs)) != 0)
			return (ret);
	}

	/*
	 * If we can create records, create empty ones up to the requested
	 * record.
	 */
	if (!can_create || recno <= nrecs + 1)
		return (0);

	rdata = &dbc->my_rdata;
	rdata->flags = 0;
	rdata->size = 0;

	while (recno > ++nrecs)
		if ((ret = __ram_add(dbc,
		    &nrecs, rdata, 0, BI_DELETED)) != 0)
			return (ret);
	return (0);
}

/*
 * __ram_source --
 *	Load information about the backing file.
 */
static int
__ram_source(dbp)
	DB *dbp;
{
	BTREE *t;
	ENV *env;
	char *source;
	int ret;

	env = dbp->env;
	t = dbp->bt_internal;

	/* Find the real name, and swap out the one we had before. */
	if ((ret = __db_appname(env,
	    DB_APP_DATA, t->re_source, NULL, &source)) != 0)
		return (ret);
	__os_free(env, t->re_source);
	t->re_source = source;

	/*
	 * !!!
	 * It's possible that the backing source file is read-only.  We don't
	 * much care other than we'll complain if there are any modifications
	 * when it comes time to write the database back to the source.
	 */
	if ((t->re_fp = fopen(t->re_source, "rb")) == NULL) {
		ret = __os_get_errno();
		__db_err(env, ret, "%s", t->re_source);
		return (ret);
	}

	t->re_eof = 0;
	return (0);
}

/*
 * __ram_writeback --
 *	Rewrite the backing file.
 *
 * PUBLIC: int __ram_writeback __P((DB *));
 */
int
__ram_writeback(dbp)
	DB *dbp;
{
	BTREE *t;
	DBC *dbc;
	DBT key, data;
	DB_THREAD_INFO *ip;
	ENV *env;
	FILE *fp;
	db_recno_t keyno;
	int ret, t_ret;
	u_int8_t delim, *pad;

	t = dbp->bt_internal;
	env = dbp->env;
	fp = NULL;
	pad = NULL;

	/* If the file wasn't modified, we're done. */
	if (!t->re_modified)
		return (0);

	/* If there's no backing source file, we're done. */
	if (t->re_source == NULL) {
		t->re_modified = 0;
		return (0);
	}

	/*
	 * We step through the records, writing each one out.  Use the record
	 * number and the dbp->get() function, instead of a cursor, so we find
	 * and write out "deleted" or non-existent records.  The DB handle may
	 * be threaded, so allocate memory as we go.
	 */
	memset(&key, 0, sizeof(key));
	key.size = sizeof(db_recno_t);
	key.data = &keyno;
	memset(&data, 0, sizeof(data));
	F_SET(&data, DB_DBT_REALLOC);

	/* Allocate a cursor. */
	ENV_GET_THREAD_INFO(env, ip);
	if ((ret = __db_cursor(dbp, ip, NULL, &dbc, 0)) != 0)
		return (ret);

	/*
	 * Read any remaining records into the tree.
	 *
	 * !!!
	 * This is why we can't support transactions when applications specify
	 * backing (re_source) files.  At this point we have to read in the
	 * rest of the records from the file so that we can write all of the
	 * records back out again, which could modify a page for which we'd
	 * have to log changes and which we don't have locked.  This could be
	 * partially fixed by taking a snapshot of the entire file during the
	 * DB->open as DB->open is transaction protected.  But, if a checkpoint
	 * occurs then, the part of the log holding the copy of the file could
	 * be discarded, and that would make it impossible to recover in the
	 * face of disaster.  This could all probably be fixed, but it would
	 * require transaction protecting the backing source file.
	 *
	 * XXX
	 * This could be made to work now that we have transactions protecting
	 * file operations.  Margo has specifically asked for the privilege of
	 * doing this work.
	 */
	if ((ret =
	    __ram_update(dbc, DB_MAX_RECORDS, 0)) != 0 && ret != DB_NOTFOUND)
		goto err;

	/*
	 * Close any existing file handle and re-open the file, truncating it.
	 */
	if (t->re_fp != NULL) {
		if (fclose(t->re_fp) != 0) {
			ret = __os_get_errno();
			__db_err(env, ret, "%s", t->re_source);
			goto err;
		}
		t->re_fp = NULL;
	}
	if ((fp = fopen(t->re_source, "wb")) == NULL) {
		ret = __os_get_errno();
		__db_err(env, ret, "%s", t->re_source);
		goto err;
	}

	/*
	 * We'll need the delimiter if we're doing variable-length records,
	 * and the pad character if we're doing fixed-length records.
	 */
	delim = t->re_delim;
	for (keyno = 1;; ++keyno) {
		switch (ret = __db_get(dbp, ip, NULL, &key, &data, 0)) {
		case 0:
			if (data.size != 0 &&
			    fwrite(data.data, 1, data.size, fp) != data.size)
				goto write_err;
			break;
		case DB_KEYEMPTY:
			if (F_ISSET(dbp, DB_AM_FIXEDLEN)) {
				if (pad == NULL) {
					if ((ret = __os_malloc(
					    env, t->re_len, &pad)) != 0)
						goto err;
					memset(pad, t->re_pad, t->re_len);
				}
				if (fwrite(pad, 1, t->re_len, fp) != t->re_len)
					goto write_err;
			}
			break;
		case DB_NOTFOUND:
			ret = 0;
			goto done;
		default:
			goto err;
		}
		if (!F_ISSET(dbp, DB_AM_FIXEDLEN) &&
		    fwrite(&delim, 1, 1, fp) != 1) {
write_err:		ret = __os_get_errno();
			__db_err(env, ret,
			    "%s: write failed to backing file", t->re_source);
			goto err;
		}
	}

err:
done:	/* Close the file descriptor. */
	if (fp != NULL && fclose(fp) != 0) {
		t_ret = __os_get_errno();
		__db_err(env, t_ret, "%s", t->re_source);
		if (ret == 0)
			ret = t_ret;
	}

	/* Discard the cursor. */
	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	/* Discard memory allocated to hold the data items. */
	if (data.data != NULL)
		__os_ufree(env, data.data);
	if (pad != NULL)
		__os_free(env, pad);

	if (ret == 0)
		t->re_modified = 0;

	return (ret);
}

/*
 * __ram_sread --
 *	Read records from a source file.
 */
static int
__ram_sread(dbc, top)
	DBC *dbc;
	db_recno_t top;
{
	BTREE *t;
	DB *dbp;
	DBT data, *rdata;
	db_recno_t recno;
	size_t len;
	int ch, ret, was_modified;

	t = dbc->dbp->bt_internal;
	dbp = dbc->dbp;
	was_modified = t->re_modified;

	if ((ret = __bam_nrecs(dbc, &recno)) != 0)
		return (ret);

	/*
	 * Use the record key return memory, it's only a short-term use.
	 * The record data return memory is used by __bam_iitem, which
	 * we'll indirectly call, so use the key so as not to collide.
	 */
	len = F_ISSET(dbp, DB_AM_FIXEDLEN) ? t->re_len : 256;
	rdata = &dbc->my_rkey;
	if (rdata->ulen < len) {
		if ((ret = __os_realloc(
		    dbp->env, len, &rdata->data)) != 0) {
			rdata->ulen = 0;
			rdata->data = NULL;
			return (ret);
		}
		rdata->ulen = (u_int32_t)len;
	}

	memset(&data, 0, sizeof(data));
	while (recno < top) {
		data.data = rdata->data;
		data.size = 0;
		if (F_ISSET(dbp, DB_AM_FIXEDLEN))
			for (len = t->re_len; len > 0; --len) {
				if ((ch = fgetc(t->re_fp)) == EOF) {
					if (data.size == 0)
						goto eof;
					break;
				}
				((u_int8_t *)data.data)[data.size++] = ch;
			}
		else
			for (;;) {
				if ((ch = fgetc(t->re_fp)) == EOF) {
					if (data.size == 0)
						goto eof;
					break;
				}
				if (ch == t->re_delim)
					break;

				((u_int8_t *)data.data)[data.size++] = ch;
				if (data.size == rdata->ulen) {
					if ((ret = __os_realloc(dbp->env,
					    rdata->ulen *= 2,
					    &rdata->data)) != 0) {
						rdata->ulen = 0;
						rdata->data = NULL;
						return (ret);
					} else
						data.data = rdata->data;
				}
			}

		/*
		 * Another process may have read this record from the input
		 * file and stored it into the database already, in which
		 * case we don't need to repeat that operation.  We detect
		 * this by checking if the last record we've read is greater
		 * or equal to the number of records in the database.
		 */
		if (t->re_last >= recno) {
			++recno;
			if ((ret = __ram_add(dbc, &recno, &data, 0, 0)) != 0)
				goto err;
		}
		++t->re_last;
	}

	if (0) {
eof:		t->re_eof = 1;
		ret = DB_NOTFOUND;
	}
err:	if (!was_modified)
		t->re_modified = 0;

	return (ret);
}

/*
 * __ram_add --
 *	Add records into the tree.
 */
static int
__ram_add(dbc, recnop, data, flags, bi_flags)
	DBC *dbc;
	db_recno_t *recnop;
	DBT *data;
	u_int32_t flags, bi_flags;
{
	BTREE_CURSOR *cp;
	int exact, ret, stack, t_ret;

	cp = (BTREE_CURSOR *)dbc->internal;

retry:	/* Find the slot for insertion. */
	if ((ret = __bam_rsearch(dbc, recnop,
	    SR_INSERT | (flags == DB_APPEND ? SR_APPEND : 0), 1, &exact)) != 0)
		return (ret);
	stack = 1;

	/* Copy the page into the cursor. */
	STACK_TO_CURSOR(cp, ret);
	if (ret != 0)
		goto err;

	if (exact && flags == DB_NOOVERWRITE && !CD_ISSET(cp) &&
	    !B_DISSET(GET_BKEYDATA(dbc->dbp, cp->page, cp->indx)->type)) {
		ret = DB_KEYEXIST;
		goto err;
	}

	/*
	 * The application may modify the data based on the selected record
	 * number.
	 */
	if (flags == DB_APPEND && dbc->dbp->db_append_recno != NULL &&
	    (ret = dbc->dbp->db_append_recno(dbc->dbp, data, *recnop)) != 0)
		goto err;

	/*
	 * Select the arguments for __bam_iitem() and do the insert.  If the
	 * key is an exact match, or we're replacing the data item with a
	 * new data item, replace the current item.  If the key isn't an exact
	 * match, we're inserting a new key/data pair, before the search
	 * location.
	 */
	switch (ret = __bam_iitem(dbc,
	    NULL, data, exact ? DB_CURRENT : DB_BEFORE, bi_flags)) {
	case 0:
		/*
		 * Don't adjust anything.
		 *
		 * If we inserted a record, no cursors need adjusting because
		 * the only new record it's possible to insert is at the very
		 * end of the tree.  The necessary adjustments to the internal
		 * page counts were made by __bam_iitem().
		 *
		 * If we overwrote a record, no cursors need adjusting because
		 * future DBcursor->get calls will simply return the underlying
		 * record (there's no adjustment made for the DB_CURRENT flag
		 * when a cursor get operation immediately follows a cursor
		 * delete operation, and the normal adjustment for the DB_NEXT
		 * flag is still correct).
		 */
		break;
	case DB_NEEDSPLIT:
		/* Discard the stack of pages and split the page. */
		(void)__bam_stkrel(dbc, STK_CLRDBC);
		stack = 0;

		if ((ret = __bam_split(dbc, recnop, NULL)) != 0)
			goto err;

		goto retry;
		/* NOTREACHED */
	default:
		goto err;
	}

err:	if (stack && (t_ret = __bam_stkrel(dbc, STK_CLRDBC)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
