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
#include "dbinc/fop.h"
#include "dbinc/hash.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

/*
 * __crdel_metasub_recover --
 *	Recovery function for metasub.
 *
 * PUBLIC: int __crdel_metasub_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__crdel_metasub_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__crdel_metasub_args *argp;
	DB_THREAD_INFO *ip;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_p, ret, t_ret;

	ip = ((DB_TXNHEAD *)info)->thread_info;
	pagep = NULL;
	REC_PRINT(__crdel_metasub_print);
	REC_INTRO(__crdel_metasub_read, ip, 0);

	/*
	 * If we are undoing this operation, but the DB that we got back
	 * was never really opened, then this open was an in-memory open
	 * that did not finish. We can let the file creation take care
	 * of any necessary undo/cleanup.
	 */
	if (DB_UNDO(op) && !F_ISSET(file_dbp, DB_AM_OPEN_CALLED))
		goto done;

	if ((ret = __memp_fget(mpf, &argp->pgno,
	    ip, NULL, 0, &pagep)) != 0) {
		/* If this is an in-memory file, this might be OK. */
		if (F_ISSET(file_dbp, DB_AM_INMEM) &&
		    (ret = __memp_fget(mpf, &argp->pgno, ip, NULL,
		    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &pagep)) == 0) {
			LSN_NOT_LOGGED(LSN(pagep));
		} else {
			*lsnp = argp->prev_lsn;
			ret = 0;
			goto out;
		}
	}

	cmp_p = LOG_COMPARE(&LSN(pagep), &argp->lsn);
	CHECK_LSN(env, op, cmp_p, &LSN(pagep), &argp->lsn);

	if (cmp_p == 0 && DB_REDO(op)) {
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		memcpy(pagep, argp->page.data, argp->page.size);
		LSN(pagep) = *lsnp;

		/*
		 * If this was an in-memory database and we are re-creating
		 * and this is the meta-data page, then we need to set up a
		 * bunch of fields in the dbo as well.
		 */
		if (F_ISSET(file_dbp, DB_AM_INMEM) &&
		    argp->pgno == PGNO_BASE_MD &&
		    (ret = __db_meta_setup(file_dbp->env, file_dbp,
		    file_dbp->dname, (DBMETA *)pagep, 0, DB_CHK_META)) != 0)
			goto out;
	} else if (DB_UNDO(op)) {
		/*
		 * We want to undo this page creation.  The page creation
		 * happened in two parts.  First, we called __db_pg_alloc which
		 * was logged separately. Then we wrote the meta-data onto
		 * the page.  So long as we restore the LSN, then the recovery
		 * for __db_pg_alloc will do everything else.
		 *
		 * Don't bother checking the lsn on the page.  If we are
		 * rolling back the next thing is that this page will get
		 * freed.  Opening the subdb will have reinitialized the
		 * page, but not the lsn.
		 */
		REC_DIRTY(mpf, ip, file_dbp->priority, &pagep);
		LSN(pagep) = argp->lsn;
	}

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (pagep != NULL && (t_ret = __memp_fput(mpf,
	     ip, pagep, file_dbp->priority)) != 0 &&
	    ret == 0)
		ret = t_ret;

	REC_CLOSE;
}

