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
#include "dbinc/log.h"
#include "dbinc/mp.h"

#define	IS_BTREE_PAGE(pagep)						\
	(TYPE(pagep) == P_IBTREE ||					\
	 TYPE(pagep) == P_LBTREE || TYPE(pagep) == P_LDUP)

/*
 * __bam_split_recover --
 *	Recovery function for split.
 *
 * PUBLIC: int __bam_split_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_split_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_split_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_LSN *plsnp;
	DB_MPOOLFILE *mpf;
	PAGE *_lp, *lp, *np, *pp, *_rp, *rp, *sp;
	db_pgno_t pgno, parent_pgno;
	u_int32_t ptype, size;
	int cmp, l_update, p_update, r_update, ret, rootsplit, t_ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__bam_split_print);

	_lp = lp = np = pp = _rp = rp = NULL;
	sp = NULL;

	REC_INTRO(__bam_split_read, ip, 0);

	if ((ret = __db_cursor_int(file_dbp, ip, NULL,
	    (argp->opflags & SPL_RECNO) ?  DB_RECNO : DB_BTREE,
	    PGNO_INVALID, 0, NULL, &dbc)) != 0)
		goto out;
	if (argp->opflags & SPL_NRECS)
		F_SET((BTREE_CURSOR *)dbc->internal, C_RECNUM);
	F_SET(dbc, DBC_RECOVER);

	/*
	 * There are two kinds of splits that we have to recover from.  The
	 * first is a root-page split, where the root page is split from a
	 * leaf page into an internal page and two new leaf pages are created.
	 * The second is where a page is split into two pages, and a new key
	 * is inserted into the parent page.
	 *
	 * DBTs are not aligned in log records, so we need to copy the page
	 * so that we can access fields within it throughout this routine.
	 * Although we could hardcode the unaligned copies in this routine,
	 * we will be calling into regular btree functions with this page,
	 * so it's got to be aligned.  Copying it into allocated memory is
	 * the only way to guarantee this.
	 */
	if ((ret = __os_malloc(env, argp->pg.size, &sp)) != 0)
		goto out;
	memcpy(sp, argp->pg.data, argp->pg.size);

	pgno = PGNO(sp);
	parent_pgno = argp->ppgno;
	rootsplit = parent_pgno == pgno;

	/* Get the pages going down the tree. */
	REC_FGET(mpf, ip, parent_pgno, &pp, left);
left:	REC_FGET(mpf, ip, argp->left, &lp, right);
right:	REC_FGET(mpf, ip, argp->right, &rp, redo);

redo:	if (DB_REDO(op)) {
		l_update = r_update = p_update = 0;
		/*
		 * Decide if we need to resplit the page.
		 *
		 * If this is a root split, then the root has to exist unless
		 * we have truncated it due to a future deallocation.
		 */
		if (pp != NULL) {
			if (rootsplit)
				plsnp = &LSN(argp->pg.data);
			else
				plsnp = &argp->plsn;
			cmp = LOG_COMPARE(&LSN(pp), plsnp);
			CHECK_LSN(env, op, cmp, &LSN(pp), plsnp);
			if (cmp == 0)
				p_update = 1;
		}

		if (lp != NULL) {
			cmp = LOG_COMPARE(&LSN(lp), &argp->llsn);
			CHECK_LSN(env, op, cmp, &LSN(lp), &argp->llsn);
			if (cmp == 0)
				l_update = 1;
		}

		if (rp != NULL) {
			cmp = LOG_COMPARE(&LSN(rp), &argp->rlsn);
			CHECK_LSN(env, op, cmp, &LSN(rp), &argp->rlsn);
			if (cmp == 0)
				r_update = 1;
		}

		if (!p_update && !l_update && !r_update)
			goto check_next;

		/* Allocate and initialize new left/right child pages. */
		if ((ret = __os_malloc(env, file_dbp->pgsize, &_lp)) != 0 ||
		    (ret = __os_malloc(env, file_dbp->pgsize, &_rp)) != 0)
			goto out;
		if (rootsplit) {
			P_INIT(_lp, file_dbp->pgsize, argp->left,
			    PGNO_INVALID,
			    ISINTERNAL(sp) ? PGNO_INVALID : argp->right,
			    LEVEL(sp), TYPE(sp));
			P_INIT(_rp, file_dbp->pgsize, argp->right,
			    ISINTERNAL(sp) ?  PGNO_INVALID : argp->left,
			    PGNO_INVALID, LEVEL(sp), TYPE(sp));
		} else {
			P_INIT(_lp, file_dbp->pgsize, PGNO(sp),
			    ISINTERNAL(sp) ? PGNO_INVALID : PREV_PGNO(sp),
			    ISINTERNAL(sp) ? PGNO_INVALID : argp->right,
			    LEVEL(sp), TYPE(sp));
			P_INIT(_rp, file_dbp->pgsize, argp->right,
			    ISINTERNAL(sp) ? PGNO_INVALID : sp->pgno,
			    ISINTERNAL(sp) ? PGNO_INVALID : NEXT_PGNO(sp),
			    LEVEL(sp), TYPE(sp));
		}

		/* Split the page. */
		if ((ret = __bam_copy(file_dbp, sp, _lp, 0, argp->indx)) != 0 ||
		    (ret = __bam_copy(file_dbp, sp, _rp, argp->indx,
		    NUM_ENT(sp))) != 0)
			goto out;

		if (l_update) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &lp);
			memcpy(lp, _lp, file_dbp->pgsize);
			lp->lsn = *lsnp;
		}

		if (r_update) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &rp);
			memcpy(rp, _rp, file_dbp->pgsize);
			rp->lsn = *lsnp;
		}

		/*
		 * Drop the latches on the lower level pages before
		 * getting an exclusive latch on the higher level page.
		 */
		if (lp != NULL && (ret = __memp_fput(mpf,
		    ip, lp, file_dbp->priority)) && ret == 0)
			goto out;
		lp = NULL;
		if (rp != NULL && (ret = __memp_fput(mpf,
		    ip, rp, file_dbp->priority)) && ret == 0)
			goto out;
		rp = NULL;
		/*
		 * If the parent page is wrong, update it.
		 * Initialize the page.  If it is a root page update
		 * the record counts if needed and put the first record in.
		 * Then insert the record for the right hand child page.
		 */
		if (p_update) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &pp);
			if (argp->opflags & SPL_RECNO)
				ptype = P_IRECNO;
			else
				ptype = P_IBTREE;

			if (rootsplit) {
				P_INIT(pp, file_dbp->pgsize, pgno, PGNO_INVALID,
				    PGNO_INVALID, _lp->level + 1, ptype);
				if (argp->opflags & SPL_NRECS) {
					RE_NREC_SET(pp,
					    __bam_total(file_dbp, _lp) +
					    __bam_total(file_dbp, _rp));
				}
				if ((ret = __db_pitem_nolog(dbc, pp,
				    argp->pindx, argp->pentry.size,
				    &argp->pentry, NULL)) != 0)
					goto out;

			}
			if ((ret = __db_pitem_nolog(dbc, pp, argp->pindx + 1,
			    argp->rentry.size, &argp->rentry, NULL)) != 0)
				goto out;
			pp->lsn = *lsnp;
		}

