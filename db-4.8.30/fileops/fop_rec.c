/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/fop.h"
#include "dbinc/db_am.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __fop_rename_recover_int
    __P((ENV *, DBT *, DB_LSN *, db_recops, void *, int));
static int __fop_rename_42_recover_int
    __P((ENV *, DBT *, DB_LSN *, db_recops, void *, int));

/*
 * The transactional guarantees Berkeley DB provides for file
 * system level operations (database physical file create, delete,
 * rename) are based on our understanding of current file system
 * semantics; a system that does not provide these semantics and
 * guarantees could be in danger.
 *
 * First, as in standard database changes, fsync and fdatasync must
 * work: when applied to the log file, the records written into the
 * log must be transferred to stable storage.
 *
 * Second, it must not be possible for the log file to be removed
 * without previous file system level operations being flushed to
 * stable storage.  Berkeley DB applications write log records
 * describing file system operations into the log, then perform the
 * file system operation, then commit the enclosing transaction
 * (which flushes the log file to stable storage).  Subsequently,
 * a database environment checkpoint may make it possible for the
 * application to remove the log file containing the record of the
 * file system operation.  DB's transactional guarantees for file
 * system operations require the log file removal not succeed until
 * all previous filesystem operations have been flushed to stable
 * storage.  In other words, the flush of the log file, or the
 * removal of the log file, must block until all previous
 * filesystem operations have been flushed to stable storage.  This
 * semantic is not, as far as we know, required by any existing
 * standards document, but we have never seen a filesystem where
 * it does not apply.
 */

/*
 * __fop_create_recover --
 *	Recovery function for create.
 *
 * PUBLIC: int __fop_create_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_create_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__fop_create_args *argp;
	DB_FH *fhp;
	DBMETA *meta;
	u_int8_t mbuf[DBMETASIZE];
	int ret;
	char *real_name;
	const char *dirname;

	COMPQUIET(info, NULL);

	real_name = NULL;
	REC_PRINT(__fop_create_print);
	REC_NOOP_INTRO(__fop_create_read);
	meta = (DBMETA *)mbuf;

	if (argp->dirname.size == 0)
		dirname = NULL;
	else
		dirname = (const char *)argp->dirname.data;

	if ((ret = __db_appname(env, (APPNAME)argp->appname == DB_APP_DATA ?
	    DB_APP_RECOVER : (APPNAME)argp->appname,
	    (const char *)argp->name.data, &dirname, &real_name)) != 0)
		goto out;

	if (DB_UNDO(op)) {
		/*
		 * If the file was opened in mpool, we must mark it as
		 * dead via nameop which will also unlink the file.
		 */
		if (__os_open(env, real_name, 0, 0, 0, &fhp) == 0) {
			if (__fop_read_meta(env,
			    real_name, mbuf, DBMETASIZE, fhp, 1, NULL) == 0 &&
			    __db_chk_meta(env, NULL, meta, 1) == 0) {
				if ((ret = __memp_nameop(env,
				    meta->uid, NULL, real_name, NULL, 0)) != 0)
					goto out;
			} else {
				(void)__os_closehandle(env, fhp);
				goto do_unlink;
			}
			(void)__os_closehandle(env, fhp);
		} else
do_unlink:		(void)__os_unlink(env, real_name, 0);
	} else if (DB_REDO(op)) {
		if ((ret = __os_open(env, real_name, 0,
		    DB_OSO_CREATE, (int)argp->mode, &fhp)) == 0)
			(void)__os_closehandle(env, fhp);
		else
			goto out;
	}

	*lsnp = argp->prev_lsn;

out: if (real_name != NULL)
		__os_free(env, real_name);

	REC_NOOP_CLOSE;
}

