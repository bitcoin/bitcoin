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
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/btree.h"

static int __bam_page __P((DBC *, EPG *, EPG *));
static int __bam_psplit __P((DBC *, EPG *, PAGE *, PAGE *, db_indx_t *));
static int __bam_root __P((DBC *, EPG *));

/*
 * __bam_split --
 *	Split a page.
 *
 * PUBLIC: int __bam_split __P((DBC *, void *, db_pgno_t *));
 */
int
__bam_split(dbc, arg, root_pgnop)
	DBC *dbc;
	void *arg;
	db_pgno_t *root_pgnop;
{
	BTREE_CURSOR *cp;
	DB_LOCK metalock, next_lock;
	enum { UP, DOWN } dir;
	db_pgno_t pgno, next_pgno, root_pgno;
	int exact, level, ret;

	cp = (BTREE_CURSOR *)dbc->internal;
	root_pgno = cp->root;
	LOCK_INIT(next_lock);
	next_pgno = PGNO_INVALID;

	/*
	 * First get a lock on the metadata page, we will have to allocate
	 * pages and cannot get a lock while we have the search tree pinnned.
	 */

	pgno = PGNO_BASE_MD;
	if ((ret = __db_lget(dbc,
	    0, pgno, DB_LOCK_WRITE, 0, &metalock)) != 0)
		goto err;

	/*
	 * The locking protocol we use to avoid deadlock to acquire locks by
	 * walking down the tree, but we do it as lazily as possible, locking
	 * the root only as a last resort.  We expect all stack pages to have
	 * been discarded before we're called; we discard all short-term locks.
	 *
	 * When __bam_split is first called, we know that a leaf page was too
	 * full for an insert.  We don't know what leaf page it was, but we
	 * have the key/recno that caused the problem.  We call XX_search to
	 * reacquire the leaf page, but this time get both the leaf page and
	 * its parent, locked.  We then split the leaf page and see if the new
	 * internal key will fit into the parent page.  If it will, we're done.
	 *
	 * If it won't, we discard our current locks and repeat the process,
	 * only this time acquiring the parent page and its parent, locked.
	 * This process repeats until we succeed in the split, splitting the
	 * root page as the final resort.  The entire process then repeats,
	 * as necessary, until we split a leaf page.
	 *
	 * XXX
	 * A traditional method of speeding this up is to maintain a stack of
	 * the pages traversed in the original search.  You can detect if the
	 * stack is correct by storing the page's LSN when it was searched and
	 * comparing that LSN with the current one when it's locked during the
	 * split.  This would be an easy change for this code, but I have no
	 * numbers that indicate it's worthwhile.
	 */
	for (dir = UP, level = LEAFLEVEL;; dir == UP ? ++level : --level) {
		/*
		 * Acquire a page and its parent, locked.
		 */
retry:		if ((ret = (dbc->dbtype == DB_BTREE ?
		    __bam_search(dbc, PGNO_INVALID,
			arg, SR_WRPAIR, level, NULL, &exact) :
		    __bam_rsearch(dbc,
			(db_recno_t *)arg, SR_WRPAIR, level, &exact))) != 0)
			break;

		if (cp->csp[0].page->pgno == root_pgno) {
			/* we can overshoot the top of the tree. */
			level = cp->csp[0].page->level;
			if (root_pgnop != NULL)
				*root_pgnop = root_pgno;
		} else if (root_pgnop != NULL)
			*root_pgnop = cp->csp[-1].page->pgno;

		/*
		 * Split the page if it still needs it (it's possible another
		 * thread of control has already split the page).  If we are
		 * guaranteed that two items will fit on the page, the split
		 * is no longer necessary.
		 */
		if (2 * B_MAXSIZEONPAGE(cp->ovflsize)
		    <= (db_indx_t)P_FREESPACE(dbc->dbp, cp->csp[0].page)) {
			if ((ret = __bam_stkrel(dbc, STK_NOLOCK)) != 0)
				goto err;
			goto no_split;
		}

		/*
		 * We need to try to lock the next page so we can update
		 * its PREV.
		 */
		if (dbc->dbtype == DB_BTREE && ISLEAF(cp->csp->page) &&
		    (pgno = NEXT_PGNO(cp->csp->page)) != PGNO_INVALID) {
			TRY_LOCK(dbc, pgno,
			     next_pgno, next_lock, DB_LOCK_WRITE, retry);
			if (ret != 0)
				goto err;
		}
		ret = cp->csp[0].page->pgno == root_pgno ?
		    __bam_root(dbc, &cp->csp[0]) :
		    __bam_page(dbc, &cp->csp[-1], &cp->csp[0]);
		BT_STK_CLR(cp);

		switch (ret) {
		case 0:
no_split:		/* Once we've split the leaf page, we're done. */
			if (level == LEAFLEVEL)
				goto done;

			/* Switch directions. */
			if (dir == UP)
				dir = DOWN;
			break;
		case DB_NEEDSPLIT:
			/*
			 * It's possible to fail to split repeatedly, as other
			 * threads may be modifying the tree, or the page usage
			 * is sufficiently bad that we don't get enough space
			 * the first time.
			 */
			if (dir == DOWN)
				dir = UP;
			break;
		default:
			goto err;
		}
	}

err:	if (root_pgnop != NULL)
		*root_pgnop = cp->root;
done:	(void)__LPUT(dbc, metalock);
	(void)__TLPUT(dbc, next_lock);
	return (ret);
}

/*
 * __bam_root --
 *	Split the root page of a btree.
 */
