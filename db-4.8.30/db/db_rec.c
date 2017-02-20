/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 2010 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/hash.h"

static int __db_pg_free_recover_int __P((ENV *, DB_THREAD_INFO *,
    __db_pg_freedata_args *, DB *, DB_LSN *, DB_MPOOLFILE *, db_recops, int));
static int __db_pg_free_recover_42_int __P((ENV *, DB_THREAD_INFO *,
    __db_pg_freedata_42_args *,
    DB *, DB_LSN *, DB_MPOOLFILE *, db_recops, int));

/*
 * PUBLIC: int __db_addrem_recover
 * PUBLIC:    __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 *
 * This log message is generated whenever we add or remove a duplicate
 * to/from a duplicate page.  On recover, we just do the opposite.
 */
int
__db_addrem_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_addrem_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, modified, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__db_addrem_print);
	REC_INTRO(__db_addrem_read, ip, 1);

	REC_FGET(mpf, ip, argp->pgno, &pagep, done);
	modified = 0;

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->pagelsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == DB_ADD_DUP) ||
	    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == DB_REM_DUP)) {
		/* Need to redo an add, or undo a delete. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if ((ret = __db_pitem(dbc, pagep, argp->indx, argp->nbytes,
		    argp->hdr.size == 0 ? NULL : &argp->hdr,
		    argp->dbt.size == 0 ? NULL : &argp->dbt)) != 0)
			goto out;
		modified = 1;

	} else if ((cmp_n == 0 && DB_UNDO(op) && argp->opcode == DB_ADD_DUP) ||
	    (cmp_p == 0 && DB_REDO(op) && argp->opcode == DB_REM_DUP)) {
		/* Need to undo an add, or redo a delete. */
		REC_DIRTY(mpf, ip, dbc->priority, &pagep);
		if ((ret = __db_ditem(dbc,
		    pagep, argp->indx, argp->nbytes)) != 0)
			goto out;
		modified = 1;
	}

	if (modified) {
		if (DB_REDO(op))
			LSN(pagep) = *lsnp;
		else
			LSN(pagep) = argp->pagelsn;
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
 * PUBLIC: int __db_big_recover
 * PUBLIC:     __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_big_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_big_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, modified, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__db_big_print);
	REC_INTRO(__db_big_read, ip, 0);

	REC_FGET(mpf, ip, argp->pgno, &pagep, ppage);
	modified = 0;

	/*
	 * There are three pages we need to check.  The one on which we are
	 * adding data, the previous one whose next_pointer may have
	 * been updated, and the next one whose prev_pointer may have
	 * been updated.
	 */
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->pagelsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == DB_ADD_BIG) ||
	    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == DB_REM_BIG)) {
		/* We are either redo-ing an add, or undoing a delete. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize, argp->pgno, argp->prev_pgno,
			argp->next_pgno, 0, P_OVERFLOW);
		OV_LEN(pagep) = argp->dbt.size;
		OV_REF(pagep) = 1;
		memcpy((u_int8_t *)pagep + P_OVERHEAD(file_dbp), argp->dbt.data,
		    argp->dbt.size);
		PREV_PGNO(pagep) = argp->prev_pgno;
		modified = 1;
	} else if ((cmp_n == 0 && DB_UNDO(op) && argp->opcode == DB_ADD_BIG) ||
	    (cmp_p == 0 && DB_REDO(op) && argp->opcode == DB_REM_BIG)) {
		/*
		 * We are either undo-ing an add or redo-ing a delete.
		 * The page is about to be reclaimed in either case, so
		 * there really isn't anything to do here.
		 */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		modified = 1;
	} else if (cmp_p == 0 && DB_REDO(op) && argp->opcode == DB_APPEND_BIG) {
		/* We are redoing an append. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		memcpy((u_int8_t *)pagep + P_OVERHEAD(file_dbp) +
		    OV_LEN(pagep), argp->dbt.data, argp->dbt.size);
		OV_LEN(pagep) += argp->dbt.size;
		modified = 1;
	} else if (cmp_n == 0 && DB_UNDO(op) && argp->opcode == DB_APPEND_BIG) {
		/* We are undoing an append. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		OV_LEN(pagep) -= argp->dbt.size;
		memset((u_int8_t *)pagep + P_OVERHEAD(file_dbp) +
		    OV_LEN(pagep), 0, argp->dbt.size);
		modified = 1;
	}
	if (modified)
		LSN(pagep) = DB_REDO(op) ? *lsnp : argp->pagelsn;

	ret = __memp_fput(mpf, ip, pagep, file_dbp->priority);
	pagep = NULL;
	if (ret != 0)
		goto out;

	/*
	 * We only delete a whole chain of overflow items, and appends only
	 * apply to a single page.  Adding a page is the only case that
	 * needs to update the chain.
	 */
	if (argp->opcode != DB_ADD_BIG)
		goto done;

	/* Now check the previous page. */
ppage:	if (argp->prev_pgno != PGNO_INVALID) {
		REC_FGET(mpf, ip, argp->prev_pgno, &pagep, npage);
		modified = 0;

		cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
		cmp_p = LOG_COMPARE(&LSN(pagep), &argp->prevlsn);
		CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->prevlsn);
		CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);

		if (cmp_p == 0 && DB_REDO(op) && argp->opcode == DB_ADD_BIG) {
			/* Redo add, undo delete. */
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			NEXT_PGNO(pagep) = argp->pgno;
			modified = 1;
		} else if (cmp_n == 0 &&
		    DB_UNDO(op) && argp->opcode == DB_ADD_BIG) {
			/* Redo delete, undo add. */
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			NEXT_PGNO(pagep) = argp->next_pgno;
			modified = 1;
		}
		if (modified)
			LSN(pagep) = DB_REDO(op) ? *lsnp : argp->prevlsn;
		ret = __memp_fput(mpf, ip, pagep, file_dbp->priority);
		pagep = NULL;
		if (ret != 0)
			goto out;
	}
	pagep = NULL;

	/* Now check the next page.  Can only be set on a delete. */