check_next:	/*
		 * Finally, redo the next-page link if necessary.  This is of
		 * interest only if it wasn't a root split -- inserting a new
		 * page in the tree requires that any following page have its
		 * previous-page pointer updated to our new page.  The next
		 * page must exist because we're redoing the operation.
		 */
		if (!rootsplit && argp->npgno != PGNO_INVALID) {
			REC_FGET(mpf, ip, argp->npgno, &np, done);
			cmp = LOG_COMPARE(&LSN(np), &argp->nlsn);
			CHECK_LSN(env, op, cmp, &LSN(np), &argp->nlsn);
			if (cmp == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &np);
				PREV_PGNO(np) = argp->right;
				np->lsn = *lsnp;
			}
		}
	} else {
		/*
		 * If it's a root split and the left child ever existed, update
		 * its LSN.   Otherwise its the split page. If
		 * right child ever existed, root split or not, update its LSN.
		 * The undo of the page allocation(s) will restore them to the
		 * free list.
		 */
		if (rootsplit && lp != NULL &&
		    LOG_COMPARE(lsnp, &LSN(lp)) == 0) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &lp);
			lp->lsn = argp->llsn;
		}
		if (rp != NULL &&
		    LOG_COMPARE(lsnp, &LSN(rp)) == 0) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &rp);
			rp->lsn = argp->rlsn;
		}
		/*
		 * Drop the lower level pages before getting an exclusive
		 * latch on  the parent.
		 */
		if (rp != NULL && (ret = __memp_fput(mpf,
		    ip, rp, file_dbp->priority)))
			goto out;
		rp = NULL;

		/*
		 * Check the state of the split page.  If its a rootsplit
		 * then thats the rootpage otherwise its the left page.
		 */
		if (rootsplit) {
			DB_ASSERT(env, pgno == argp->ppgno);
			if (lp != NULL && (ret = __memp_fput(mpf, ip,
			     lp, file_dbp->priority)) != 0)
				goto out;
			lp = pp;
			pp = NULL;
		}
		if (lp != NULL) {
			cmp = LOG_COMPARE(lsnp, &LSN(lp));
			CHECK_ABORT(env, op, cmp, &LSN(lp), lsnp);
			if (cmp == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &lp);
				memcpy(lp, argp->pg.data, argp->pg.size);
				if ((ret = __memp_fput(mpf,
				    ip, lp, file_dbp->priority)))
					goto out;
				lp = NULL;
			}
		}

		/*
		 * Next we can update the parent removing the new index.
		 */
		if (pp != NULL) {
			DB_ASSERT(env, !rootsplit);
			cmp = LOG_COMPARE(lsnp, &LSN(pp));
			CHECK_ABORT(env, op, cmp, &LSN(pp), lsnp);
			if (cmp == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &pp);
				if (argp->opflags & SPL_RECNO)
					size = RINTERNAL_SIZE;
				else
					size  = BINTERNAL_SIZE(
					    GET_BINTERNAL(file_dbp,
					    pp, argp->pindx + 1)->len);

				if ((ret = __db_ditem(dbc, pp,
				    argp->pindx + 1, size)) != 0)
					goto out;
				pp->lsn = argp->plsn;
			}
		}

		/*
		 * Finally, undo the next-page link if necessary.  This is of
		 * interest only if it wasn't a root split -- inserting a new
		 * page in the tree requires that any following page have its
		 * previous-page pointer updated to our new page.  Since it's
		 * possible that the next-page never existed, we ignore it as
		 * if there's nothing to undo.
		 */
		if (!rootsplit && argp->npgno != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf, &argp->npgno,
			    ip, NULL, DB_MPOOL_EDIT, &np)) != 0) {
				np = NULL;
				goto done;
			}
			if (LOG_COMPARE(lsnp, &LSN(np)) == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &np);
				PREV_PGNO(np) = argp->left;
				np->lsn = argp->nlsn;
			}
		}
	}

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	/* Free any pages that are left. */
	if (lp != NULL && (t_ret = __memp_fput(mpf,
	    ip, lp, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (np != NULL && (t_ret = __memp_fput(mpf,
	    ip, np, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (rp != NULL && (t_ret = __memp_fput(mpf,
	     ip, rp, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (pp != NULL && (t_ret = __memp_fput(mpf,
	    ip, pp, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;

	/* Free any allocated space. */
	if (_lp != NULL)
		__os_free(env, _lp);
	if (_rp != NULL)
		__os_free(env, _rp);
	if (sp != NULL)
		__os_free(env, sp);

	REC_CLOSE;
}
/*
 * __bam_split_recover --
 *	Recovery function for split.
 *
 * PUBLIC: int __bam_split_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_split_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_split_42_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *_lp, *lp, *np, *pp, *_rp, *rp, *sp;
	db_pgno_t pgno, root_pgno;
	u_int32_t ptype;
	int cmp, l_update, p_update, r_update, rc, ret, rootsplit, t_ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__bam_split_print);

	_lp = lp = np = pp = _rp = rp = NULL;
	sp = NULL;

	REC_INTRO(__bam_split_42_read, ip, 0);

	/*
	 * There are two kinds of splits that we have to recover from.  The
	 * first is a root-page split, where the root page is split from a
	 * leaf page into an internal page and two new leaf pages are created.
	 * The second is where a page is split into two pages, and a new key
	 * is inserted into the parent page.
	 *
	 * DBTs are not aligned in log records, so we need to copy the page
	 * so that we can access fields within it throughout this routine.
	 * Although we could hardcode the unaligned copies in this routine,
	 * we will be calling into regular btree functions with this page,
	 * so it's got to be aligned.  Copying it into allocated memory is
	 * the only way to guarantee this.
	 */
	if ((ret = __os_malloc(env, argp->pg.size, &sp)) != 0)
		goto out;
	memcpy(sp, argp->pg.data, argp->pg.size);

	pgno = PGNO(sp);
	root_pgno = argp->root_pgno;
	rootsplit = root_pgno != PGNO_INVALID;
	REC_FGET(mpf, ip, argp->left, &lp, right);
right:	REC_FGET(mpf, ip, argp->right, &rp, redo);

redo:	if (DB_REDO(op)) {
		l_update = r_update = p_update = 0;
		/*
		 * Decide if we need to resplit the page.
		 *
		 * If this is a root split, then the root has to exist unless
		 * we have truncated it due to a future deallocation.
		 */
		if (rootsplit) {
			REC_FGET(mpf, ip, root_pgno, &pp, do_left);
			cmp = LOG_COMPARE(&LSN(pp), &LSN(argp->pg.data));
			CHECK_LSN(env, op,
			    cmp, &LSN(pp), &LSN(argp->pg.data));
			p_update = cmp  == 0;
		}

do_left:	if (lp != NULL) {
			cmp = LOG_COMPARE(&LSN(lp), &argp->llsn);
			CHECK_LSN(env, op, cmp, &LSN(lp), &argp->llsn);
			if (cmp == 0)
				l_update = 1;
		}

		if (rp != NULL) {
			cmp = LOG_COMPARE(&LSN(rp), &argp->rlsn);
			CHECK_LSN(env, op, cmp, &LSN(rp), &argp->rlsn);
			if (cmp == 0)
				r_update = 1;
		}

		if (!p_update && !l_update && !r_update)
			goto check_next;

		/* Allocate and initialize new left/right child pages. */
		if ((ret = __os_malloc(env, file_dbp->pgsize, &_lp)) != 0 ||
		    (ret = __os_malloc(env, file_dbp->pgsize, &_rp)) != 0)
			goto out;
		if (rootsplit) {
			P_INIT(_lp, file_dbp->pgsize, argp->left,
			    PGNO_INVALID,
			    ISINTERNAL(sp) ? PGNO_INVALID : argp->right,
			    LEVEL(sp), TYPE(sp));
			P_INIT(_rp, file_dbp->pgsize, argp->right,
			    ISINTERNAL(sp) ?  PGNO_INVALID : argp->left,
			    PGNO_INVALID, LEVEL(sp), TYPE(sp));
		} else {
			P_INIT(_lp, file_dbp->pgsize, PGNO(sp),
			    ISINTERNAL(sp) ? PGNO_INVALID : PREV_PGNO(sp),
			    ISINTERNAL(sp) ? PGNO_INVALID : argp->right,
			    LEVEL(sp), TYPE(sp));
			P_INIT(_rp, file_dbp->pgsize, argp->right,
			    ISINTERNAL(sp) ? PGNO_INVALID : sp->pgno,
			    ISINTERNAL(sp) ? PGNO_INVALID : NEXT_PGNO(sp),
			    LEVEL(sp), TYPE(sp));
		}

		/* Split the page. */
		if ((ret = __bam_copy(file_dbp, sp, _lp, 0, argp->indx)) != 0 ||
		    (ret = __bam_copy(file_dbp, sp, _rp, argp->indx,
		    NUM_ENT(sp))) != 0)
			goto out;

		if (l_update) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &lp);
			memcpy(lp, _lp, file_dbp->pgsize);
			lp->lsn = *lsnp;
			if ((ret = __memp_fput(mpf,
			     ip, lp, file_dbp->priority)) != 0)
				goto out;
			lp = NULL;
		}

		if (r_update) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &rp);
			memcpy(rp, _rp, file_dbp->pgsize);
			rp->lsn = *lsnp;
			if ((ret = __memp_fput(mpf,
			    ip, rp, file_dbp->priority)) != 0)
				goto out;
			rp = NULL;
		}

		/*
		 * If the parent page is wrong, update it.  This is of interest
		 * only if it was a root split, since root splits create parent
		 * pages.  All other splits modify a parent page, but those are
		 * separately logged and recovered.
		 */
		if (rootsplit && p_update) {
			if (IS_BTREE_PAGE(sp)) {
				ptype = P_IBTREE;
				rc = argp->opflags & SPL_NRECS ? 1 : 0;
			} else {
				ptype = P_IRECNO;
				rc = 1;
			}

			REC_DIRTY(mpf, ip, file_dbp->priority, &pp);
			P_INIT(pp, file_dbp->pgsize, root_pgno,
			    PGNO_INVALID, PGNO_INVALID, _lp->level + 1, ptype);
			RE_NREC_SET(pp, rc ? __bam_total(file_dbp, _lp) +
			    __bam_total(file_dbp, _rp) : 0);

			pp->lsn = *lsnp;
			if ((ret = __memp_fput(mpf,
			     ip, pp, file_dbp->priority)) != 0)
				goto out;
			pp = NULL;
		}

check_next:	/*
		 * Finally, redo the next-page link if necessary.  This is of
		 * interest only if it wasn't a root split -- inserting a new
		 * page in the tree requires that any following page have its
		 * previous-page pointer updated to our new page.  The next
		 * page must exist because we're redoing the operation.
		 */
		if (!rootsplit && argp->npgno != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf, &argp->npgno,
			    ip, NULL, 0, &np)) != 0) {
				if (ret != DB_PAGE_NOTFOUND) {
					ret = __db_pgerr(
					    file_dbp, argp->npgno, ret);
					goto out;
				} else
					goto done;
			}
			cmp = LOG_COMPARE(&LSN(np), &argp->nlsn);
			CHECK_LSN(env, op, cmp, &LSN(np), &argp->nlsn);
			if (cmp == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &np);
				PREV_PGNO(np) = argp->right;
				np->lsn = *lsnp;
				if ((ret = __memp_fput(mpf, ip,
				    np, file_dbp->priority)) != 0)
					goto out;
				np = NULL;
			}
		}
	} else {
		/*
		 * If the split page is wrong, replace its contents with the
		 * logged page contents.  If the page doesn't exist, it means
		 * that the create of the page never happened, nor did any of
		 * the adds onto the page that caused the split, and there's
		 * really no undo-ing to be done.
		 */
		if ((ret = __memp_fget(mpf, &pgno, ip, NULL,
		    DB_MPOOL_EDIT, &pp)) != 0) {
			pp = NULL;
			goto lrundo;
		}
		if (LOG_COMPARE(lsnp, &LSN(pp)) == 0) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &pp);
			memcpy(pp, argp->pg.data, argp->pg.size);
			if ((ret = __memp_fput(mpf,
			     ip, pp, file_dbp->priority)) != 0)
				goto out;
			pp = NULL;
		}

		/*
		 * If it's a root split and the left child ever existed, update
		 * its LSN.  (If it's not a root split, we've updated the left
		 * page already -- it's the same as the split page.) If the
		 * right child ever existed, root split or not, update its LSN.
		 * The undo of the page allocation(s) will restore them to the
		 * free list.
		 */
lrundo:		if ((rootsplit && lp != NULL) || rp != NULL) {
			if (rootsplit && lp != NULL &&
			    LOG_COMPARE(lsnp, &LSN(lp)) == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &lp);
				lp->lsn = argp->llsn;
				if ((ret = __memp_fput(mpf, ip,
				    lp, file_dbp->priority)) != 0)
					goto out;
				lp = NULL;
			}
			if (rp != NULL &&
			    LOG_COMPARE(lsnp, &LSN(rp)) == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &rp);
				rp->lsn = argp->rlsn;
				if ((ret = __memp_fput(mpf, ip,
				     rp, file_dbp->priority)) != 0)
					goto out;
				rp = NULL;
			}
		}

		/*
		 * Finally, undo the next-page link if necessary.  This is of
		 * interest only if it wasn't a root split -- inserting a new
		 * page in the tree requires that any following page have its
		 * previous-page pointer updated to our new page.  Since it's
		 * possible that the next-page never existed, we ignore it as
		 * if there's nothing to undo.
		 */
		if (!rootsplit && argp->npgno != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf, &argp->npgno,
			    ip, NULL, DB_MPOOL_EDIT, &np)) != 0) {
				np = NULL;
				goto done;
			}
			if (LOG_COMPARE(lsnp, &LSN(np)) == 0) {
				REC_DIRTY(mpf, ip, file_dbp->priority, &np);
				PREV_PGNO(np) = argp->left;
				np->lsn = argp->nlsn;
				if (__memp_fput(mpf,
				     ip, np, file_dbp->priority))
					goto out;
				np = NULL;
			}
		}
	}

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	/* Free any pages that weren't dirtied. */
	if (pp != NULL && (t_ret = __memp_fput(mpf,
	    ip, pp, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (lp != NULL && (t_ret = __memp_fput(mpf,
	    ip, lp, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (np != NULL && (t_ret = __memp_fput(mpf,
	    ip, np, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (rp != NULL && (t_ret = __memp_fput(mpf,
	     ip, rp, file_dbp->priority)) != 0 && ret == 0)
		ret = t_ret;

	/* Free any allocated space. */
	if (_lp != NULL)
		__os_free(env, _lp);
	if (_rp != NULL)
		__os_free(env, _rp);
	if (sp != NULL)
		__os_free(env, sp);

	REC_CLOSE;
}

/*
 * __bam_rsplit_recover --
 *	Recovery function for a reverse split.
 *
 * PUBLIC: int __bam_rsplit_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_rsplit_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_rsplit_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_LSN copy_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pgno_t pgno, root_pgno;
	db_recno_t rcnt;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_rsplit_print);
	REC_INTRO(__bam_rsplit_read, ip, 1);

	/* Fix the root page. */
	pgno = root_pgno = argp->root_pgno;
	if ((ret = __memp_fget(mpf, &pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, pgno, ret);
			goto out;
		} else
			goto do_page;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->rootlsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->rootlsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/*
		 * Copy the new data to the root page.  If it is not now a
		 * leaf page we need to restore the record number.  We could
		 * try to determine if C_RECNUM was set in the btree, but
		 * that's not really necessary since the field is not used
		 * otherwise.
		 */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		rcnt = RE_NREC(pagep);
		memcpy(pagep, argp->pgdbt.data, argp->pgdbt.size);
		if (LEVEL(pagep) > LEAFLEVEL)
			RE_NREC_SET(pagep, rcnt);
		pagep->pgno = root_pgno;
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize, root_pgno,
		    argp->nrec, PGNO_INVALID, pagep->level + 1,
		    IS_BTREE_PAGE(pagep) ? P_IBTREE : P_IRECNO);
		if ((ret = __db_pitem(dbc, pagep, 0,
		    argp->rootent.size, &argp->rootent, NULL)) != 0)
			goto out;
		pagep->lsn = argp->rootlsn;
	}
	if ((ret = __memp_fput(mpf, ip, pagep, dbc->priority)) != 0)
		goto out;

do_page:
	/*
	 * Fix the page copied over the root page.  It's possible that the
	 * page never made it to disk, or was truncated so if the page
	 * doesn't exist, it's okay and there's nothing further to do.
	 */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}
	(void)__ua_memcpy(&copy_lsn, &LSN(argp->pgdbt.data), sizeof(DB_LSN));
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &copy_lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &copy_lsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		memcpy(pagep, argp->pgdbt.data, argp->pgdbt.size);
	}
	if ((ret = __memp_fput(mpf, ip, pagep, dbc->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, dbc->priority);
	REC_CLOSE;
}

/*
 * __bam_adj_recover --
 *	Recovery function for adj.
 *
 * PUBLIC: int __bam_adj_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_adj_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_adj_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_adj_print);
	REC_INTRO(__bam_adj_read, ip, 1);

	/* Get the page; if it never existed and we're undoing, we're done. */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if ((ret = __bam_adjindx(dbc,
		    pagep, argp->indx, argp->indx_copy, argp->is_insert)) != 0)
			goto out;

		LSN(pagep) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if ((ret = __bam_adjindx(dbc,
		    pagep, argp->indx, argp->indx_copy, !argp->is_insert)) != 0)
			goto out;

		LSN(pagep) = argp->lsn;
	}
	if ((ret = __memp_fput(mpf, ip, pagep, dbc->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, dbc->priority);
	REC_CLOSE;
}

/*
 * __bam_cadjust_recover --
 *	Recovery function for the adjust of a count change in an internal
 *	page.
 *
 * PUBLIC: int __bam_cadjust_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_cadjust_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_cadjust_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_cadjust_print);
	REC_INTRO(__bam_cadjust_read, ip, 0);

	/* Get the page; if it never existed and we're undoing, we're done. */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		if (IS_BTREE_PAGE(pagep)) {
			GET_BINTERNAL(file_dbp, pagep, argp->indx)->nrecs +=
			    argp->adjust;
			if (argp->opflags & CAD_UPDATEROOT)
				RE_NREC_ADJ(pagep, argp->adjust);
		} else {
			GET_RINTERNAL(file_dbp, pagep, argp->indx)->nrecs +=
			    argp->adjust;
			if (argp->opflags & CAD_UPDATEROOT)
				RE_NREC_ADJ(pagep, argp->adjust);
		}

		LSN(pagep) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		if (IS_BTREE_PAGE(pagep)) {
			GET_BINTERNAL(file_dbp, pagep, argp->indx)->nrecs -=
			    argp->adjust;
			if (argp->opflags & CAD_UPDATEROOT)
				RE_NREC_ADJ(pagep, -(argp->adjust));
		} else {
			GET_RINTERNAL(file_dbp, pagep, argp->indx)->nrecs -=
			    argp->adjust;
			if (argp->opflags & CAD_UPDATEROOT)
				RE_NREC_ADJ(pagep, -(argp->adjust));
		}
		LSN(pagep) = argp->lsn;
	}
	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __bam_cdel_recover --
 *	Recovery function for the intent-to-delete of a cursor record.
 *
 * PUBLIC: int __bam_cdel_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_cdel_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_cdel_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	u_int32_t indx;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_cdel_print);
	REC_INTRO(__bam_cdel_read, ip, 0);

	/* Get the page; if it never existed and we're undoing, we're done. */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		indx = argp->indx + (TYPE(pagep) == P_LBTREE ? O_INDX : 0);
		B_DSET(GET_BKEYDATA(file_dbp, pagep, indx)->type);

		LSN(pagep) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		indx = argp->indx + (TYPE(pagep) == P_LBTREE ? O_INDX : 0);
		B_DCLR(GET_BKEYDATA(file_dbp, pagep, indx)->type);

		if ((ret = __bam_ca_delete(
		    file_dbp, argp->pgno, argp->indx, 0, NULL)) != 0)
			goto out;

		LSN(pagep) = argp->lsn;
	}
	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __bam_repl_recover --
 *	Recovery function for page item replacement.
 *
 * PUBLIC: int __bam_repl_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_repl_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_repl_args *argp;
	DB_THREAD_INFO *ip;
	BKEYDATA *bk;
	BINTERNAL *bi;
	DB *file_dbp;
	DBC *dbc;
	DBT dbt;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, ret;
	u_int32_t len;
	u_int8_t *dp, *p;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_repl_print);
	REC_INTRO(__bam_repl_read, ip, 1);

	/* Get the page; if it never existed and we're undoing, we're done. */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/*
		 * Need to redo update described.
		 *
		 * Re-build the replacement item.
		 */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (TYPE(pagep) == P_IBTREE) {
			/* Point at the internal struct past the type. */
			bi = GET_BINTERNAL(file_dbp, pagep, argp->indx);
			dp = &bi->unused;
			len = bi->len +
			     SSZA(BINTERNAL, data) - SSZ(BINTERNAL, unused);
		} else  {
			bk = GET_BKEYDATA(file_dbp, pagep, argp->indx);
			dp = bk->data;
			len = bk->len;
		}
		memset(&dbt, 0, sizeof(dbt));
		dbt.size = argp->prefix + argp->suffix + argp->repl.size;
		if ((ret = __os_malloc(env, dbt.size, &dbt.data)) != 0)
			goto out;
		p = dbt.data;
		memcpy(p, dp, argp->prefix);
		p += argp->prefix;
		memcpy(p, argp->repl.data, argp->repl.size);
		p += argp->repl.size;
		memcpy(p, dp + (len - argp->suffix), argp->suffix);

		/* isdeleted has become the type flag for non-leaf replace */
		ret = __bam_ritem(dbc,
		     pagep, argp->indx, &dbt, argp->isdeleted);
		__os_free(env, dbt.data);
		if (ret != 0)
			goto out;

		LSN(pagep) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/*
		 * Need to undo update described.
		 *
		 * Re-build the original item.
		 */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (TYPE(pagep) == P_IBTREE) {
			/* Point at the internal struct past the type. */
			bi = GET_BINTERNAL(file_dbp, pagep, argp->indx);
			dp = &bi->unused;
			len = bi->len +
			     SSZA(BINTERNAL, data) - SSZ(BINTERNAL, unused);
		} else  {
			bk = GET_BKEYDATA(file_dbp, pagep, argp->indx);
			dp = bk->data;
			len = bk->len;
		}
		memset(&dbt, 0, sizeof(dbt));
		dbt.size = argp->prefix + argp->suffix + argp->orig.size;
		if ((ret = __os_malloc(env, dbt.size, &dbt.data)) != 0)
			goto out;
		p = dbt.data;
		memcpy(p, dp, argp->prefix);
		p += argp->prefix;
		memcpy(p, argp->orig.data, argp->orig.size);
		p += argp->orig.size;
		memcpy(p, dp + (len - argp->suffix), argp->suffix);

		ret = __bam_ritem(dbc,
		     pagep, argp->indx, &dbt, argp->isdeleted);
		__os_free(env, dbt.data);
		if (ret != 0)
			goto out;

		/* Reset the deleted flag, if necessary. */
		if (argp->isdeleted && LEVEL(pagep) == LEAFLEVEL)
			B_DSET(GET_BKEYDATA(file_dbp, pagep, argp->indx)->type);

		LSN(pagep) = argp->lsn;
	}
	if ((ret = __memp_fput(mpf, ip, pagep, dbc->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, dbc->priority);
	REC_CLOSE;
}

/*
 * __bam_root_recover --
 *	Recovery function for setting the root page on the meta-data page.
 *
 * PUBLIC: int __bam_root_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_root_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_root_args *argp;
	DB_THREAD_INFO *ip;
	BTMETA *meta;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	meta = NULL;
	REC_PRINT(__bam_root_print);
	REC_INTRO(__bam_root_read, ip, 0);

	if ((ret = __memp_fget(mpf, &argp->meta_pgno, ip, NULL,
	    0, &meta)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->meta_pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(meta));
	cmp_p = LOG_COMPARE(&LSN(meta), &argp->meta_lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(meta), &argp->meta_lsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(meta), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		meta->root = argp->root_pgno;
		meta->dbmeta.lsn = *lsnp;
		((BTREE *)file_dbp->bt_internal)->bt_root = meta->root;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Nothing to undo except lsn. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		meta->dbmeta.lsn = argp->meta_lsn;
	}
	if ((ret = __memp_fput(mpf, ip, meta, file_dbp->priority)) != 0)
		goto out;
	meta = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (meta != NULL)
		(void)__memp_fput(mpf, ip, meta, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __bam_curadj_recover --
 *	Transaction abort function to undo cursor adjustments.
 *	This should only be triggered by subtransaction aborts.
 *
 * PUBLIC: int __bam_curadj_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_curadj_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_curadj_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	int ret;

	COMPQUIET(mpf, NULL);

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__bam_curadj_print);
	REC_INTRO(__bam_curadj_read, ip, 1);

	ret = 0;
	if (op != DB_TXN_ABORT)
		goto done;

	switch (argp->mode) {
	case DB_CA_DI:
		if ((ret = __bam_ca_di(dbc, argp->from_pgno,
		    argp->from_indx, -(int)argp->first_indx)) != 0)
			goto out;
		break;
	case DB_CA_DUP:
		if ((ret = __bam_ca_undodup(file_dbp, argp->first_indx,
		    argp->from_pgno, argp->from_indx, argp->to_indx)) != 0)
			goto out;
		break;

	case DB_CA_RSPLIT:
		if ((ret =
		    __bam_ca_rsplit(dbc, argp->to_pgno, argp->from_pgno)) != 0)
			goto out;
		break;

	case DB_CA_SPLIT:
		if ((ret = __bam_ca_undosplit(file_dbp, argp->from_pgno,
		    argp->to_pgno, argp->left_pgno, argp->from_indx)) != 0)
			goto out;
		break;
	}

done:	*lsnp = argp->prev_lsn;
out:	REC_CLOSE;
}

/*
 * __bam_rcuradj_recover --
 *	Transaction abort function to undo cursor adjustments in rrecno.
 *	This should only be triggered by subtransaction aborts.
 *
 * PUBLIC: int __bam_rcuradj_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_rcuradj_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_rcuradj_args *argp;
	DB_THREAD_INFO *ip;
	BTREE_CURSOR *cp;
	DB *file_dbp;
	DBC *dbc, *rdbc;
	DB_MPOOLFILE *mpf;
	int ret, t_ret;

	COMPQUIET(mpf, NULL);

	ip = ((DB_TXNHEAD *)info)->thread_info;
	rdbc = NULL;
	REC_PRINT(__bam_rcuradj_print);
	REC_INTRO(__bam_rcuradj_read, ip, 1);

	ret = t_ret = 0;

	if (op != DB_TXN_ABORT)
		goto done;

	/*
	 * We don't know whether we're in an offpage dup set, and
	 * thus don't know whether the dbc REC_INTRO has handed us is
	 * of a reasonable type.  It's certainly unset, so if this is
	 * an offpage dup set, we don't have an OPD cursor.  The
	 * simplest solution is just to allocate a whole new cursor
	 * for our use;  we're only really using it to hold pass some
	 * state into __ram_ca, and this way we don't need to make
	 * this function know anything about how offpage dups work.
	 */
	if ((ret = __db_cursor_int(file_dbp, NULL,
		NULL, DB_RECNO, argp->root, 0, NULL, &rdbc)) != 0)
		goto out;

	cp = (BTREE_CURSOR *)rdbc->internal;
	F_SET(cp, C_RENUMBER);
	cp->recno = argp->recno;

	switch (argp->mode) {
	case CA_DELETE:
		/*
		 * The way to undo a delete is with an insert.  Since
		 * we're undoing it, the delete flag must be set.
		 */
		F_SET(cp, C_DELETED);
		F_SET(cp, C_RENUMBER);	/* Just in case. */
		cp->order = argp->order;
		if ((ret = __ram_ca(rdbc, CA_ICURRENT, NULL)) != 0)
			goto out;
		break;
	case CA_IAFTER:
	case CA_IBEFORE:
	case CA_ICURRENT:
		/*
		 * The way to undo an insert is with a delete.  The delete
		 * flag is unset to start with.
		 */
		F_CLR(cp, C_DELETED);
		cp->order = INVALID_ORDER;
		if ((ret = __ram_ca(rdbc, CA_DELETE, NULL)) != 0)
			goto out;
		break;
	}

done:	*lsnp = argp->prev_lsn;
out:	if (rdbc != NULL && (t_ret = __dbc_close(rdbc)) != 0 && ret == 0)
		ret = t_ret;
	REC_CLOSE;
}

/*
 * __bam_relink_recover --
 *	Recovery function for relink.
 *
 * PUBLIC: int __bam_relink_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_relink_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_relink_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_relink_print);
	REC_INTRO(__bam_relink_read, ip, 0);

	/*
	 * There are up to three pages we need to check -- the page, and the
	 * previous and next pages, if they existed.  For a page add operation,
	 * the current page is the result of a split and is being recovered
	 * elsewhere, so all we need do is recover the next page.
	 */
	if (argp->next == PGNO_INVALID)
		goto prev;
	if ((ret = __memp_fget(mpf, &argp->next, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->next, ret);
			goto out;
		} else
			goto prev;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn_next);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn_next);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Redo the remove or replace. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		if (argp->new_pgno == PGNO_INVALID)
			pagep->prev_pgno = argp->prev;
		else
			pagep->prev_pgno = argp->new_pgno;

		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Undo the remove or replace. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->prev_pgno = argp->pgno;

		pagep->lsn = argp->lsn_next;
	}

	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