static int
__bam_root(dbc, cp)
	DBC *dbc;
	EPG *cp;
{
	DB *dbp;
	DBT log_dbt, rootent[2];
	DB_LOCK llock, rlock;
	DB_LSN log_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *lp, *rp;
	db_indx_t split;
	u_int32_t opflags;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	lp = rp = NULL;
	LOCK_INIT(llock);
	LOCK_INIT(rlock);
	COMPQUIET(log_dbt.data, NULL);

	/* Yeah, right. */
	if (cp->page->level >= MAXBTREELEVEL) {
		__db_errx(dbp->env,
		    "Too many btree levels: %d", cp->page->level);
		return (ENOSPC);
	}

	if ((ret = __memp_dirty(mpf,
	    &cp->page, dbc->thread_info, dbc->txn, dbc->priority, 0)) != 0)
		goto err;

	/* Create new left and right pages for the split. */
	if ((ret = __db_new(dbc, TYPE(cp->page), &llock, &lp)) != 0 ||
	    (ret = __db_new(dbc, TYPE(cp->page), &rlock, &rp)) != 0)
		goto err;
	P_INIT(lp, dbp->pgsize, lp->pgno,
	    PGNO_INVALID, ISINTERNAL(cp->page) ? PGNO_INVALID : rp->pgno,
	    cp->page->level, TYPE(cp->page));
	P_INIT(rp, dbp->pgsize, rp->pgno,
	    ISINTERNAL(cp->page) ?  PGNO_INVALID : lp->pgno, PGNO_INVALID,
	    cp->page->level, TYPE(cp->page));

	/* Split the page. */
	if ((ret = __bam_psplit(dbc, cp, lp, rp, &split)) != 0)
		goto err;

	if (DBC_LOGGING(dbc)) {
		memset(&log_dbt, 0, sizeof(log_dbt));
		if ((ret =
		    __os_malloc(dbp->env, dbp->pgsize, &log_dbt.data)) != 0)
			goto err;
		log_dbt.size = dbp->pgsize;
		memcpy(log_dbt.data, cp->page, dbp->pgsize);
	}

	/* Clean up the new root page. */
	if ((ret = (dbc->dbtype == DB_RECNO ?
	    __ram_root(dbc, cp->page, lp, rp) :
	    __bam_broot(dbc, cp->page, split, lp, rp))) != 0) {
		if (DBC_LOGGING(dbc))
			__os_free(dbp->env, log_dbt.data);
		goto err;
	}

	/* Log the change. */
	if (DBC_LOGGING(dbc)) {
		memset(rootent, 0, sizeof(rootent));
		rootent[0].data = GET_BINTERNAL(dbp, cp->page, 0);
		rootent[1].data = GET_BINTERNAL(dbp, cp->page, 1);
		if (dbc->dbtype == DB_RECNO)
			rootent[0].size = rootent[1].size = RINTERNAL_SIZE;
		else {
			rootent[0].size = BINTERNAL_SIZE(
			    ((BINTERNAL *)rootent[0].data)->len);
			rootent[1].size = BINTERNAL_SIZE(
			    ((BINTERNAL *)rootent[1].data)->len);
		}
		ZERO_LSN(log_lsn);
		opflags = F_ISSET(
		    (BTREE_CURSOR *)dbc->internal, C_RECNUM) ? SPL_NRECS : 0;
		if (dbc->dbtype == DB_RECNO)
			opflags |= SPL_RECNO;
		ret = __bam_split_log(dbp,
		    dbc->txn, &LSN(cp->page), 0, PGNO(lp), &LSN(lp), PGNO(rp),
		    &LSN(rp), (u_int32_t)NUM_ENT(lp), PGNO_INVALID, &log_lsn,
		    dbc->internal->root, &LSN(cp->page), 0,
		    &log_dbt, &rootent[0], &rootent[1], opflags);

		__os_free(dbp->env, log_dbt.data);

		if (ret != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(LSN(cp->page));
	LSN(lp) = LSN(cp->page);
	LSN(rp) = LSN(cp->page);

	/* Adjust any cursors. */
	ret = __bam_ca_split(dbc, cp->page->pgno, lp->pgno, rp->pgno, split, 1);

	/* Success or error: release pages and locks. */
err:	if (cp->page != NULL && (t_ret = __memp_fput(mpf,
	     dbc->thread_info, cp->page, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	cp->page = NULL;

	/*
	 * We are done.  Put or downgrade all our locks and release
	 * the pages.
	 */
	if ((t_ret = __TLPUT(dbc, llock)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __TLPUT(dbc, rlock)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __TLPUT(dbc, cp->lock)) != 0 && ret == 0)
		ret = t_ret;
	if (lp != NULL && (t_ret = __memp_fput(mpf,
	     dbc->thread_info, lp, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (rp != NULL && (t_ret = __memp_fput(mpf,
	     dbc->thread_info, rp, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __bam_page --
 *	Split the non-root page of a btree.
 */
static int
__bam_page(dbc, pp, cp)
	DBC *dbc;
	EPG *pp, *cp;
{
	BTREE_CURSOR *bc;
	DB *dbp;
	DBT log_dbt, rentry;
	DB_LOCK rplock;
	DB_LSN log_lsn;
	DB_LSN save_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *lp, *rp, *alloc_rp, *tp;
	db_indx_t split;
	u_int32_t opflags;
	int ret, t_ret;

	dbp = dbc->dbp;
	mpf = dbp->mpf;
	alloc_rp = lp = rp = tp = NULL;
	LOCK_INIT(rplock);
	ret = -1;

	/*
	 * Create new left page for the split, and fill in everything
	 * except its LSN and next-page page number.
	 *
	 * Create a new right page for the split, and fill in everything
	 * except its LSN and page number.
	 *
	 * We malloc space for both the left and right pages, so we don't get
	 * a new page from the underlying buffer pool until we know the split
	 * is going to succeed.  The reason is that we can't release locks
	 * acquired during the get-a-new-page process because metadata page
	 * locks can't be discarded on failure since we may have modified the
	 * free list.  So, if you assume that we're holding a write lock on the
	 * leaf page which ran out of space and started this split (e.g., we
	 * have already written records to the page, or we retrieved a record
	 * from it with the DB_RMW flag set), failing in a split with both a
	 * leaf page locked and the metadata page locked can potentially lock
	 * up the tree badly, because we've violated the rule of always locking
	 * down the tree, and never up.
	 */
	if ((ret = __os_malloc(dbp->env, dbp->pgsize * 2, &lp)) != 0)
		goto err;
	P_INIT(lp, dbp->pgsize, PGNO(cp->page),
	    ISINTERNAL(cp->page) ?  PGNO_INVALID : PREV_PGNO(cp->page),
	    ISINTERNAL(cp->page) ?  PGNO_INVALID : 0,
	    cp->page->level, TYPE(cp->page));

	rp = (PAGE *)((u_int8_t *)lp + dbp->pgsize);
	P_INIT(rp, dbp->pgsize, 0,
	    ISINTERNAL(cp->page) ? PGNO_INVALID : PGNO(cp->page),
	    ISINTERNAL(cp->page) ? PGNO_INVALID : NEXT_PGNO(cp->page),
	    cp->page->level, TYPE(cp->page));

	/*
	 * Split right.
	 *
	 * Only the indices are sorted on the page, i.e., the key/data pairs
	 * aren't, so it's simpler to copy the data from the split page onto
	 * two new pages instead of copying half the data to a new right page
	 * and compacting the left page in place.  Since the left page can't
	 * change, we swap the original and the allocated left page after the
	 * split.
	 */
	if ((ret = __bam_psplit(dbc, cp, lp, rp, &split)) != 0)
		goto err;

	/*
	 * Test to see if we are going to be able to insert the new pages into
	 * the parent page.  The interesting failure here is that the parent
	 * page can't hold the new keys, and has to be split in turn, in which
	 * case we want to release all the locks we can.
	 */
	if ((ret = __bam_pinsert(dbc, pp, split, lp, rp, BPI_SPACEONLY)) != 0)
		goto err;

	/*
	 * We've got everything locked down we need, and we know the split
	 * is going to succeed.  Go and get the additional page we'll need.
	 */
	if ((ret = __db_new(dbc, TYPE(cp->page), &rplock, &alloc_rp)) != 0)
		goto err;

	/*
	 * Prepare to fix up the previous pointer of any leaf page following
	 * the split page.  Our caller has already write locked the page so
	 * we can get it without deadlocking on the parent latch.
	 */
	if (ISLEAF(cp->page) && NEXT_PGNO(cp->page) != PGNO_INVALID &&
	    (ret = __memp_fget(mpf, &NEXT_PGNO(cp->page),
	    dbc->thread_info, dbc->txn, DB_MPOOL_DIRTY, &tp)) != 0)
		goto err;

	/*
	 * Fix up the page numbers we didn't have before.  We have to do this
	 * before calling __bam_pinsert because it may copy a page number onto
	 * the parent page and it takes the page number from its page argument.
	 */
	PGNO(rp) = NEXT_PGNO(lp) = PGNO(alloc_rp);

	DB_ASSERT(dbp->env, IS_DIRTY(cp->page));
	DB_ASSERT(dbp->env, IS_DIRTY(pp->page));

	/* Actually update the parent page. */
	if ((ret = __bam_pinsert(dbc, pp, split, lp, rp, BPI_NOLOGGING)) != 0)
		goto err;

	bc = (BTREE_CURSOR *)dbc->internal;
	/* Log the change. */
	if (DBC_LOGGING(dbc)) {
		memset(&log_dbt, 0, sizeof(log_dbt));
		log_dbt.data = cp->page;
		log_dbt.size = dbp->pgsize;
		memset(&rentry, 0, sizeof(rentry));
		rentry.data = GET_BINTERNAL(dbp, pp->page, pp->indx + 1);
		opflags = F_ISSET(bc, C_RECNUM) ? SPL_NRECS : 0;
		if (dbc->dbtype == DB_RECNO) {
			opflags |= SPL_RECNO;
			rentry.size = RINTERNAL_SIZE;
		} else
			rentry.size =
			    BINTERNAL_SIZE(((BINTERNAL *)rentry.data)->len);
		if (tp == NULL)
			ZERO_LSN(log_lsn);
		if ((ret = __bam_split_log(dbp, dbc->txn, &LSN(cp->page), 0,
		    PGNO(cp->page), &LSN(cp->page), PGNO(alloc_rp),
		    &LSN(alloc_rp), (u_int32_t)NUM_ENT(lp),
		    tp == NULL ? 0 : PGNO(tp), tp == NULL ? &log_lsn : &LSN(tp),
		    PGNO(pp->page), &LSN(pp->page), pp->indx,
		    &log_dbt, NULL, &rentry, opflags)) != 0) {
			/*
			 * Undo the update to the parent page, which has not
			 * been logged yet. This must succeed.
			 */
			t_ret = __db_ditem_nolog(dbc, pp->page,
			    pp->indx + 1, rentry.size);
			DB_ASSERT(dbp->env, t_ret == 0);

			goto err;
		}

	} else
		LSN_NOT_LOGGED(LSN(cp->page));

	/* Update the LSNs for all involved pages. */
	LSN(alloc_rp) = LSN(cp->page);
	LSN(lp) = LSN(cp->page);
	LSN(rp) = LSN(cp->page);
	LSN(pp->page) = LSN(cp->page);
	if (tp != NULL) {
		/* Log record has been written; now it is safe to update next page. */
		PREV_PGNO(tp) = PGNO(rp);
		LSN(tp) = LSN(cp->page);
	}

	/*
	 * Copy the left and right pages into place.  There are two paths
	 * through here.  Either we are logging and we set the LSNs in the
	 * logging path.  However, if we are not logging, then we do not
	 * have valid LSNs on lp or rp.  The correct LSNs to use are the
	 * ones on the page we got from __db_new or the one that was
	 * originally on cp->page.  In both cases, we save the LSN from the
	 * real database page (not a malloc'd one) and reapply it after we
	 * do the copy.
	 */
	save_lsn = alloc_rp->lsn;
	memcpy(alloc_rp, rp, LOFFSET(dbp, rp));
	memcpy((u_int8_t *)alloc_rp + HOFFSET(rp),
	    (u_int8_t *)rp + HOFFSET(rp), dbp->pgsize - HOFFSET(rp));
	alloc_rp->lsn = save_lsn;

	save_lsn = cp->page->lsn;
	memcpy(cp->page, lp, LOFFSET(dbp, lp));
	memcpy((u_int8_t *)cp->page + HOFFSET(lp),
	    (u_int8_t *)lp + HOFFSET(lp), dbp->pgsize - HOFFSET(lp));
	cp->page->lsn = save_lsn;

	/* Adjust any cursors. */
	if ((ret = __bam_ca_split(dbc,
	    PGNO(cp->page), PGNO(cp->page), PGNO(rp), split, 0)) != 0)
		goto err;

	__os_free(dbp->env, lp);

	/*
	 * Success -- write the real pages back to the store.
	 */
	if ((t_ret = __memp_fput(mpf,
	    dbc->thread_info, alloc_rp, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __TLPUT(dbc, rplock)) != 0 && ret == 0)
		ret = t_ret;
	if (tp != NULL) {
		if ((t_ret = __memp_fput(mpf,
		    dbc->thread_info, tp, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
	}
	if ((t_ret = __bam_stkrel(dbc, STK_CLRDBC)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);

err:	if (lp != NULL)
		__os_free(dbp->env, lp);
	if (alloc_rp != NULL)
		(void)__memp_fput(mpf,
		    dbc->thread_info, alloc_rp, dbc->priority);
	if (tp != NULL)
		(void)__memp_fput(mpf, dbc->thread_info, tp, dbc->priority);

	if (pp->page != NULL)
		(void)__memp_fput(mpf,
		     dbc->thread_info, pp->page, dbc->priority);

	if (ret == DB_NEEDSPLIT)
		(void)__LPUT(dbc, pp->lock);
	else
		(void)__TLPUT(dbc, pp->lock);

	(void)__memp_fput(mpf, dbc->thread_info, cp->page, dbc->priority);

	/*
	 * We don't drop the left and right page locks.  If we doing dirty
	 * reads then we need to hold the locks until we abort the transaction.
	 * If we are not transactional, we are hosed anyway as the tree
	 * is trashed.  It may be better not to leak the locks.
	 */

	if (dbc->txn == NULL)
		(void)__LPUT(dbc, rplock);

	if (dbc->txn == NULL || ret == DB_NEEDSPLIT)
		(void)__LPUT(dbc, cp->lock);

	return (ret);
}

/*
 * __bam_broot --
 *	Fix up the btree root page after it has been split.
 * PUBLIC: int __bam_broot __P((DBC *, PAGE *, u_int32_t, PAGE *, PAGE *));
 */
int
__bam_broot(dbc, rootp, split, lp, rp)
	DBC *dbc;
	u_int32_t split;
	PAGE *rootp, *lp, *rp;
{
	BINTERNAL bi, bi0, *child_bi;
	BKEYDATA *child_bk;
	BOVERFLOW bo, *child_bo;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT hdr, hdr0, data;
	db_pgno_t root_pgno;
	int ret;

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;
	child_bo = NULL;
	data.data = NULL;
	memset(&bi, 0, sizeof(bi));

	switch (TYPE(rootp)) {
	case P_IBTREE:
		/* Copy the first key of the child page onto the root page. */
		child_bi = GET_BINTERNAL(dbp, rootp, split);
		switch (B_TYPE(child_bi->type)) {
		case B_KEYDATA:
			bi.len = child_bi->len;
			B_TSET(bi.type, B_KEYDATA);
			bi.pgno = rp->pgno;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			if ((ret = __os_malloc(dbp->env,
			    child_bi->len, &data.data)) != 0)
				return (ret);
			memcpy(data.data, child_bi->data, child_bi->len);
			data.size = child_bi->len;
			break;
		case B_OVERFLOW:
			/* Reuse the overflow key. */
			child_bo = (BOVERFLOW *)child_bi->data;
			memset(&bo, 0, sizeof(bo));
			bo.type = B_OVERFLOW;
			bo.tlen = child_bo->tlen;
			bo.pgno = child_bo->pgno;
			bi.len = BOVERFLOW_SIZE;
			B_TSET(bi.type, B_OVERFLOW);
			bi.pgno = rp->pgno;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			DB_SET_DBT(data, &bo, BOVERFLOW_SIZE);
			break;
		case B_DUPLICATE:
		default:
			goto pgfmt;
		}
		break;
	case P_LDUP:
	case P_LBTREE:
		/* Copy the first key of the child page onto the root page. */
		child_bk = GET_BKEYDATA(dbp, rootp, split);
		switch (B_TYPE(child_bk->type)) {
		case B_KEYDATA:
			bi.len = child_bk->len;
			B_TSET(bi.type, B_KEYDATA);
			bi.pgno = rp->pgno;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			if ((ret = __os_malloc(dbp->env,
			     child_bk->len, &data.data)) != 0)
				return (ret);
			memcpy(data.data, child_bk->data, child_bk->len);
			data.size = child_bk->len;
			break;
		case B_OVERFLOW:
			/* Copy the overflow key. */
			child_bo = (BOVERFLOW *)child_bk;
			memset(&bo, 0, sizeof(bo));
			bo.type = B_OVERFLOW;
			bo.tlen = child_bo->tlen;
			memset(&hdr, 0, sizeof(hdr));
			if ((ret = __db_goff(dbc, &hdr, child_bo->tlen,
			     child_bo->pgno, &hdr.data, &hdr.size)) == 0)
				ret = __db_poff(dbc, &hdr, &bo.pgno);

			if (hdr.data != NULL)
				__os_free(dbp->env, hdr.data);
			if (ret != 0)
				return (ret);

			bi.len = BOVERFLOW_SIZE;
			B_TSET(bi.type, B_OVERFLOW);
			bi.pgno = rp->pgno;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			DB_SET_DBT(data, &bo, BOVERFLOW_SIZE);
			break;
		case B_DUPLICATE:
		default:
			goto pgfmt;
		}
		break;
	default:
pgfmt:		return (__db_pgfmt(dbp->env, rp->pgno));
	}
	/*
	 * If the root page was a leaf page, change it into an internal page.
	 * We copy the key we split on (but not the key's data, in the case of
	 * a leaf page) to the new root page.
	 */
	root_pgno = cp->root;
	P_INIT(rootp, dbp->pgsize,
	    root_pgno, PGNO_INVALID, PGNO_INVALID, lp->level + 1, P_IBTREE);

	/*
	 * The btree comparison code guarantees that the left-most key on any
	 * internal btree page is never used, so it doesn't need to be filled
	 * in.  Set the record count if necessary.
	 */
	memset(&bi0, 0, sizeof(bi0));
	B_TSET(bi0.type, B_KEYDATA);
	bi0.pgno = lp->pgno;
	if (F_ISSET(cp, C_RECNUM)) {
		bi0.nrecs = __bam_total(dbp, lp);
		RE_NREC_SET(rootp, bi0.nrecs);
		bi.nrecs = __bam_total(dbp, rp);
		RE_NREC_ADJ(rootp, bi.nrecs);
	}
	DB_SET_DBT(hdr0, &bi0, SSZA(BINTERNAL, data));
	if ((ret = __db_pitem_nolog(dbc, rootp,
	    0, BINTERNAL_SIZE(0), &hdr0, NULL)) != 0)
		goto err;
	ret = __db_pitem_nolog(dbc, rootp, 1,
	    BINTERNAL_SIZE(data.size), &hdr, &data);

err:	if (data.data != NULL && child_bo == NULL)
		__os_free(dbp->env, data.data);
	return (ret);
}

/*
 * __ram_root --
 *	Fix up the recno root page after it has been split.
 * PUBLIC:  int __ram_root __P((DBC *, PAGE *, PAGE *, PAGE *));
 */
int
__ram_root(dbc, rootp, lp, rp)
	DBC *dbc;
	PAGE *rootp, *lp, *rp;
{
	DB *dbp;
	DBT hdr;
	RINTERNAL ri;
	db_pgno_t root_pgno;
	int ret;

	dbp = dbc->dbp;
	root_pgno = dbc->internal->root;

	/* Initialize the page. */
	P_INIT(rootp, dbp->pgsize,
	    root_pgno, PGNO_INVALID, PGNO_INVALID, lp->level + 1, P_IRECNO);

	/* Initialize the header. */
	DB_SET_DBT(hdr, &ri, RINTERNAL_SIZE);

	/* Insert the left and right keys, set the header information. */
	ri.pgno = lp->pgno;
	ri.nrecs = __bam_total(dbp, lp);
	if ((ret = __db_pitem_nolog(dbc,
	     rootp, 0, RINTERNAL_SIZE, &hdr, NULL)) != 0)
		return (ret);
	RE_NREC_SET(rootp, ri.nrecs);
	ri.pgno = rp->pgno;
	ri.nrecs = __bam_total(dbp, rp);
	if ((ret = __db_pitem_nolog(dbc,
	    rootp, 1, RINTERNAL_SIZE, &hdr, NULL)) != 0)
		return (ret);
	RE_NREC_ADJ(rootp, ri.nrecs);
	return (0);
}

/*
 * __bam_pinsert --
 *	Insert a new key into a parent page, completing the split.
 *
 * PUBLIC: int __bam_pinsert
 * PUBLIC:     __P((DBC *, EPG *, u_int32_t, PAGE *, PAGE *, int));
 */
int
__bam_pinsert(dbc, parent, split, lchild, rchild, flags)
	DBC *dbc;
	EPG *parent;
	u_int32_t split;
	PAGE *lchild, *rchild;
	int flags;
{
	BINTERNAL bi, *child_bi;
	BKEYDATA *child_bk, *tmp_bk;
	BOVERFLOW bo, *child_bo;
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT a, b, hdr, data;
	EPG *child;
	PAGE *ppage;
	RINTERNAL ri;
	db_indx_t off;
	db_recno_t nrecs;
	size_t (*func) __P((DB *, const DBT *, const DBT *));
	int (*pitem) __P((DBC *, PAGE *, u_int32_t, u_int32_t, DBT *, DBT *));
	u_int32_t n, nbytes, nksize, oldsize, size;
	int ret;

	dbp = dbc->dbp;
	cp = (BTREE_CURSOR *)dbc->internal;
	t = dbp->bt_internal;
	ppage = parent->page;
	child = parent + 1;

	/* If handling record numbers, count records split to the right page. */
	nrecs = F_ISSET(cp, C_RECNUM) &&
	    !LF_ISSET(BPI_SPACEONLY) ? __bam_total(dbp, rchild) : 0;

	/*
	 * Now we insert the new page's first key into the parent page, which
	 * completes the split.  The parent points to a PAGE and a page index
	 * offset, where the new key goes ONE AFTER the index, because we split
	 * to the right.
	 *
	 * XXX
	 * Some btree algorithms replace the key for the old page as well as
	 * the new page.  We don't, as there's no reason to believe that the
	 * first key on the old page is any better than the key we have, and,
	 * in the case of a key being placed at index 0 causing the split, the
	 * key is unavailable.
	 */
	off = parent->indx + O_INDX;
	if (LF_ISSET(BPI_REPLACE))
		oldsize = TYPE(ppage) == P_IRECNO ? RINTERNAL_PSIZE :
		    BINTERNAL_PSIZE(GET_BINTERNAL(dbp, ppage, off)->len);
	else
		oldsize = 0;

	/*
	 * Calculate the space needed on the parent page.
	 *
	 * Prefix trees: space hack used when inserting into BINTERNAL pages.
	 * Retain only what's needed to distinguish between the new entry and
	 * the LAST entry on the page to its left.  If the keys compare equal,
	 * retain the entire key.  We ignore overflow keys, and the entire key
	 * must be retained for the next-to-leftmost key on the leftmost page
	 * of each level, or the search will fail.  Applicable ONLY to internal
	 * pages that have leaf pages as children.  Further reduction of the
	 * key between pairs of internal pages loses too much information.
	 */
	switch (TYPE(child->page)) {
	case P_IBTREE:
		child_bi = GET_BINTERNAL(dbp, child->page, split);
		nbytes = BINTERNAL_PSIZE(child_bi->len);

		if (P_FREESPACE(dbp, ppage) + oldsize < nbytes)
			return (DB_NEEDSPLIT);
		if (LF_ISSET(BPI_SPACEONLY))
			return (0);

		switch (B_TYPE(child_bi->type)) {
		case B_KEYDATA:
			/* Add a new record for the right page. */
			memset(&bi, 0, sizeof(bi));
			bi.len = child_bi->len;
			B_TSET(bi.type, B_KEYDATA);
			bi.pgno = rchild->pgno;
			bi.nrecs = nrecs;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			DB_SET_DBT(data, child_bi->data, child_bi->len);
			size = BINTERNAL_SIZE(child_bi->len);
			break;
		case B_OVERFLOW:
			/* Reuse the overflow key. */
			child_bo = (BOVERFLOW *)child_bi->data;
			memset(&bo, 0, sizeof(bo));
			bo.type = B_OVERFLOW;
			bo.tlen = child_bo->tlen;
			bo.pgno = child_bo->pgno;
			bi.len = BOVERFLOW_SIZE;
			B_TSET(bi.type, B_OVERFLOW);
			bi.pgno = rchild->pgno;
			bi.nrecs = nrecs;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			DB_SET_DBT(data, &bo, BOVERFLOW_SIZE);
			size = BINTERNAL_SIZE(BOVERFLOW_SIZE);
			break;
		case B_DUPLICATE:
		default:
			goto pgfmt;
		}
		break;
	case P_LDUP:
	case P_LBTREE:
		child_bk = GET_BKEYDATA(dbp, child->page, split);
		switch (B_TYPE(child_bk->type)) {
		case B_KEYDATA:
			nbytes = BINTERNAL_PSIZE(child_bk->len);
			nksize = child_bk->len;

			/*
			 * Prefix compression:
			 * We set t->bt_prefix to NULL if we have a comparison
			 * callback but no prefix compression callback.  But,
			 * if we're splitting in an off-page duplicates tree,
			 * we still have to do some checking.  If using the
			 * default off-page duplicates comparison routine we
			 * can use the default prefix compression callback. If
			 * not using the default off-page duplicates comparison
			 * routine, we can't do any kind of prefix compression
			 * as there's no way for an application to specify a
			 * prefix compression callback that corresponds to its
			 * comparison callback.
			 *
			 * No prefix compression if we don't have a compression
			 * function, or the key we'd compress isn't a normal
			 * key (for example, it references an overflow page).
			 *
			 * Generate a parent page key for the right child page
			 * from a comparison of the last key on the left child
			 * page and the first key on the right child page.
			 */
			if (F_ISSET(dbc, DBC_OPD)) {
				if (dbp->dup_compare == __bam_defcmp)
					func = __bam_defpfx;
				else
					func = NULL;
			} else
				func = t->bt_prefix;
			if (func == NULL)
				goto noprefix;
			tmp_bk = GET_BKEYDATA(dbp, lchild, NUM_ENT(lchild) -
			    (TYPE(lchild) == P_LDUP ? O_INDX : P_INDX));
			if (B_TYPE(tmp_bk->type) != B_KEYDATA)
				goto noprefix;
			DB_INIT_DBT(a, tmp_bk->data, tmp_bk->len);
			DB_INIT_DBT(b, child_bk->data, child_bk->len);
			nksize = (u_int32_t)func(dbp, &a, &b);
			if ((n = BINTERNAL_PSIZE(nksize)) < nbytes)
				nbytes = n;
			else
				nksize = child_bk->len;

noprefix:		if (P_FREESPACE(dbp, ppage) + oldsize < nbytes)
				return (DB_NEEDSPLIT);
			if (LF_ISSET(BPI_SPACEONLY))
				return (0);

			memset(&bi, 0, sizeof(bi));
			bi.len = nksize;
			B_TSET(bi.type, B_KEYDATA);
			bi.pgno = rchild->pgno;
			bi.nrecs = nrecs;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			DB_SET_DBT(data, child_bk->data, nksize);
			size = BINTERNAL_SIZE(nksize);
			break;
		case B_OVERFLOW:
			nbytes = BINTERNAL_PSIZE(BOVERFLOW_SIZE);

			if (P_FREESPACE(dbp, ppage) + oldsize < nbytes)
				return (DB_NEEDSPLIT);
			if (LF_ISSET(BPI_SPACEONLY))
				return (0);

			/* Copy the overflow key. */
			child_bo = (BOVERFLOW *)child_bk;
			memset(&bo, 0, sizeof(bo));
			bo.type = B_OVERFLOW;
			bo.tlen = child_bo->tlen;
			memset(&hdr, 0, sizeof(hdr));
			if ((ret = __db_goff(dbc, &hdr, child_bo->tlen,
			     child_bo->pgno, &hdr.data, &hdr.size)) == 0)
				ret = __db_poff(dbc, &hdr, &bo.pgno);

			if (hdr.data != NULL)
				__os_free(dbp->env, hdr.data);
			if (ret != 0)
				return (ret);

			memset(&bi, 0, sizeof(bi));
			bi.len = BOVERFLOW_SIZE;
			B_TSET(bi.type, B_OVERFLOW);
			bi.pgno = rchild->pgno;
			bi.nrecs = nrecs;
			DB_SET_DBT(hdr, &bi, SSZA(BINTERNAL, data));
			DB_SET_DBT(data, &bo, BOVERFLOW_SIZE);
			size = BINTERNAL_SIZE(BOVERFLOW_SIZE);

			break;
		case B_DUPLICATE:
		default:
			goto pgfmt;
		}
		break;
	case P_IRECNO:
	case P_LRECNO:
		nbytes = RINTERNAL_PSIZE;

		if (P_FREESPACE(dbp, ppage) + oldsize < nbytes)
			return (DB_NEEDSPLIT);
		if (LF_ISSET(BPI_SPACEONLY))
			return (0);

		/* Add a new record for the right page. */
		DB_SET_DBT(hdr, &ri, RINTERNAL_SIZE);
		ri.pgno = rchild->pgno;
		ri.nrecs = nrecs;
		size = RINTERNAL_SIZE;
		data.size = 0;
		/*
		 * For now, we are locking internal recno nodes so
		 * use two steps.
		 */
		if (LF_ISSET(BPI_REPLACE)) {
			if ((ret = __bam_ditem(dbc, ppage, off)) != 0)
				return (ret);
			LF_CLR(BPI_REPLACE);
		}
		break;
	default:
pgfmt:		return (__db_pgfmt(dbp->env, PGNO(child->page)));
	}

	if (LF_ISSET(BPI_REPLACE)) {
		DB_ASSERT(dbp->env, !LF_ISSET(BPI_NOLOGGING));
		if ((ret = __bam_irep(dbc, ppage,
		    off, &hdr, data.size != 0 ? &data : NULL)) != 0)
			return (ret);
	} else {
		if (LF_ISSET(BPI_NOLOGGING))
			pitem = __db_pitem_nolog;
		else
			pitem = __db_pitem;

		if ((ret = pitem(dbc, ppage,
		    off, size, &hdr, data.size != 0 ? &data : NULL)) != 0)
			return (ret);
	}

	/*
	 * If a Recno or Btree with record numbers AM page, or an off-page
	 * duplicates tree, adjust the parent page's left page record count.
	 */
	if (F_ISSET(cp, C_RECNUM) && !LF_ISSET(BPI_NORECNUM)) {
		/* Log the change. */
		if (DBC_LOGGING(dbc)) {
			if ((ret = __bam_cadjust_log(dbp, dbc->txn,
			    &LSN(ppage), 0, PGNO(ppage), &LSN(ppage),
			    parent->indx, -(int32_t)nrecs, 0)) != 0)
				return (ret);
		} else
			LSN_NOT_LOGGED(LSN(ppage));

		/* Update the left page count. */
		if (dbc->dbtype == DB_RECNO)
			GET_RINTERNAL(dbp, ppage, parent->indx)->nrecs -= nrecs;
		else
			GET_BINTERNAL(dbp, ppage, parent->indx)->nrecs -= nrecs;
	}

	return (0);
}

/*
 * __bam_psplit --
 *	Do the real work of splitting the page.
 */
static int
__bam_psplit(dbc, cp, lp, rp, splitret)
	DBC *dbc;
	EPG *cp;
	PAGE *lp, *rp;
	db_indx_t *splitret;
{
	DB *dbp;
	PAGE *pp;
	db_indx_t half, *inp, nbytes, off, splitp, top;
	int adjust, cnt, iflag, isbigkey, ret;

	dbp = dbc->dbp;
	pp = cp->page;
	inp = P_INP(dbp, pp);
	adjust = TYPE(pp) == P_LBTREE ? P_INDX : O_INDX;

	/*
	 * If we're splitting the first (last) page on a level because we're
	 * inserting (appending) a key to it, it's likely that the data is
	 * sorted.  Moving a single item to the new page is less work and can
	 * push the fill factor higher than normal.  This is trivial when we
	 * are splitting a new page before the beginning of the tree, all of
	 * the interesting tests are against values of 0.
	 *
	 * Catching appends to the tree is harder.  In a simple append, we're
	 * inserting an item that sorts past the end of the tree; the cursor
	 * will point past the last element on the page.  But, in trees with
	 * duplicates, the cursor may point to the last entry on the page --
	 * in this case, the entry will also be the last element of a duplicate
	 * set (the last because the search call specified the SR_DUPLAST flag).
	 * The only way to differentiate between an insert immediately before
	 * the last item in a tree or an append after a duplicate set which is
	 * also the last item in the tree is to call the comparison function.
	 * When splitting internal pages during an append, the search code
	 * guarantees the cursor always points to the largest page item less
	 * than the new internal entry.  To summarize, we want to catch three
	 * possible index values:
	 *
	 *	NUM_ENT(page)		Btree/Recno leaf insert past end-of-tree
	 *	NUM_ENT(page) - O_INDX	Btree or Recno internal insert past EOT
	 *	NUM_ENT(page) - P_INDX	Btree leaf insert past EOT after a set
	 *				    of duplicates
	 *
	 * two of which, (NUM_ENT(page) - O_INDX or P_INDX) might be an insert
	 * near the end of the tree, and not after the end of the tree at all.
	 * Do a simple test which might be wrong because calling the comparison
	 * functions is expensive.  Regardless, it's not a big deal if we're
	 * wrong, we'll do the split the right way next time.
	 */
	off = 0;
	if (NEXT_PGNO(pp) == PGNO_INVALID && cp->indx >= NUM_ENT(pp) - adjust)
		off = NUM_ENT(pp) - adjust;
	else if (PREV_PGNO(pp) == PGNO_INVALID && cp->indx == 0)
		off = adjust;
	if (off != 0)
		goto sort;

	/*
	 * Split the data to the left and right pages.  Try not to split on
	 * an overflow key.  (Overflow keys on internal pages will slow down
	 * searches.)  Refuse to split in the middle of a set of duplicates.
	 *
	 * First, find the optimum place to split.
	 *
	 * It's possible to try and split past the last record on the page if
	 * there's a very large record at the end of the page.  Make sure this
	 * doesn't happen by bounding the check at the next-to-last entry on
	 * the page.
	 *
	 * Note, we try and split half the data present on the page.  This is
	 * because another process may have already split the page and left
	 * it half empty.  We don't try and skip the split -- we don't know
	 * how much space we're going to need on the page, and we may need up
	 * to half the page for a big item, so there's no easy test to decide
	 * if we need to split or not.  Besides, if two threads are inserting
	 * data into the same place in the database, we're probably going to
	 * need more space soon anyway.
	 */
	top = NUM_ENT(pp) - adjust;
	half = (dbp->pgsize - HOFFSET(pp)) / 2;
	for (nbytes = 0, off = 0; off < top && nbytes < half; ++off)
		switch (TYPE(pp)) {
		case P_IBTREE:
			if (B_TYPE(
			    GET_BINTERNAL(dbp, pp, off)->type) == B_KEYDATA)
				nbytes += BINTERNAL_SIZE(
				   GET_BINTERNAL(dbp, pp, off)->len);
			else
				nbytes += BINTERNAL_SIZE(BOVERFLOW_SIZE);
			break;
		case P_LBTREE:
			if (B_TYPE(GET_BKEYDATA(dbp, pp, off)->type) ==
			    B_KEYDATA)
				nbytes += BKEYDATA_SIZE(GET_BKEYDATA(dbp,
				    pp, off)->len);
			else
				nbytes += BOVERFLOW_SIZE;

			++off;
			/* FALLTHROUGH */
		case P_LDUP:
		case P_LRECNO:
			if (B_TYPE(GET_BKEYDATA(dbp, pp, off)->type) ==
			    B_KEYDATA)
				nbytes += BKEYDATA_SIZE(GET_BKEYDATA(dbp,
				    pp, off)->len);
			else
				nbytes += BOVERFLOW_SIZE;
			break;
		case P_IRECNO:
			nbytes += RINTERNAL_SIZE;
			break;
		default:
			return (__db_pgfmt(dbp->env, pp->pgno));
		}
sort:	splitp = off;

	/*
	 * Splitp is either at or just past the optimum split point.  If the
	 * tree type is such that we're going to promote a key to an internal
	 * page, and our current choice is an overflow key, look for something
	 * close by that's smaller.
	 */
	switch (TYPE(pp)) {
	case P_IBTREE:
		iflag = 1;
		isbigkey =
		    B_TYPE(GET_BINTERNAL(dbp, pp, off)->type) != B_KEYDATA;
		break;
	case P_LBTREE:
	case P_LDUP:
		iflag = 0;
		isbigkey = B_TYPE(GET_BKEYDATA(dbp, pp, off)->type) !=
		    B_KEYDATA;
		break;
	default:
		iflag = isbigkey = 0;
	}
	if (isbigkey)
		for (cnt = 1; cnt <= 3; ++cnt) {
			off = splitp + cnt * adjust;
			if (off < (db_indx_t)NUM_ENT(pp) &&
			    ((iflag && B_TYPE(
			    GET_BINTERNAL(dbp, pp,off)->type) == B_KEYDATA) ||
			    B_TYPE(GET_BKEYDATA(dbp, pp, off)->type) ==
			    B_KEYDATA)) {
				splitp = off;
				break;
			}
			if (splitp <= (db_indx_t)(cnt * adjust))
				continue;
			off = splitp - cnt * adjust;
			if (iflag ? B_TYPE(
			    GET_BINTERNAL(dbp, pp, off)->type) == B_KEYDATA :
			    B_TYPE(GET_BKEYDATA(dbp, pp, off)->type) ==
			    B_KEYDATA) {
				splitp = off;
				break;
			}
		}

	/*
	 * We can't split in the middle a set of duplicates.  We know that
	 * no duplicate set can take up more than about 25% of the page,
	 * because that's the point where we push it off onto a duplicate
	 * page set.  So, this loop can't be unbounded.
	 */
	if (TYPE(pp) == P_LBTREE &&
	    inp[splitp] == inp[splitp - adjust])
		for (cnt = 1;; ++cnt) {
			off = splitp + cnt * adjust;
			if (off < NUM_ENT(pp) &&
			    inp[splitp] != inp[off]) {
				splitp = off;
				break;
			}
			if (splitp <= (db_indx_t)(cnt * adjust))
				continue;
			off = splitp - cnt * adjust;
			if (inp[splitp] != inp[off]) {
				splitp = off + adjust;
				break;
			}
		}

	/* We're going to split at splitp. */
	if ((ret = __bam_copy(dbp, pp, lp, 0, splitp)) != 0)
		return (ret);
	if ((ret = __bam_copy(dbp, pp, rp, splitp, NUM_ENT(pp))) != 0)
		return (ret);

	*splitret = splitp;
	return (0);
}

/*
 * __bam_copy --
 *	Copy a set of records from one page to another.
 *
 * PUBLIC: int __bam_copy __P((DB *, PAGE *, PAGE *, u_int32_t, u_int32_t));
 */
int
__bam_copy(dbp, pp, cp, nxt, stop)
	DB *dbp;
	PAGE *pp, *cp;
	u_int32_t nxt, stop;
{
	BINTERNAL internal;
	db_indx_t *cinp, nbytes, off, *pinp;

	cinp = P_INP(dbp, cp);
	pinp = P_INP(dbp, pp);
	/*
	 * Nxt is the offset of the next record to be placed on the target page.
	 */
	for (off = 0; nxt < stop; ++nxt, ++NUM_ENT(cp), ++off) {
		switch (TYPE(pp)) {
		case P_IBTREE:
			if (off == 0 && nxt != 0)
				nbytes = BINTERNAL_SIZE(0);
			else if (B_TYPE(
			    GET_BINTERNAL(dbp, pp, nxt)->type) == B_KEYDATA)
				nbytes = BINTERNAL_SIZE(
				    GET_BINTERNAL(dbp, pp, nxt)->len);
			else
				nbytes = BINTERNAL_SIZE(BOVERFLOW_SIZE);
			break;
		case P_LBTREE:
			/*
			 * If we're on a key and it's a duplicate, just copy
			 * the offset.
			 */
			if (off != 0 && (nxt % P_INDX) == 0 &&
			    pinp[nxt] == pinp[nxt - P_INDX]) {
				cinp[off] = cinp[off - P_INDX];
				continue;
			}
			/* FALLTHROUGH */
		case P_LDUP:
		case P_LRECNO:
			if (B_TYPE(GET_BKEYDATA(dbp, pp, nxt)->type) ==
			    B_KEYDATA)
				nbytes = BKEYDATA_SIZE(GET_BKEYDATA(dbp,
				    pp, nxt)->len);
			else
				nbytes = BOVERFLOW_SIZE;
			break;
		case P_IRECNO:
			nbytes = RINTERNAL_SIZE;
			break;
		default:
			return (__db_pgfmt(dbp->env, pp->pgno));
		}
		cinp[off] = HOFFSET(cp) -= nbytes;
		if (off == 0 && nxt != 0 && TYPE(pp) == P_IBTREE) {
			internal.len = 0;
			UMRW_SET(internal.unused);
			internal.type = B_KEYDATA;
			internal.pgno = GET_BINTERNAL(dbp, pp, nxt)->pgno;
			internal.nrecs = GET_BINTERNAL(dbp, pp, nxt)->nrecs;
			memcpy(P_ENTRY(dbp, cp, off), &internal, nbytes);
		}
		else
			memcpy(P_ENTRY(dbp, cp, off),
			     P_ENTRY(dbp, pp, nxt), nbytes);
	}
	return (0);
}