/*
 * __crdel_inmem_create_recover --
 *	Recovery function for inmem_create.
 *
 * PUBLIC: int __crdel_inmem_create_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__crdel_inmem_create_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__crdel_inmem_create_args *argp;
	DB *dbp;
	int do_close, ret, t_ret;

	COMPQUIET(info, NULL);

	dbp = NULL;
	do_close = 0;
	REC_PRINT(__crdel_inmem_create_print);
	REC_NOOP_INTRO(__crdel_inmem_create_read);

	/* First, see if the DB handle already exists. */
	if (argp->fileid == DB_LOGFILEID_INVALID) {
		if (DB_REDO(op))
			ret = ENOENT;
		else
			ret = 0;
	} else
		ret = __dbreg_id_to_db(env, argp->txnp, &dbp, argp->fileid, 0);

	if (DB_REDO(op)) {
		/*
		 * If the dbreg failed, that means that we're creating a
		 * tmp file.
		 */
		if (ret != 0) {
			if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
				goto out;

			F_SET(dbp, DB_AM_RECOVER | DB_AM_INMEM);
			memcpy(dbp->fileid, argp->fid.data, DB_FILE_ID_LEN);
			if (((ret = __os_strdup(env,
			    argp->name.data, &dbp->dname)) != 0))
				goto out;

			/*
			 * This DBP is never going to be entered into the
			 * dbentry table, so if we leave it open here,
			 * then we're going to lose it.
			 */
			do_close = 1;
		}

		/* Now, set the fileid. */
		memcpy(dbp->fileid, argp->fid.data, argp->fid.size);
		if ((ret = __memp_set_fileid(dbp->mpf, dbp->fileid)) != 0)
			goto out;
		dbp->preserve_fid = 1;
		MAKE_INMEM(dbp);
		if ((ret = __env_setup(dbp,
		    NULL, NULL, argp->name.data, TXN_INVALID, 0)) != 0)
			goto out;
		ret = __env_mpool(dbp, argp->name.data, 0);

		if (ret == ENOENT) {
			dbp->pgsize = argp->pgsize;
			if ((ret = __env_mpool(dbp,
			    argp->name.data, DB_CREATE)) != 0)
				goto out;
		} else if (ret != 0)
			goto out;
	}

	if (DB_UNDO(op)) {
		if (ret == 0)
			ret = __memp_nameop(env, argp->fid.data, NULL,
			    (const char *)argp->name.data,  NULL, 1);

		if (ret == ENOENT || ret == DB_DELETED)
			ret = 0;
		else
			goto out;
	}

	*lsnp = argp->prev_lsn;

out:	if (dbp != NULL) {
		t_ret = 0;

		if (do_close || ret != 0)
			t_ret = __db_close(dbp, NULL, DB_NOSYNC);
		if (t_ret != 0 && ret == 0)
			ret = t_ret;
	}
	REC_NOOP_CLOSE;
}

/*
 * __crdel_inmem_rename_recover --
 *	Recovery function for inmem_rename.
 *
 * PUBLIC: int __crdel_inmem_rename_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__crdel_inmem_rename_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__crdel_inmem_rename_args *argp;
	u_int8_t *fileid;
	int ret;

	COMPQUIET(info, NULL);

	REC_PRINT(__crdel_inmem_rename_print);
	REC_NOOP_INTRO(__crdel_inmem_rename_read);
	fileid = argp->fid.data;

	/* Void out errors because the files may or may not still exist. */
	if (DB_REDO(op))
		(void)__memp_nameop(env, fileid,
		    (const char *)argp->newname.data,
		    (const char *)argp->oldname.data,
		    (const char *)argp->newname.data, 1);

	if (DB_UNDO(op))
		(void)__memp_nameop(env, fileid,
		    (const char *)argp->oldname.data,
		    (const char *)argp->newname.data,
		    (const char *)argp->oldname.data, 1);

	*lsnp = argp->prev_lsn;
	ret = 0;

	REC_NOOP_CLOSE;
}

/*
 * __crdel_inmem_remove_recover --
 *	Recovery function for inmem_remove.
 *
 * PUBLIC: int __crdel_inmem_remove_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__crdel_inmem_remove_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__crdel_inmem_remove_args *argp;
	int ret;

	COMPQUIET(info, NULL);

	REC_PRINT(__crdel_inmem_remove_print);
	REC_NOOP_INTRO(__crdel_inmem_remove_read);

	/*
	 * Since removes are delayed; there is no undo for a remove; only redo.
	 * The remove may fail, which is OK.
	 */
	if (DB_REDO(op)) {
		(void)__memp_nameop(env,
		    argp->fid.data, NULL, argp->name.data, NULL, 1);
	}

	*lsnp = argp->prev_lsn;
	ret = 0;

	REC_NOOP_CLOSE;
}
