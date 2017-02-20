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

/*
 * __db_traverse_big
 *	Traverse a chain of overflow pages and call the callback routine
 * on each one.  The calling convention for the callback is:
 *	callback(dbc, page, cookie, did_put),
 * where did_put is a return value indicating if the page in question has
 * already been returned to the mpool.
 *
 * PUBLIC: int __db_traverse_big __P((DBC *, db_pgno_t,
 * PUBLIC:	int (*)(DBC *, PAGE *, void *, int *), void *));
 */
int
__db_traverse_big(dbc, pgno, callback, cookie)
	DBC *dbc;
	db_pgno_t pgno;
	int (*callback) __P((DBC *, PAGE *, void *, int *));
	void *cookie;
{
	DB_MPOOLFILE *mpf;
	PAGE *p;
	int did_put, ret;

	mpf = dbc->dbp->mpf;

	do {
		did_put = 0;
		if ((ret = __memp_fget(mpf,
		     &pgno, dbc->thread_info, dbc->txn, 0, &p)) != 0)
			return (ret);
		/*
		 * If we are freeing pages only process the overflow
		 * chain if the head of the chain has a refcount of 1.
		 */
		pgno = NEXT_PGNO(p);
		if (callback == __db_truncate_callback && OV_REF(p) != 1)
			pgno = PGNO_INVALID;
		if ((ret = callback(dbc, p, cookie, &did_put)) == 0 &&
		    !did_put)
			ret = __memp_fput(mpf,
			     dbc->thread_info, p, dbc->priority);
	} while (ret == 0 && pgno != PGNO_INVALID);

	return (ret);
}

/*
 * __db_reclaim_callback
 * This is the callback routine used during a delete of a subdatabase.
 * we are traversing a btree or hash table and trying to free all the
 * pages.  Since they share common code for duplicates and overflow
 * items, we traverse them identically and use this routine to do the
 * actual free.  The reason that this is callback is because hash uses
 * the same traversal code for statistics gathering.
 *
 * PUBLIC: int __db_reclaim_callback __P((DBC *, PAGE *, void *, int *));
 */
int
__db_reclaim_callback(dbc, p, cookie, putp)
	DBC *dbc;
	PAGE *p;
	void *cookie;
	int *putp;
{
	DB *dbp;
	int ret;

	COMPQUIET(cookie, NULL);
	dbp = dbc->dbp;

	/*
	 * We don't want to log the free of the root with the subdb.
	 * If we abort then the subdb may not be openable to undo
	 * the free.
	 */
	if ((dbp->type == DB_BTREE || dbp->type == DB_RECNO) &&
	    PGNO(p) == ((BTREE *)dbp->bt_internal)->bt_root)
		return (0);
	if ((ret = __db_free(dbc, p)) != 0)
		return (ret);
	*putp = 1;

	return (0);
}

/*
 * __db_truncate_callback
 * This is the callback routine used during a truncate.
 * we are traversing a btree or hash table and trying to free all the
 * pages.
 *
 * PUBLIC: int __db_truncate_callback __P((DBC *, PAGE *, void *, int *));
 */
