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
 * __bam_get_root --
 *	Fetch the root of a tree and see if we want to keep
 * it in the stack.
 *
 * PUBLIC: int __bam_get_root __P((DBC *, db_pgno_t, int, u_int32_t, int *));
 */
int
__bam_get_root(dbc, pg, slevel, flags, stack)
	DBC *dbc;
	db_pgno_t pg;
	int slevel;
	u_int32_t flags;
	int *stack;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	PAGE *h;
	db_lockmode_t lock_mode;
	u_int32_t get_mode;
	int ret, t_ret;

	LOCK_INIT(lock);
	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	/*
	 * If write-locking pages, we need to know whether or not to acquire a
	 * write lock on a page before getting it.  This depends on how deep it
	 * is in tree, which we don't know until we acquire the root page.  So,
	 * if we need to lock the root page we may have to upgrade it later,
	 * because we won't get the correct lock initially.
	 *
	 * Retrieve the root page.
	 */
try_again:
	*stack = LF_ISSET(SR_STACK) &&
	      (dbc->dbtype == DB_RECNO || F_ISSET(cp, C_RECNUM));
	lock_mode = DB_LOCK_READ;
	if (*stack ||
	    LF_ISSET(SR_DEL) || (LF_ISSET(SR_NEXT) && LF_ISSET(SR_WRITE)))
		lock_mode = DB_LOCK_WRITE;
	if ((lock_mode == DB_LOCK_WRITE || F_ISSET(dbc, DBC_DOWNREV) ||
	    dbc->dbtype == DB_RECNO || F_ISSET(cp, C_RECNUM))) {
lock_it:	if ((ret = __db_lget(dbc, 0, pg, lock_mode, 0, &lock)) != 0)
			return (ret);
	}

	/*
	 * Get the root.  If the root happens to be a leaf page then
	 * we are supposed to get a read lock on it before latching
	 * it.  So if we have not locked it do a try get first.
	 * If we can't get the root shared, then get a lock on it and
	 * then wait for the latch.
	 */
	if (lock_mode == DB_LOCK_WRITE)
		get_mode = DB_MPOOL_DIRTY;
	else if (LOCK_ISSET(lock) || !STD_LOCKING(dbc))
		get_mode = 0;
	else
		get_mode = DB_MPOOL_TRY;

	if ((ret = __memp_fget(mpf, &pg,
	    dbc->thread_info, dbc->txn, get_mode, &h)) != 0) {
		if (ret == DB_LOCK_NOTGRANTED)
			goto lock_it;
		/* Did not read it, so we can release the lock */
		(void)__LPUT(dbc, lock);
		return (ret);
	}

	/*
	 * Decide if we need to dirty and/or lock this page.
	 * We must not hold the latch while we get the lock.
	 */
	if (!*stack &&
	    ((LF_ISSET(SR_PARENT) && (u_int8_t)(slevel + 1) >= LEVEL(h)) ||
	    LEVEL(h) == LEAFLEVEL ||
	    (LF_ISSET(SR_START) && slevel == LEVEL(h)))) {
		*stack = 1;
		/* If we already have the write lock, we are done. */
		if (dbc->dbtype == DB_RECNO || F_ISSET(cp, C_RECNUM)) {
			if (lock_mode == DB_LOCK_WRITE)
				goto done;
			if ((ret = __LPUT(dbc, lock)) != 0)
				return (ret);
		}

		/*
		 * Now that we know what level the root is at, do we need a
		 * write lock?  If not and we got the lock before latching
		 * we are done.
		 */
		if (LEVEL(h) != LEAFLEVEL || LF_ISSET(SR_WRITE)) {
			lock_mode = DB_LOCK_WRITE;
			/* Drop the read lock if we got it above. */
			if ((ret = __LPUT(dbc, lock)) != 0)
				return (ret);
		} else if (LOCK_ISSET(lock))
			goto done;
		if (!STD_LOCKING(dbc)) {
			if (lock_mode != DB_LOCK_WRITE)
				goto done;
			if ((ret = __memp_dirty(mpf, &h, dbc->thread_info,
			    dbc->txn, dbc->priority, 0)) != 0) {
				if (h != NULL)
					(void)__memp_fput(mpf,
					    dbc->thread_info, h, dbc->priority);
				return (ret);
			}
		} else {
			/* Try to lock the page without waiting first. */
			if ((ret = __db_lget(dbc,
			    0, pg, lock_mode, DB_LOCK_NOWAIT, &lock)) == 0) {
				if (lock_mode == DB_LOCK_WRITE && (ret =
				    __memp_dirty(mpf, &h, dbc->thread_info,
				    dbc->txn, dbc->priority, 0)) != 0) {
					if (h != NULL)
						(void)__memp_fput(mpf,
						    dbc->thread_info, h,
						    dbc->priority);
					return (ret);
				}
				goto done;
			}

			t_ret = __memp_fput(mpf,
			    dbc->thread_info, h, dbc->priority);

			if (ret == DB_LOCK_DEADLOCK ||
			    ret == DB_LOCK_NOTGRANTED)
				ret = 0;
			if (ret == 0)
				ret = t_ret;

			if (ret != 0)
				return (ret);

			if ((ret = __db_lget(dbc,
			     0, pg, lock_mode, 0, &lock)) != 0)
				return (ret);
			if ((ret = __memp_fget(mpf,
			     &pg, dbc->thread_info, dbc->txn,
			     lock_mode == DB_LOCK_WRITE ? DB_MPOOL_DIRTY : 0,
			     &h)) != 0) {
				/* Did not read it, release the lock */
				(void)__LPUT(dbc, lock);
				return (ret);
			}
		}
		/*
		 * While getting dirty or locked we need to drop the mutex
		 * so someone else could get in and split the root.
		 */
		if (!((LF_ISSET(SR_PARENT) &&
		    (u_int8_t)(slevel + 1) >= LEVEL(h)) ||
		    LEVEL(h) == LEAFLEVEL ||
		    (LF_ISSET(SR_START) && slevel == LEVEL(h)))) {
			/* Someone else split the root, start over. */
			ret = __memp_fput(mpf,
			    dbc->thread_info, h, dbc->priority);
			if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
				ret = t_ret;
			if (ret != 0)
				return (ret);
			goto try_again;
		}
	}