npage:	if (argp->next_pgno != PGNO_INVALID) {
		REC_FGET(mpf, ip, argp->next_pgno, &pagep, done);
		modified = 0;

		cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
		cmp_p = LOG_COMPARE(&LSN(pagep), &argp->nextlsn);
		CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->nextlsn);
		CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
		if (cmp_p == 0 && DB_REDO(op)) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			PREV_PGNO(pagep) = PGNO_INVALID;
			modified = 1;
		} else if (cmp_n == 0 && DB_UNDO(op)) {
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			PREV_PGNO(pagep) = argp->pgno;
			modified = 1;
		}
		if (modified)
			LSN(pagep) = DB_REDO(op) ? *lsnp : argp->nextlsn;
		ret = __memp_fput(mpf, ip, pagep, file_dbp->priority);
		pagep = NULL;
		if (ret != 0)
			goto out;
	}
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __db_ovref_recover --
 *	Recovery function for __db_ovref().
 *
 * PUBLIC: int __db_ovref_recover
 * PUBLIC:     __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_ovref_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_ovref_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__db_ovref_print);
	REC_INTRO(__db_ovref_read, ip, 0);

	REC_FGET(mpf, ip, argp->pgno, &pagep, done);

	cmp = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp, &LSN(pagep), &argp->lsn);
	if (cmp == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		OV_REF(pagep) += argp->adjust;
		pagep->lsn = *lsnp;
	} else if (LOG_COMPARE(lsnp, &LSN(pagep)) == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		OV_REF(pagep) -= argp->adjust;
		pagep->lsn = argp->lsn;
	}
	ret = __memp_fput(mpf, ip, pagep, file_dbp->priority);
	pagep = NULL;
	if (ret != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __db_debug_recover --
 *	Recovery function for debug.
 *
 * PUBLIC: int __db_debug_recover __P((ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_debug_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_debug_args *argp;
	int ret;

	COMPQUIET(op, DB_TXN_ABORT);
	COMPQUIET(info, NULL);

	REC_PRINT(__db_debug_print);
	REC_NOOP_INTRO(__db_debug_read);

	*lsnp = argp->prev_lsn;
	ret = 0;

	REC_NOOP_CLOSE;
}

/*
 * __db_noop_recover --
 *	Recovery function for noop.
 *
 * PUBLIC: int __db_noop_recover __P((ENV *,
 * PUBLIC:      DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_noop_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_noop_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__db_noop_print);
	REC_INTRO(__db_noop_read, ip, 0);

	REC_FGET(mpf, ip, argp->pgno, &pagep, done);

	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->prevlsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->prevlsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);
	if (cmp_p == 0 && DB_REDO(op)) {
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		LSN(pagep) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		LSN(pagep) = argp->prevlsn;
	}
	ret = __memp_fput(mpf, ip, pagep, file_dbp->priority);
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
out:	if (pagep != NULL)
		(void)__memp_fput(mpf,
		    ip, pagep, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __db_pg_alloc_recover --
 *	Recovery function for pg_alloc.
 *
 * PUBLIC: int __db_pg_alloc_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_alloc_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_alloc_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DBMETA *meta;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pgno_t pgno;
	int cmp_n, cmp_p, created, level, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	meta = NULL;
	pagep = NULL;
	created = 0;
	REC_PRINT(__db_pg_alloc_print);
	REC_INTRO(__db_pg_alloc_read, ip, 0);

	/*
	 * Fix up the metadata page.  If we're redoing the operation, we have
	 * to get the metadata page and update its LSN and its free pointer.
	 * If we're undoing the operation and the page was ever created, we put
	 * it on the freelist.
	 */
	pgno = PGNO_BASE_MD;
	if ((ret = __memp_fget(mpf, &pgno, ip, NULL, 0, &meta)) != 0) {
		/* The metadata page must always exist on redo. */
		if (DB_REDO(op)) {
			ret = __db_pgerr(file_dbp, pgno, ret);
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
		LSN(meta) = *lsnp;
		meta->free = argp->next;
		if (argp->pgno > meta->last_pgno)
			meta->last_pgno = argp->pgno;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		LSN(meta) = argp->meta_lsn;
		/*
		 * If the page has a zero LSN then its newly created and
		 * will be truncated rather than go on the free list.
		 */
		if (!IS_ZERO_LSN(argp->page_lsn))
			meta->free = argp->pgno;
		meta->last_pgno = argp->last_pgno;
	}

#ifdef HAVE_FTRUNCATE
	/*
	 * check to see if we are keeping a sorted freelist, if so put
	 * this back in the in memory list.  It must be the first element.
	 */
	if (op == DB_TXN_ABORT && !IS_ZERO_LSN(argp->page_lsn)) {
		db_pgno_t *list;
		u_int32_t nelem;

		if ((ret = __memp_get_freelist(mpf, &nelem, &list)) != 0)
			goto out;
		if (list != NULL && (nelem == 0 || *list != argp->pgno)) {
			if ((ret =
			    __memp_extend_freelist(mpf, nelem + 1, &list)) != 0)
				goto out;
			if (nelem != 0)
				memmove(list + 1, list, nelem * sizeof(*list));
			*list = argp->pgno;
		}
	}
#endif

	/*
	 * Fix up the allocated page. If the page does not exist
	 * and we can truncate it then don't create it.
	 * Otherwise if we're redoing the operation, we have
	 * to get the page (creating it if it doesn't exist), and update its
	 * LSN.  If we're undoing the operation, we have to reset the page's
	 * LSN and put it on the free list.
	 */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		/*
		 * We have to be able to identify if a page was newly
		 * created so we can recover it properly.  We cannot simply
		 * look for an empty header, because hash uses a pgin
		 * function that will set the header.  Instead, we explicitly
		 * try for the page without CREATE and if that fails, then
		 * create it.
		 */
		if (DB_UNDO(op))
			goto do_truncate;
		if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL,
		    DB_MPOOL_CREATE, &pagep)) != 0) {
			if (DB_UNDO(op) && ret == ENOSPC)
				goto do_truncate;
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		}
		created = 1;
	}

	/* Fix up the allocated page. */
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->page_lsn);

	/*
	 * If an initial allocation is aborted and then reallocated during
	 * an archival restore the log record will have an LSN for the page
	 * but the page will be empty.
	 */
	if (IS_ZERO_LSN(LSN(pagep)))
		cmp_p = 0;

	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->page_lsn);
	/*
	 * Another special case we have to handle is if we ended up with a
	 * page of all 0's which can happen if we abort between allocating a
	 * page in mpool and initializing it.  In that case, even if we're
	 * undoing, we need to re-initialize the page.
	 */
	if (DB_REDO(op) && cmp_p == 0) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		switch (argp->ptype) {
		case P_LBTREE:
		case P_LRECNO:
		case P_LDUP:
			level = LEAFLEVEL;
			break;
		default:
			level = 0;
			break;
		}
		P_INIT(pagep, file_dbp->pgsize,
		    argp->pgno, PGNO_INVALID, PGNO_INVALID, level, argp->ptype);

		pagep->lsn = *lsnp;
	} else if (DB_UNDO(op) && (cmp_n == 0 || created)) {
		/*
		 * This is where we handle the case of a 0'd page (pagep->pgno
		 * is equal to PGNO_INVALID).
		 * Undo the allocation, reinitialize the page and
		 * link its next pointer to the free list.
		 */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize,
		    argp->pgno, PGNO_INVALID, argp->next, 0, P_INVALID);

		pagep->lsn = argp->page_lsn;
	}