/*
 * __fop_create_42_recover --
 *	Recovery function for create.
 *
 * PUBLIC: int __fop_create_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_create_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__fop_create_args *argp;
	DB_FH *fhp;
	DBMETA *meta;
	u_int8_t mbuf[DBMETASIZE];
	int ret;
	char *real_name;

	COMPQUIET(info, NULL);

	real_name = NULL;
	REC_PRINT(__fop_create_print);
	REC_NOOP_INTRO(__fop_create_read);
	meta = (DBMETA *)mbuf;

	if ((ret = __db_appname(env, (APPNAME)argp->appname,
	    (const char *)argp->name.data, NULL, &real_name)) != 0)
		goto out;

	if (DB_UNDO(op)) {
		/*
		 * If the file was opened in mpool, we must mark it as
		 * dead via nameop which will also unlink the file.
		 */
		if (__os_open(env, real_name, 0, 0, 0, &fhp) == 0) {
			if (__fop_read_meta(env,
			    real_name, mbuf, DBMETASIZE, fhp, 1, NULL) == 0 &&
			    __db_chk_meta(env, NULL, meta, 1) == 0) {
				if ((ret = __memp_nameop(env,
				    meta->uid, NULL, real_name, NULL, 0)) != 0)
					goto out;
			} else
				goto do_unlink;
			(void)__os_closehandle(env, fhp);
		} else
do_unlink:		(void)__os_unlink(env, real_name, 0);
	} else if (DB_REDO(op)) {
		if ((ret = __os_open(env, real_name, 0,
		    DB_OSO_CREATE, (int)argp->mode, &fhp)) == 0)
			(void)__os_closehandle(env, fhp);
		else
			goto out;
	}

	*lsnp = argp->prev_lsn;

out: if (real_name != NULL)
		__os_free(env, real_name);

	REC_NOOP_CLOSE;
}

/*
 * __fop_remove_recover --
 *	Recovery function for remove.
 *
 * PUBLIC: int __fop_remove_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_remove_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__fop_remove_args *argp;
	int ret;
	char *real_name;

	COMPQUIET(info, NULL);

	real_name = NULL;
	REC_PRINT(__fop_remove_print);
	REC_NOOP_INTRO(__fop_remove_read);

	if ((ret = __db_appname(env, (APPNAME)argp->appname,
	    (const char *)argp->name.data, NULL, &real_name)) != 0)
		goto out;

	/* Its ok if the file is not there. */
	if (DB_REDO(op))
		(void)__memp_nameop(env,
		    (u_int8_t *)argp->fid.data, NULL, real_name, NULL, 0);

	*lsnp = argp->prev_lsn;
out:	if (real_name != NULL)
		__os_free(env, real_name);
	REC_NOOP_CLOSE;
}