int
__db_truncate_callback(dbc, p, cookie, putp)
	DBC *dbc;
	PAGE *p;
	void *cookie;
	int *putp;
{
	DB *dbp;
	DBT ddbt, ldbt;
	DB_MPOOLFILE *mpf;
	db_indx_t indx, len, off, tlen, top;
	u_int8_t *hk, type;
	u_int32_t *countp;
	int ret;

	top = NUM_ENT(p);
	dbp = dbc->dbp;
	mpf = dbp->mpf;
	countp = cookie;
	*putp = 1;

	switch (TYPE(p)) {
	case P_LBTREE:
		/* Skip for off-page duplicates and deleted items. */
		for (indx = 0; indx < top; indx += P_INDX) {
			type = GET_BKEYDATA(dbp, p, indx + O_INDX)->type;
			if (!B_DISSET(type) && B_TYPE(type) != B_DUPLICATE)
				++*countp;
		}
		/* FALLTHROUGH */
	case P_IBTREE:
	case P_IRECNO:
	case P_INVALID:
		if (dbp->type != DB_HASH &&
		    ((BTREE *)dbp->bt_internal)->bt_root == PGNO(p)) {
			type = dbp->type == DB_RECNO ? P_LRECNO : P_LBTREE;
			goto reinit;
		}
		break;
	case P_OVERFLOW:
		if ((ret = __memp_dirty(mpf,
		    &p, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
			return (ret);
		if (DBC_LOGGING(dbc)) {
			if ((ret = __db_ovref_log(dbp, dbc->txn,
			    &LSN(p), 0, p->pgno, -1, &LSN(p))) != 0)
				return (ret);
		} else
			LSN_NOT_LOGGED(LSN(p));
		if (--OV_REF(p) != 0)
			*putp = 0;
		break;
	case P_LRECNO:
		for (indx = 0; indx < top; indx += O_INDX) {
			type = GET_BKEYDATA(dbp, p, indx)->type;
			if (!B_DISSET(type))
				++*countp;
		}

		if (((BTREE *)dbp->bt_internal)->bt_root == PGNO(p)) {
			type = P_LRECNO;
			goto reinit;
		}
		break;
	case P_LDUP:
		/* Correct for deleted items. */
		for (indx = 0; indx < top; indx += O_INDX)
			if (!B_DISSET(GET_BKEYDATA(dbp, p, indx)->type))
				++*countp;

		break;
	case P_HASH:
		/* Correct for on-page duplicates and deleted items. */
		for (indx = 0; indx < top; indx += P_INDX) {
			switch (*H_PAIRDATA(dbp, p, indx)) {
			case H_OFFDUP:
				break;
			case H_OFFPAGE:
			case H_KEYDATA:
				++*countp;
				break;
			case H_DUPLICATE:
				tlen = LEN_HDATA(dbp, p, 0, indx);
				hk = H_PAIRDATA(dbp, p, indx);
				for (off = 0; off < tlen;
				    off += len + 2 * sizeof(db_indx_t)) {
					++*countp;
					memcpy(&len,
					    HKEYDATA_DATA(hk)
					    + off, sizeof(db_indx_t));
				}
				break;
			default:
				return (__db_pgfmt(dbp->env, p->pgno));
			}
		}
		/* Don't free the head of the bucket. */
		if (PREV_PGNO(p) == PGNO_INVALID) {
			type = P_HASH;

reinit:			if ((ret = __memp_dirty(mpf, &p,
			    dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
				return (ret);
			*putp = 0;
			if (DBC_LOGGING(dbc)) {
				memset(&ldbt, 0, sizeof(ldbt));
				memset(&ddbt, 0, sizeof(ddbt));
				ldbt.data = p;
				ldbt.size = P_OVERHEAD(dbp);
				ldbt.size += p->entries * sizeof(db_indx_t);
				ddbt.data = (u_int8_t *)p + HOFFSET(p);
				ddbt.size = dbp->pgsize - HOFFSET(p);
				if ((ret = __db_pg_init_log(dbp,
				    dbc->txn, &LSN(p), 0,
				    p->pgno, &ldbt, &ddbt)) != 0)
					return (ret);
			} else
				LSN_NOT_LOGGED(LSN(p));

			P_INIT(p, dbp->pgsize, PGNO(p), PGNO_INVALID,
			    PGNO_INVALID, type == P_HASH ? 0 : 1, type);
		}
		break;
	default:
		return (__db_pgfmt(dbp->env, p->pgno));
	}

	if (*putp == 1) {
		if ((ret = __db_free(dbc, p)) != 0)
			return (ret);
	} else {
		if ((ret = __memp_fput(mpf, dbc->thread_info, p,
		    dbc->priority)) != 0)
			return (ret);
		*putp = 1;
	}

	return (0);
}