do_truncate:
	/*
	 * If the page was newly created, give it back.
	 */
	if ((pagep == NULL || IS_ZERO_LSN(LSN(pagep))) &&
	    IS_ZERO_LSN(argp->page_lsn) && DB_UNDO(op)) {
		/* Discard the page. */
		if (pagep != NULL) {
			if ((ret = __memp_fput(mpf, ip,
			    pagep, DB_PRIORITY_VERY_LOW)) != 0)
				goto out;
			pagep = NULL;
		}
		/* Give the page back to the OS. */
		if (meta->last_pgno <= argp->pgno && (ret = __memp_ftruncate(
		    mpf, NULL, ip, argp->pgno, MP_TRUNC_RECOVER)) != 0)
			goto out;
	}

	if (pagep != NULL) {
		ret = __memp_fput(mpf, ip, pagep, file_dbp->priority);
		pagep = NULL;
		if (ret != 0)
			goto out;
	}

	ret = __memp_fput(mpf, ip, meta, file_dbp->priority);
	meta = NULL;
	if (ret != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	if (meta != NULL)
		(void)__memp_fput(mpf, ip, meta, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __db_pg_free_recover_int --
 */
static int
__db_pg_free_recover_int(env, ip, argp, file_dbp, lsnp, mpf, op, data)
	ENV *env;
	DB_THREAD_INFO *ip;
	__db_pg_freedata_args *argp;
	DB *file_dbp;
	DB_LSN *lsnp;
	DB_MPOOLFILE *mpf;
	db_recops op;
	int data;
{
	DBMETA *meta;
	DB_LSN copy_lsn;
	PAGE *pagep, *prevp;
	int cmp_n, cmp_p, is_meta, ret;

	meta = NULL;
	pagep = prevp = NULL;

	/*
	 * Get the "metapage".  This will either be the metapage
	 * or the previous page in the free list if we are doing
	 * sorted allocations.  If its a previous page then
	 * we will not be truncating.
	 */
	is_meta = argp->meta_pgno == PGNO_BASE_MD;

	REC_FGET(mpf, ip, argp->meta_pgno, &meta, check_meta);

	if (argp->meta_pgno != PGNO_BASE_MD)
		prevp = (PAGE *)meta;

	cmp_n = LOG_COMPARE(lsnp, &LSN(meta));
	cmp_p = LOG_COMPARE(&LSN(meta), &argp->meta_lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(meta), &argp->meta_lsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(meta), lsnp);

	/*
	 * Fix up the metadata page.  If we're redoing or undoing the operation
	 * we get the page and update its LSN, last and free pointer.
	 */
	if (cmp_p == 0 && DB_REDO(op)) {
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		/*
		 * If we are at the end of the file truncate, otherwise
		 * put on the free list.
		*/
		if (argp->pgno == argp->last_pgno)
			meta->last_pgno = argp->pgno - 1;
		else if (is_meta)
			meta->free = argp->pgno;
		else
			NEXT_PGNO(prevp) = argp->pgno;
		LSN(meta) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo the deallocation. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		if (is_meta) {
			if (meta->last_pgno < argp->pgno)
				meta->last_pgno = argp->pgno;
			meta->free = argp->next;
		} else
			NEXT_PGNO(prevp) = argp->next;
		LSN(meta) = argp->meta_lsn;
	}

check_meta:
	if (ret != 0 && is_meta) {
		/* The metadata page must always exist. */
		ret = __db_pgerr(file_dbp, argp->meta_pgno, ret);
		goto out;
	}

	/*
	 * Get the freed page.  Don't create the page if we are going to
	 * free it.  If we're redoing the operation we get the page and
	 * explicitly discard its contents, then update its LSN. If we're
	 * undoing the operation, we get the page and restore its header.
	 */
	if (DB_REDO(op) || (is_meta && meta->last_pgno < argp->pgno)) {
		if ((ret = __memp_fget(mpf, &argp->pgno,
		    ip, NULL, 0, &pagep)) != 0) {
			if (ret != DB_PAGE_NOTFOUND)
				goto out;
			if (is_meta &&
			    DB_REDO(op) && meta->last_pgno <= argp->pgno)
				goto trunc;
			goto done;
		}
	} else if ((ret = __memp_fget(mpf, &argp->pgno,
	   ip, NULL, DB_MPOOL_CREATE, &pagep)) != 0)
		goto out;

	(void)__ua_memcpy(&copy_lsn, &LSN(argp->header.data), sizeof(DB_LSN));
	cmp_n = IS_ZERO_LSN(LSN(pagep)) ? 0 : LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &copy_lsn);

	/*
	 * This page got extended by a later allocation,
	 * but its allocation was not in the scope of this
	 * recovery pass.
	 */
	if (IS_ZERO_LSN(LSN(pagep)))
		cmp_p = 0;

	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &copy_lsn);
	if (DB_REDO(op) &&
	    (cmp_p == 0 ||
	    (IS_ZERO_LSN(copy_lsn) &&
	    LOG_COMPARE(&LSN(pagep), &argp->meta_lsn) <= 0))) {
		/* Need to redo the deallocation. */
		/*
		 * The page can be truncated if it was truncated at runtime
		 * and the current metapage reflects the truncation.
		 */
		if (is_meta && meta->last_pgno <= argp->pgno &&
		    argp->last_pgno <= argp->pgno) {
			if ((ret = __memp_fput(mpf, ip,
			    pagep, DB_PRIORITY_VERY_LOW)) != 0)
				goto out;
			pagep = NULL;
trunc:			if ((ret = __memp_ftruncate(mpf, NULL, ip,
			    argp->pgno, MP_TRUNC_RECOVER)) != 0)
				goto out;
		} else if (argp->last_pgno == argp->pgno) {
			/* The page was truncated at runtime, zero it out. */
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			P_INIT(pagep, 0, PGNO_INVALID,
			    PGNO_INVALID, PGNO_INVALID, 0, P_INVALID);
			ZERO_LSN(pagep->lsn);
		} else {
			REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
			P_INIT(pagep, file_dbp->pgsize,
			    argp->pgno, PGNO_INVALID, argp->next, 0, P_INVALID);
			pagep->lsn = *lsnp;

		}
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to reallocate the page. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		memcpy(pagep, argp->header.data, argp->header.size);
		if (data)
			memcpy((u_int8_t*)pagep + HOFFSET(pagep),
			     argp->data.data, argp->data.size);
	}
	if (pagep != NULL &&
	    (ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;

	pagep = NULL;
#ifdef HAVE_FTRUNCATE
	/*
	 * If we are keeping an in memory free list remove this
	 * element from the list.
	 */
	if (op == DB_TXN_ABORT && argp->pgno != argp->last_pgno) {
		db_pgno_t *lp;
		u_int32_t nelem, pos;

		if ((ret = __memp_get_freelist(mpf, &nelem, &lp)) != 0)
			goto out;
		if (lp != NULL) {
			pos = 0;
			if (!is_meta) {
				__db_freelist_pos(argp->pgno, lp, nelem, &pos);

				/* 
				 * If we aborted after logging but before
				 * updating the free list don't do anything.
				*/
				if (argp->pgno != lp[pos]) {
					DB_ASSERT(env,
					    argp->meta_pgno == lp[pos]);
					goto done;
				}
				DB_ASSERT(env,
				    argp->meta_pgno == lp[pos - 1]);
			} else if (nelem != 0 && argp->pgno != lp[pos])
				goto done;

			if (pos < nelem)
				memmove(&lp[pos], &lp[pos + 1],
				    ((nelem - pos) - 1) * sizeof(*lp));

			/* Shrink the list */
			if ((ret =
			    __memp_extend_freelist(mpf, nelem - 1, &lp)) != 0)
				goto out;
		}
	}
#endif
done:
	if (meta != NULL &&
	     (ret = __memp_fput(mpf, ip,  meta, file_dbp->priority)) != 0)
		goto out;
	meta = NULL;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip,  pagep, file_dbp->priority);
	if (meta != NULL)
		(void)__memp_fput(mpf, ip,  meta, file_dbp->priority);

	return (ret);
}

/*
 * __db_pg_free_recover --
 *	Recovery function for pg_free.
 *
 * PUBLIC: int __db_pg_free_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_free_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_free_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	int ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_free_print);
	REC_INTRO(__db_pg_free_read, ip, 0);

	ret = __db_pg_free_recover_int(env, ip,
	     (__db_pg_freedata_args *)argp, file_dbp, lsnp, mpf, op, 0);

done:	*lsnp = argp->prev_lsn;
out:
	REC_CLOSE;
}

/*
 * __db_pg_freedata_recover --
 *	Recovery function for pg_freedata.
 *
 * PUBLIC: int __db_pg_freedata_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_freedata_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_freedata_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	int ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_freedata_print);
	REC_INTRO(__db_pg_freedata_read, ip, 0);

	ret = __db_pg_free_recover_int(env,
	    ip, argp, file_dbp, lsnp, mpf, op, 1);

done:	*lsnp = argp->prev_lsn;
out:
	REC_CLOSE;
}

/*
 * __db_cksum_recover --
 *	Recovery function for checksum failure log record.
 *
 * PUBLIC: int __db_cksum_recover __P((ENV *,
 * PUBLIC:      DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_cksum_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_cksum_args *argp;
	int ret;

	COMPQUIET(info, NULL);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(op, DB_TXN_ABORT);

	REC_PRINT(__db_cksum_print);

	if ((ret = __db_cksum_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	/*
	 * We had a checksum failure -- the only option is to run catastrophic
	 * recovery.
	 */
	if (F_ISSET(env, ENV_RECOVER_FATAL))
		ret = 0;
	else {
		__db_errx(env,
		    "Checksum failure requires catastrophic recovery");
		ret = __env_panic(env, DB_RUNRECOVERY);
	}

	__os_free(env, argp);
	return (ret);
}

