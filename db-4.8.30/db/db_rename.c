/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001, 2010 Oracle and/or its affiliates.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/fop.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __db_rename __P((DB *, DB_THREAD_INFO *,
	     DB_TXN *, const char *, const char *, const char *));
static int __db_subdb_rename __P((DB *, DB_THREAD_INFO *,
	     DB_TXN *, const char *, const char *, const char *));

/*
 * __env_dbrename_pp
 *	ENV->dbrename pre/post processing.
 *
 * PUBLIC: int __env_dbrename_pp __P((DB_ENV *, DB_TXN *,
 * PUBLIC:     const char *, const char *, const char *, u_int32_t));
 */
int
__env_dbrename_pp(dbenv, txn, name, subdb, newname, flags)
	DB_ENV *dbenv;
	DB_TXN *txn;
	const char *name, *subdb, *newname;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret, txn_local;

	env = dbenv->env;
	dbp = NULL;
	txn_local = 0;

	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->dbrename");

	/*
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if ((ret = __db_fchk(env, "DB->rename", flags, DB_AUTO_COMMIT)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __env_rep_enter(env, 1)) != 0) {
		handle_check = 0;
		goto err;
	}

	/*
	 * Create local transaction as necessary, check for consistent
	 * transaction usage.
	 */
	if (IS_ENV_AUTO_COMMIT(env, txn, flags)) {
		if ((ret = __db_txn_auto_init(env, ip, &txn)) != 0)
			goto err;
		txn_local = 1;
	} else
		if (txn != NULL && !TXN_ON(env) &&
		    (!CDB_LOCKING(env) || !F_ISSET(txn, TXN_CDSGROUP))) {
			ret = __db_not_txn_env(env);
			goto err;
		}

	LF_CLR(DB_AUTO_COMMIT);

	if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
		goto err;

	ret = __db_rename_int(dbp, ip, txn, name, subdb, newname);

	if (txn_local) {
		/*
		 * We created the DBP here and when we commit/abort, we'll
		 * release all the transactional locks, including the handle
		 * lock; mark the handle cleared explicitly.
		 */
		LOCK_INIT(dbp->handle_lock);
		dbp->locker = NULL;
	} else if (txn != NULL) {
		/*
		 * We created this handle locally so we need to close it and
		 * clean it up.  Unfortunately, it's holding transactional
		 * or CDS group locks that need to persist until the end of
		 * transaction.  If we invalidate the locker (dbp->locker),
		 * then the close won't free these locks prematurely.
		 */
		 dbp->locker = NULL;
	}

err:	if (txn_local && (t_ret =
	    __db_txn_auto_resolve(env, txn, 0, ret)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * We never opened this dbp for real, so don't include a transaction
	 * handle, and use NOSYNC to avoid calling into mpool.
	 *
	 * !!!
	 * Note we're reversing the order of operations: we started the txn and
	 * then opened the DB handle; we're resolving the txn and then closing
	 * closing the DB handle -- it's safer.
	 */
	if (dbp != NULL &&
	    (t_ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_rename_pp
 *	DB->rename pre/post processing.
 *
 * PUBLIC: int __db_rename_pp __P((DB *,
 * PUBLIC:     const char *, const char *, const char *, u_int32_t));
 */
int
__db_rename_pp(dbp, name, subdb, newname, flags)
	DB *dbp;
	const char *name, *subdb, *newname;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;
	handle_check = 0;

	/*
	 * Validate arguments, continuing to destroy the handle on failure.
	 *
	 * Cannot use DB_ILLEGAL_AFTER_OPEN directly because it returns.
	 *
	 * !!!
	 * We have a serious problem if we're here with a handle used to open
	 * a database -- we'll destroy the handle, and the application won't
	 * ever be able to close the database.
	 */
	if (F_ISSET(dbp, DB_AM_OPEN_CALLED))
		return (__db_mi_open(env, "DB->rename", 1));

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB->rename", flags, 0)) != 0)
		return (ret);

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, NULL, DB_LOCK_INVALIDID, 0)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 1, 0)) != 0) {
		handle_check = 0;
		goto err;
	}

	/* Rename the file. */
	ret = __db_rename(dbp, ip, NULL, name, subdb, newname);

	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_rename
 *	DB->rename method.
 *
 */