prev:	if (argp->prev == PGNO_INVALID)
		goto done;
	if ((ret = __memp_fget(mpf, &argp->prev, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->prev, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn_prev);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn_prev);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Redo the relink. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		if (argp->new_pgno == PGNO_INVALID)
			pagep->next_pgno = argp->next;
		else
			pagep->next_pgno = argp->new_pgno;

		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Undo the relink. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->next_pgno = argp->pgno;
		pagep->lsn = argp->lsn_prev;
	}

	if ((ret = __memp_fput(mpf,
	     ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __bam_merge_44_recover --
 *	Recovery function for merge.
 *
 * PUBLIC: int __bam_merge_44_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_merge_44_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_merge_44_args *argp;
	DB_THREAD_INFO *ip;
	BKEYDATA *bk;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_indx_t indx, *ninp, *pinp;
	u_int32_t size;
	u_int8_t *bp;
	int cmp_n, cmp_p, i, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__bam_merge_44_print);
	REC_INTRO(__bam_merge_44_read, ip, 1);

	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto next;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(file_dbp->env, op, cmp_p, &LSN(pagep), &argp->lsn);

	if (cmp_p == 0 && DB_REDO(op)) {
		/*
		 * If the header is provided the page is empty, copy the
		 * needed data.
		 */
		DB_ASSERT(env, argp->hdr.size == 0 || NUM_ENT(pagep) == 0);
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (argp->hdr.size != 0) {
			P_INIT(pagep, file_dbp->pgsize, pagep->pgno,
			     PREV_PGNO(argp->hdr.data),
			     NEXT_PGNO(argp->hdr.data),
			     LEVEL(argp->hdr.data), TYPE(argp->hdr.data));
		}
		if (TYPE(pagep) == P_OVERFLOW) {
			OV_REF(pagep) = OV_REF(argp->hdr.data);
			OV_LEN(pagep) = OV_LEN(argp->hdr.data);
			bp = (u_int8_t *) pagep + P_OVERHEAD(file_dbp);
			memcpy(bp, argp->data.data, argp->data.size);
		} else {
			/* Copy the data segment. */
			bp = (u_int8_t *)pagep +
			     (db_indx_t)(HOFFSET(pagep) - argp->data.size);
			memcpy(bp, argp->data.data, argp->data.size);

			/* Copy index table offset past the current entries. */
			pinp = P_INP(file_dbp, pagep) + NUM_ENT(pagep);
			ninp = argp->ind.data;
			for (i = 0;
			     i < (int)(argp->ind.size / sizeof(*ninp)); i++)
				*pinp++ = *ninp++
				      - (file_dbp->pgsize - HOFFSET(pagep));
			HOFFSET(pagep) -= argp->data.size;
			NUM_ENT(pagep) += i;
		}
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && !DB_REDO(op)) {
		/*
		 * Since logging is logical at the page level
		 * we cannot just truncate the data space.  Delete
		 * the proper number of items from the logical end
		 * of the page.
		 */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		for (i = 0; i < (int)(argp->ind.size / sizeof(*ninp)); i++) {
			indx = NUM_ENT(pagep) - 1;
			if (P_INP(file_dbp, pagep)[indx] ==
			     P_INP(file_dbp, pagep)[indx - P_INDX]) {
				NUM_ENT(pagep)--;
				continue;
			}
			switch (TYPE(pagep)) {
			case P_LBTREE:
			case P_LRECNO:
			case P_LDUP:
				bk = GET_BKEYDATA(file_dbp, pagep, indx);
				size = BITEM_SIZE(bk);
				break;

			case P_IBTREE:
				size = BINTERNAL_SIZE(
				     GET_BINTERNAL(file_dbp, pagep, indx)->len);
				break;
			case P_IRECNO:
				size = RINTERNAL_SIZE;
				break;

			default:
				ret = __db_pgfmt(env, PGNO(pagep));
				goto out;
			}
			if ((ret =
			     __db_ditem(dbc, pagep, indx, size)) != 0)
				goto out;
		}
		if (argp->ind.size == 0)
			HOFFSET(pagep) = file_dbp->pgsize;
		pagep->lsn = argp->lsn;
	}

	if ((ret = __memp_fput(mpf, ip, pagep, dbc->priority)) != 0)
		goto out;

next:	if ((ret = __memp_fget(mpf, &argp->npgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->nlsn);
	CHECK_LSN(file_dbp->env, op, cmp_p, &LSN(pagep), &argp->nlsn);

	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to truncate the page. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		HOFFSET(pagep) = file_dbp->pgsize;
		NUM_ENT(pagep) = 0;
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && !DB_REDO(op)) {
		/* Need to put the data back on the page. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (TYPE(pagep) == P_OVERFLOW) {
			OV_REF(pagep) = OV_REF(argp->hdr.data);
			OV_LEN(pagep) = OV_LEN(argp->hdr.data);
			bp = (u_int8_t *) pagep + P_OVERHEAD(file_dbp);
			memcpy(bp, argp->data.data, argp->data.size);
		} else {
			bp = (u_int8_t *)pagep +
			     (db_indx_t)(HOFFSET(pagep) - argp->data.size);
			memcpy(bp, argp->data.data, argp->data.size);

			/* Copy index table. */
			pinp = P_INP(file_dbp, pagep) + NUM_ENT(pagep);
			ninp = argp->ind.data;
			for (i = 0;
			    i < (int)(argp->ind.size / sizeof(*ninp)); i++)
				*pinp++ = *ninp++;
			HOFFSET(pagep) -= argp->data.size;
			NUM_ENT(pagep) = i;
		}
		pagep->lsn = argp->nlsn;
	}

	if ((ret = __memp_fput(mpf,
	     ip, pagep, dbc->priority)) != 0)
		goto out;
done:
	*lsnp = argp->prev_lsn;
	ret = 0;

out:	REC_CLOSE;
}

/*
 * __bam_merge_recover --
 *	Recovery function for merge.
 *
 * PUBLIC: int __bam_merge_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_merge_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_merge_args *argp;
	DB_THREAD_INFO *ip;
	BKEYDATA *bk;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_indx_t indx, *ninp, *pinp;
	u_int32_t size;
	u_int8_t *bp;
	int cmp_n, cmp_p, i, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__bam_merge_print);
	REC_INTRO(__bam_merge_read, ip, 1);

	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto next;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(file_dbp->env, op, cmp_p, &LSN(pagep), &argp->lsn);
	CHECK_ABORT(file_dbp->env, op, cmp_n, &LSN(pagep), lsnp);

	if (cmp_p == 0 && DB_REDO(op)) {
		/*
		 * When pg_copy is set, we are copying onto a new page.
		 */
		DB_ASSERT(env, !argp->pg_copy || NUM_ENT(pagep) == 0);
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (argp->pg_copy) {
			P_INIT(pagep, file_dbp->pgsize, pagep->pgno,
			     PREV_PGNO(argp->hdr.data),
			     NEXT_PGNO(argp->hdr.data),
			     LEVEL(argp->hdr.data), TYPE(argp->hdr.data));
		}
		if (TYPE(pagep) == P_OVERFLOW) {
			OV_REF(pagep) = OV_REF(argp->hdr.data);
			OV_LEN(pagep) = OV_LEN(argp->hdr.data);
			bp = (u_int8_t *)pagep + P_OVERHEAD(file_dbp);
			memcpy(bp, argp->data.data, argp->data.size);
		} else {
			/* Copy the data segment. */
			bp = (u_int8_t *)pagep +
			     (db_indx_t)(HOFFSET(pagep) - argp->data.size);
			memcpy(bp, argp->data.data, argp->data.size);

			/* Copy index table offset past the current entries. */
			pinp = P_INP(file_dbp, pagep) + NUM_ENT(pagep);
			ninp = P_INP(file_dbp, argp->hdr.data);
			for (i = 0; i < NUM_ENT(argp->hdr.data); i++)
				*pinp++ = *ninp++
				      - (file_dbp->pgsize - HOFFSET(pagep));
			HOFFSET(pagep) -= argp->data.size;
			NUM_ENT(pagep) += i;
		}
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && !DB_REDO(op)) {
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (TYPE(pagep) == P_OVERFLOW) {
			HOFFSET(pagep) = file_dbp->pgsize;
			goto setlsn;
		}

		/*
		 * Since logging is logical at the page level we cannot just
		 * truncate the data space.  Delete the proper number of items
		 * from the logical end of the page.
		 */
		for (i = 0; i < NUM_ENT(argp->hdr.data); i++) {
			indx = NUM_ENT(pagep) - 1;
			if (P_INP(file_dbp, pagep)[indx] ==
			     P_INP(file_dbp, pagep)[indx - P_INDX]) {
				NUM_ENT(pagep)--;
				continue;
			}
			switch (TYPE(pagep)) {
			case P_LBTREE:
			case P_LRECNO:
			case P_LDUP:
				bk = GET_BKEYDATA(file_dbp, pagep, indx);
				size = BITEM_SIZE(bk);
				break;

			case P_IBTREE:
				size = BINTERNAL_SIZE(
				     GET_BINTERNAL(file_dbp, pagep, indx)->len);
				break;
			case P_IRECNO:
				size = RINTERNAL_SIZE;
				break;

			default:
				ret = __db_pgfmt(env, PGNO(pagep));
				goto out;
			}
			if ((ret = __db_ditem(dbc, pagep, indx, size)) != 0)
				goto out;
		}
setlsn:		pagep->lsn = argp->lsn;
	}

	if ((ret = __memp_fput(mpf, ip, pagep, dbc->priority)) != 0)
		goto out;