/*
 * __db_pg_init_recover --
 *	Recovery function to reinit pages after truncation.
 *
 * PUBLIC: int __db_pg_init_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_init_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_init_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_LSN copy_lsn;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, ret, type;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_init_print);
	REC_INTRO(__db_pg_init_read, ip, 0);

	mpf = file_dbp->mpf;
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			if (ret == DB_PAGE_NOTFOUND)
				goto done;
			else {
				ret = __db_pgerr(file_dbp, argp->pgno, ret);
				goto out;
			}
		}

		/*
		 * This page was truncated and may simply not have
		 * had an item written to it yet.  This should only
		 * happen on hash databases, so confirm that.
		 */
		DB_ASSERT(env, file_dbp->type == DB_HASH);
		if ((ret = __memp_fget(mpf, &argp->pgno,
		    ip, NULL, DB_MPOOL_CREATE, &pagep)) != 0) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		}
	}

	(void)__ua_memcpy(&copy_lsn, &LSN(argp->header.data), sizeof(DB_LSN));
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &copy_lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &copy_lsn);
	CHECK_ABORT(env, op, cmp_n, &LSN(pagep), lsnp);

	if (cmp_p == 0 && DB_REDO(op)) {
		if (TYPE(pagep) == P_HASH)
			type = P_HASH;
		else
			type = file_dbp->type == DB_RECNO ? P_LRECNO : P_LBTREE;
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize, PGNO(pagep), PGNO_INVALID,
		    PGNO_INVALID, TYPE(pagep) == P_HASH ? 0 : 1, type);
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Put the data back on the page. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		memcpy(pagep, argp->header.data, argp->header.size);
		if (argp->data.size > 0)
			memcpy((u_int8_t*)pagep + HOFFSET(pagep),
			     argp->data.data, argp->data.size);
	}
	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
