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
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"
#include "dbinc/db_am.h"

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
 * __fop_create --
 * Create a (transactionally protected) file system object.  This is used
 * to create DB files now, potentially blobs, queue extents and anything
 * else you wish to store in a file system object.
 *
 * PUBLIC: int __fop_create __P((ENV *, DB_TXN *,
 * PUBLIC:     DB_FH **, const char *, const char **, APPNAME, int, u_int32_t));
 */
int
__fop_create(env, txn, fhpp, name, dirp, appname, mode, flags)
	ENV *env;
	DB_TXN *txn;
	DB_FH **fhpp;
	const char *name, **dirp;
	APPNAME appname;
	int mode;
	u_int32_t flags;
{
	DBT data, dirdata;
	DB_FH *fhp;
	DB_LSN lsn;
	int ret;
	char *real_name;

	real_name = NULL;
	fhp = NULL;

	if ((ret = __db_appname(env, appname, name, dirp, &real_name)) != 0)
		return (ret);

	if (mode == 0)
		mode = DB_MODE_600;

	if (DBENV_LOGGING(env)
#if !defined(DEBUG_WOP)
	    && txn != NULL
#endif
	    ) {
		DB_INIT_DBT(data, name, strlen(name) + 1);
		if (dirp != NULL && *dirp != NULL)
			DB_INIT_DBT(dirdata, *dirp, strlen(*dirp) + 1);
		else
			memset(&dirdata, 0, sizeof(dirdata));
		if ((ret = __fop_create_log(env, txn, &lsn,
		    flags | DB_FLUSH,
		    &data, &dirdata, (u_int32_t)appname, (u_int32_t)mode)) != 0)
			goto err;
	}

	DB_ENV_TEST_RECOVERY(env, DB_TEST_POSTLOG, ret, name);

	if (fhpp == NULL)
		fhpp = &fhp;
	ret = __os_open(
	    env, real_name, 0, DB_OSO_CREATE | DB_OSO_EXCL, mode, fhpp);

err:
DB_TEST_RECOVERY_LABEL
	if (fhpp == &fhp && fhp != NULL)
		(void)__os_closehandle(env, fhp);
	if (real_name != NULL)
		__os_free(env, real_name);
	return (ret);
}

/*
 * __fop_remove --
 *	Remove a file system object.
 *
 * PUBLIC: int __fop_remove __P((ENV *, DB_TXN *,
 * PUBLIC:     u_int8_t *, const char *, const char **, APPNAME, u_int32_t));
 */
int
__fop_remove(env, txn, fileid, name, dirp, appname, flags)
	ENV *env;
	DB_TXN *txn;
	u_int8_t *fileid;
	const char *name, **dirp;
	APPNAME appname;
	u_int32_t flags;
{
	DBT fdbt, ndbt;
	DB_LSN lsn;
	char *real_name;
	int ret;

	real_name = NULL;

	if ((ret = __db_appname(env, appname, name, dirp, &real_name)) != 0)
		goto err;

	if (!IS_REAL_TXN(txn)) {
		if (fileid != NULL && (ret = __memp_nameop(
		    env, fileid, NULL, real_name, NULL, 0)) != 0)
			goto err;
	} else {
		if (DBENV_LOGGING(env)
#if !defined(DEBUG_WOP)
		    && txn != NULL
#endif
		) {
			memset(&fdbt, 0, sizeof(ndbt));
			fdbt.data = fileid;
			fdbt.size = fileid == NULL ? 0 : DB_FILE_ID_LEN;
			DB_INIT_DBT(ndbt, name, strlen(name) + 1);
			if ((ret = __fop_remove_log(env, txn, &lsn,
			    flags, &ndbt, &fdbt, (u_int32_t)appname)) != 0)
				goto err;
		}
		ret = __txn_remevent(env, txn, real_name, fileid, 0);
	}

err:	if (real_name != NULL)
		__os_free(env, real_name);
	return (ret);
}