/*
 * __fop_write_recover --
 *	Recovery function for writechunk.
 *
 * PUBLIC: int __fop_write_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_write_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__fop_write_args *argp;
	int ret;

	COMPQUIET(info, NULL);

	REC_PRINT(__fop_write_print);
	REC_NOOP_INTRO(__fop_write_read);

	ret = 0;
	if (DB_UNDO(op))
		DB_ASSERT(env, argp->flag != 0);
	else if (DB_REDO(op))
		ret = __fop_write(env,
		    argp->txnp, argp->name.data,
		    argp->dirname.size == 0 ? NULL : argp->dirname.data,
		    (APPNAME)argp->appname == DB_APP_DATA ? DB_APP_RECOVER :
		    (APPNAME)argp->appname,
		    NULL, argp->pgsize, argp->pageno, argp->offset,
		    argp->page.data, argp->page.size, argp->flag, 0);

	if (ret == 0)
		*lsnp = argp->prev_lsn;
	REC_NOOP_CLOSE;
}

/*
 * __fop_write_42_recover --
 *	Recovery function for writechunk.
 *
 * PUBLIC: int __fop_write_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_write_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__fop_write_args *argp;
	int ret;

	COMPQUIET(info, NULL);

	REC_PRINT(__fop_write_print);
	REC_NOOP_INTRO(__fop_write_read);

	ret = 0;
	if (DB_UNDO(op))
		DB_ASSERT(env, argp->flag != 0);
	else if (DB_REDO(op))
		ret = __fop_write(env,
		    argp->txnp, argp->name.data, NULL, (APPNAME)argp->appname,
		    NULL, argp->pgsize, argp->pageno, argp->offset,
		    argp->page.data, argp->page.size, argp->flag, 0);

	if (ret == 0)
		*lsnp = argp->prev_lsn;
	REC_NOOP_CLOSE;
}

/*
 * __fop_rename_recover --
 *	Recovery functions for rename.  There are two variants that
 * both use the same utility function.  Had we known about this on day
 * one, we would have simply added a parameter.  However, since we need
 * to retain old records for backward compatibility (online-upgrade)
 * wrapping the two seems like the right solution.
 *
 * PUBLIC: int __fop_rename_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 *
 * PUBLIC: int __fop_rename_noundo_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_rename_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	return (__fop_rename_recover_int(env, dbtp, lsnp, op, info, 1));
}

int
__fop_rename_noundo_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	return (__fop_rename_recover_int(env, dbtp, lsnp, op, info, 0));
}

static int
__fop_rename_recover_int(env, dbtp, lsnp, op, info, undo)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
	int undo;
{
	__fop_rename_args *argp;
	APPNAME appname;
	DB_FH *fhp;
	DBMETA *meta;
	u_int8_t *fileid, mbuf[DBMETASIZE];
	int ret;
	char *real_new, *real_old, *src;
	const char *dirname;

	COMPQUIET(info, NULL);

	fhp = NULL;
	meta = (DBMETA *)&mbuf[0];
	ret = 0;
	real_new = real_old = NULL;

	REC_PRINT(__fop_rename_print);
	REC_NOOP_INTRO(__fop_rename_read);
	fileid = argp->fileid.data;

	if (argp->dirname.size == 0)
		dirname = NULL;
	else
		dirname = (const char *)argp->dirname.data;

	if ((APPNAME)argp->appname == DB_APP_DATA)
		appname = DB_APP_RECOVER;
	else
		appname = (APPNAME)argp->appname;

	if ((ret = __db_appname(env, appname, (const char *)argp->newname.data,
	    &dirname, &real_new)) != 0)
		goto out;
	if ((ret = __db_appname(env, appname, (const char *)argp->oldname.data,
	    &dirname, &real_old)) != 0)
		goto out;

	/*
	 * Verify that we are manipulating the correct file.  We should always
	 * be OK on an ABORT or an APPLY, but during recovery, we have to
	 * check.
	 */
	if (op != DB_TXN_ABORT && op != DB_TXN_APPLY) {
		src = DB_UNDO(op) ? real_new : real_old;
		/*
		 * Interpret any error as meaning that the file either doesn't
		 * exist, doesn't have a meta-data page, or is in some other
		 * way, shape or form, incorrect, so that we should not restore
		 * it.
		 */
		if (__os_open(env, src, 0, 0, 0, &fhp) != 0)
			goto done;
		if (__fop_read_meta(env,
		    src, mbuf, DBMETASIZE, fhp, 1, NULL) != 0)
			goto done;
		if (__db_chk_meta(env, NULL, meta, 1) != 0)
			goto done;
		if (memcmp(argp->fileid.data, meta->uid, DB_FILE_ID_LEN) != 0)
			goto done;
		(void)__os_closehandle(env, fhp);
		fhp = NULL;
		if (DB_REDO(op)) {
			/*
			 * Check to see if the target file exists.  If it
			 * does and it does not have the proper id then
			 * it is a later version.  We just remove the source
			 * file since the state of the world is beyond this
			 * point.
			 */
			if (__os_open(env, real_new, 0, 0, 0, &fhp) == 0 &&
			    __fop_read_meta(env, src, mbuf,
			    DBMETASIZE, fhp, 1, NULL) == 0 &&
			    __db_chk_meta(env, NULL, meta, 1) == 0 &&
			    memcmp(argp->fileid.data,
			    meta->uid, DB_FILE_ID_LEN) != 0) {
				(void)__memp_nameop(env,
				    fileid, NULL, real_old, NULL, 0);
				goto done;
			}
		}
	}

	if (undo && DB_UNDO(op))
		(void)__memp_nameop(env, fileid,
		    (const char *)argp->oldname.data, real_new, real_old, 0);
	if (DB_REDO(op))
		(void)__memp_nameop(env, fileid,
		    (const char *)argp->newname.data, real_old, real_new, 0);