out:
	REC_CLOSE;
}

/*
 * __db_pg_trunc_recover --
 *	Recovery function for pg_trunc.
 *
 * PUBLIC: int __db_pg_trunc_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_trunc_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
#ifdef HAVE_FTRUNCATE
	__db_pg_trunc_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DBMETA *meta;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pglist_t *pglist, *lp;
	db_pgno_t last_pgno, *list;
	u_int32_t felem, nelem, pos;
	int ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_trunc_print);
	REC_INTRO(__db_pg_trunc_read, ip, 1);

	pglist = (db_pglist_t *) argp->list.data;
	nelem = argp->list.size / sizeof(db_pglist_t);
	if (DB_REDO(op)) {
		/*
		 * First call __db_pg_truncate to find the truncation
		 * point, truncate the file and return the new last_pgno.
		 */
		last_pgno = argp->last_pgno;
		if ((ret = __db_pg_truncate(dbc, NULL, pglist,
		    NULL, &nelem, argp->next_free, &last_pgno, lsnp, 1)) != 0)
			goto out;

		if (argp->last_free != PGNO_INVALID) {
			/* 
			 * Update the next pointer of the last page in
			 * the freelist.  If the truncation point is
			 * beyond next_free then this is still in the freelist
			 * otherwise the last_free page is at the end.
			 */
			if ((ret = __memp_fget(mpf,
			    &argp->last_free, ip, NULL, 0, &meta)) == 0) {
				if (LOG_COMPARE(&LSN(meta),
				     &argp->last_lsn) == 0) {
					REC_DIRTY(mpf,
					    ip, dbc->priority, &meta);
					if (pglist->pgno > last_pgno)
						NEXT_PGNO(meta) = PGNO_INVALID;
					else
						NEXT_PGNO(meta) = pglist->pgno;
					LSN(meta) = *lsnp;
				}
				if ((ret = __memp_fput(mpf, ip,
				    meta, file_dbp->priority)) != 0)
					goto out;
				meta = NULL;
			} else if (ret != DB_PAGE_NOTFOUND)
				goto out;
		}
		if ((ret = __memp_fget(mpf, &argp->meta, ip, NULL,
		    0, &meta)) != 0)
			goto out;
		if (LOG_COMPARE(&LSN(meta), &argp->meta_lsn) == 0) {
			REC_DIRTY(mpf, ip, dbc->priority, &meta);
			if (argp->last_free == PGNO_INVALID) {
				if (nelem == 0)
					meta->free = PGNO_INVALID;
				else
					meta->free = pglist->pgno;
			}
			meta->last_pgno = last_pgno;
			LSN(meta) = *lsnp;
		}
	} else {
		/* Put the free list back in its original order. */
		for (lp = pglist; lp < &pglist[nelem]; lp++) {
			if ((ret = __memp_fget(mpf, &lp->pgno, ip,
			    NULL, DB_MPOOL_CREATE, &pagep)) != 0)
				goto out;
			if (IS_ZERO_LSN(LSN(pagep)) ||
			     LOG_COMPARE(&LSN(pagep), lsnp) == 0) {
				REC_DIRTY(mpf, ip, dbc->priority, &pagep);
				P_INIT(pagep, file_dbp->pgsize, lp->pgno,
				    PGNO_INVALID, lp->next_pgno, 0, P_INVALID);
				LSN(pagep) = lp->lsn;
			}
			if ((ret = __memp_fput(mpf,
			    ip, pagep, file_dbp->priority)) != 0)
				goto out;
		}
		/*
		 * Link the truncated part back into the free list.
		 * Its either after the last_free page or direclty
		 * linked to the metadata page.
		 */
		if (argp->last_free != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf, &argp->last_free,
			    ip, NULL, DB_MPOOL_EDIT, &meta)) == 0) {
				if (LOG_COMPARE(&LSN(meta), lsnp) == 0) {
					NEXT_PGNO(meta) = argp->next_free;
					LSN(meta) = argp->last_lsn;
				}
				if ((ret = __memp_fput(mpf, ip,
				    meta, file_dbp->priority)) != 0)
					goto out;
			} else if (ret != DB_PAGE_NOTFOUND)
				goto out;
			meta = NULL;
		}
		if ((ret = __memp_fget(mpf, &argp->meta,
		    ip, NULL, DB_MPOOL_EDIT, &meta)) != 0)
			goto out;
		if (LOG_COMPARE(&LSN(meta), lsnp) == 0) {
			REC_DIRTY(mpf, ip, dbc->priority, &meta);
			/*
			 * If we had to break up the list last_pgno
			 * may only represent the end of the block.
			 */
			if (meta->last_pgno < argp->last_pgno)
				meta->last_pgno = argp->last_pgno;
			if (argp->last_free == PGNO_INVALID)
				meta->free = argp->next_free;
			LSN(meta) = argp->meta_lsn;
		}
	}

	if ((ret = __memp_fput(mpf, ip, meta, file_dbp->priority)) != 0)
		goto out;

	if (op == DB_TXN_ABORT) {
		/*
		 * Put the pages back on the in memory free list.
		 * If this is part of a multi-record truncate then
		 * we need to find this batch, it may not be at the end.
		 * If we aborted while writing one of the log records
		 * then this set may still be in the list.
		 */
		if ((ret = __memp_get_freelist(mpf, &felem, &list)) != 0)
			goto out;
		if (list != NULL) {
			if (felem != 0 && list[felem - 1] > pglist->pgno) {
				__db_freelist_pos(
				    pglist->pgno, list, felem, &pos);
				DB_ASSERT(env, pos < felem);
				if (pglist->pgno == list[pos])
					goto done;
				pos++;
			} else if (felem != 0 &&
			    list[felem - 1] == pglist->pgno)
				goto done;
			else 
				pos = felem;
			if ((ret = __memp_extend_freelist(
			    mpf, felem + nelem, &list)) != 0)
				goto out;
			if (pos != felem)
				memmove(&list[nelem + pos], &list[pos],
				    sizeof(*list) * (felem - pos));
			for (lp = pglist; lp < &pglist[nelem]; lp++)
				list[pos++] = lp->pgno;
		}
	}

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	REC_CLOSE;
#else
	/*
	 * If HAVE_FTRUNCATE is not defined, we'll never see pg_trunc records
	 * to recover.
	 */
	COMPQUIET(env, NULL);
	COMPQUIET(dbtp, NULL);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(op,  DB_TXN_ABORT);
	COMPQUIET(info, NULL);
	return (EINVAL);
