/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"

/*
 * __bam_rsearch --
 *	Search a btree for a record number.
 *
 * PUBLIC: int __bam_rsearch __P((DBC *, db_recno_t *, u_int32_t, int, int *));
 */
int
__bam_rsearch(dbc, recnop, flags, stop, exactp)
	DBC *dbc;
	db_recno_t *recnop;
	u_int32_t flags;
	int stop, *exactp;
{
	BINTERNAL *bi;
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h;
	RINTERNAL *ri;
	db_indx_t adjust, deloffset, indx, top;
	db_lockmode_t lock_mode;
	db_pgno_t pg;
	db_recno_t recno, t_recno, total;
	u_int32_t get_mode;
	int ret, stack, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	h = NULL;

	BT_STK_CLR(cp);

	/*
	 * There are several ways we search a btree tree.  The flags argument
	 * specifies if we're acquiring read or write locks and if we are
	 * locking pairs of pages.  In addition, if we're adding or deleting
	 * an item, we have to lock the entire tree, regardless.  See btree.h
	 * for more details.
	 *
	 * If write-locking pages, we need to know whether or not to acquire a
	 * write lock on a page before getting it.  This depends on how deep it
	 * is in tree, which we don't know until we acquire the root page.  So,
	 * if we need to lock the root page we may have to upgrade it later,
	 * because we won't get the correct lock initially.
	 *
	 * Retrieve the root page.
	 */

	if ((ret = __bam_get_root(dbc, cp->root, stop, flags, &stack)) != 0)
		return (ret);
	lock_mode = cp->csp->lock_mode;
	get_mode = lock_mode == DB_LOCK_WRITE ? DB_MPOOL_DIRTY : 0;
	lock = cp->csp->lock;
	h = cp->csp->page;

	BT_STK_CLR(cp);
	/*
	 * If appending to the tree, set the record number now -- we have the
	 * root page locked.
	 *
	 * Delete only deletes exact matches, read only returns exact matches.
	 * Note, this is different from __bam_search(), which returns non-exact
	 * matches for read.
	 *
	 * The record may not exist.  We can only return the correct location
	 * for the record immediately after the last record in the tree, so do
	 * a fast check now.
	 */
	total = RE_NREC(h);
	if (LF_ISSET(SR_APPEND)) {
		*exactp = 0;
		*recnop = recno = total + 1;
	} else {
		recno = *recnop;
		if (recno <= total)
			*exactp = 1;
		else {
			*exactp = 0;
			if (!LF_ISSET(SR_PAST_EOF) || recno > total + 1) {
				/*
				 * Keep the page locked for serializability.
				 *
				 * XXX
				 * This leaves the root page locked, which will
				 * eliminate any concurrency.  A possible fix
				 * would be to lock the last leaf page instead.
				 */
				ret = __memp_fput(mpf,
				    dbc->thread_info, h, dbc->priority);
				if ((t_ret =
				    __TLPUT(dbc, lock)) != 0 && ret == 0)
					ret = t_ret;
				return (ret == 0 ? DB_NOTFOUND : ret);
			}
		}
	}

	/*
	 * !!!
	 * Record numbers in the tree are 0-based, but the recno is
	 * 1-based.  All of the calculations below have to take this
	 * into account.
	 */
	for (total = 0;;) {
		switch (TYPE(h)) {
		case P_LBTREE:
			if (LF_ISSET(SR_MAX)) {
				indx = NUM_ENT(h) - 2;
				goto enter;
			}
			/* FALLTHROUGH */
		case P_LDUP:
			if (LF_ISSET(SR_MAX)) {
				indx = NUM_ENT(h) - 1;
				goto enter;
			}
			recno -= total;
			/*
			 * There may be logically deleted records on the page.
			 * If there are enough, the record may not exist.
			 */
			if (TYPE(h) == P_LBTREE) {
				adjust = P_INDX;
				deloffset = O_INDX;
			} else {
				adjust = O_INDX;
				deloffset = 0;
			}
			for (t_recno = 0, indx = 0;; indx += adjust) {
				if (indx >= NUM_ENT(h)) {
					*exactp = 0;
					if (!LF_ISSET(SR_PAST_EOF) ||
					    recno > t_recno + 1) {
						ret = __memp_fput(mpf,
						    dbc->thread_info,
						    h, dbc->priority);
						h = NULL;
						if ((t_ret = __TLPUT(dbc,
						    lock)) != 0 && ret == 0)
							ret = t_ret;
						if (ret == 0)
							ret = DB_NOTFOUND;
						goto err;
					}
				}
				if (!B_DISSET(GET_BKEYDATA(dbp, h,
				    indx + deloffset)->type) &&
				    ++t_recno == recno)
					break;
			}

			BT_STK_ENTER(env, cp, h, indx, lock, lock_mode, ret);
			if (ret != 0)
				goto err;
			if (LF_ISSET(SR_BOTH))
				goto get_prev;
			return (0);
		case P_IBTREE:
			if (LF_ISSET(SR_MAX)) {
				indx = NUM_ENT(h);
				bi = GET_BINTERNAL(dbp, h, indx - 1);
			} else for (indx = 0, top = NUM_ENT(h);;) {
				bi = GET_BINTERNAL(dbp, h, indx);
				if (++indx == top || total + bi->nrecs >= recno)
					break;
				total += bi->nrecs;
			}
			pg = bi->pgno;
			break;
		case P_LRECNO:
			if (LF_ISSET(SR_MAX))
				recno = NUM_ENT(h);
			else
				recno -= total;

			/* Correct from 1-based to 0-based for a page offset. */
			--recno;
enter:			BT_STK_ENTER(env, cp, h, recno, lock, lock_mode, ret);
			if (ret != 0)
				goto err;
			if (LF_ISSET(SR_BOTH)) {
get_prev:			DB_ASSERT(env, LF_ISSET(SR_NEXT));
				/*
				 * We have a NEXT tree, now add the sub tree
				 * that points gets to the previous page.
				 */
				cp->csp++;
				indx = cp->sp->indx - 1;
				h = cp->sp->page;
				if (TYPE(h) == P_IRECNO) {
					ri = GET_RINTERNAL(dbp, h, indx);
					pg = ri->pgno;
				} else {
					DB_ASSERT(env, TYPE(h) == P_IBTREE);
					bi = GET_BINTERNAL(dbp, h, indx);
					pg = bi->pgno;
				}
				LF_CLR(SR_NEXT | SR_BOTH);
				LF_SET(SR_MAX);
				stack = 1;
				h = NULL;
				goto lock_next;
			}
			return (0);
		case P_IRECNO:
			if (LF_ISSET(SR_MAX)) {
				indx = NUM_ENT(h);
				ri = GET_RINTERNAL(dbp, h, indx - 1);
			} else for (indx = 0, top = NUM_ENT(h);;) {
				ri = GET_RINTERNAL(dbp, h, indx);
				if (++indx == top || total + ri->nrecs >= recno)
					break;
				total += ri->nrecs;
			}
			pg = ri->pgno;
			break;
		default:
			return (__db_pgfmt(env, h->pgno));
		}
		--indx;

		/* Return if this is the lowest page wanted. */
		if (stop == LEVEL(h)) {
			BT_STK_ENTER(env, cp, h, indx, lock, lock_mode, ret);
			if (ret != 0)
				goto err;
			return (0);
		}
		if (stack) {
			BT_STK_PUSH(env, cp, h, indx, lock, lock_mode, ret);
			if (ret != 0)
				goto err;
			h = NULL;

			lock_mode = DB_LOCK_WRITE;
			get_mode = DB_MPOOL_DIRTY;
			if ((ret =
			    __db_lget(dbc, 0, pg, lock_mode, 0, &lock)) != 0)
				goto err;
		} else if (LF_ISSET(SR_NEXT)) {
			/*
			 * For RECNO if we are doing a NEXT search the
			 * search recno is the one we are looking for
			 * but we want to keep the stack from the spanning
			 * node on down.  We only know we have the spanning
			 * node when its child's index is 0, so save
			 * each node and discard the tree when we find out
			 * its not needed.
			 */
			if (indx != 0 && cp->sp->page != NULL) {
				BT_STK_POP(cp);
				if ((ret = __bam_stkrel(dbc, STK_NOLOCK)) != 0)
					goto err;
			}

			BT_STK_PUSH(env, cp, h, indx, lock, lock_mode, ret);
			h = NULL;
			if (ret != 0)
				goto err;
lock_next:		if ((ret =
			    __db_lget(dbc, 0, pg, lock_mode, 0, &lock)) != 0)
				goto err;
		} else {
			/*
			 * Decide if we want to return a pointer to the next
			 * page in the stack.  If we do, write lock it and
			 * never unlock it.
			 */
			if ((LF_ISSET(SR_PARENT) &&
			    (u_int8_t)(stop + 1) >= (u_int8_t)(LEVEL(h) - 1)) ||
			    (LEVEL(h) - 1) == LEAFLEVEL)
				stack = 1;

			if ((ret = __memp_fput(mpf,
			    dbc->thread_info, h, dbc->priority)) != 0)
				goto err;
			h = NULL;

			lock_mode = stack &&
			    LF_ISSET(SR_WRITE) ? DB_LOCK_WRITE : DB_LOCK_READ;
			if (lock_mode == DB_LOCK_WRITE)
				get_mode = DB_MPOOL_DIRTY;
			if ((ret = __db_lget(dbc,
			    LCK_COUPLE_ALWAYS, pg, lock_mode, 0, &lock)) != 0) {
				/*
				 * If we fail, discard the lock we held.  This
				 * is OK because this only happens when we are
				 * descending the tree holding read-locks.
				 */
				(void)__LPUT(dbc, lock);
				goto err;
			}
		}

		if ((ret = __memp_fget(mpf, &pg,
		     dbc->thread_info, dbc->txn, get_mode, &h)) != 0)
			goto err;
	}
	/* NOTREACHED */

err:	if (h != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	BT_STK_POP(cp);
	(void)__bam_stkrel(dbc, 0);

	return (ret);
}

/*
 * __bam_adjust --
 *	Adjust the tree after adding or deleting a record.
 *
 * PUBLIC: int __bam_adjust __P((DBC *, int32_t));
 */
int
__bam_adjust(dbc, adjust)
	DBC *dbc;
	int32_t adjust;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_MPOOLFILE *mpf;
	EPG *epg;
	PAGE *h;
	db_pgno_t root_pgno;
	int ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	root_pgno = cp->root;

	/* Update the record counts for the tree. */
	for (epg = cp->sp; epg <= cp->csp; ++epg) {
		h = epg->page;
		if (TYPE(h) == P_IBTREE || TYPE(h) == P_IRECNO) {
			ret = __memp_dirty(mpf, &h,
			    dbc->thread_info, dbc->txn, dbc->priority, 0);
			epg->page = h;
			if (ret != 0)
				return (ret);
			if (DBC_LOGGING(dbc)) {
				if ((ret = __bam_cadjust_log(dbp, dbc->txn,
				    &LSN(h), 0, PGNO(h), &LSN(h),
				    (u_int32_t)epg->indx, adjust,
				    PGNO(h) == root_pgno ?
				    CAD_UPDATEROOT : 0)) != 0)
					return (ret);
			} else
				LSN_NOT_LOGGED(LSN(h));

			if (TYPE(h) == P_IBTREE)
				GET_BINTERNAL(dbp, h, epg->indx)->nrecs +=
				    adjust;
			else
				GET_RINTERNAL(dbp, h, epg->indx)->nrecs +=
				    adjust;

			if (PGNO(h) == root_pgno)
				RE_NREC_ADJ(h, adjust);
		}
	}
	return (0);
}

