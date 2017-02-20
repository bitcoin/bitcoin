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
#include "dbinc/fop.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/lock.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __db_dbtxn_remove __P((DB *,
    DB_THREAD_INFO *, DB_TXN *, const char *, const char *));
static int __db_subdb_remove __P((DB *,
    DB_THREAD_INFO *, DB_TXN *, const char *, const char *));

/*
 * __env_dbremove_pp
 *	ENV->dbremove pre/post processing.
 *
 * PUBLIC: int __env_dbremove_pp __P((DB_ENV *,
 * PUBLIC:     DB_TXN *, const char *, const char *, u_int32_t));
 */
int
__env_dbremove_pp(dbenv, txn, name, subdb, flags)
	DB_ENV *dbenv;
	DB_TXN *txn;
	const char *name, *subdb;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret, txn_local;

	dbp = NULL;
	env = dbenv->env;
	txn_local = 0;

	ENV_ILLEGAL_BEFORE_OPEN(env, "DB_ENV->dbremove");

	/*
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if ((ret = __db_fchk(env,
		"DB->remove", flags, DB_AUTO_COMMIT | DB_TXN_NOT_DURABLE)) != 0)
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
	if (LF_ISSET(DB_TXN_NOT_DURABLE) &&
	    (ret = __db_set_flags(dbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;
	LF_CLR(DB_TXN_NOT_DURABLE);

	ret = __db_remove_int(dbp, ip, txn, name, subdb, flags);

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
		 * We created this handle locally so we need to close it
		 * and clean it up.  Unfortunately, it's holding transactional
		 * locks that need to persist until the end of transaction.
		 * If we invalidate the locker id (dbp->locker), then the close
		 * won't free these locks prematurely.
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
	 * closing the DB handle -- a DB handle cannot be closed before
	 * resolving the txn.
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
 * __db_remove_pp
 *	DB->remove pre/post processing.
 *
 * PUBLIC: int __db_remove_pp
 * PUBLIC:     __P((DB *, const char *, const char *, u_int32_t));
 */
int
__db_remove_pp(dbp, name, subdb, flags)
	DB *dbp;
	const char *name, *subdb;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

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
		return (__db_mi_open(env, "DB->remove", 1));

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB->remove", flags, 0)) != 0)
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

	/* Remove the file. */
	ret = __db_remove(dbp, ip, NULL, name, subdb, flags);

	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_remove
 *	DB->remove method.
 *
 * PUBLIC: int __db_remove __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:     DB_TXN *, const char *, const char *, u_int32_t));
 */