#endif
}
/*
 * __db_pg_sort_44_recover --
 *	Recovery function for pg_sort.
 * This is deprecated and kept for replication upgrades.
 *
 * PUBLIC: int __db_pg_sort_44_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_sort_44_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
#ifdef HAVE_FTRUNCATE
	__db_pg_sort_44_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DBMETA *meta;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pglist_t *pglist, *lp;
	db_pgno_t pgno, *list;
	u_int32_t felem, nelem;
	int ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_sort_44_print);
	REC_INTRO(__db_pg_sort_44_read, ip, 1);

	pglist = (db_pglist_t *) argp->list.data;
	nelem = argp->list.size / sizeof(db_pglist_t);
	if (DB_REDO(op)) {
		pgno = argp->last_pgno;
		__db_freelist_sort(pglist, nelem);
		if ((ret = __db_pg_truncate(dbc, NULL,
		    pglist, NULL, &nelem, PGNO_INVALID, &pgno, lsnp, 1)) != 0)
			goto out;

		if (argp->last_free != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf,
			    &argp->last_free, ip, NULL, 0, &meta)) == 0) {
				if (LOG_COMPARE(&LSN(meta),
				     &argp->last_lsn) == 0) {
					REC_DIRTY(mpf,
					    ip, dbc->priority, &meta);
					NEXT_PGNO(meta) = PGNO_INVALID;
					LSN(meta) = *lsnp;
				}
				if ((ret = __memp_fput(mpf, ip,
				    meta, file_dbp->priority)) != 0)
					goto out;
				meta = NULL;
			} else if (ret != DB_PAGE_NOTFOUND)
				goto out;
		}
		if ((ret = __memp_fget(mpf, &argp->meta, ip, NULL,
		    0, &meta)) != 0)
			goto out;
		if (LOG_COMPARE(&LSN(meta), &argp->meta_lsn) == 0) {
			REC_DIRTY(mpf, ip, dbc->priority, &meta);
			if (argp->last_free == PGNO_INVALID) {
				if (nelem == 0)
					meta->free = PGNO_INVALID;
				else
					meta->free = pglist->pgno;
			}
			meta->last_pgno = pgno;
			LSN(meta) = *lsnp;
		}
	} else {
		/* Put the free list back in its original order. */
		for (lp = pglist; lp < &pglist[nelem]; lp++) {
			if ((ret = __memp_fget(mpf, &lp->pgno, ip,
			    NULL, DB_MPOOL_CREATE, &pagep)) != 0)
				goto out;
			if (IS_ZERO_LSN(LSN(pagep)) ||
			     LOG_COMPARE(&LSN(pagep), lsnp) == 0) {
				REC_DIRTY(mpf, ip, dbc->priority, &pagep);
				if (lp == &pglist[nelem - 1])
					pgno = PGNO_INVALID;
				else
					pgno = lp[1].pgno;

				P_INIT(pagep, file_dbp->pgsize,
				    lp->pgno, PGNO_INVALID, pgno, 0, P_INVALID);
				LSN(pagep) = lp->lsn;
			}
			if ((ret = __memp_fput(mpf,
			    ip, pagep, file_dbp->priority)) != 0)
				goto out;
		}
		if (argp->last_free != PGNO_INVALID) {
			if ((ret = __memp_fget(mpf, &argp->last_free,
			    ip, NULL, DB_MPOOL_EDIT, &meta)) == 0) {
				if (LOG_COMPARE(&LSN(meta), lsnp) == 0) {
					NEXT_PGNO(meta) = pglist->pgno;
					LSN(meta) = argp->last_lsn;
				}
				if ((ret = __memp_fput(mpf, ip,
				    meta, file_dbp->priority)) != 0)
					goto out;
			} else if (ret != DB_PAGE_NOTFOUND)
				goto out;
			meta = NULL;
		}
		if ((ret = __memp_fget(mpf, &argp->meta,
		    ip, NULL, DB_MPOOL_EDIT, &meta)) != 0)
			goto out;
		if (LOG_COMPARE(&LSN(meta), lsnp) == 0) {
			REC_DIRTY(mpf, ip, dbc->priority, &meta);
			meta->last_pgno = argp->last_pgno;
			if (argp->last_free == PGNO_INVALID)
				meta->free = pglist->pgno;
			LSN(meta) = argp->meta_lsn;
		}
	}
	if (op == DB_TXN_ABORT) {
		if ((ret = __memp_get_freelist(mpf, &felem, &list)) != 0)
			goto out;
		if (list != NULL) {
			DB_ASSERT(env, felem == 0 ||
			    argp->last_free == list[felem - 1]);
			if ((ret = __memp_extend_freelist(
			    mpf, felem + nelem, &list)) != 0)
				goto out;
			for (lp = pglist; lp < &pglist[nelem]; lp++)
				list[felem++] = lp->pgno;
		}
	}

	if ((ret = __memp_fput(mpf, ip, meta, file_dbp->priority)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	REC_CLOSE;
#else
	/*
	 * If HAVE_FTRUNCATE is not defined, we'll never see pg_sort records
	 * to recover.
	 */
	COMPQUIET(env, NULL);
	COMPQUIET(dbtp, NULL);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(op,  DB_TXN_ABORT);
	COMPQUIET(info, NULL);
	return (EINVAL);
#endif
}

/*
 * __db_pg_alloc_42_recover --
 *	Recovery function for pg_alloc.
 *
 * PUBLIC: int __db_pg_alloc_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_alloc_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_alloc_42_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DBMETA *meta;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pgno_t pgno;
	int cmp_n, cmp_p, created, level, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	meta = NULL;
	pagep = NULL;
	created = 0;
	REC_PRINT(__db_pg_alloc_42_print);
	REC_INTRO(__db_pg_alloc_42_read, ip, 0);

	/*
	 * Fix up the metadata page.  If we're redoing the operation, we have
	 * to get the metadata page and update its LSN and its free pointer.
	 * If we're undoing the operation and the page was ever created, we put
	 * it on the freelist.
	 */
	pgno = PGNO_BASE_MD;
	if ((ret = __memp_fget(mpf, &pgno, ip, NULL, 0, &meta)) != 0) {
		/* The metadata page must always exist on redo. */
		if (DB_REDO(op)) {
			ret = __db_pgerr(file_dbp, pgno, ret);
			goto out;
		} else
			goto done;
	}
	cmp_n = LOG_COMPARE(lsnp, &LSN(meta));
	cmp_p = LOG_COMPARE(&LSN(meta), &argp->meta_lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(meta), &argp->meta_lsn);
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		LSN(meta) = *lsnp;
		meta->free = argp->next;
		if (argp->pgno > meta->last_pgno)
			meta->last_pgno = argp->pgno;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		goto no_rollback;
	}

	/*
	 * Fix up the allocated page. If the page does not exist
	 * and we can truncate it then don't create it.
	 * Otherwise if we're redoing the operation, we have
	 * to get the page (creating it if it doesn't exist), and update its
	 * LSN.  If we're undoing the operation, we have to reset the page's
	 * LSN and put it on the free list, or truncate it.
	 */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		/*
		 * We have to be able to identify if a page was newly
		 * created so we can recover it properly.  We cannot simply
		 * look for an empty header, because hash uses a pgin
		 * function that will set the header.  Instead, we explicitly
		 * try for the page without CREATE and if that fails, then
		 * create it.
		 */
		if ((ret = __memp_fget(mpf, &argp->pgno,
		    ip, NULL, DB_MPOOL_CREATE, &pagep)) != 0) {
			if (DB_UNDO(op) && ret == ENOSPC)
				goto do_truncate;
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		}
		created = 1;
	}

	/* Fix up the allocated page. */
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->page_lsn);

	/*
	 * If an initial allocation is aborted and then reallocated during
	 * an archival restore the log record will have an LSN for the page
	 * but the page will be empty.
	 */
	if (IS_ZERO_LSN(LSN(pagep)) ||
	    (IS_ZERO_LSN(argp->page_lsn) && IS_INIT_LSN(LSN(pagep))))
		cmp_p = 0;

	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->page_lsn);
	/*
	 * Another special case we have to handle is if we ended up with a
	 * page of all 0's which can happen if we abort between allocating a
	 * page in mpool and initializing it.  In that case, even if we're
	 * undoing, we need to re-initialize the page.
	 */
	if (DB_REDO(op) && cmp_p == 0) {
		/* Need to redo update described. */
		switch (argp->ptype) {
		case P_LBTREE:
		case P_LRECNO:
		case P_LDUP:
			level = LEAFLEVEL;
			break;
		default:
			level = 0;
			break;
		}
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize,
		    argp->pgno, PGNO_INVALID, PGNO_INVALID, level, argp->ptype);

		pagep->lsn = *lsnp;
	} else if (DB_UNDO(op) && (cmp_n == 0 || created)) {
		/*
		 * This is where we handle the case of a 0'd page (pagep->pgno
		 * is equal to PGNO_INVALID).
		 * Undo the allocation, reinitialize the page and
		 * link its next pointer to the free list.
		 */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize,
		    argp->pgno, PGNO_INVALID, argp->next, 0, P_INVALID);

		pagep->lsn = argp->page_lsn;
	}