/*
 * __fop_write
 *
 * Write "size" bytes from "buf" to file "name" beginning at offset "off."
 * If the file is open, supply a handle in fhp.  Istmp indicate if this is
 * an operation that needs to be undone in the face of failure (i.e., if
 * this is a write to a temporary file, we're simply going to remove the
 * file, so don't worry about undoing the write).
 *
 * Currently, we *only* use this with istmp true.  If we need more general
 * handling, then we'll have to zero out regions on abort (and possibly
 * log the before image of the data in the log record).
 *
 * PUBLIC: int __fop_write __P((ENV *, DB_TXN *,
 * PUBLIC:     const char *, const char *, APPNAME, DB_FH *, u_int32_t,
 * PUBLIC:     db_pgno_t, u_int32_t, void *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__fop_write(env, txn,
    name, dirname, appname, fhp, pgsize, pageno, off, buf, size, istmp, flags)
	ENV *env;
	DB_TXN *txn;
	const char *name, *dirname;
	APPNAME appname;
	DB_FH *fhp;
	u_int32_t pgsize;
	db_pgno_t pageno;
	u_int32_t off;
	void *buf;
	u_int32_t size, istmp, flags;
{
	DBT data, namedbt, dirdbt;
	DB_LSN lsn;
	size_t nbytes;
	int local_open, ret, t_ret;
	char *real_name;

	DB_ASSERT(env, istmp != 0);

	ret = local_open = 0;
	real_name = NULL;

	if (DBENV_LOGGING(env)
#if !defined(DEBUG_WOP)
	    && txn != NULL
#endif
	    ) {
		memset(&data, 0, sizeof(data));
		data.data = buf;
		data.size = size;
		DB_INIT_DBT(namedbt, name, strlen(name) + 1);
		if (dirname != NULL)
			DB_INIT_DBT(dirdbt, dirname, strlen(dirname) + 1);
		else
			memset(&dirdbt, 0, sizeof(dirdbt));
		if ((ret = __fop_write_log(env, txn,
		    &lsn, flags, &namedbt, &dirdbt, (u_int32_t)appname,
		    pgsize, pageno, off, &data, istmp)) != 0)
			goto err;
	}

	if (fhp == NULL) {
		/* File isn't open; we need to reopen it. */
		if ((ret = __db_appname(env,
		    appname, name, &dirname, &real_name)) != 0)
			return (ret);

		if ((ret = __os_open(env, real_name, 0, 0, 0, &fhp)) != 0)
			goto err;
		local_open = 1;
	}

	/* Seek to offset. */
	if ((ret = __os_seek(env, fhp, pageno, pgsize, off)) != 0)
		goto err;

	/* Now do the write. */
	if ((ret = __os_write(env, fhp, buf, size, &nbytes)) != 0)
		goto err;

err:	if (local_open &&
	    (t_ret = __os_closehandle(env, fhp)) != 0 && ret == 0)
			ret = t_ret;

	if (real_name != NULL)
		__os_free(env, real_name);
	return (ret);
}

/*
 * __fop_rename --
 *	Change a file's name.
 *
 * PUBLIC: int __fop_rename __P((ENV *, DB_TXN *, const char *, const char *,
 * PUBLIC:      const char **, u_int8_t *, APPNAME, int, u_int32_t));
 */
int
__fop_rename(env, txn, oldname, newname, dirp, fid, appname, with_undo, flags)
	ENV *env;
	DB_TXN *txn;
	const char *oldname;
	const char *newname;
	const char **dirp;
	u_int8_t *fid;
	APPNAME appname;
	int with_undo;
	u_int32_t flags;
{
	DBT fiddbt, dir, new, old;
	DB_LSN lsn;
	int ret;
	char *n, *o;

	o = n = NULL;
	if ((ret = __db_appname(env, appname, oldname, dirp, &o)) != 0)
		goto err;
	if ((ret = __db_appname(env, appname, newname, dirp, &n)) != 0)
		goto err;

	if (DBENV_LOGGING(env)
#if !defined(DEBUG_WOP)
	    && txn != NULL
#endif
	    ) {
		DB_INIT_DBT(old, oldname, strlen(oldname) + 1);
		DB_INIT_DBT(new, newname, strlen(newname) + 1);
		if (dirp != NULL && *dirp != NULL)
			DB_INIT_DBT(dir, *dirp, strlen(*dirp) + 1);
		else
			memset(&dir, 0, sizeof(dir));
		memset(&fiddbt, 0, sizeof(fiddbt));
		fiddbt.data = fid;
		fiddbt.size = DB_FILE_ID_LEN;
		if (with_undo)
			ret = __fop_rename_log(env,
			    txn, &lsn, flags | DB_FLUSH,
			    &old, &new, &dir, &fiddbt, (u_int32_t)appname);
		else
			ret = __fop_rename_noundo_log(env,
			    txn, &lsn, flags | DB_FLUSH,
			    &old, &new, &dir, &fiddbt, (u_int32_t)appname);
		if (ret != 0)
			goto err;
	}

	ret = __memp_nameop(env, fid, newname, o, n, 0);

err:	if (o != NULL)
		__os_free(env, o);
	if (n != NULL)
		__os_free(env, n);
	return (ret);
}