static int
__db_rename(dbp, ip, txn, name, subdb, newname)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb, *newname;
{
	int ret, t_ret;

	ret = __db_rename_int(dbp, ip, txn, name, subdb, newname);

	if ((t_ret = __db_close(dbp, txn, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_rename_int
 *	Worker function for DB->rename method; the close of the dbp is
 * left in the wrapper routine.
 *
 * PUBLIC: int __db_rename_int __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:      DB_TXN *, const char *, const char *, const char *));
 */
int
__db_rename_int(dbp, ip, txn, name, subdb, newname)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb, *newname;
{
	ENV *env;
	int ret;
	char *old, *real_name;

	env = dbp->env;
	real_name = NULL;

	DB_TEST_RECOVERY(dbp, DB_TEST_PREDESTROY, ret, name);

	if (name == NULL && subdb == NULL) {
		__db_errx(env, "Rename on temporary files invalid");
		ret = EINVAL;
		goto err;
	}

	if (name == NULL)
		MAKE_INMEM(dbp);
	else if (subdb != NULL) {
		ret = __db_subdb_rename(dbp, ip, txn, name, subdb, newname);
		goto err;
	}

	/*
	 * From here on down, this pertains to files or in-memory databases.
	 *
	 * Find the real name of the file.
	 */
	if (F_ISSET(dbp, DB_AM_INMEM)) {
		old = (char *)subdb;
		real_name = (char *)subdb;
	} else {
		if ((ret = __db_appname(env, DB_APP_DATA,
		    name, &dbp->dirname, &real_name)) != 0)
			goto err;
		old = (char *)name;
	}
	DB_ASSERT(env, old != NULL);

	if ((ret = __fop_remove_setup(dbp, txn, real_name, 0)) != 0)
		goto err;

	if (dbp->db_am_rename != NULL &&
	    (ret = dbp->db_am_rename(dbp, ip, txn, name, subdb, newname)) != 0)
		goto err;

	/*
	 * The transactional case and non-transactional case are
	 * quite different.  In the non-transactional case, we simply
	 * do the rename.  In the transactional case, since we need
	 * the ability to back out and maintain locking, we have to
	 * create a temporary object as a placeholder.  This is all
	 * taken care of in the fop layer.
	 */
	if (IS_REAL_TXN(txn)) {
		if ((ret = __fop_dummy(dbp, txn, old, newname)) != 0)
			goto err;
	} else {
		if ((ret = __fop_dbrename(dbp, old, newname)) != 0)
			goto err;
	}

	/*
	 * I am pretty sure that we haven't gotten a dbreg id, so calling
	 * dbreg_filelist_update is not necessary.
	 */
	DB_ASSERT(env, dbp->log_filename == NULL ||
	    dbp->log_filename->id == DB_LOGFILEID_INVALID);

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTDESTROY, ret, newname);

DB_TEST_RECOVERY_LABEL
err:	if (!F_ISSET(dbp, DB_AM_INMEM) && real_name != NULL)
		__os_free(env, real_name);

	return (ret);
}

/*
 * __db_subdb_rename --
 *	Rename a subdatabase.
 */
static int
__db_subdb_rename(dbp, ip, txn, name, subdb, newname)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb, *newname;
{
	DB *mdbp;
	ENV *env;
	PAGE *meta;
	int ret, t_ret;

	mdbp = NULL;
	meta = NULL;
	env = dbp->env;

	/*
	 * We have not opened this dbp so it isn't marked as a subdb,
	 * but it ought to be.
	 */
	F_SET(dbp, DB_AM_SUBDB);

	/*
	 * Rename the entry in the main database.  We need to first
	 * get the meta-data page number (via MU_OPEN) so that we can
	 * read the meta-data page and obtain a handle lock.  Once we've
	 * done that, we can proceed to do the rename in the master.
	 */
	if ((ret = __db_master_open(dbp, ip, txn, name, 0, 0, &mdbp)) != 0)
		goto err;

	if ((ret = __db_master_update(mdbp, dbp, ip, txn, subdb, dbp->type,
	    MU_OPEN, NULL, 0)) != 0)
		goto err;

	if ((ret = __memp_fget(mdbp->mpf, &dbp->meta_pgno,
	    ip, txn, 0, &meta)) != 0)
		goto err;
	memcpy(dbp->fileid, ((DBMETA *)meta)->uid, DB_FILE_ID_LEN);
	if ((ret = __fop_lock_handle(env,
	    dbp, mdbp->locker, DB_LOCK_WRITE, NULL, NOWAIT_FLAG(txn))) != 0)
		goto err;

	ret = __memp_fput(mdbp->mpf, ip, meta, dbp->priority);
	meta = NULL;
	if (ret != 0)
		goto err;

	if ((ret = __db_master_update(mdbp, dbp, ip, txn,
	    subdb, dbp->type, MU_RENAME, newname, 0)) != 0)
		goto err;

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTDESTROY, ret, name);

DB_TEST_RECOVERY_LABEL
err:
	if (meta != NULL && (t_ret =
	    __memp_fput(mdbp->mpf, ip, meta, dbp->priority)) != 0 && ret == 0)
		ret = t_ret;

	if (mdbp != NULL &&
	    (t_ret = __db_close(mdbp, txn, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