do_truncate:
	/*
	 * We cannot undo things from 4.2 land, because we nolonger
	 * have limbo processing.
	 */
	if ((pagep == NULL || IS_ZERO_LSN(LSN(pagep))) &&
	    IS_ZERO_LSN(argp->page_lsn) && DB_UNDO(op)) {
no_rollback:	__db_errx(env,
"Cannot replicate prepared transactions from master running release 4.2 ");
		ret = __env_panic(env, EINVAL);
	}

	if (pagep != NULL &&
	    (ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

	if ((ret = __memp_fput(mpf, ip, meta, file_dbp->priority)) != 0)
		goto out;
	meta = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	if (meta != NULL)
		(void)__memp_fput(mpf, ip, meta, file_dbp->priority);
	REC_CLOSE;
}

/*
 * __db_pg_free_recover_42_int --
 */
static int
__db_pg_free_recover_42_int(env, ip, argp, file_dbp, lsnp, mpf, op, data)
	ENV *env;
	DB_THREAD_INFO *ip;
	__db_pg_freedata_42_args *argp;
	DB *file_dbp;
	DB_LSN *lsnp;
	DB_MPOOLFILE *mpf;
	db_recops op;
	int data;
{
	DBMETA *meta;
	DB_LSN copy_lsn;
	PAGE *pagep, *prevp;
	int cmp_n, cmp_p, is_meta, ret;

	meta = NULL;
	pagep = NULL;
	prevp = NULL;

	/*
	 * Get the "metapage".  This will either be the metapage
	 * or the previous page in the free list if we are doing
	 * sorted allocations.  If its a previous page then
	 * we will not be truncating.
	 */
	is_meta = argp->meta_pgno == PGNO_BASE_MD;

	REC_FGET(mpf, ip, argp->meta_pgno, &meta, check_meta);

	if (argp->meta_pgno != PGNO_BASE_MD)
		prevp = (PAGE *)meta;

	cmp_n = LOG_COMPARE(lsnp, &LSN(meta));
	cmp_p = LOG_COMPARE(&LSN(meta), &argp->meta_lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(meta), &argp->meta_lsn);

	/*
	 * Fix up the metadata page.  If we're redoing or undoing the operation
	 * we get the page and update its LSN, last and free pointer.
	 */
	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo the deallocation. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		if (prevp == NULL)
			meta->free = argp->pgno;
		else
			NEXT_PGNO(prevp) = argp->pgno;
		/*
		 * If this was a compensating transaction and
		 * we are a replica, then we never executed the
		 * original allocation which incremented meta->free.
		 */
		if (prevp == NULL && meta->last_pgno < meta->free)
			meta->last_pgno = meta->free;
		LSN(meta) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo the deallocation. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &meta);
		if (prevp == NULL)
			meta->free = argp->next;
		else
			NEXT_PGNO(prevp) = argp->next;
		LSN(meta) = argp->meta_lsn;
		if (prevp == NULL && meta->last_pgno < argp->pgno)
			meta->last_pgno = argp->pgno;
	}