next:	if ((ret = __memp_fget(mpf, &argp->npgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto done;
	}

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->nlsn);
	CHECK_LSN(file_dbp->env, op, cmp_p, &LSN(pagep), &argp->nlsn);

	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to truncate the page. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		HOFFSET(pagep) = file_dbp->pgsize;
		NUM_ENT(pagep) = 0;
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && !DB_REDO(op)) {
		/* Need to put the data back on the page. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if (TYPE(pagep) == P_OVERFLOW) {
			OV_REF(pagep) = OV_REF(argp->hdr.data);
			OV_LEN(pagep) = OV_LEN(argp->hdr.data);
			bp = (u_int8_t *)pagep + P_OVERHEAD(file_dbp);
			memcpy(bp, argp->data.data, argp->data.size);
		} else {
			bp = (u_int8_t *)pagep +
			     (db_indx_t)(HOFFSET(pagep) - argp->data.size);
			memcpy(bp, argp->data.data, argp->data.size);

			/* Copy index table. */
			pinp = P_INP(file_dbp, pagep) + NUM_ENT(pagep);
			ninp = P_INP(file_dbp, argp->hdr.data);
			for (i = 0; i < NUM_ENT(argp->hdr.data); i++)
				*pinp++ = *ninp++;
			HOFFSET(pagep) -= argp->data.size;
			NUM_ENT(pagep) += i;
		}
		pagep->lsn = argp->nlsn;
	}

	if ((ret = __memp_fput(mpf,
	     ip, pagep, dbc->priority)) != 0)
		goto out;