int
__db_remove(dbp, ip, txn, name, subdb, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb;
	u_int32_t flags;
{
	int ret, t_ret;

	ret = __db_remove_int(dbp, ip, txn, name, subdb, flags);

	if ((t_ret = __db_close(dbp, txn, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_remove_int
 *	Worker function for the DB->remove method.
 *
 * PUBLIC: int __db_remove_int __P((DB *, DB_THREAD_INFO *,
 * PUBLIC:    DB_TXN *, const char *, const char *, u_int32_t));
 */
int
__db_remove_int(dbp, ip, txn, name, subdb, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb;
	u_int32_t flags;
{
	ENV *env;
	int ret;
	char *real_name, *tmpname;

	env = dbp->env;
	real_name = tmpname = NULL;

	if (name == NULL && subdb == NULL) {
		__db_errx(env, "Remove on temporary files invalid");
		ret = EINVAL;
		goto err;
	}

	if (name == NULL) {
		MAKE_INMEM(dbp);
		real_name = (char *)subdb;
	} else if (subdb != NULL) {
		ret = __db_subdb_remove(dbp, ip, txn, name, subdb);
		goto err;
	}

	/* Handle transactional file removes separately. */
	if (IS_REAL_TXN(txn)) {
		ret = __db_dbtxn_remove(dbp, ip, txn, name, subdb);
		goto err;
	}

	/*
	 * The remaining case is a non-transactional file remove.
	 *
	 * Find the real name of the file.
	 */
	if (!F_ISSET(dbp, DB_AM_INMEM) && (ret = __db_appname(env,
	    DB_APP_DATA, name, &dbp->dirname, &real_name)) != 0)
		goto err;

	/*
	 * If this is a file and force is set, remove the temporary file, which
	 * may have been left around.  Ignore errors because the temporary file
	 * might not exist.
	 */
	if (!F_ISSET(dbp, DB_AM_INMEM) && LF_ISSET(DB_FORCE) &&
	    (ret = __db_backup_name(env, real_name, NULL, &tmpname)) == 0)
		(void)__os_unlink(env, tmpname, 0);

	if ((ret = __fop_remove_setup(dbp, NULL, real_name, 0)) != 0)
		goto err;

	if (dbp->db_am_remove != NULL &&
	    (ret = dbp->db_am_remove(dbp, ip, NULL, name, subdb, flags)) != 0)
		goto err;

	ret = F_ISSET(dbp, DB_AM_INMEM) ?
	    __db_inmem_remove(dbp, NULL, real_name) :
	    __fop_remove(env,
	    NULL, dbp->fileid, name, &dbp->dirname, DB_APP_DATA,
	    F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0);

err:	if (!F_ISSET(dbp, DB_AM_INMEM) && real_name != NULL)
		__os_free(env, real_name);
	if (tmpname != NULL)
		__os_free(env, tmpname);

	return (ret);
}

/*
 * __db_inmem_remove --
 *	Removal of a named in-memory database.
 *
 * PUBLIC: int __db_inmem_remove __P((DB *, DB_TXN *, const char *));
 */
int
__db_inmem_remove(dbp, txn, name)
	DB *dbp;
	DB_TXN *txn;
	const char *name;
{
	DBT fid_dbt, name_dbt;
	DB_LOCKER *locker;
	DB_LSN lsn;
	ENV *env;
	int ret;

	env = dbp->env;
	locker = NULL;

	DB_ASSERT(env, name != NULL);

	/* This had better exist if we are trying to do a remove. */
	(void)__memp_set_flags(dbp->mpf, DB_MPOOL_NOFILE, 1);
	if ((ret = __memp_fopen(dbp->mpf, NULL,
	    name, &dbp->dirname, 0, 0, 0)) != 0)
		return (ret);
	if ((ret = __memp_get_fileid(dbp->mpf, dbp->fileid)) != 0)
		return (ret);
	dbp->preserve_fid = 1;

	if (LOCKING_ON(env)) {
		if (dbp->locker == NULL &&
		    (ret = __lock_id(env, NULL, &dbp->locker)) != 0)
			return (ret);
		locker = txn == NULL ? dbp->locker : txn->locker;
	}

	/*
	 * In a transactional environment, we'll play the same game we play
	 * for databases in the file system -- create a temporary database
	 * and put it in with the current name and then rename this one to
	 * another name.  We'll then use a commit-time event to remove the
	 * entry.
	 */
	if ((ret =
	    __fop_lock_handle(env, dbp, locker, DB_LOCK_WRITE, NULL, 0)) != 0)
		return (ret);

	if (!IS_REAL_TXN(txn))
		ret = __memp_nameop(env, dbp->fileid, NULL, name, NULL, 1);
	else if (LOGGING_ON(env)) {
		if (txn != NULL && (ret =
		    __txn_remevent(env, txn, name, dbp->fileid, 1)) != 0)
			return (ret);

		DB_INIT_DBT(name_dbt, name, strlen(name) + 1);
		DB_INIT_DBT(fid_dbt, dbp->fileid, DB_FILE_ID_LEN);
		ret = __crdel_inmem_remove_log(
		    env, txn, &lsn, 0, &name_dbt, &fid_dbt);
	}

	return (ret);
}

/*
 * __db_subdb_remove --
 *	Remove a subdatabase.
 */
static int
__db_subdb_remove(dbp, ip, txn, name, subdb)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb;
{
	DB *mdbp, *sdbp;
	int ret, t_ret;

	mdbp = sdbp = NULL;

	/* Open the subdatabase. */
	if ((ret = __db_create_internal(&sdbp, dbp->env, 0)) != 0)
		goto err;
	if (F_ISSET(dbp, DB_AM_NOT_DURABLE) &&
		(ret = __db_set_flags(sdbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;
	if ((ret = __db_open(sdbp, ip,
	    txn, name, subdb, DB_UNKNOWN, DB_WRITEOPEN, 0, PGNO_BASE_MD)) != 0)
		goto err;

	DB_TEST_RECOVERY(sdbp, DB_TEST_PREDESTROY, ret, name);

	/* Free up the pages in the subdatabase. */
	switch (sdbp->type) {
		case DB_BTREE:
		case DB_RECNO:
			if ((ret = __bam_reclaim(sdbp, ip, txn)) != 0)
				goto err;
			break;
		case DB_HASH:
			if ((ret = __ham_reclaim(sdbp, ip, txn)) != 0)
				goto err;
			break;
		case DB_QUEUE:
		case DB_UNKNOWN:
		default:
			ret = __db_unknown_type(
			    sdbp->env, "__db_subdb_remove", sdbp->type);
			goto err;
	}

	/*
	 * Remove the entry from the main database and free the subdatabase
	 * metadata page.
	 */
	if ((ret = __db_master_open(sdbp, ip, txn, name, 0, 0, &mdbp)) != 0)
		goto err;

	if ((ret = __db_master_update(mdbp,
	    sdbp, ip, txn, subdb, sdbp->type, MU_REMOVE, NULL, 0)) != 0)
		goto err;

	DB_TEST_RECOVERY(sdbp, DB_TEST_POSTDESTROY, ret, name);

DB_TEST_RECOVERY_LABEL
err:
	/* Close the main and subdatabases. */
	if ((t_ret = __db_close(sdbp, txn, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	if (mdbp != NULL &&
	    (t_ret = __db_close(mdbp, txn, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

static int
__db_dbtxn_remove(dbp, ip, txn, name, subdb)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	const char *name, *subdb;
{
	ENV *env;
	int ret;
	char *tmpname;

	env = dbp->env;
	tmpname = NULL;

	/*
	 * This is a transactional remove, so we have to keep the name
	 * of the file locked until the transaction commits.  As a result,
	 * we implement remove by renaming the file to some other name
	 * (which creates a dummy named file as a placeholder for the
	 * file being rename/dremoved) and then deleting that file as
	 * a delayed remove at commit.
	 */
	if ((ret = __db_backup_name(env,
	    F_ISSET(dbp, DB_AM_INMEM) ? subdb : name, txn, &tmpname)) != 0)
		return (ret);

	DB_TEST_RECOVERY(dbp, DB_TEST_PREDESTROY, ret, name);

	if ((ret = __db_rename_int(dbp,
	    txn->thread_info, txn, name, subdb, tmpname)) != 0)
		goto err;

	/*
	 * The internal removes will also translate into delayed removes.
	 */
	if (dbp->db_am_remove != NULL &&
	    (ret = dbp->db_am_remove(dbp, ip, txn, tmpname, NULL, 0)) != 0)
		goto err;

	ret = F_ISSET(dbp, DB_AM_INMEM) ?
	     __db_inmem_remove(dbp, txn, tmpname) :
	    __fop_remove(env,
	    txn, dbp->fileid, tmpname, &dbp->dirname, DB_APP_DATA,
	    F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0);

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTDESTROY, ret, name);

err:
DB_TEST_RECOVERY_LABEL
	if (tmpname != NULL)
		__os_free(env, tmpname);

	return (ret);
}