check_meta:
	if (ret != 0 && is_meta) {
		/* The metadata page must always exist. */
		ret = __db_pgerr(file_dbp, argp->meta_pgno, ret);
		goto out;
	}

	/*
	 * Get the freed page.  If we support truncate then don't
	 * create the page if we are going to free it.  If we're
	 * redoing the operation we get the page and explicitly discard
	 * its contents, then update its LSN.  If we're undoing the
	 * operation, we get the page and restore its header.
	 * If we don't support truncate, then we must create the page
	 * and roll it back.
	 */
	if ((ret = __memp_fget(mpf, &argp->pgno,
	    ip, NULL, DB_MPOOL_CREATE, &pagep)) != 0)
		goto out;

	(void)__ua_memcpy(&copy_lsn, &LSN(argp->header.data), sizeof(DB_LSN));
	cmp_n = IS_ZERO_LSN(LSN(pagep)) ? 0 : LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &copy_lsn);

	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &copy_lsn);
	if (DB_REDO(op) &&
	    (cmp_p == 0 ||
	    (IS_ZERO_LSN(copy_lsn) &&
	    LOG_COMPARE(&LSN(pagep), &argp->meta_lsn) <= 0))) {
		/* Need to redo the deallocation. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		P_INIT(pagep, file_dbp->pgsize,
		    argp->pgno, PGNO_INVALID, argp->next, 0, P_INVALID);
		pagep->lsn = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to reallocate the page. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		memcpy(pagep, argp->header.data, argp->header.size);
		if (data)
			memcpy((u_int8_t*)pagep + HOFFSET(pagep),
			     argp->data.data, argp->data.size);
	}
	if (pagep != NULL &&
	    (ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;

	pagep = NULL;
	if (meta != NULL &&
	    (ret = __memp_fput(mpf, ip, meta, file_dbp->priority)) != 0)
		goto out;
	meta = NULL;

	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	if (meta != NULL)
		(void)__memp_fput(mpf, ip, meta, file_dbp->priority);

	return (ret);
}

/*
 * __db_pg_free_42_recover --
 *	Recovery function for pg_free.
 *
 * PUBLIC: int __db_pg_free_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_free_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_free_42_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	int ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_free_42_print);
	REC_INTRO(__db_pg_free_42_read, ip, 0);

	ret = __db_pg_free_recover_42_int(env, ip,
	     (__db_pg_freedata_42_args *)argp, file_dbp, lsnp, mpf, op, 0);

done:	*lsnp = argp->prev_lsn;
out:
	REC_CLOSE;
}

/*
 * __db_pg_freedata_42_recover --
 *	Recovery function for pg_freedata.
 *
 * PUBLIC: int __db_pg_freedata_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_pg_freedata_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_pg_freedata_42_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	int ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	REC_PRINT(__db_pg_freedata_42_print);
	REC_INTRO(__db_pg_freedata_42_read, ip, 0);

	ret = __db_pg_free_recover_42_int(
	    env, ip, argp, file_dbp, lsnp, mpf, op, 1);

done:	*lsnp = argp->prev_lsn;
out:
	REC_CLOSE;
}

/*
 * __db_relink_42_recover --
 *	Recovery function for relink.
 *
 * PUBLIC: int __db_relink_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__db_relink_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__db_relink_42_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, modified, ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__db_relink_42_print);
	REC_INTRO(__db_relink_42_read, ip, 0);

	/*
	 * There are up to three pages we need to check -- the page, and the
	 * previous and next pages, if they existed.  For a page add operation,
	 * the current page is the result of a split and is being recovered
	 * elsewhere, so all we need do is recover the next page.
	 */
	if ((ret = __memp_fget(mpf, &argp->pgno, ip, NULL, 0, &pagep)) != 0) {
		if (DB_REDO(op)) {
			ret = __db_pgerr(file_dbp, argp->pgno, ret);
			goto out;
		}
		goto next2;
	}
	if (argp->opcode == DB_ADD_PAGE_COMPAT)
		goto next1;

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
next1:	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

next2:	if ((ret = __memp_fget(mpf, &argp->next, ip, NULL, 0, &pagep)) != 0) {
		if (DB_REDO(op)) {
			ret = __db_pgerr(file_dbp, argp->next, ret);
			goto out;
		}
		goto prev;
	}
	modified = 0;
	cmp_n = LOG_COMPARE(lsnp, &LSN(pagep));
	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn_next);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn_next);
	if ((argp->opcode == DB_REM_PAGE_COMPAT && cmp_p == 0 && DB_REDO(op)) ||
	    (argp->opcode == DB_ADD_PAGE_COMPAT && cmp_n == 0 && DB_UNDO(op))) {
		/* Redo the remove or undo the add. */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		pagep->prev_pgno = argp->prev;
		modified = 1;
	} else if ((argp->opcode == DB_REM_PAGE_COMPAT &&
	    cmp_n == 0 && DB_UNDO(op)) ||
	    (argp->opcode == DB_ADD_PAGE_COMPAT && cmp_p == 0 && DB_REDO(op))) {
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
	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;
	if (argp->opcode == DB_ADD_PAGE_COMPAT)
		goto done;

prev:	if ((ret = __memp_fget(mpf, &argp->prev, ip, NULL, 0, &pagep)) != 0) {
		if (DB_REDO(op)) {
			ret = __db_pgerr(file_dbp, argp->prev, ret);
			goto out;
		}
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
	if ((ret = __memp_fput(mpf, ip, pagep, file_dbp->priority)) != 0)
		goto out;
	pagep = NULL;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL)
		(void)__memp_fput(mpf, ip, pagep, file_dbp->priority);
	REC_CLOSE;
}