done:	*lsnp = argp->prev_lsn;
out:	if (real_new != NULL)
		__os_free(env, real_new);
	if (real_old != NULL)
		__os_free(env, real_old);
	if (fhp != NULL)
		(void)__os_closehandle(env, fhp);

	REC_NOOP_CLOSE;
}
/*
 * __fop_rename_42_recover --
 *	Recovery functions for rename.  There are two variants that
 * both use the same utility function.  Had we known about this on day
 * one, we would have simply added a parameter.  However, since we need
 * to retain old records for backward compatibility (online-upgrade)
 * wrapping the two seems like the right solution.
 *
 * PUBLIC: int __fop_rename_42_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 *
 * PUBLIC: int __fop_rename_noundo_46_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_rename_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	return (__fop_rename_42_recover_int(env, dbtp, lsnp, op, info, 1));
}

int
__fop_rename_noundo_46_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	return (__fop_rename_42_recover_int(env, dbtp, lsnp, op, info, 0));
}

static int
__fop_rename_42_recover_int(env, dbtp, lsnp, op, info, undo)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
	int undo;
{
	__fop_rename_args *argp;
	DB_FH *fhp;
	DBMETA *meta;
	u_int8_t *fileid, mbuf[DBMETASIZE];
	int ret;
	char *real_new, *real_old, *src;

	COMPQUIET(info, NULL);

	fhp = NULL;
	meta = (DBMETA *)&mbuf[0];
	ret = 0;
	real_new = real_old = NULL;

	REC_PRINT(__fop_rename_print);
	REC_NOOP_INTRO(__fop_rename_read);
	fileid = argp->fileid.data;

	if ((ret = __db_appname(env, (APPNAME)argp->appname,
	    (const char *)argp->newname.data, NULL, &real_new)) != 0)
		goto out;
	if ((ret = __db_appname(env, (APPNAME)argp->appname,
	    (const char *)argp->oldname.data, NULL, &real_old)) != 0)
		goto out;

	/*
	 * Verify that we are manipulating the correct file.  We should always
	 * be OK on an ABORT or an APPLY, but during recovery, we have to
	 * check.
	 */
	if (op != DB_TXN_ABORT && op != DB_TXN_APPLY) {
		src = DB_UNDO(op) ? real_new : real_old;
		/*
		 * Interpret any error as meaning that the file either doesn't
		 * exist, doesn't have a meta-data page, or is in some other
		 * way, shape or form, incorrect, so that we should not restore
		 * it.
		 */
		if (__os_open(env, src, 0, 0, 0, &fhp) != 0)
			goto done;
		if (__fop_read_meta(env,
		    src, mbuf, DBMETASIZE, fhp, 1, NULL) != 0)
			goto done;
		if (__db_chk_meta(env, NULL, meta, 1) != 0)
			goto done;
		if (memcmp(argp->fileid.data, meta->uid, DB_FILE_ID_LEN) != 0)
			goto done;
		(void)__os_closehandle(env, fhp);
		fhp = NULL;
		if (DB_REDO(op)) {
			/*
			 * Check to see if the target file exists.  If it
			 * does and it does not have the proper id then
			 * it is a later version.  We just remove the source
			 * file since the state of the world is beyond this
			 * point.
			 */
			if (__os_open(env, real_new, 0, 0, 0, &fhp) == 0 &&
			    __fop_read_meta(env, src, mbuf,
			    DBMETASIZE, fhp, 1, NULL) == 0 &&
			    __db_chk_meta(env, NULL, meta, 1) == 0 &&
			    memcmp(argp->fileid.data,
			    meta->uid, DB_FILE_ID_LEN) != 0) {
				(void)__memp_nameop(env,
				    fileid, NULL, real_old, NULL, 0);
				goto done;
			}
		}
	}

	if (undo && DB_UNDO(op))
		(void)__memp_nameop(env, fileid,
		    (const char *)argp->oldname.data, real_new, real_old, 0);
	if (DB_REDO(op))
		(void)__memp_nameop(env, fileid,
		    (const char *)argp->newname.data, real_old, real_new, 0);