/*
 * __bam_nrecs --
 *	Return the number of records in the tree.
 *
 * PUBLIC: int __bam_nrecs __P((DBC *, db_recno_t *));
 */
int
__bam_nrecs(dbc, rep)
	DBC *dbc;
	db_recno_t *rep;
{
	DB *dbp;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	PAGE *h;
	db_pgno_t pgno;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;

	pgno = dbc->internal->root;
	if ((ret = __db_lget(dbc, 0, pgno, DB_LOCK_READ, 0, &lock)) != 0)
		return (ret);
	if ((ret = __memp_fget(mpf, &pgno,
	     dbc->thread_info, dbc->txn, 0, &h)) != 0)
		return (ret);

	*rep = RE_NREC(h);

	ret = __memp_fput(mpf, dbc->thread_info, h, dbc->priority);
	if ((t_ret = __TLPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __bam_total --
 *	Return the number of records below a page.
 *
 * PUBLIC: db_recno_t __bam_total __P((DB *, PAGE *));
 */
db_recno_t
__bam_total(dbp, h)
	DB *dbp;
	PAGE *h;
{
	db_recno_t nrecs;
	db_indx_t indx, top;

	nrecs = 0;
	top = NUM_ENT(h);

	switch (TYPE(h)) {
	case P_LBTREE:
		/* Check for logically deleted records. */
		for (indx = 0; indx < top; indx += P_INDX)
			if (!B_DISSET(
			    GET_BKEYDATA(dbp, h, indx + O_INDX)->type))
				++nrecs;
		break;
	case P_LDUP:
		/* Check for logically deleted records. */
		for (indx = 0; indx < top; indx += O_INDX)
			if (!B_DISSET(GET_BKEYDATA(dbp, h, indx)->type))
				++nrecs;
		break;
	case P_IBTREE:
		for (indx = 0; indx < top; indx += O_INDX)
			nrecs += GET_BINTERNAL(dbp, h, indx)->nrecs;
		break;
	case P_LRECNO:
		nrecs = NUM_ENT(h);
		break;
	case P_IRECNO:
		for (indx = 0; indx < top; indx += O_INDX)
			nrecs += GET_RINTERNAL(dbp, h, indx)->nrecs;
		break;
	}

	return (nrecs);
}
