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
 * Copyright (c) 1990, 1993, 1994, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Mike Olson.
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
 * __bam_ditem --
 *	Delete one or more entries from a page.
 *
 * PUBLIC: int __bam_ditem __P((DBC *, PAGE *, u_int32_t));
 */
int
__bam_ditem(dbc, h, indx)
	DBC *dbc;
	PAGE *h;
	u_int32_t indx;
{
	BINTERNAL *bi;
	BKEYDATA *bk;
	DB *dbp;
	u_int32_t nbytes;
	int ret;
	db_indx_t *inp;

	dbp = dbc->dbp;
	inp = P_INP(dbp, h);

	/* The page should already have been dirtied by our caller. */
	DB_ASSERT(dbp->env, IS_DIRTY(h));

	switch (TYPE(h)) {
	case P_IBTREE:
		bi = GET_BINTERNAL(dbp, h, indx);
		switch (B_TYPE(bi->type)) {
		case B_DUPLICATE:
		case B_KEYDATA:
			nbytes = BINTERNAL_SIZE(bi->len);
			break;
		case B_OVERFLOW:
			nbytes = BINTERNAL_SIZE(bi->len);
			if ((ret =
			    __db_doff(dbc, ((BOVERFLOW *)bi->data)->pgno)) != 0)
				return (ret);
			break;
		default:
			return (__db_pgfmt(dbp->env, PGNO(h)));
		}
		break;
	case P_IRECNO:
		nbytes = RINTERNAL_SIZE;
		break;
	case P_LBTREE:
		/*
		 * If it's a duplicate key, discard the index and don't touch
		 * the actual page item.
		 *
		 * !!!
		 * This works because no data item can have an index matching
		 * any other index so even if the data item is in a key "slot",
		 * it won't match any other index.
		 */
		if ((indx % 2) == 0) {
			/*
			 * Check for a duplicate after us on the page.  NOTE:
			 * we have to delete the key item before deleting the
			 * data item, otherwise the "indx + P_INDX" calculation
			 * won't work!
			 */
			if (indx + P_INDX < (u_int32_t)NUM_ENT(h) &&
			    inp[indx] == inp[indx + P_INDX])
				return (__bam_adjindx(dbc,
				    h, indx, indx + O_INDX, 0));
			/*
			 * Check for a duplicate before us on the page.  It
			 * doesn't matter if we delete the key item before or
			 * after the data item for the purposes of this one.
			 */
			if (indx > 0 && inp[indx] == inp[indx - P_INDX])
				return (__bam_adjindx(dbc,
				    h, indx, indx - P_INDX, 0));
		}
		/* FALLTHROUGH */
	case P_LDUP:
	case P_LRECNO:
		bk = GET_BKEYDATA(dbp, h, indx);
		switch (B_TYPE(bk->type)) {
		case B_DUPLICATE:
			nbytes = BOVERFLOW_SIZE;
			break;
		case B_OVERFLOW:
			nbytes = BOVERFLOW_SIZE;
			if ((ret = __db_doff(
			    dbc, (GET_BOVERFLOW(dbp, h, indx))->pgno)) != 0)
				return (ret);
			break;
		case B_KEYDATA:
			nbytes = BKEYDATA_SIZE(bk->len);
			break;
		default:
			return (__db_pgfmt(dbp->env, PGNO(h)));
		}
		break;
	default:
		return (__db_pgfmt(dbp->env, PGNO(h)));
	}

	/* Delete the item and mark the page dirty. */
	if ((ret = __db_ditem(dbc, h, indx, nbytes)) != 0)
		return (ret);

	return (0);
}

/*
 * __bam_adjindx --
 *	Adjust an index on the page.
 *
 * PUBLIC: int __bam_adjindx __P((DBC *, PAGE *, u_int32_t, u_int32_t, int));
 */