done:	*lsnp = argp->prev_lsn;
out:	if (real_new != NULL)
		__os_free(env, real_new);
	if (real_old != NULL)
		__os_free(env, real_old);
	if (fhp != NULL)
		(void)__os_closehandle(env, fhp);

	REC_NOOP_CLOSE;
}

/*
 * __fop_file_remove_recover --
 *	Recovery function for file_remove.  On the REDO pass, we need to
 * make sure no one recreated the file while we weren't looking.  On an
 * undo pass must check if the file we are interested in is the one that
 * exists and then set the status of the child transaction depending on
 * what we find out.
 *
 * PUBLIC: int __fop_file_remove_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__fop_file_remove_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__fop_file_remove_args *argp;
	DBMETA *meta;
	DB_FH *fhp;
	size_t len;
	u_int8_t mbuf[DBMETASIZE];
	u_int32_t cstat, ret_stat;
	int is_real, is_tmp, ret;
	char *real_name;

	fhp = NULL;
	meta = (DBMETA *)&mbuf[0];
	is_real = is_tmp = 0;
	real_name = NULL;
	REC_PRINT(__fop_file_remove_print);
	REC_NOOP_INTRO(__fop_file_remove_read);

	/*
	 * This record is only interesting on the backward, forward, and
	 * apply phases.
	 */
	if (op != DB_TXN_BACKWARD_ROLL &&
	    op != DB_TXN_FORWARD_ROLL && op != DB_TXN_APPLY)
		goto done;

	if ((ret = __db_appname(env, (APPNAME)argp->appname,
	    argp->name.data, NULL, &real_name)) != 0)
		goto out;

	/* Verify that we are manipulating the correct file.  */
	len = 0;
	if (__os_open(env, real_name, 0, 0, 0, &fhp) != 0 ||
	    (ret = __fop_read_meta(env, real_name,
	    mbuf, DBMETASIZE, fhp, 1, &len)) != 0) {
		/*
		 * If len is non-zero, then the file exists and has something
		 * in it, but that something isn't a full meta-data page, so
		 * this is very bad.  Bail out!
		 */
		if (len != 0)
			goto out;

		/* File does not exist. */
		cstat = TXN_EXPECTED;
	} else {
		/*
		 * We can ignore errors here since we'll simply fail the
		 * checks below and assume this is the wrong file.
		 */
		(void)__db_chk_meta(env, NULL, meta, 1);
		is_real =
		    memcmp(argp->real_fid.data, meta->uid, DB_FILE_ID_LEN) == 0;
		is_tmp =
		    memcmp(argp->tmp_fid.data, meta->uid, DB_FILE_ID_LEN) == 0;

		if (!is_real && !is_tmp)
			/* File exists, but isn't what we were removing. */
			cstat = TXN_IGNORE;
		else
			/* File exists and is the one that we were removing. */
			cstat = TXN_COMMIT;
	}
	if (fhp != NULL) {
		(void)__os_closehandle(env, fhp);
		fhp = NULL;
	}

	if (DB_UNDO(op)) {
		/* On the backward pass, we leave a note for the child txn. */
		if ((ret = __db_txnlist_update(env,
		    info, argp->child, cstat, NULL, &ret_stat, 1)) != 0)
			goto out;
	} else if (DB_REDO(op)) {
		/*
		 * On the forward pass, check if someone recreated the
		 * file while we weren't looking.
		 */
		if (cstat == TXN_COMMIT)
			(void)__memp_nameop(env,
			    is_real ? argp->real_fid.data : argp->tmp_fid.data,
			    NULL, real_name, NULL, 0);
	}

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (real_name != NULL)
		__os_free(env, real_name);
	if (fhp != NULL)
		(void)__os_closehandle(env, fhp);
	REC_NOOP_CLOSE;
}