done:	BT_STK_ENTER(dbp->env, cp, h, 0, lock, lock_mode, ret);

	return (ret);
}

/*
 * __bam_search --
 *	Search a btree for a key.
 *
 * PUBLIC: int __bam_search __P((DBC *, db_pgno_t,
 * PUBLIC:     const DBT *, u_int32_t, int, db_recno_t *, int *));
 */
int
__bam_search(dbc, root_pgno, key, flags, slevel, recnop, exactp)
	DBC *dbc;
	db_pgno_t root_pgno;
	const DBT *key;
	u_int32_t flags;
	int slevel, *exactp;
	db_recno_t *recnop;
{
	BTREE *t;
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_LOCK lock, saved_lock;
	DB_MPOOLFILE *mpf;
	ENV *env;
	PAGE *h, *parent_h;
	db_indx_t base, i, indx, *inp, lim;
	db_lockmode_t lock_mode;
	db_pgno_t pg, saved_pg;
	db_recno_t recno;
	int adjust, cmp, deloffset, ret, set_stack, stack, t_ret;
	int getlock, was_next;
	int (*func) __P((DB *, const DBT *, const DBT *));
	u_int32_t get_mode, wait;
	u_int8_t level, saved_level;

	dbp = dbc->dbp;
	env = dbp->env;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	h = NULL;
	parent_h = NULL;
	t = dbp->bt_internal;
	recno = 0;
	t_ret = 0;

	BT_STK_CLR(cp);
	LOCK_INIT(saved_lock);
	LOCK_INIT(lock);
	was_next = LF_ISSET(SR_NEXT);
	wait = DB_LOCK_NOWAIT;

	/*
	 * There are several ways we search a btree tree.  The flags argument
	 * specifies if we're acquiring read or write latches, if we position
	 * to the first or last item in a set of duplicates, if we return
	 * deleted items, and if we are latching pairs of pages.  In addition,
	 * if we're modifying record numbers, we have to latch the entire tree
	 * regardless.  See btree.h for more details.
	 */

	if (root_pgno == PGNO_INVALID)
		root_pgno = cp->root;
	saved_pg = root_pgno;
	saved_level = MAXBTREELEVEL;
retry:	if ((ret = __bam_get_root(dbc, root_pgno, slevel, flags, &stack)) != 0)
		goto err;
	lock_mode = cp->csp->lock_mode;
	get_mode = lock_mode == DB_LOCK_WRITE ? DB_MPOOL_DIRTY : 0;
	h = cp->csp->page;
	pg = PGNO(h);
	lock = cp->csp->lock;
	set_stack = stack;
	/*
	 * Determine if we need to lock interiror nodes.
	 * If we have record numbers we always lock.  Otherwise we only
	 * need to do this if we are write locking and we are returning
	 * a stack of nodes.  SR_NEXT will eventually get a stack and
	 * release the locks above that level.
	 */
	if (F_ISSET(dbc, DBC_DOWNREV)) {
		getlock = 1;
		wait = 0;
	} else
		getlock = F_ISSET(cp, C_RECNUM) ||
		   (lock_mode == DB_LOCK_WRITE &&
		   (stack || LF_ISSET(SR_NEXT | SR_DEL)));

	/*
	 * If we are asked a level that is above the root,
	 * just return the root.  This can happen if the tree
	 * collapses while we are trying to lock the root.
	 */
	if (!LF_ISSET(SR_START) && LEVEL(h) < slevel)
		goto done;

	BT_STK_CLR(cp);

	/* Choose a comparison function. */
	func = F_ISSET(dbc, DBC_OPD) ?
	    (dbp->dup_compare == NULL ? __bam_defcmp : dbp->dup_compare) :
	    t->bt_compare;

	for (;;) {
		if (TYPE(h) == P_LBTREE)
			adjust = P_INDX;
		else {
			/*
			 * It is possible to catch an internal page as a change
			 * is being backed out.  Its leaf pages will be locked
			 * but we must be sure we get to one.  If the page
			 * is not populated enough lock it.
			 */
			if (TYPE(h) != P_LDUP && NUM_ENT(h) == 0) {
				getlock = 1;
				level = LEVEL(h) + 1;
				if ((ret = __memp_fput(mpf, dbc->thread_info,
				     h, dbc->priority)) != 0)
					goto err;
				goto lock_next;
			}
			adjust = O_INDX;
		}
		inp = P_INP(dbp, h);
		if (LF_ISSET(SR_MIN | SR_MAX)) {
			if (LF_ISSET(SR_MIN) || NUM_ENT(h) == 0)
				indx = 0;
			else if (TYPE(h) == P_LBTREE)
				indx = NUM_ENT(h) - 2;
			else
				indx = NUM_ENT(h) - 1;

			if (LEVEL(h) == LEAFLEVEL ||
			     (!LF_ISSET(SR_START) && LEVEL(h) == slevel)) {
				if (LF_ISSET(SR_NEXT))
					goto get_next;
				goto found;
			}
			goto next;
		}
		/*
		 * Do a binary search on the current page.  If we're searching
		 * a Btree leaf page, we have to walk the indices in groups of
		 * two.  If we're searching an internal page or a off-page dup
		 * page, they're an index per page item.  If we find an exact
		 * match on a leaf page, we're done.
		 */
		DB_BINARY_SEARCH_FOR(base, lim, NUM_ENT(h), adjust) {
			DB_BINARY_SEARCH_INCR(indx, base, lim, adjust);
			if ((ret = __bam_cmp(dbc, key, h, indx,
			    func, &cmp)) != 0)
				goto err;
			if (cmp == 0) {
				if (LEVEL(h) == LEAFLEVEL ||
				    (!LF_ISSET(SR_START) &&
				    LEVEL(h) == slevel)) {
					if (LF_ISSET(SR_NEXT))
						goto get_next;
					goto found;
				}
				goto next;
			}
			if (cmp > 0)
				DB_BINARY_SEARCH_SHIFT_BASE(indx, base,
				    lim, adjust);
		}

		/*
		 * No match found.  Base is the smallest index greater than
		 * key and may be zero or a last + O_INDX index.
		 *
		 * If it's a leaf page or the stopping point,
		 * return base as the "found" value.
		 * Delete only deletes exact matches.
		 */
		if (LEVEL(h) == LEAFLEVEL ||
		    (!LF_ISSET(SR_START) && LEVEL(h) == slevel)) {
			*exactp = 0;

			if (LF_ISSET(SR_EXACT)) {
				ret = DB_NOTFOUND;
				goto err;
			}

			if (LF_ISSET(SR_STK_ONLY)) {
				BT_STK_NUM(env, cp, h, base, ret);
				if ((t_ret =
				    __LPUT(dbc, lock)) != 0 && ret == 0)
					ret = t_ret;
				if ((t_ret = __memp_fput(mpf, dbc->thread_info,
				     h, dbc->priority)) != 0 && ret == 0)
					ret = t_ret;
				h = NULL;
				if (ret != 0)
					goto err;
				goto done;
			}
			if (LF_ISSET(SR_NEXT)) {
get_next:			/*
				 * The caller could have asked for a NEXT
				 * at the root if the tree recently collapsed.
				 */
				if (PGNO(h) == root_pgno) {
					ret = DB_NOTFOUND;
					goto err;
				}

				indx = cp->sp->indx + 1;
				if (indx == NUM_ENT(cp->sp->page)) {
					ret = DB_NOTFOUND;
					cp->csp++;
					goto err;
				}
				/*
				 * If we want both the key page and the next
				 * page, push the key page on the stack
				 * otherwise save the root of the subtree
				 * and drop the rest of the subtree.
				 * Search down again starting at the
				 * next child of the root of this subtree.
				 */
				LF_SET(SR_MIN);
				LF_CLR(SR_NEXT);
				set_stack = stack = 1;
				if (LF_ISSET(SR_BOTH)) {
					cp->csp++;
					BT_STK_PUSH(env,
					    cp, h, indx, lock, lock_mode, ret);
					if (ret != 0)
						goto err;
					LOCK_INIT(lock);
					h = cp->sp->page;
					pg = GET_BINTERNAL(dbp, h, indx)->pgno;
					level = LEVEL(h);
					h = NULL;
					goto lock_next;
				} else {
					if ((ret = __LPUT(dbc, lock)) != 0)
						goto err;
					if ((ret = __memp_fput(mpf,
					    dbc->thread_info,
					    h, dbc->priority)) != 0)
						goto err;
					h = cp->sp->page;
					cp->sp->page = NULL;
					lock = cp->sp->lock;
					LOCK_INIT(cp->sp->lock);
					if ((ret = __bam_stkrel(dbc,
					    STK_NOLOCK)) != 0)
						goto err;
					goto next;
				}
			}

			/*
			 * !!!
			 * Possibly returning a deleted record -- DB_SET_RANGE,
			 * DB_KEYFIRST and DB_KEYLAST don't require an exact
			 * match, and we don't want to walk multiple pages here
			 * to find an undeleted record.  This is handled by the
			 * calling routine.
			 */
			if (LF_ISSET(SR_DEL) && cp->csp == cp->sp)
				cp->csp++;
			BT_STK_ENTER(env, cp, h, base, lock, lock_mode, ret);
			if (ret != 0)
				goto err;
			goto done;
		}

		/*
		 * If it's not a leaf page, record the internal page (which is
		 * a parent page for the key).  Decrement the base by 1 if it's
		 * non-zero so that if a split later occurs, the inserted page
		 * will be to the right of the saved page.
		 */
		indx = base > 0 ? base - O_INDX : base;

		/*
		 * If we're trying to calculate the record number, sum up
		 * all the record numbers on this page up to the indx point.
		 */
next:		if (recnop != NULL)
			for (i = 0; i < indx; ++i)
				recno += GET_BINTERNAL(dbp, h, i)->nrecs;

		pg = GET_BINTERNAL(dbp, h, indx)->pgno;
		level = LEVEL(h);

		/* See if we are at the level to start stacking. */
		if (LF_ISSET(SR_START) && slevel == level)
			set_stack = stack = 1;

		if (LF_ISSET(SR_STK_ONLY)) {
			if (slevel == LEVEL(h)) {
				BT_STK_NUM(env, cp, h, indx, ret);
				if ((t_ret = __memp_fput(mpf, dbc->thread_info,
				    h, dbc->priority)) != 0 && ret == 0)
					ret = t_ret;
				h = NULL;
				if (ret != 0)
					goto err;
				goto done;
			}
			BT_STK_NUMPUSH(env, cp, h, indx, ret);
			(void)__memp_fput(mpf,
			    dbc->thread_info, h, dbc->priority);
			h = NULL;
		} else if (stack) {
			/* Return if this is the lowest page wanted. */
			if (LF_ISSET(SR_PARENT) && slevel == level) {
				BT_STK_ENTER(env,
				    cp, h, indx, lock, lock_mode, ret);
				if (ret != 0)
					goto err;
				goto done;
			}
			if (LF_ISSET(SR_DEL) && NUM_ENT(h) > 1) {
				/*
				 * There was a page with a singleton pointer
				 * to a non-empty subtree.
				 */
				cp->csp--;
				if ((ret = __bam_stkrel(dbc, STK_NOLOCK)) != 0)
					goto err;
				set_stack = stack = 0;
				goto do_del;
			}
			BT_STK_PUSH(env,
			    cp, h, indx, lock, lock_mode, ret);
			if (ret != 0)
				goto err;

			LOCK_INIT(lock);
			get_mode = DB_MPOOL_DIRTY;
			lock_mode = DB_LOCK_WRITE;
			goto lock_next;
		} else {
			/*
			 * Decide if we want to return a reference to the next
			 * page in the return stack.  If so, latch it and don't
			 * unlatch it.  We will want to stack things on the
			 * next iteration.  The stack variable cannot be
			 * set until we leave this clause. If we are locking
			 * then we must lock this level before getting the page.
			 */
			if ((LF_ISSET(SR_PARENT) &&
			    (u_int8_t)(slevel + 1) >= (level - 1)) ||
			    (level - 1) == LEAFLEVEL)
				set_stack = 1;

			/*
			 * Check for a normal search.  If so, we need to
			 * latch couple the parent/chid buffers.
			 */
			if (!LF_ISSET(SR_DEL | SR_NEXT)) {
				parent_h = h;
				goto lock_next;
			}

			/*
			 * Returning a subtree.  See if we have hit the start
			 * point if so save the parent and set stack.
			 * Otherwise free the parent and temporarily
			 * save this one.
			 * For SR_DEL we need to find a page with 1 entry.
			 * For SR_NEXT we want find the minimal subtree
			 * that contains the key and the next page.
			 * We save pages as long as we are at the right
			 * edge of the subtree.  When we leave the right
			 * edge, then drop the subtree.
			 */

			if ((LF_ISSET(SR_DEL) && NUM_ENT(h) == 1)) {
				/*
				 * We are pushing the things on the stack,
				 * set the stack variable now to indicate this
				 * has happened.
				 */
				stack = set_stack = 1;
				LF_SET(SR_WRITE);
				/* Push the parent. */
				cp->csp++;
				/* Push this node. */
				BT_STK_PUSH(env, cp, h,
				     indx, lock, DB_LOCK_NG, ret);
				if (ret != 0)
					goto err;
				LOCK_INIT(lock);
			} else {
			/*
			 * See if we want to save the tree so far.
			 * If we are looking for the next key,
			 * then we must save this node if we are
			 * at the end of the page.  If not then
			 * discard anything we have saved so far.
			 * For delete only keep one node until
			 * we find a singleton.
			 */
do_del:				if (cp->csp->page != NULL) {
					if (LF_ISSET(SR_NEXT) &&
					     indx == NUM_ENT(h) - 1)
						cp->csp++;
					else if ((ret =
					    __bam_stkrel(dbc, STK_NOLOCK)) != 0)
						goto err;
				}
				/* Save this node. */
				BT_STK_ENTER(env, cp,
				    h, indx, lock, lock_mode, ret);
				if (ret != 0)
					goto err;
				LOCK_INIT(lock);
			}

lock_next:		h = NULL;

			if (set_stack && LF_ISSET(SR_WRITE)) {
				lock_mode = DB_LOCK_WRITE;
				get_mode = DB_MPOOL_DIRTY;
				getlock = 1;
			}
			/*
			 * If we are retrying and we are back at the same
			 * page then we already have it locked.  If we are
			 * at a different page we want to lock couple and
			 * release that lock.
			 */
			if (level - 1 == saved_level) {
				if ((ret = __LPUT(dbc, lock)) != 0)
					goto err;
				lock = saved_lock;
				LOCK_INIT(saved_lock);
				saved_level = MAXBTREELEVEL;
				if (pg == saved_pg)
					goto skip_lock;
			}
			if ((getlock || level - 1 == LEAFLEVEL) &&
			    (ret = __db_lget(dbc, LCK_COUPLE_ALWAYS,
			    pg, lock_mode, wait, &lock)) != 0) {
				/*
				 * If we are doing DEL or NEXT then we
				 * have an extra level saved in the stack,
				 * push it so it will get freed.
				 */
				if (LF_ISSET(SR_DEL | SR_NEXT) && !stack)
					cp->csp++;
				/*
				 * If we fail, discard the lock we held.
				 * This is ok because we will either search
				 * again or exit without actually looking
				 * at the data.
				 */
				if ((t_ret = __LPUT(dbc, lock)) != 0 &&
				    ret == 0)
					ret = t_ret;
				/*
				 * If we blocked at a different level release
				 * the previous saved lock.
				 */
				if ((t_ret = __LPUT(dbc, saved_lock)) != 0 &&
				    ret == 0)
					ret = t_ret;
				if (wait == 0 || (ret != DB_LOCK_NOTGRANTED &&
				     ret != DB_LOCK_DEADLOCK))
					goto err;

				/* Relase the parent if we are holding it. */
				if (parent_h != NULL &&
				    (ret = __memp_fput(mpf, dbc->thread_info,
				    parent_h, dbc->priority)) != 0)
					goto err;
				parent_h = NULL;

				BT_STK_POP(cp);
				if ((ret = __bam_stkrel(dbc, STK_NOLOCK)) != 0)
					goto err;
				if ((ret = __db_lget(dbc,
				    0, pg, lock_mode, 0, &saved_lock)) != 0)
					goto err;
				/*
				 * A very strange case: if this page was
				 * freed while we wait then we cannot hold
				 * the lock on it while we reget the root
				 * latch because allocation is one place
				 * we lock while holding a latch.
				 * Noone can have a free page locked, so
				 * check for that case.  We do this by
				 * checking the level, since it will be 0
				 * if free and we might as well see if this
				 * page moved and drop the lock in that case.
				 */
				if ((ret = __memp_fget(mpf, &pg,
				     dbc->thread_info,
				     dbc->txn, get_mode, &h)) != 0 &&
				     ret != DB_PAGE_NOTFOUND)
					goto err;

				if (ret != 0 || LEVEL(h) != level - 1) {
					ret = __LPUT(dbc, saved_lock);
					if (ret != 0)
						goto err;
					pg = root_pgno;
					saved_level = MAXBTREELEVEL;
				}
				if (h != NULL && (ret = __memp_fput(mpf,
				    dbc->thread_info, h, dbc->priority)) != 0)
					goto err;
				h = NULL;

				if (was_next) {
					LF_CLR(SR_MIN);
					LF_SET(SR_NEXT);
				}
				/*
				 * We have the lock but we dropped the
				 * latch so we need to search again. If
				 * we get back to the same page then all
				 * is good, otherwise we need to try to
				 * lock the new page.
				 */
				saved_pg = pg;
				saved_level = level - 1;
				goto retry;
			}
skip_lock:		stack = set_stack;
		}
		/* Get the child page. */
		if ((ret = __memp_fget(mpf, &pg,
		     dbc->thread_info, dbc->txn, get_mode, &h)) != 0)
			goto err;
		/* Release the parent. */
		if (parent_h != NULL && (ret = __memp_fput(mpf,
		    dbc->thread_info, parent_h, dbc->priority)) != 0)
			goto err;
		parent_h = NULL;
	}
	/* NOTREACHED */

found:	*exactp = 1;

	/*
	 * If we got here, we know that we have a Btree leaf or off-page
	 * duplicates page.  If it's a Btree leaf page, we have to handle
	 * on-page duplicates.
	 *
	 * If there are duplicates, go to the first/last one.  This is
	 * safe because we know that we're not going to leave the page,
	 * all duplicate sets that are not on overflow pages exist on a
	 * single leaf page.
	 */
	if (TYPE(h) == P_LBTREE && NUM_ENT(h) > P_INDX) {
		if (LF_ISSET(SR_DUPLAST))
			while (indx < (db_indx_t)(NUM_ENT(h) - P_INDX) &&
			    inp[indx] == inp[indx + P_INDX])
				indx += P_INDX;
		else if (LF_ISSET(SR_DUPFIRST))
			while (indx > 0 &&
			    inp[indx] == inp[indx - P_INDX])
				indx -= P_INDX;
	}

	/*
	 * Now check if we are allowed to return deleted items; if not, then
	 * find the next (or previous) non-deleted duplicate entry.  (We do
	 * not move from the original found key on the basis of the SR_DELNO
	 * flag.)
	 */
	DB_ASSERT(env, recnop == NULL || LF_ISSET(SR_DELNO));
	if (LF_ISSET(SR_DELNO)) {
		deloffset = TYPE(h) == P_LBTREE ? O_INDX : 0;
		if (LF_ISSET(SR_DUPLAST))
			while (B_DISSET(GET_BKEYDATA(dbp,
			    h, indx + deloffset)->type) && indx > 0 &&
			    inp[indx] == inp[indx - adjust])
				indx -= adjust;
		else
			while (B_DISSET(GET_BKEYDATA(dbp,
			    h, indx + deloffset)->type) &&
			    indx < (db_indx_t)(NUM_ENT(h) - adjust) &&
			    inp[indx] == inp[indx + adjust])
				indx += adjust;

		/*
		 * If we weren't able to find a non-deleted duplicate, return
		 * DB_NOTFOUND.
		 */
		if (B_DISSET(GET_BKEYDATA(dbp, h, indx + deloffset)->type)) {
			ret = DB_NOTFOUND;
			goto err;
		}

		/*
		 * Increment the record counter to point to the found element.
		 * Ignore any deleted key/data pairs.  There doesn't need to
		 * be any correction for duplicates, as Btree doesn't support
		 * duplicates and record numbers in the same tree.
		 */
		if (recnop != NULL) {
			DB_ASSERT(env, TYPE(h) == P_LBTREE);

			for (i = 0; i < indx; i += P_INDX)
				if (!B_DISSET(
				    GET_BKEYDATA(dbp, h, i + O_INDX)->type))
					++recno;

			/* Correct the number for a 0-base. */
			*recnop = recno + 1;
		}
	}

	if (LF_ISSET(SR_STK_ONLY)) {
		BT_STK_NUM(env, cp, h, indx, ret);
		if ((t_ret = __memp_fput(mpf,
		     dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
			ret = t_ret;
	} else {
		if (LF_ISSET(SR_DEL) && cp->csp == cp->sp)
			cp->csp++;
		BT_STK_ENTER(env, cp, h, indx, lock, lock_mode, ret);
	}
	if (ret != 0)
		goto err;

	cp->csp->lock = lock;
	DB_ASSERT(env, parent_h == NULL);

done:	if ((ret = __LPUT(dbc, saved_lock)) != 0)
		return (ret);

	return (0);

err:	if (ret == 0)
		ret = t_ret;
	if (h != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, h, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (parent_h != NULL && (t_ret = __memp_fput(mpf,
	    dbc->thread_info, parent_h, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;

	/* Keep any not-found page locked for serializability. */
	if ((t_ret = __TLPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;

	(void)__LPUT(dbc, saved_lock);

	BT_STK_POP(cp);
	(void)__bam_stkrel(dbc, 0);

	return (ret);
}

/*
 * __bam_stkrel --
 *	Release all pages currently held in the stack.
 *
 * PUBLIC: int __bam_stkrel __P((DBC *, u_int32_t));
 */
int
__bam_stkrel(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_MPOOLFILE *mpf;
	EPG *epg;
	int ret, t_ret;

	DB_ASSERT(NULL, dbc != NULL);
	dbp = dbc->dbp;
	mpf = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;

	/*
	 * Release inner pages first.
	 *
	 * The caller must be sure that setting STK_NOLOCK will not effect
	 * either serializability or recoverability.
	 */
	for (ret = 0, epg = cp->sp; epg <= cp->csp; ++epg) {
		if (epg->page != NULL) {
			if (LF_ISSET(STK_CLRDBC) && cp->page == epg->page) {
				cp->page = NULL;
				LOCK_INIT(cp->lock);
			}
			if ((t_ret = __memp_fput(mpf, dbc->thread_info,
			     epg->page, dbc->priority)) != 0 && ret == 0)
				ret = t_ret;
			epg->page = NULL;
		}
		/*
		 * We set this if we need to release our pins,
		 * but are not logically ready to have the pages
		 * visible.
		 */
		if (LF_ISSET(STK_PGONLY))
			continue;
		if (LF_ISSET(STK_NOLOCK)) {
			if ((t_ret = __LPUT(dbc, epg->lock)) != 0 && ret == 0)
				ret = t_ret;
		} else
			if ((t_ret = __TLPUT(dbc, epg->lock)) != 0 && ret == 0)
				ret = t_ret;
	}

	/* Clear the stack, all pages have been released. */
	if (!LF_ISSET(STK_PGONLY))
		BT_STK_CLR(cp);

	return (ret);
}

/*
 * __bam_stkgrow --
 *	Grow the stack.
 *
 * PUBLIC: int __bam_stkgrow __P((ENV *, BTREE_CURSOR *));
 */
int
__bam_stkgrow(env, cp)
	ENV *env;
	BTREE_CURSOR *cp;
{
	EPG *p;
	size_t entries;
	int ret;

	entries = cp->esp - cp->sp;

	if ((ret = __os_calloc(env, entries * 2, sizeof(EPG), &p)) != 0)
		return (ret);
	memcpy(p, cp->sp, entries * sizeof(EPG));
	if (cp->sp != cp->stack)
		__os_free(env, cp->sp);
	cp->sp = p;
	cp->csp = p + entries;
	cp->esp = p + entries * 2;
	return (0);
}