done:
	*lsnp = argp->prev_lsn;
	ret = 0;

out:	REC_CLOSE;
}

/*
 * __bam_pgno_recover --
 *	Recovery function for page number replacment.
 *
 * PUBLIC: int __bam_pgno_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_pgno_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	BINTERNAL *bi;
	__bam_pgno_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep, *npagep;
	db_pgno_t *pgnop;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__bam_pgno_print);
	REC_INTRO(__bam_pgno_read, ip, 0);

	REC_FGET(mpf, ip, argp->pgno, &pagep, done);

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(file_dbp->env, op, cmp_p, &LSN(pagep), &argp->lsn);
	CHECK_ABORT(file_dbp->env, op, cmp_n, &LSN(pagep), lsnp);

	if ((cmp_p == 0 && DB_REDO(op)) || (cmp_n == 0 && !DB_REDO(op))) {
		switch (TYPE(pagep)) {
		case P_IBTREE:
			/*
			 * An internal record can have both a overflow
			 * and child pointer.  Fetch the page to see
			 * which it is.
			 */
			bi = GET_BINTERNAL(file_dbp, pagep, argp->indx);
			if (B_TYPE(bi->type) == B_OVERFLOW) {
				REC_FGET(mpf, ip, argp->npgno, &npagep, out);

				if (TYPE(npagep) == P_OVERFLOW)
					pgnop =
					     &((BOVERFLOW *)(bi->data))->pgno;
				else
					pgnop = &bi->pgno;
				if ((ret = __memp_fput(mpf, ip,
				    npagep, file_dbp->priority)) != 0)
					goto out;
				break;
			}
			pgnop = &bi->pgno;
			break;
		case P_IRECNO:
			pgnop =
			     &GET_RINTERNAL(file_dbp, pagep, argp->indx)->pgno;
			break;
		default:
			pgnop =
			     &GET_BOVERFLOW(file_dbp, pagep, argp->indx)->pgno;
			break;
		}

		if (DB_REDO(op)) {
			/* Need to redo update described. */
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			*pgnop = argp->npgno;
			pagep->lsn = *lsnp;
		} else {
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			*pgnop = argp->opgno;
			pagep->lsn = argp->lsn;
		}
	}

	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;