int
__bam_adjindx(dbc, h, indx, indx_copy, is_insert)
	DBC *dbc;
	PAGE *h;
	u_int32_t indx, indx_copy;
	int is_insert;
{
	DB *dbp;
	db_indx_t copy, *inp;
	int ret;

	dbp = dbc->dbp;
	inp = P_INP(dbp, h);

	/* Log the change. */
	if (DBC_LOGGING(dbc)) {
	    if ((ret = __bam_adj_log(dbp, dbc->txn, &LSN(h), 0,
		PGNO(h), &LSN(h), indx, indx_copy, (u_int32_t)is_insert)) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(LSN(h));

	/* Shuffle the indices and mark the page dirty. */
	if (is_insert) {
		copy = inp[indx_copy];
		if (indx != NUM_ENT(h))
			memmove(&inp[indx + O_INDX], &inp[indx],
			    sizeof(db_indx_t) * (NUM_ENT(h) - indx));
		inp[indx] = copy;
		++NUM_ENT(h);
	} else {
		--NUM_ENT(h);
		if (indx != NUM_ENT(h))
			memmove(&inp[indx], &inp[indx + O_INDX],
			    sizeof(db_indx_t) * (NUM_ENT(h) - indx));
	}

	return (0);
}

/*
 * __bam_dpages --
 *	Delete a set of locked pages.
 *
 * PUBLIC: int __bam_dpages __P((DBC *, int, int));
 */
int
__bam_dpages(dbc, use_top, flags)
	DBC *dbc;
	int use_top;
	int flags;
{
	BINTERNAL *bi;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT a, b;
	DB_LOCK c_lock, p_lock;
	DB_MPOOLFILE *mpf;
	EPG *epg, *save_sp, *stack_epg;
	PAGE *child, *parent;
	db_indx_t nitems;
	db_pgno_t pgno, root_pgno;
	db_recno_t rcnt;
	int done, ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	nitems = 0;
	pgno = PGNO_INVALID;

	/*
	 * We have the entire stack of deletable pages locked.
	 *
	 * Btree calls us with the first page in the stack is to have a
	 * single item deleted, and the rest of the pages are to be removed.
	 *
	 * Recno always has a stack to the root and __bam_merge operations
	 * may have unneeded items in the sack.  We find the lowest page
	 * in the stack that has more than one record in it and start there.
	 */
	ret = 0;
	if (use_top)
		stack_epg = cp->sp;
	else
		for (stack_epg = cp->csp; stack_epg > cp->sp; --stack_epg)
			if (NUM_ENT(stack_epg->page) > 1)
				break;
	epg = stack_epg;
	/*
	 * !!!
	 * There is an interesting deadlock situation here.  We have to relink
	 * the leaf page chain around the leaf page being deleted.  Consider
	 * a cursor walking through the leaf pages, that has the previous page
	 * read-locked and is waiting on a lock for the page we're deleting.
	 * It will deadlock here.  Before we unlink the subtree, we relink the
	 * leaf page chain.
	 */
	if (LF_ISSET(BTD_RELINK) && LEVEL(cp->csp->page) == 1 &&
	    (ret = __bam_relink(dbc, cp->csp->page, NULL, PGNO_INVALID)) != 0)
		goto discard;

	/*
	 * Delete the last item that references the underlying pages that are
	 * to be deleted, and adjust cursors that reference that page.  Then,
	 * save that page's page number and item count and release it.  If
	 * the application isn't retaining locks because it's running without
	 * transactions, this lets the rest of the tree get back to business
	 * immediately.
	 */
	if ((ret = __memp_dirty(mpf,
	    &epg->page, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
		goto discard;
	if ((ret = __bam_ditem(dbc, epg->page, epg->indx)) != 0)
		goto discard;
	if ((ret = __bam_ca_di(dbc, PGNO(epg->page), epg->indx, -1)) != 0)
		goto discard;

	if (LF_ISSET(BTD_UPDATE) && epg->indx == 0) {
		save_sp = cp->csp;
		cp->csp = epg;
		ret = __bam_pupdate(dbc, epg->page);
		cp->csp = save_sp;
		if (ret != 0)
			goto discard;
	}

	pgno = PGNO(epg->page);
	nitems = NUM_ENT(epg->page);

	ret = __memp_fput(mpf, dbc->thread_info, epg->page, dbc->priority);
	epg->page = NULL;
	if ((t_ret = __TLPUT(dbc, epg->lock)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		goto err_inc;

	/* Then, discard any pages that we don't care about. */
discard: for (epg = cp->sp; epg < stack_epg; ++epg) {
		if ((t_ret = __memp_fput(mpf, dbc->thread_info,
		     epg->page, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		epg->page = NULL;
		if ((t_ret = __TLPUT(dbc, epg->lock)) != 0 && ret == 0)
			ret = t_ret;
	}
	if (ret != 0)
		goto err;

	/* Free the rest of the pages in the stack. */
	while (++epg <= cp->csp) {
		if ((ret = __memp_dirty(mpf, &epg->page,
		    dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
			goto err;
		/*
		 * Delete page entries so they will be restored as part of
		 * recovery.  We don't need to do cursor adjustment here as
		 * the pages are being emptied by definition and so cannot
		 * be referenced by a cursor.
		 */
		if (NUM_ENT(epg->page) != 0) {
			DB_ASSERT(dbp->env, LEVEL(epg->page) != 1);

			if ((ret = __bam_ditem(dbc, epg->page, epg->indx)) != 0)
				goto err;
			/*
			 * Sheer paranoia: if we find any pages that aren't
			 * emptied by the delete, someone else added an item
			 * while we were walking the tree, and we discontinue
			 * the delete.  Shouldn't be possible, but we check
			 * regardless.
			 */
			if (NUM_ENT(epg->page) != 0)
				goto err;
		}

		ret = __db_free(dbc, epg->page);
		if (cp->page == epg->page)
			cp->page = NULL;
		epg->page = NULL;
		if ((t_ret = __TLPUT(dbc, epg->lock)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err_inc;
	}

	if (0) {
err_inc:	++epg;
err:		for (; epg <= cp->csp; ++epg) {
			if (epg->page != NULL) {
				(void)__memp_fput(mpf, dbc->thread_info,
				     epg->page, dbc->priority);
				epg->page = NULL;
			}
			(void)__TLPUT(dbc, epg->lock);
		}
		BT_STK_CLR(cp);
		return (ret);
	}
	BT_STK_CLR(cp);

	/*
	 * If we just deleted the next-to-last item from the root page, the
	 * tree can collapse one or more levels.  While there remains only a
	 * single item on the root page, write lock the last page referenced
	 * by the root page and copy it over the root page.
	 */
	root_pgno = cp->root;
	if (pgno != root_pgno || nitems != 1)
		return (0);

	for (done = 0; !done;) {
		/* Initialize. */
		parent = child = NULL;
		LOCK_INIT(p_lock);
		LOCK_INIT(c_lock);

		/* Lock the root. */
		pgno = root_pgno;
		if ((ret =
		    __db_lget(dbc, 0, pgno, DB_LOCK_WRITE, 0, &p_lock)) != 0)
			goto stop;
		if ((ret = __memp_fget(mpf, &pgno, dbc->thread_info, dbc->txn,
		    DB_MPOOL_DIRTY, &parent)) != 0)
			goto stop;

		if (NUM_ENT(parent) != 1)
			goto stop;

		switch (TYPE(parent)) {
		case P_IBTREE:
			/*
			 * If this is overflow, then try to delete it.
			 * The child may or may not still point at it.
			 */
			bi = GET_BINTERNAL(dbp, parent, 0);
			if (B_TYPE(bi->type) == B_OVERFLOW)
				if ((ret = __db_doff(dbc,
				    ((BOVERFLOW *)bi->data)->pgno)) != 0)
					goto stop;
			pgno = bi->pgno;
			break;
		case P_IRECNO:
			pgno = GET_RINTERNAL(dbp, parent, 0)->pgno;
			break;
		default:
			goto stop;
		}

		/* Lock the child page. */
		if ((ret =
		    __db_lget(dbc, 0, pgno, DB_LOCK_WRITE, 0, &c_lock)) != 0)
			goto stop;
		if ((ret = __memp_fget(mpf, &pgno, dbc->thread_info, dbc->txn,
		    DB_MPOOL_DIRTY, &child)) != 0)
			goto stop;

		/* Log the change. */
		if (DBC_LOGGING(dbc)) {
			memset(&a, 0, sizeof(a));
			a.data = child;
			a.size = dbp->pgsize;
			memset(&b, 0, sizeof(b));
			b.data = P_ENTRY(dbp, parent, 0);
			b.size = TYPE(parent) == P_IRECNO ? RINTERNAL_SIZE :
			    BINTERNAL_SIZE(((BINTERNAL *)b.data)->len);
			if ((ret = __bam_rsplit_log(dbp, dbc->txn,
			    &child->lsn, 0, PGNO(child), &a, PGNO(parent),
			    RE_NREC(parent), &b, &parent->lsn)) != 0)
				goto stop;
		} else
			LSN_NOT_LOGGED(child->lsn);

		/*
		 * Make the switch.
		 *
		 * One fixup -- internal pages below the top level do not store
		 * a record count, so we have to preserve it if we're not
		 * converting to a leaf page.  Note also that we are about to
		 * overwrite the parent page, including its LSN.  This is OK
		 * because the log message we wrote describing this update
		 * stores its LSN on the child page.  When the child is copied
		 * onto the parent, the correct LSN is copied into place.
		 */
		COMPQUIET(rcnt, 0);
		if (F_ISSET(cp, C_RECNUM) && LEVEL(child) > LEAFLEVEL)
			rcnt = RE_NREC(parent);
		memcpy(parent, child, dbp->pgsize);
		PGNO(parent) = root_pgno;
		if (F_ISSET(cp, C_RECNUM) && LEVEL(child) > LEAFLEVEL)
			RE_NREC_SET(parent, rcnt);

		/* Adjust the cursors. */
		if ((ret = __bam_ca_rsplit(dbc, PGNO(child), root_pgno)) != 0)
			goto stop;

		/*
		 * Free the page copied onto the root page and discard its
		 * lock.  (The call to __db_free() discards our reference
		 * to the page.)
		 */
		if ((ret = __db_free(dbc, child)) != 0) {
			child = NULL;
			goto stop;
		}
		child = NULL;

		if (0) {
stop:			done = 1;
		}
		if ((t_ret = __TLPUT(dbc, p_lock)) != 0 && ret == 0)
			ret = t_ret;
		if (parent != NULL &&
		    (t_ret = __memp_fput(mpf, dbc->thread_info,
		    parent, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
		if ((t_ret = __TLPUT(dbc, c_lock)) != 0 && ret == 0)
			ret = t_ret;
		if (child != NULL &&
		    (t_ret = __memp_fput(mpf, dbc->thread_info,
		    child, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
	}

	return (ret);
}

/*
 * __bam_relink --
 *	Relink around a deleted page.
 *
 * PUBLIC: int __bam_relink __P((DBC *, PAGE *, PAGE *, db_pgno_t));
 *	Otherp can be either the previous or the next page to use if
 * the caller already holds that page.
 */
int
__bam_relink(dbc, pagep, otherp, new_pgno)
	DBC *dbc;
	PAGE *pagep, *otherp;
	db_pgno_t new_pgno;
{
	DB *dbp;
	DB_LOCK npl, ppl;
	DB_LSN *nlsnp, *plsnp, ret_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *np, *pp;
	int ret, t_ret;

	dbp = dbc->dbp;
	np = pp = NULL;
	LOCK_INIT(npl);
	LOCK_INIT(ppl);
	nlsnp = plsnp = NULL;
	mpf = dbp->mpf;
	ret = 0;

	/*
	 * Retrieve the one/two pages.  The caller must have them locked
	 * because the parent is latched. For a remove, we may need
	 * two pages (the before and after).  For an add, we only need one
	 * because, the split took care of the prev.
	 */
	if (pagep->next_pgno != PGNO_INVALID) {
		if (((np = otherp) == NULL ||
		    PGNO(otherp) != pagep->next_pgno) &&
		    (ret = __memp_fget(mpf, &pagep->next_pgno,
		    dbc->thread_info, dbc->txn, DB_MPOOL_DIRTY, &np)) != 0) {
			ret = __db_pgerr(dbp, pagep->next_pgno, ret);
			goto err;
		}
		nlsnp = &np->lsn;
	}
	if (pagep->prev_pgno != PGNO_INVALID) {
		if (((pp = otherp) == NULL ||
		    PGNO(otherp) != pagep->prev_pgno) &&
		    (ret = __memp_fget(mpf, &pagep->prev_pgno,
		    dbc->thread_info, dbc->txn, DB_MPOOL_DIRTY, &pp)) != 0) {
			ret = __db_pgerr(dbp, pagep->prev_pgno, ret);
			goto err;
		}
		plsnp = &pp->lsn;
	}

	/* Log the change. */
	if (DBC_LOGGING(dbc)) {
		if ((ret = __bam_relink_log(dbp, dbc->txn, &ret_lsn, 0,
		    pagep->pgno, new_pgno, pagep->prev_pgno, plsnp,
		    pagep->next_pgno, nlsnp)) != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(ret_lsn);
	if (np != NULL)
		np->lsn = ret_lsn;
	if (pp != NULL)
		pp->lsn = ret_lsn;

	/*
	 * Modify and release the two pages.
	 */
	if (np != NULL) {
		if (new_pgno == PGNO_INVALID)
			np->prev_pgno = pagep->prev_pgno;
		else
			np->prev_pgno = new_pgno;
		if (np != otherp)
			ret = __memp_fput(mpf,
			    dbc->thread_info, np, dbc->priority);
		if ((t_ret = __TLPUT(dbc, npl)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;
	}

	if (pp != NULL) {
		if (new_pgno == PGNO_INVALID)
			pp->next_pgno = pagep->next_pgno;
		else
			pp->next_pgno = new_pgno;
		if (pp != otherp)
			ret = __memp_fput(mpf,
			    dbc->thread_info, pp, dbc->priority);
		if ((t_ret = __TLPUT(dbc, ppl)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;
	}
	return (0);

err:	if (np != NULL && np != otherp)
		(void)__memp_fput(mpf, dbc->thread_info, np, dbc->priority);
	if (pp != NULL && pp != otherp)
		(void)__memp_fput(mpf, dbc->thread_info, pp, dbc->priority);
	return (ret);
}

/*
 * __bam_pupdate --
 *	Update parent key pointers up the tree.
 *
 * PUBLIC: int __bam_pupdate __P((DBC *, PAGE *));
 */
int
__bam_pupdate(dbc, lpg)
	DBC *dbc;
	PAGE *lpg;
{
	BTREE_CURSOR *cp;
	ENV *env;
	EPG *epg;
	int ret;

	env = dbc->env;
	cp = (BTREE_CURSOR *)dbc->internal;
	ret = 0;

	/*
	 * Update the parents up the tree.  __bam_pinsert only looks at the
	 * left child if is a leaf page, so we don't need to change it.  We
	 * just do a delete and insert; a replace is possible but reusing
	 * pinsert is better.
	 */
	for (epg = &cp->csp[-1]; epg >= cp->sp; epg--) {
		if ((ret = __memp_dirty(dbc->dbp->mpf, &epg->page,
		    dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
			return (ret);
		epg->indx--;
		if ((ret = __bam_pinsert(dbc, epg, 0,
		    lpg, epg[1].page, BPI_NORECNUM | BPI_REPLACE)) != 0) {
			if (ret == DB_NEEDSPLIT) {
				/* This should not happen. */
				__db_errx(env,
				     "Not enough room in parent: %s: page %lu",
				     dbc->dbp->fname, (u_long)PGNO(epg->page));
				ret = __env_panic(env, EINVAL);
			}
			epg->indx++;
			return (ret);
		}
		epg->indx++;
	}
	return (ret);
}