done:
	*lsnp = argp->prev_lsn;
	ret = 0;

out:	REC_CLOSE;
}

/*
 * __bam_relink_43_recover --
 *	Recovery function for relink.
 *
 * PUBLIC: int __bam_relink_43_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__bam_relink_43_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__bam_relink_43_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, modified, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__bam_relink_43_print);
	REC_INTRO(__bam_relink_43_read, ip, 0);

	/*
	 * There are up to three pages we need to check -- the page, and the
	 * previous and next pages, if they existed.  For a page add operation,
	 * the current page is the result of a split and is being recovered
	 * elsewhere, so all we need do is recover the next page.
	 */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		} else
			goto next2;
	}

	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Redo the relink. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->lsn = *lsnp;
	} else if (LOG_COMPARE(lsnp, &LSN(pagep)) == 0 && DB_UNDO(op)) {
		/* Undo the relink. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->next_pgno = argp->next;
		pagep->prev_pgno = argp->prev;
		pagep->lsn = argp->lsn;
	}
	if ((ret = __memp_fput(mpf,
	     ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

next2: if ((ret = __memp_fget(mpf, &argp->next, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->next, ret);
			goto out;
		} else
			goto prev;
	}

	modified = 0;
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn_next);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn_next);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Redo the remove or undo the add. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->prev_pgno = argp->prev;
		modified = 1;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Undo the remove or redo the add. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->prev_pgno = argp->pgno;
		modified = 1;
	}
	if (modified) {
		if (DB_UNDO(op))
			pagep->lsn = argp->lsn_next;
		else
			pagep->lsn = *lsnp;
	}
	if ((ret = __memp_fput(mpf,
	     ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

prev: if ((ret = __memp_fget(mpf, &argp->prev, ip, NULL, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND) {
			ret = __db_pgerr(file_dbp, argp->prev, ret);
			goto out;
		} else
			goto done;
	}

	modified = 0;
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn_prev);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn_prev);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Redo the relink. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->next_pgno = argp->next;
		modified = 1;
	} else if (LOG_COMPARE(lsnp, &LSN(pagep)) == 0 && DB_UNDO(op)) {
		/* Undo the relink. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->next_pgno = argp->pgno;
		modified = 1;
	}
	if (modified) {
		if (DB_UNDO(op))
			pagep->lsn = argp->lsn_prev;
		else
			pagep->lsn = *lsnp;
	}
	if ((ret = __memp_fput(mpf,
	     ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}
