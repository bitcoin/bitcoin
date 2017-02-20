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
#include "dbinc/hash.h"
#ifndef HAVE_QUEUE
#include "dbinc/qam.h"			/* For __db_no_queue_am(). */
#endif
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"
#include "dbinc/txn.h"

static int __db_associate_arg __P((DB *, DB *,
	       int (*)(DB *, const DBT *, const DBT *, DBT *), u_int32_t));
static int __dbc_del_arg __P((DBC *, u_int32_t));
static int __dbc_pget_arg __P((DBC *, DBT *, u_int32_t));
static int __dbc_put_arg __P((DBC *, DBT *, DBT *, u_int32_t));
static int __db_curinval __P((const ENV *));
static int __db_cursor_arg __P((DB *, u_int32_t));
static int __db_del_arg __P((DB *, DBT *, u_int32_t));
static int __db_get_arg __P((const DB *, DBT *, DBT *, u_int32_t));
static int __db_join_arg __P((DB *, DBC **, u_int32_t));
static int __db_open_arg __P((DB *,
	       DB_TXN *, const char *, const char *, DBTYPE, u_int32_t));
static int __db_pget_arg __P((DB *, DBT *, u_int32_t));
static int __db_put_arg __P((DB *, DBT *, DBT *, u_int32_t));
static int __dbt_ferr __P((const DB *, const char *, const DBT *, int));
static int __db_associate_foreign_arg __P((DB *, DB *,
		int (*)(DB *, const DBT *, DBT *, const DBT *, int *),
		u_int32_t));

/*
 * These functions implement the Berkeley DB API.  They are organized in a
 * layered fashion.  The interface functions (XXX_pp) perform all generic
 * error checks (for example, PANIC'd region, replication state change
 * in progress, inconsistent transaction usage), call function-specific
 * check routines (_arg) to check for proper flag usage, etc., do pre-amble
 * processing (incrementing handle counts, handling local transactions),
 * call the function and then do post-amble processing (local transactions,
 * decrement handle counts).
 *
 * The basic structure is:
 *	Check for simple/generic errors (PANIC'd region)
 *	Check if replication is changing state (increment handle count).
 *	Call function-specific argument checking routine
 *	Create internal transaction if necessary
 *	Call underlying worker function
 *	Commit/abort internal transaction if necessary
 *	Decrement handle count
 */

/*
 * __db_associate_pp --
 *	DB->associate pre/post processing.
 *
 * PUBLIC: int __db_associate_pp __P((DB *, DB_TXN *, DB *,
 * PUBLIC:     int (*)(DB *, const DBT *, const DBT *, DBT *), u_int32_t));
 */
int
__db_associate_pp(dbp, txn, sdbp, callback, flags)
	DB *dbp, *sdbp;
	DB_TXN *txn;
	int (*callback) __P((DB *, const DBT *, const DBT *, DBT *));
	u_int32_t flags;
{
	DBC *sdbc;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret, txn_local;

	env = dbp->env;
	txn_local = 0;

	STRIP_AUTO_COMMIT(flags);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	    (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	/*
	 * Secondary cursors may have the primary's lock file ID, so we need
	 * to make sure that no older cursors are lying around when we make
	 * the transition.
	 */
	if (TAILQ_FIRST(&sdbp->active_queue) != NULL ||
	    TAILQ_FIRST(&sdbp->join_queue) != NULL) {
		__db_errx(env,
    "Databases may not become secondary indices while cursors are open");
		ret = EINVAL;
		goto err;
	}

	if ((ret = __db_associate_arg(dbp, sdbp, callback, flags)) != 0)
		goto err;

	/*
	 * Create a local transaction as necessary, check for consistent
	 * transaction usage, and, if we have no transaction but do have
	 * locking on, acquire a locker id for the handle lock acquisition.
	 */
	if (IS_DB_AUTO_COMMIT(dbp, txn)) {
		if ((ret = __txn_begin(env, ip, NULL, &txn, 0)) != 0)
			goto err;
		txn_local = 1;
	}

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 0)) != 0)
		goto err;

	while ((sdbc = TAILQ_FIRST(&sdbp->free_queue)) != NULL)
		if ((ret = __dbc_destroy(sdbc)) != 0)
			goto err;

	ret = __db_associate(dbp, ip, txn, sdbp, callback, flags);

err:	if (txn_local &&
	    (t_ret = __db_txn_auto_resolve(env, txn, 0, ret)) && ret == 0)
		ret = t_ret;

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_associate_arg --
 *	Check DB->associate arguments.
 */
static int
__db_associate_arg(dbp, sdbp, callback, flags)
	DB *dbp, *sdbp;
	int (*callback) __P((DB *, const DBT *, const DBT *, DBT *));
	u_int32_t flags;
{
	ENV *env;
	int ret;

	env = dbp->env;

	if (F_ISSET(sdbp, DB_AM_SECONDARY)) {
		__db_errx(env,
		    "Secondary index handles may not be re-associated");
		return (EINVAL);
	}
	if (F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env,
		    "Secondary indices may not be used as primary databases");
		return (EINVAL);
	}
	if (F_ISSET(dbp, DB_AM_DUP)) {
		__db_errx(env,
		    "Primary databases may not be configured with duplicates");
		return (EINVAL);
	}
	if (F_ISSET(dbp, DB_AM_RENUMBER)) {
		__db_errx(env,
	    "Renumbering recno databases may not be used as primary databases");
		return (EINVAL);
	}

	/*
	 * It's OK for the primary and secondary to not share an environment IFF
	 * the environments are local to the DB handle.  (Specifically, cursor
	 * adjustment will work correctly in this case.)  The environment being
	 * local implies the environment is not configured for either locking or
	 * transactions, as neither of those could work correctly.
	 */
	if (dbp->env != sdbp->env &&
	    (!F_ISSET(dbp->env, ENV_DBLOCAL) ||
	     !F_ISSET(sdbp->env, ENV_DBLOCAL))) {
		__db_errx(env,
	    "The primary and secondary must be opened in the same environment");
		return (EINVAL);
	}
	if ((DB_IS_THREADED(dbp) && !DB_IS_THREADED(sdbp)) ||
	    (!DB_IS_THREADED(dbp) && DB_IS_THREADED(sdbp))) {
		__db_errx(env,
	    "The DB_THREAD setting must be the same for primary and secondary");
		return (EINVAL);
	}
	if (callback == NULL &&
	    (!F_ISSET(dbp, DB_AM_RDONLY) || !F_ISSET(sdbp, DB_AM_RDONLY))) {
		__db_errx(env,
    "Callback function may be NULL only when database handles are read-only");
		return (EINVAL);
	}

	if ((ret = __db_fchk(env, "DB->associate", flags, DB_CREATE |
	    DB_IMMUTABLE_KEY)) != 0)
		return (ret);

	return (0);
}

/*
 * __db_close_pp --
 *	DB->close pre/post processing.
 *
 * PUBLIC: int __db_close_pp __P((DB *, u_int32_t));
 */
int
__db_close_pp(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;
	ret = 0;

	/*
	 * Close a DB handle -- as a handle destructor, we can't fail.
	 *
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if (flags != 0 && flags != DB_NOSYNC)
		ret = __db_ferr(env, "DB->close", 0);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (t_ret = __db_rep_enter(dbp, 0, 0, 0)) != 0) {
		handle_check = 0;
		if (ret == 0)
			ret = t_ret;
	}

	if ((t_ret = __db_close(dbp, NULL, flags)) != 0 && ret == 0)
		ret = t_ret;

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_cursor_pp --
 *	DB->cursor pre/post processing.
 *
 * PUBLIC: int __db_cursor_pp __P((DB *, DB_TXN *, DBC **, u_int32_t));
 */
int
__db_cursor_pp(dbp, txn, dbcp, flags)
	DB *dbp;
	DB_TXN *txn;
	DBC **dbcp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	REGENV *renv;
	int rep_blocked, ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->cursor");

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	rep_blocked = 0;
	if (txn == NULL && IS_ENV_REPLICATED(env)) {
		if ((ret = __op_rep_enter(env)) != 0)
			goto err;
		rep_blocked = 1;
		renv = env->reginfo->primary;
		if (dbp->timestamp != renv->rep_timestamp) {
			__db_errx(env, "%s %s",
		    "replication recovery unrolled committed transactions;",
		    "open DB and DBcursor handles must be closed");
			ret = DB_REP_HANDLE_DEAD;
			goto err;
		}
	}
	if ((ret = __db_cursor_arg(dbp, flags)) != 0)
		goto err;

	/*
	 * Check for consistent transaction usage.  For now, assume this
	 * cursor might be used for read operations only (in which case
	 * it may not require a txn).  We'll check more stringently in
	 * c_del and c_put.  (Note this means the read-op txn tests have
	 * to be a subset of the write-op ones.)
	 */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 1)) != 0)
		goto err;

	ret = __db_cursor(dbp, ip, txn, dbcp, flags);

err:	/* Release replication block on error. */
	if (ret != 0 && rep_blocked)
		(void)__op_rep_exit(env);

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_cursor --
 *	DB->cursor.
 *
 * PUBLIC: int __db_cursor __P((DB *,
 * PUBLIC:      DB_THREAD_INFO *, DB_TXN *, DBC **, u_int32_t));
 */
int
__db_cursor(dbp, ip, txn, dbcp, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DBC **dbcp;
	u_int32_t flags;
{
	DBC *dbc;
	ENV *env;
	db_lockmode_t mode;
	int ret;

	env = dbp->env;

	if (MULTIVERSION(dbp) && txn == NULL && (LF_ISSET(DB_TXN_SNAPSHOT) ||
	    F_ISSET(env->dbenv, DB_ENV_TXN_SNAPSHOT))) {
		if ((ret =
		    __txn_begin(env, ip, NULL, &txn, DB_TXN_SNAPSHOT)) != 0)
			return (ret);
		F_SET(txn, TXN_PRIVATE);
	}

	if ((ret = __db_cursor_int(dbp, ip, txn, dbp->type, PGNO_INVALID,
	    LF_ISSET(DB_CURSOR_BULK | DB_CURSOR_TRANSIENT), NULL, &dbc)) != 0)
		return (ret);

	/*
	 * If this is CDB, do all the locking in the interface, which is
	 * right here.
	 */
	if (CDB_LOCKING(env)) {
		mode = (LF_ISSET(DB_WRITELOCK)) ? DB_LOCK_WRITE :
		    ((LF_ISSET(DB_WRITECURSOR) || txn != NULL) ?
		    DB_LOCK_IWRITE : DB_LOCK_READ);
		if ((ret = __lock_get(env, dbc->locker, 0,
		    &dbc->lock_dbt, mode, &dbc->mylock)) != 0)
			goto err;
		if (LF_ISSET(DB_WRITECURSOR))
			F_SET(dbc, DBC_WRITECURSOR);
		if (LF_ISSET(DB_WRITELOCK))
			F_SET(dbc, DBC_WRITER);
	}

	if (LF_ISSET(DB_READ_UNCOMMITTED) ||
	    (txn != NULL && F_ISSET(txn, TXN_READ_UNCOMMITTED)))
		F_SET(dbc, DBC_READ_UNCOMMITTED);

	if (LF_ISSET(DB_READ_COMMITTED) ||
	    (txn != NULL && F_ISSET(txn, TXN_READ_COMMITTED)))
		F_SET(dbc, DBC_READ_COMMITTED);

	*dbcp = dbc;
	return (0);

err:	(void)__dbc_close(dbc);
	return (ret);
}

/*
 * __db_cursor_arg --
 *	Check DB->cursor arguments.
 */
static int
__db_cursor_arg(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	ENV *env;

	env = dbp->env;

	/*
	 * DB_READ_COMMITTED and DB_READ_UNCOMMITTED require locking.
	 */
	if (LF_ISSET(DB_READ_COMMITTED | DB_READ_UNCOMMITTED)) {
		if (!LOCKING_ON(env))
			return (__db_fnl(env, "DB->cursor"));
	}

	LF_CLR(DB_CURSOR_BULK |
	    DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_TXN_SNAPSHOT);

	/* Check for invalid function flags. */
	if (LF_ISSET(DB_WRITECURSOR)) {
		if (DB_IS_READONLY(dbp))
			return (__db_rdonly(env, "DB->cursor"));
		if (!CDB_LOCKING(env))
			return (__db_ferr(env, "DB->cursor", 0));
		LF_CLR(DB_WRITECURSOR);
	} else if (LF_ISSET(DB_WRITELOCK)) {
		if (DB_IS_READONLY(dbp))
			return (__db_rdonly(env, "DB->cursor"));
		LF_CLR(DB_WRITELOCK);
	}

	if (flags != 0)
		return (__db_ferr(env, "DB->cursor", 0));

	return (0);
}

/*
 * __db_del_pp --
 *	DB->del pre/post processing.
 *
 * PUBLIC: int __db_del_pp __P((DB *, DB_TXN *, DBT *, u_int32_t));
 */
int
__db_del_pp(dbp, txn, key, flags)
	DB *dbp;
	DB_TXN *txn;
	DBT *key;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret, txn_local;

	env = dbp->env;
	txn_local = 0;

	STRIP_AUTO_COMMIT(flags);
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->del");

#ifdef CONFIG_TEST
	if (IS_REP_MASTER(env))
		DB_TEST_WAIT(env, env->test_check);
#endif
	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	     (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
			handle_check = 0;
			goto err;
	}

	if ((ret = __db_del_arg(dbp, key, flags)) != 0)
		goto err;

	/* Create local transaction as necessary. */
	if (IS_DB_AUTO_COMMIT(dbp, txn)) {
		if ((ret = __txn_begin(env, ip, NULL, &txn, 0)) != 0)
			goto err;
		txn_local = 1;
	}

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 0)) != 0)
		goto err;

	ret = __db_del(dbp, ip, txn, key, flags);

err:	if (txn_local &&
	    (t_ret = __db_txn_auto_resolve(env, txn, 0, ret)) && ret == 0)
		ret = t_ret;

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	__dbt_userfree(env, key, NULL, NULL);
	return (ret);
}

/*
 * __db_del_arg --
 *	Check DB->delete arguments.
 */
static int
__db_del_arg(dbp, key, flags)
	DB *dbp;
	DBT *key;
	u_int32_t flags;
{
	ENV *env;
	int ret;

	env = dbp->env;

	/* Check for changes to a read-only tree. */
	if (DB_IS_READONLY(dbp))
		return (__db_rdonly(env, "DB->del"));

	/* Check for invalid function flags. */
	switch (flags) {
	case DB_CONSUME:
		if (dbp->type != DB_QUEUE)
			return (__db_ferr(env, "DB->del", 0));
		goto copy;
	case DB_MULTIPLE:
	case DB_MULTIPLE_KEY:
		if (!F_ISSET(key, DB_DBT_BULK)) {
			__db_errx(env,
		"DB->del with DB_MULTIPLE(_KEY) requires multiple key records");
			return (EINVAL);
		}
		/* FALL THROUGH */
	case 0:
copy:		if ((ret = __dbt_usercopy(env, key)) != 0)
			return (ret);
		break;
	default:
		return (__db_ferr(env, "DB->del", 0));
	}

	return (0);
}

/*
 * __db_exists --
 *	DB->exists implementation.
 *
 * PUBLIC: int __db_exists __P((DB *, DB_TXN *, DBT *, u_int32_t));
 */
int
__db_exists(dbp, txn, key, flags)
	DB *dbp;
	DB_TXN *txn;
	DBT *key;
	u_int32_t flags;
{
	DBT data;
	int ret;

	/*
	 * Most flag checking is done in the DB->get call, we only check for
	 * specific incompatibilities here.  This saves making __get_arg
	 * aware of the exist method's API constraints.
	 */
	STRIP_AUTO_COMMIT(flags);	
	if ((ret = __db_fchk(dbp->env, "DB->exists", flags,
	    DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_RMW)) != 0)
		return (ret);

	/*
	 * Configure a data DBT that returns no bytes so there's no copy
	 * of the data.
	 */
	memset(&data, 0, sizeof(data));
	data.dlen = 0;
	data.flags = DB_DBT_PARTIAL | DB_DBT_USERMEM;

	return (dbp->get(dbp, txn, key, &data, flags));
}

/*
 * db_fd_pp --
 *	DB->fd pre/post processing.
 *
 * PUBLIC: int __db_fd_pp __P((DB *, int *));
 */
int
__db_fd_pp(dbp, fdp)
	DB *dbp;
	int *fdp;
{
	DB_FH *fhp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->fd");

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0)
		goto err;

	/*
	 * !!!
	 * There's no argument checking to be done.
	 *
	 * !!!
	 * The actual method call is simple, do it inline.
	 *
	 * XXX
	 * Truly spectacular layering violation.
	 */
	if ((ret = __mp_xxx_fh(dbp->mpf, &fhp)) == 0) {
		if (fhp == NULL) {
			*fdp = -1;
			__db_errx(env,
			    "Database does not have a valid file handle");
			ret = ENOENT;
		} else
			*fdp = fhp->fd;
	}

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_get_pp --
 *	DB->get pre/post processing.
 *
 * PUBLIC: int __db_get_pp __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
 */
int
__db_get_pp(dbp, txn, key, data, flags)
	DB *dbp;
	DB_TXN *txn;
	DBT *key, *data;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	u_int32_t mode;
	int handle_check, ignore_lease, ret, t_ret, txn_local;

	env = dbp->env;
	mode = 0;
	txn_local = 0;

	STRIP_AUTO_COMMIT(flags);
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->get");

	ignore_lease = LF_ISSET(DB_IGNORE_LEASE) ? 1 : 0;
	LF_CLR(DB_IGNORE_LEASE);

	if ((ret = __db_get_arg(dbp, key, data, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	     (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
			handle_check = 0;
			goto err;
	}

	if (LF_ISSET(DB_READ_UNCOMMITTED))
		mode = DB_READ_UNCOMMITTED;
	else if ((flags & DB_OPFLAGS_MASK) == DB_CONSUME ||
	    (flags & DB_OPFLAGS_MASK) == DB_CONSUME_WAIT) {
		mode = DB_WRITELOCK;
		if (IS_DB_AUTO_COMMIT(dbp, txn)) {
			if ((ret = __txn_begin(env, ip, NULL, &txn, 0)) != 0)
				goto err;
			txn_local = 1;
		}
	}

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID,
	    mode == DB_WRITELOCK || LF_ISSET(DB_RMW) ? 0 : 1)) != 0)
		goto err;

	ret = __db_get(dbp, ip, txn, key, data, flags);
	/*
	 * Check for master leases.
	 */
	if (ret == 0 &&
	    IS_REP_MASTER(env) && IS_USING_LEASES(env) && !ignore_lease)
		ret = __rep_lease_check(env, 1);

err:	if (txn_local &&
	    (t_ret = __db_txn_auto_resolve(env, txn, 0, ret)) && ret == 0)
		ret = t_ret;

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	__dbt_userfree(env, key, NULL, data);
	return (ret);
}

/*
 * __db_get --
 *	DB->get.
 *
 * PUBLIC: int __db_get __P((DB *,
 * PUBLIC:     DB_THREAD_INFO *, DB_TXN *, DBT *, DBT *, u_int32_t));
 */
int
__db_get(dbp, ip, txn, key, data, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DBT *key, *data;
	u_int32_t flags;
{
	DBC *dbc;
	u_int32_t mode;
	int ret, t_ret;

	/*
	 * The DB_CURSOR_TRANSIENT flag indicates that we're just doing a single
	 * operation with this cursor, and that in case of error we don't need
	 * to restore it to its old position.  Thus, we can perform the get
	 * without duplicating the cursor, saving some cycles in this common
	 * case.
	 */
	mode = DB_CURSOR_TRANSIENT;
	if (LF_ISSET(DB_READ_UNCOMMITTED)) {
		mode |= DB_READ_UNCOMMITTED;
		LF_CLR(DB_READ_UNCOMMITTED);
	} else if (LF_ISSET(DB_READ_COMMITTED)) {
		mode |= DB_READ_COMMITTED;
		LF_CLR(DB_READ_COMMITTED);
	} else if ((flags & DB_OPFLAGS_MASK) == DB_CONSUME ||
	    (flags & DB_OPFLAGS_MASK) == DB_CONSUME_WAIT)
		mode |= DB_WRITELOCK;

	if ((ret = __db_cursor(dbp, ip, txn, &dbc, mode)) != 0)
		return (ret);

	DEBUG_LREAD(dbc, txn, "DB->get", key, NULL, flags);

	/*
	 * The semantics of bulk gets are different for DB->get vs DBC->get.
	 * Mark the cursor so the low-level bulk get routines know which
	 * behavior we want.
	 */
	F_SET(dbc, DBC_FROM_DB_GET);

	/*
	 * SET_RET_MEM indicates that if key and/or data have no DBT
	 * flags set and DB manages the returned-data memory, that memory
	 * will belong to this handle, not to the underlying cursor.
	 */
	SET_RET_MEM(dbc, dbp);

	if (LF_ISSET(~(DB_RMW | DB_MULTIPLE)) == 0)
		LF_SET(DB_SET);

#ifdef HAVE_PARTITION
	if (F_ISSET(dbc, DBC_PARTITIONED))
		ret = __partc_get(dbc, key, data, flags);
	else
#endif
		ret = __dbc_get(dbc, key, data, flags);

	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_get_arg --
 *	DB->get argument checking, used by both DB->get and DB->pget.
 */
static int
__db_get_arg(dbp, key, data, flags)
	const DB *dbp;
	DBT *key, *data;
	u_int32_t flags;
{
	ENV *env;
	int dirty, multi, ret;

	env = dbp->env;

	/*
	 * Check for read-modify-write validity.  DB_RMW doesn't make sense
	 * with CDB cursors since if you're going to write the cursor, you
	 * had to create it with DB_WRITECURSOR.  Regardless, we check for
	 * LOCKING_ON and not STD_LOCKING, as we don't want to disallow it.
	 * If this changes, confirm that DB does not itself set the DB_RMW
	 * flag in a path where CDB may have been configured.
	 */
	dirty = 0;
	if (LF_ISSET(DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_RMW)) {
		if (!LOCKING_ON(env))
			return (__db_fnl(env, "DB->get"));
		if ((ret = __db_fcchk(env, "DB->get",
		    flags, DB_READ_UNCOMMITTED, DB_READ_COMMITTED)) != 0)
			return (ret);
		if (LF_ISSET(DB_READ_COMMITTED | DB_READ_UNCOMMITTED))
			dirty = 1;
		LF_CLR(DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_RMW);
	}

	multi = 0;
	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY)) {
		if (LF_ISSET(DB_MULTIPLE_KEY))
			goto multi_err;
		multi = LF_ISSET(DB_MULTIPLE) ? 1 : 0;
		LF_CLR(DB_MULTIPLE);
	}

	/* Check for invalid function flags. */
	switch (flags) {
	case DB_GET_BOTH:
		if ((ret = __dbt_usercopy(env, data)) != 0)
			return (ret);
		/* FALLTHROUGH */
	case 0:
		if ((ret = __dbt_usercopy(env, key)) != 0) {
			__dbt_userfree(env, key, NULL, data);
			return (ret);
		}
		break;
	case DB_SET_RECNO:
		if (!F_ISSET(dbp, DB_AM_RECNUM))
			goto err;
		if ((ret = __dbt_usercopy(env, key)) != 0)
			return (ret);
		break;
	case DB_CONSUME:
	case DB_CONSUME_WAIT:
		if (dirty) {
			__db_errx(env,
		    "%s is not supported with DB_CONSUME or DB_CONSUME_WAIT",
			     LF_ISSET(DB_READ_UNCOMMITTED) ?
			     "DB_READ_UNCOMMITTED" : "DB_READ_COMMITTED");
			return (EINVAL);
		}
		if (multi)
multi_err:		return (__db_ferr(env, "DB->get", 1));
		if (dbp->type == DB_QUEUE)
			break;
		/* FALLTHROUGH */
	default:
err:		return (__db_ferr(env, "DB->get", 0));
	}

	/*
	 * Check for invalid key/data flags.
	 */
	if ((ret =
	    __dbt_ferr(dbp, "key", key, DB_RETURNS_A_KEY(dbp, flags))) != 0)
		return (ret);
	if ((ret = __dbt_ferr(dbp, "data", data, 1)) != 0)
		return (ret);

	if (multi) {
		if (!F_ISSET(data, DB_DBT_USERMEM)) {
			__db_errx(env,
			    "DB_MULTIPLE requires DB_DBT_USERMEM be set");
			return (EINVAL);
		}
		if (F_ISSET(key, DB_DBT_PARTIAL) ||
		    F_ISSET(data, DB_DBT_PARTIAL)) {
			__db_errx(env,
			    "DB_MULTIPLE does not support DB_DBT_PARTIAL");
			return (EINVAL);
		}
		if (data->ulen < 1024 ||
		    data->ulen < dbp->pgsize || data->ulen % 1024 != 0) {
			__db_errx(env, "%s%s",
			    "DB_MULTIPLE buffers must be ",
			    "aligned, at least page size and multiples of 1KB");
			return (EINVAL);
		}
	}

	return (0);
}

/*
 * __db_join_pp --
 *	DB->join pre/post processing.
 *
 * PUBLIC: int __db_join_pp __P((DB *, DBC **, DBC **, u_int32_t));
 */
int
__db_join_pp(primary, curslist, dbcp, flags)
	DB *primary;
	DBC **curslist, **dbcp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = primary->env;

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret =
	    __db_rep_enter(primary, 1, 0, curslist[0]->txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	if ((ret = __db_join_arg(primary, curslist, flags)) == 0)
		ret = __db_join(primary, curslist, dbcp, flags);

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_join_arg --
 *	Check DB->join arguments.
 */
static int
__db_join_arg(primary, curslist, flags)
	DB *primary;
	DBC **curslist;
	u_int32_t flags;
{
	DB_TXN *txn;
	ENV *env;
	int i;

	env = primary->env;

	switch (flags) {
	case 0:
	case DB_JOIN_NOSORT:
		break;
	default:
		return (__db_ferr(env, "DB->join", 0));
	}

	if (curslist == NULL || curslist[0] == NULL) {
		__db_errx(env,
	    "At least one secondary cursor must be specified to DB->join");
		return (EINVAL);
	}

	txn = curslist[0]->txn;
	for (i = 1; curslist[i] != NULL; i++)
		if (curslist[i]->txn != txn) {
			__db_errx(env,
		    "All secondary cursors must share the same transaction");
			return (EINVAL);
		}

	return (0);
}

/*
 * __db_key_range_pp --
 *	DB->key_range pre/post processing.
 *
 * PUBLIC: int __db_key_range_pp
 * PUBLIC:     __P((DB *, DB_TXN *, DBT *, DB_KEY_RANGE *, u_int32_t));
 */
int
__db_key_range_pp(dbp, txn, key, kr, flags)
	DB *dbp;
	DB_TXN *txn;
	DBT *key;
	DB_KEY_RANGE *kr;
	u_int32_t flags;
{
	DBC *dbc;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->key_range");

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if (flags != 0)
		return (__db_ferr(env, "DB->key_range", 0));

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	     (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 1)) != 0)
		goto err;

	/*
	 * !!!
	 * The actual method call is simple, do it inline.
	 */
	switch (dbp->type) {
	case DB_BTREE:
#ifndef HAVE_BREW
		if ((ret = __dbt_usercopy(env, key)) != 0)
			goto err;

		/* Acquire a cursor. */
		if ((ret = __db_cursor(dbp, ip, txn, &dbc, 0)) != 0)
			break;

		DEBUG_LWRITE(dbc, NULL, "bam_key_range", NULL, NULL, 0);
#ifdef HAVE_PARTITION
		if (DB_IS_PARTITIONED(dbp))
			ret = __part_key_range(dbc, key, kr, flags);
		else
#endif
			ret = __bam_key_range(dbc, key, kr, flags);

		if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
			ret = t_ret;
		__dbt_userfree(env, key, NULL, NULL);
		break;
#else
		COMPQUIET(dbc, NULL);
		COMPQUIET(key, NULL);
		COMPQUIET(kr, NULL);
		/* FALLTHROUGH */
#endif
	case DB_HASH:
	case DB_QUEUE:
	case DB_RECNO:
		ret = __dbh_am_chk(dbp, DB_OK_BTREE);
		break;
	case DB_UNKNOWN:
	default:
		ret = __db_unknown_type(env, "DB->key_range", dbp->type);
		break;
	}

err:	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_open_pp --
 *	DB->open pre/post processing.
 *
 * PUBLIC: int __db_open_pp __P((DB *, DB_TXN *,
 * PUBLIC:     const char *, const char *, DBTYPE, u_int32_t, int));
 */
int
__db_open_pp(dbp, txn, fname, dname, type, flags, mode)
	DB *dbp;
	DB_TXN *txn;
	const char *fname, *dname;
	DBTYPE type;
	u_int32_t flags;
	int mode;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, nosync, remove_me, ret, t_ret, txn_local;

	env = dbp->env;
	nosync = 1;
	handle_check = remove_me = txn_local = 0;

	ENV_ENTER(env, ip);

	/*
	 * Save the file and database names and flags.  We do this here
	 * because we don't pass all of the flags down into the actual
	 * DB->open method call, we strip DB_AUTO_COMMIT at this layer.
	 */
	if ((fname != NULL &&
	    (ret = __os_strdup(env, fname, &dbp->fname)) != 0))
		goto err;
	if ((dname != NULL &&
	    (ret = __os_strdup(env, dname, &dbp->dname)) != 0))
		goto err;
	dbp->open_flags = flags;

	/* Save the current DB handle flags for refresh. */
	dbp->orig_flags = dbp->flags;

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	    (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
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
	} else if (txn != NULL && !TXN_ON(env) &&
	    (!CDB_LOCKING(env) || !F_ISSET(txn, TXN_CDSGROUP))) {
		ret = __db_not_txn_env(env);
		goto err;
	}
	LF_CLR(DB_AUTO_COMMIT);

	/*
	 * We check arguments after possibly creating a local transaction,
	 * which is unusual -- the reason is some flags are illegal if any
	 * kind of transaction is in effect.
	 */
	if ((ret = __db_open_arg(dbp, txn, fname, dname, type, flags)) == 0)
		if ((ret = __db_open(dbp, ip, txn, fname, dname, type,
		    flags, mode, PGNO_BASE_MD)) != 0)
			goto txnerr;

	/*
	 * You can open the database that describes the subdatabases in the
	 * rest of the file read-only.  The content of each key's data is
	 * unspecified and applications should never be adding new records
	 * or updating existing records.  However, during recovery, we need
	 * to open these databases R/W so we can redo/undo changes in them.
	 * Likewise, we need to open master databases read/write during
	 * rename and remove so we can be sure they're fully sync'ed, so
	 * we provide an override flag for the purpose.
	 */
	if (dname == NULL && !IS_RECOVERING(env) && !LF_ISSET(DB_RDONLY) &&
	    !LF_ISSET(DB_RDWRMASTER) && F_ISSET(dbp, DB_AM_SUBDB)) {
		__db_errx(env,
    "files containing multiple databases may only be opened read-only");
		ret = EINVAL;
		goto txnerr;
	}

	/*
	 * Success: file creations have to be synchronous, otherwise we don't
	 * care.
	 */
	if (F_ISSET(dbp, DB_AM_CREATED | DB_AM_CREATED_MSTR))
		nosync = 0;

	/* Success: don't discard the file on close. */
	F_CLR(dbp, DB_AM_DISCARD | DB_AM_CREATED | DB_AM_CREATED_MSTR);

	/*
	 * If not transactional, remove the databases/subdatabases if it is
	 * persistent.  If we're transactional, the child transaction abort
	 * cleans up.
	 */
txnerr:	if (ret != 0 && !IS_REAL_TXN(txn)) {
		remove_me = (F_ISSET(dbp, DB_AM_CREATED) &&
			(fname != NULL || dname != NULL)) ? 1 : 0;
		if (F_ISSET(dbp, DB_AM_CREATED_MSTR) ||
		    (dname == NULL && remove_me))
			/* Remove file. */
			(void)__db_remove_int(dbp,
			    ip, txn, fname, NULL, DB_FORCE);
		else if (remove_me)
			/* Remove subdatabase. */
			(void)__db_remove_int(dbp,
			    ip, txn, fname, dname, DB_FORCE);
	}

	if (txn_local && (t_ret =
	     __db_txn_auto_resolve(env, txn, nosync, ret)) && ret == 0)
		ret = t_ret;

err:	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_open_arg --
 *	Check DB->open arguments.
 */
static int
__db_open_arg(dbp, txn, fname, dname, type, flags)
	DB *dbp;
	DB_TXN *txn;
	const char *fname, *dname;
	DBTYPE type;
	u_int32_t flags;
{
	ENV *env;
	u_int32_t ok_flags;
	int ret;

	env = dbp->env;

	/* Validate arguments. */
#undef	OKFLAGS
#define	OKFLAGS								\
	(DB_AUTO_COMMIT | DB_CREATE | DB_EXCL | DB_FCNTL_LOCKING |	\
	DB_MULTIVERSION | DB_NOMMAP | DB_NO_AUTO_COMMIT | DB_RDONLY |	\
	DB_RDWRMASTER | DB_READ_UNCOMMITTED | DB_THREAD | DB_TRUNCATE)
	if ((ret = __db_fchk(env, "DB->open", flags, OKFLAGS)) != 0)
		return (ret);
	if (LF_ISSET(DB_EXCL) && !LF_ISSET(DB_CREATE))
		return (__db_ferr(env, "DB->open", 1));
	if (LF_ISSET(DB_RDONLY) && LF_ISSET(DB_CREATE))
		return (__db_ferr(env, "DB->open", 1));

#ifdef	HAVE_VXWORKS
	if (LF_ISSET(DB_TRUNCATE)) {
		__db_errx(env, "DB_TRUNCATE not supported on VxWorks");
		return (DB_OPNOTSUP);
	}
#endif
	switch (type) {
	case DB_UNKNOWN:
		if (LF_ISSET(DB_CREATE|DB_TRUNCATE)) {
			__db_errx(env,
	    "DB_UNKNOWN type specified with DB_CREATE or DB_TRUNCATE");
			return (EINVAL);
		}
		ok_flags = 0;
		break;
	case DB_BTREE:
		ok_flags = DB_OK_BTREE;
		break;
	case DB_HASH:
#ifndef HAVE_HASH
		return (__db_no_hash_am(env));
#endif
		ok_flags = DB_OK_HASH;
		break;
	case DB_QUEUE:
#ifndef HAVE_QUEUE
		return (__db_no_queue_am(env));
#endif
		ok_flags = DB_OK_QUEUE;
		break;
	case DB_RECNO:
		ok_flags = DB_OK_RECNO;
		break;
	default:
		__db_errx(env, "unknown type: %lu", (u_long)type);
		return (EINVAL);
	}
	if (ok_flags)
		DB_ILLEGAL_METHOD(dbp, ok_flags);

	/* The environment may have been created, but never opened. */
	if (!F_ISSET(env, ENV_DBLOCAL | ENV_OPEN_CALLED)) {
		__db_errx(env, "database environment not yet opened");
		return (EINVAL);
	}

	/*
	 * Historically, you could pass in an environment that didn't have a
	 * mpool, and DB would create a private one behind the scenes.  This
	 * no longer works.
	 */
	if (!F_ISSET(env, ENV_DBLOCAL) && !MPOOL_ON(env)) {
		__db_errx(env, "environment did not include a memory pool");
		return (EINVAL);
	}

	/*
	 * You can't specify threads during DB->open if subsystems in the
	 * environment weren't configured with them.
	 */
	if (LF_ISSET(DB_THREAD) && !F_ISSET(env, ENV_DBLOCAL | ENV_THREAD)) {
		__db_errx(env, "environment not created using DB_THREAD");
		return (EINVAL);
	}

	/* DB_MULTIVERSION requires a database configured for transactions. */
	if (LF_ISSET(DB_MULTIVERSION) && !IS_REAL_TXN(txn)) {
		__db_errx(env,
		    "DB_MULTIVERSION illegal without a transaction specified");
		return (EINVAL);
	}

	if (LF_ISSET(DB_MULTIVERSION) && type == DB_QUEUE) {
		__db_errx(env,
		    "DB_MULTIVERSION illegal with queue databases");
		return (EINVAL);
	}

	/* DB_TRUNCATE is neither transaction recoverable nor lockable. */
	if (LF_ISSET(DB_TRUNCATE) && (LOCKING_ON(env) || txn != NULL)) {
		__db_errx(env,
		    "DB_TRUNCATE illegal with %s specified",
		    LOCKING_ON(env) ? "locking" : "transactions");
		return (EINVAL);
	}

	/* Subdatabase checks. */
	if (dname != NULL) {
		/* QAM can only be done on in-memory subdatabases. */
		if (type == DB_QUEUE && fname != NULL) {
			__db_errx(
			    env, "Queue databases must be one-per-file");
			return (EINVAL);
		}

		/*
		 * Named in-memory databases can't support certain flags,
		 * so check here.
		 */
		if (fname == NULL)
			F_CLR(dbp, DB_AM_CHKSUM | DB_AM_ENCRYPT);
	}

	return (0);
}

/*
 * __db_pget_pp --
 *	DB->pget pre/post processing.
 *
 * PUBLIC: int __db_pget_pp
 * PUBLIC:     __P((DB *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t));
 */
int
__db_pget_pp(dbp, txn, skey, pkey, data, flags)
	DB *dbp;
	DB_TXN *txn;
	DBT *skey, *pkey, *data;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ignore_lease, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->pget");

	ignore_lease = LF_ISSET(DB_IGNORE_LEASE) ? 1 : 0;
	LF_CLR(DB_IGNORE_LEASE);

	if ((ret = __db_pget_arg(dbp, pkey, flags)) != 0 ||
	    (ret = __db_get_arg(dbp, skey, data, flags)) != 0) {
		__dbt_userfree(env, skey, pkey, data);
		return (ret);
	}

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	    (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	ret = __db_pget(dbp, ip, txn, skey, pkey, data, flags);
	/*
	 * Check for master leases.
	 */
	if (ret == 0 &&
	    IS_REP_MASTER(env) && IS_USING_LEASES(env) && !ignore_lease)
		ret = __rep_lease_check(env, 1);

err:	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	__dbt_userfree(env, skey, pkey, data);
	return (ret);
}

/*
 * __db_pget --
 *	DB->pget.
 *
 * PUBLIC: int __db_pget __P((DB *,
 * PUBLIC:     DB_THREAD_INFO *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t));
 */
int
__db_pget(dbp, ip, txn, skey, pkey, data, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	DBT *skey, *pkey, *data;
	u_int32_t flags;
{
	DBC *dbc;
	u_int32_t mode;
	int ret, t_ret;

	mode = DB_CURSOR_TRANSIENT;
	if (LF_ISSET(DB_READ_UNCOMMITTED)) {
		mode |= DB_READ_UNCOMMITTED;
		LF_CLR(DB_READ_UNCOMMITTED);
	} else if (LF_ISSET(DB_READ_COMMITTED)) {
		mode |= DB_READ_COMMITTED;
		LF_CLR(DB_READ_COMMITTED);
	}

	if ((ret = __db_cursor(dbp, ip, txn, &dbc, mode)) != 0)
		return (ret);

	SET_RET_MEM(dbc, dbp);

	DEBUG_LREAD(dbc, txn, "__db_pget", skey, NULL, flags);

	/*
	 * !!!
	 * The actual method call is simple, do it inline.
	 *
	 * The underlying cursor pget will fill in a default DBT for null
	 * pkeys, and use the cursor's returned-key memory internally to
	 * store any intermediate primary keys.  However, we've just set
	 * the returned-key memory to the DB handle's key memory, which
	 * is unsafe to use if the DB handle is threaded.  If the pkey
	 * argument is NULL, use the DBC-owned returned-key memory
	 * instead;  it'll go away when we close the cursor before we
	 * return, but in this case that's just fine, as we're not
	 * returning the primary key.
	 */
	if (pkey == NULL)
		dbc->rkey = &dbc->my_rkey;

	/*
	 * The cursor is just a perfectly ordinary secondary database cursor.
	 * Call its c_pget() method to do the dirty work.
	 */
	if (flags == 0 || flags == DB_RMW)
		flags |= DB_SET;

	ret = __dbc_pget(dbc, skey, pkey, data, flags);

	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_pget_arg --
 *	Check DB->pget arguments.
 */
static int
__db_pget_arg(dbp, pkey, flags)
	DB *dbp;
	DBT *pkey;
	u_int32_t flags;
{
	ENV *env;
	int ret;

	env = dbp->env;

	if (!F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env,
		    "DB->pget may only be used on secondary indices");
		return (EINVAL);
	}

	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY)) {
		__db_errx(env,
	"DB_MULTIPLE and DB_MULTIPLE_KEY may not be used on secondary indices");
		return (EINVAL);
	}

	/* DB_CONSUME makes no sense on a secondary index. */
	LF_CLR(DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_RMW);
	switch (flags) {
	case DB_CONSUME:
	case DB_CONSUME_WAIT:
		return (__db_ferr(env, "DB->pget", 0));
	default:
		/* __db_get_arg will catch the rest. */
		break;
	}

	/*
	 * We allow the pkey field to be NULL, so that we can make the
	 * two-DBT get calls into wrappers for the three-DBT ones.
	 */
	if (pkey != NULL &&
	    (ret = __dbt_ferr(dbp, "primary key", pkey, 1)) != 0)
		return (ret);

	if (flags == DB_GET_BOTH) {
		/* The pkey field can't be NULL if we're doing a DB_GET_BOTH. */
		if (pkey == NULL) {
			__db_errx(env,
		    "DB_GET_BOTH on a secondary index requires a primary key");
			return (EINVAL);
		}
		if ((ret = __dbt_usercopy(env, pkey)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __db_put_pp --
 *	DB->put pre/post processing.
 *
 * PUBLIC: int __db_put_pp __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
 */
int
__db_put_pp(dbp, txn, key, data, flags)
	DB *dbp;
	DB_TXN *txn;
	DBT *key, *data;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, txn_local, t_ret;

	env = dbp->env;
	txn_local = 0;

	STRIP_AUTO_COMMIT(flags);
	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->put");

	if ((ret = __db_put_arg(dbp, key, data, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	    (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	/* Create local transaction as necessary. */
	if (IS_DB_AUTO_COMMIT(dbp, txn)) {
		if ((ret = __txn_begin(env, ip, NULL, &txn, 0)) != 0)
			goto err;
		txn_local = 1;
	}

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 0)) != 0)
		goto err;

	ret = __db_put(dbp, ip, txn, key, data, flags);

err:	if (txn_local &&
	    (t_ret = __db_txn_auto_resolve(env, txn, 0, ret)) && ret == 0)
		ret = t_ret;

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	__dbt_userfree(env, key, NULL, data);
	return (ret);
}

/*
 * __db_put_arg --
 *	Check DB->put arguments.
 */
static int
__db_put_arg(dbp, key, data, flags)
	DB *dbp;
	DBT *key, *data;
	u_int32_t flags;
{
	ENV *env;
	int ret, returnkey;

	env = dbp->env;
	returnkey = 0;

	/* Check for changes to a read-only tree. */
	if (DB_IS_READONLY(dbp))
		return (__db_rdonly(env, "DB->put"));

	/* Check for puts on a secondary. */
	if (F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env, "DB->put forbidden on secondary indices");
		return (EINVAL);
	}

	if (LF_ISSET(DB_MULTIPLE_KEY | DB_MULTIPLE)) {
		if (LF_ISSET(DB_MULTIPLE) && LF_ISSET(DB_MULTIPLE_KEY))
			goto err;

		switch (LF_ISSET(DB_OPFLAGS_MASK)) {
		case 0:
		case DB_OVERWRITE_DUP:
			break;
		default:
			__db_errx(env,
       "DB->put: DB_MULTIPLE(_KEY) can only be combined with DB_OVERWRITE_DUP");
			return (EINVAL);
		}

		if (!F_ISSET(key, DB_DBT_BULK)) {
			__db_errx(env,
		   "DB->put with DB_MULTIPLE(_KEY) requires a bulk key buffer");
			return (EINVAL);
		}
	}
	if (LF_ISSET(DB_MULTIPLE)) {
		if (!F_ISSET(data, DB_DBT_BULK)) {
			__db_errx(env,
			"DB->put with DB_MULTIPLE requires a bulk data buffer");
			return (EINVAL);
		}
	}

	/* Check for invalid function flags. */
	switch (LF_ISSET(DB_OPFLAGS_MASK)) {
	case 0:
	case DB_NOOVERWRITE:
	case DB_OVERWRITE_DUP:
		break;
	case DB_APPEND:
		if (dbp->type != DB_RECNO && dbp->type != DB_QUEUE)
			goto err;
		returnkey = 1;
		break;
	case DB_NODUPDATA:
		if (F_ISSET(dbp, DB_AM_DUPSORT))
			break;
		/* FALLTHROUGH */
	default:
err:		return (__db_ferr(env, "DB->put", 0));
	}

	/*
	 * Check for invalid key/data flags.  The key may reasonably be NULL
	 * if DB_APPEND is set and the application doesn't care about the
	 * returned key.
	 */
	if (((returnkey && key != NULL) || !returnkey) &&
	    (ret = __dbt_ferr(dbp, "key", key, returnkey)) != 0)
		return (ret);
	if (!LF_ISSET(DB_MULTIPLE_KEY) &&
	    (ret = __dbt_ferr(dbp, "data", data, 0)) != 0)
		return (ret);

	/*
	 * The key parameter should not be NULL or have the "partial" flag set
	 * in a put call unless the user doesn't care about a key value we'd
	 * return.  The user tells us they don't care about the returned key by
	 * setting the key parameter to NULL or configuring the key DBT to not
	 * return any information.  (Returned keys from a put are always record
	 * numbers, and returning part of a record number  doesn't make sense:
	 * only accept a partial return if the length returned is 0.)
	 */
	if ((returnkey &&
	    key != NULL && F_ISSET(key, DB_DBT_PARTIAL) && key->dlen != 0) ||
	    (!returnkey && F_ISSET(key, DB_DBT_PARTIAL)))
		return (__db_ferr(env, "key DBT", 0));

	/* Check for partial puts in the presence of duplicates. */
	if (data != NULL && F_ISSET(data, DB_DBT_PARTIAL) &&
	    (F_ISSET(dbp, DB_AM_DUP) || F_ISSET(key, DB_DBT_DUPOK))) {
		__db_errx(env,
"a partial put in the presence of duplicates requires a cursor operation");
		return (EINVAL);
	}

	if ((flags != DB_APPEND && (ret = __dbt_usercopy(env, key)) != 0) ||
	    (!LF_ISSET(DB_MULTIPLE_KEY) &&
	    (ret = __dbt_usercopy(env, data)) != 0))
		return (ret);

	return (0);
}

/*
 * __db_compact_pp --
 *	DB->compact pre/post processing.
 *
 * PUBLIC: int __db_compact_pp __P((DB *, DB_TXN *,
 * PUBLIC:       DBT *, DBT *, DB_COMPACT *, u_int32_t, DBT *));
 */
int
__db_compact_pp(dbp, txn, start, stop, c_data, flags, end)
	DB *dbp;
	DB_TXN *txn;
	DBT *start, *stop;
	DB_COMPACT *c_data;
	u_int32_t flags;
	DBT *end;
{
	DB_COMPACT *dp, l_data;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->compact");

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if ((ret = __db_fchk(
	    env, "DB->compact", flags, DB_FREELIST_ONLY | DB_FREE_SPACE)) != 0)
		return (ret);

	/* Check for changes to a read-only database. */
	if (DB_IS_READONLY(dbp))
		return (__db_rdonly(env, "DB->compact"));

	if (start != NULL && (ret = __dbt_usercopy(env, start)) != 0)
		return (ret);
	if (stop != NULL && (ret = __dbt_usercopy(env, stop)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0,
	    txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	if (c_data == NULL) {
		dp = &l_data;
		memset(dp, 0, sizeof(*dp));
	} else
		dp = c_data;
#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbp))
		ret = __part_compact(dbp, ip, txn, start, stop, dp, flags, end);
	else
#endif
	switch (dbp->type) {
	case DB_HASH:
		if (!LF_ISSET(DB_FREELIST_ONLY))
			goto err;
		/* FALLTHROUGH */
	case DB_BTREE:
	case DB_RECNO:
		ret = __bam_compact(dbp, ip, txn, start, stop, dp, flags, end);
		break;

	default:
err:		ret = __dbh_am_chk(dbp, DB_OK_BTREE);
		break;
	}

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	__dbt_userfree(env, start, stop, NULL);
	return (ret);
}

/*
 * __db_associate_foreign_pp --
 *	DB->associate_foreign pre/post processing.
 *
 * PUBLIC: int __db_associate_foreign_pp __P((DB *, DB *,
 * PUBLIC:     int (*)(DB *, const DBT *, DBT *, const DBT *, int *),
 * PUBLIC:     u_int32_t));
 */
int
__db_associate_foreign_pp(fdbp, dbp, callback, flags)
	DB *dbp, *fdbp;
	int (*callback) __P((DB *, const DBT *, DBT *, const DBT *, int *));
	u_int32_t flags;
{
	/* Most of this is based on the implementation of associate */
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	PANIC_CHECK(env);
	STRIP_AUTO_COMMIT(flags);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	    (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}

	if ((ret = __db_associate_foreign_arg(fdbp, dbp, callback, flags)) != 0)
		goto err;

	ret = __db_associate_foreign(fdbp, dbp, callback, flags);

err:	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_associate_foreign_arg --
 *	DB->associate_foreign argument checking.
 */
static int
__db_associate_foreign_arg(fdbp, dbp, callback, flags)
	DB *dbp, *fdbp;
	int (*callback) __P((DB *, const DBT *, DBT *, const DBT *, int *));
	u_int32_t flags;
{
	ENV *env;

	env = fdbp->env;

	if (F_ISSET(fdbp, DB_AM_SECONDARY)) {
		__db_errx(env,
		    "Secondary indices may not be used as foreign databases");
		return (EINVAL);
	}
	if (F_ISSET(fdbp, DB_AM_DUP)) {
		__db_errx(env,
		    "Foreign databases may not be configured with duplicates");
		return (EINVAL);
	}
	if (F_ISSET(fdbp, DB_AM_RENUMBER)) {
		__db_errx(env,
	    "Renumbering recno databases may not be used as foreign databases");
		return (EINVAL);
	}
	if (!F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env,
		    "The associating database must be a secondary index.");
		return (EINVAL);
	}
	if (LF_ISSET(DB_FOREIGN_NULLIFY) && callback == NULL) {
		__db_errx(env,
		    "When specifying a delete action of nullify, a callback%s",
		    " function needs to be configured");
		return (EINVAL);
	} else if (!LF_ISSET(DB_FOREIGN_NULLIFY) && callback != NULL) {
		__db_errx(env,
		    "When not specifying a delete action of nullify, a%s",
		    " callback function cannot be configured");
		return (EINVAL);
	}

	return (0);
}

/*
 * __db_sync_pp --
 *	DB->sync pre/post processing.
 *
 * PUBLIC: int __db_sync_pp __P((DB *, u_int32_t));
 */
int
__db_sync_pp(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->sync");

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if (flags != 0)
		return (__db_ferr(env, "DB->sync", 0));

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}

	ret = __db_sync(dbp);

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_close_pp --
 *	DBC->close pre/post processing.
 *
 * PUBLIC: int __dbc_close_pp __P((DBC *));
 */
int
__dbc_close_pp(dbc)
	DBC *dbc;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	dbp = dbc->dbp;
	env = dbp->env;

	/*
	 * If the cursor is already closed we have a serious problem, and we
	 * assume that the cursor isn't on the active queue.  Don't do any of
	 * the remaining cursor close processing.
	 */
	if (!F_ISSET(dbc, DBC_ACTIVE)) {
		__db_errx(env, "Closing already-closed cursor");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = dbc->txn == NULL && IS_ENV_REPLICATED(env);
	ret = __dbc_close(dbc);

	/* Release replication block. */
	if (handle_check &&
	    (t_ret = __op_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_cmp_pp --
 *	DBC->cmp pre/post processing.
 *
 * PUBLIC: int __dbc_cmp_pp __P((DBC *, DBC *, int*, u_int32_t));
 */
int
__dbc_cmp_pp(dbc, other_cursor, result, flags)
	DBC *dbc, *other_cursor;
	int *result;
	u_int32_t flags;
{
	DB *dbp, *odbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	odbp = other_cursor->dbp;
	env = dbp->env;

	if (flags != 0)
		return (__db_ferr(env, "DBcursor->cmp", 0));

	if (other_cursor == NULL) {
		__db_errx(env, "DBcursor->cmp dbc pointer must not be null");
		return (EINVAL);
	}

	if (dbp != odbp) {
		__db_errx(env, 
"DBcursor->cmp both cursors must refer to the same database.");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);
	ret = __dbc_cmp(dbc, other_cursor, result);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_count_pp --
 *	DBC->count pre/post processing.
 *
 * PUBLIC: int __dbc_count_pp __P((DBC *, db_recno_t *, u_int32_t));
 */
int
__dbc_count_pp(dbc, recnop, flags)
	DBC *dbc;
	db_recno_t *recnop;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	env = dbp->env;

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 *
	 * The cursor must be initialized, return EINVAL for an invalid cursor.
	 */
	if (flags != 0)
		return (__db_ferr(env, "DBcursor->count", 0));

	if (!IS_INITIALIZED(dbc))
		return (__db_curinval(env));

	ENV_ENTER(env, ip);
	ret = __dbc_count(dbc, recnop);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_del_pp --
 *	DBC->del pre/post processing.
 *
 * PUBLIC: int __dbc_del_pp __P((DBC *, u_int32_t));
 */
int
__dbc_del_pp(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	env = dbp->env;

	if ((ret = __dbc_del_arg(dbc, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, dbc->txn, dbc->locker, 0)) != 0)
		goto err;

	DEBUG_LWRITE(dbc, dbc->txn, "DBcursor->del", NULL, NULL, flags);
	ret = __dbc_del(dbc, flags);

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_del_arg --
 *	Check DBC->del arguments.
 */
static int
__dbc_del_arg(dbc, flags)
	DBC *dbc;
	u_int32_t flags;
{
	DB *dbp;
	ENV *env;

	dbp = dbc->dbp;
	env = dbp->env;

	/* Check for changes to a read-only tree. */
	if (DB_IS_READONLY(dbp))
		return (__db_rdonly(env, "DBcursor->del"));

	/* Check for invalid function flags. */
	switch (flags) {
	case 0:
		break;
	case DB_CONSUME:
		if (dbp->type != DB_QUEUE)
			return (__db_ferr(env, "DBC->del", 0));
		break;
	case DB_UPDATE_SECONDARY:
		DB_ASSERT(env, F_ISSET(dbp, DB_AM_SECONDARY));
		break;
	default:
		return (__db_ferr(env, "DBcursor->del", 0));
	}

	/*
	 * The cursor must be initialized, return EINVAL for an invalid cursor,
	 * otherwise 0.
	 */
	if (!IS_INITIALIZED(dbc))
		return (__db_curinval(env));

	return (0);
}

/*
 * __dbc_dup_pp --
 *	DBC->dup pre/post processing.
 *
 * PUBLIC: int __dbc_dup_pp __P((DBC *, DBC **, u_int32_t));
 */
int
__dbc_dup_pp(dbc, dbcp, flags)
	DBC *dbc, **dbcp;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	env = dbp->env;

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if (flags != 0 && flags != DB_POSITION)
		return (__db_ferr(env, "DBcursor->dup", 0));

	ENV_ENTER(env, ip);
	ret = __dbc_dup(dbc, dbcp, flags);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_get_pp --
 *	DBC->get pre/post processing.
 *
 * PUBLIC: int __dbc_get_pp __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_get_pp(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ignore_lease, ret;

	dbp = dbc->dbp;
	env = dbp->env;

	ignore_lease = LF_ISSET(DB_IGNORE_LEASE) ? 1 : 0;
	LF_CLR(DB_IGNORE_LEASE);
	if ((ret = __dbc_get_arg(dbc, key, data, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	DEBUG_LREAD(dbc, dbc->txn, "DBcursor->get",
	    flags == DB_SET || flags == DB_SET_RANGE ? key : NULL, NULL, flags);
	ret = __dbc_get(dbc, key, data, flags);

	/*
	 * Check for master leases.
	 */
	if (ret == 0 &&
	    IS_REP_MASTER(env) && IS_USING_LEASES(env) && !ignore_lease)
		ret = __rep_lease_check(env, 1);

	ENV_LEAVE(env, ip);
	__dbt_userfree(env, key, NULL, data);
	return (ret);
}

/*
 * __dbc_get_arg --
 *	Common DBC->get argument checking, used by both DBC->get and DBC->pget.
 * PUBLIC: int __dbc_get_arg __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_get_arg(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp;
	ENV *env;
	int dirty, multi, ret;

	dbp = dbc->dbp;
	env = dbp->env;

	/*
	 * Typically in checking routines that modify the flags, we have
	 * to save them and restore them, because the checking routine
	 * calls the work routine.  However, this is a pure-checking
	 * routine which returns to a function that calls the work routine,
	 * so it's OK that we do not save and restore the flags, even though
	 * we modify them.
	 *
	 * Check for read-modify-write validity.  DB_RMW doesn't make sense
	 * with CDB cursors since if you're going to write the cursor, you
	 * had to create it with DB_WRITECURSOR.  Regardless, we check for
	 * LOCKING_ON and not STD_LOCKING, as we don't want to disallow it.
	 * If this changes, confirm that DB does not itself set the DB_RMW
	 * flag in a path where CDB may have been configured.
	 */
	dirty = 0;
	if (LF_ISSET(DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_RMW)) {
		if (!LOCKING_ON(env))
			return (__db_fnl(env, "DBcursor->get"));
		if (LF_ISSET(DB_READ_UNCOMMITTED))
			dirty = 1;
		LF_CLR(DB_READ_COMMITTED | DB_READ_UNCOMMITTED | DB_RMW);
	}

	multi = 0;
	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY)) {
		multi = 1;
		if (LF_ISSET(DB_MULTIPLE) && LF_ISSET(DB_MULTIPLE_KEY))
			goto multi_err;
		LF_CLR(DB_MULTIPLE | DB_MULTIPLE_KEY);
	}

	/* Check for invalid function flags. */
	switch (flags) {
	case DB_CONSUME:
	case DB_CONSUME_WAIT:
		if (dirty) {
			__db_errx(env,
    "DB_READ_UNCOMMITTED is not supported with DB_CONSUME or DB_CONSUME_WAIT");
			return (EINVAL);
		}
		if (dbp->type != DB_QUEUE)
			goto err;
		break;
	case DB_CURRENT:
	case DB_FIRST:
	case DB_NEXT:
	case DB_NEXT_DUP:
	case DB_NEXT_NODUP:
		break;
	case DB_LAST:
	case DB_PREV:
	case DB_PREV_DUP:
	case DB_PREV_NODUP:
		if (multi)
multi_err:		return (__db_ferr(env, "DBcursor->get", 1));
		break;
	case DB_GET_BOTHC:
		if (dbp->type == DB_QUEUE)
			goto err;
		/* FALLTHROUGH */
	case DB_GET_BOTH:
	case DB_GET_BOTH_RANGE:
		if ((ret = __dbt_usercopy(env, data)) != 0)
			goto err;
		/* FALLTHROUGH */
	case DB_SET:
	case DB_SET_RANGE:
		if ((ret = __dbt_usercopy(env, key)) != 0)
			goto err;
		break;
	case DB_GET_RECNO:
		/*
		 * The one situation in which this might be legal with a
		 * non-RECNUM dbp is if dbp is a secondary and its primary is
		 * DB_AM_RECNUM.
		 */
		if (!F_ISSET(dbp, DB_AM_RECNUM) &&
		    (!F_ISSET(dbp, DB_AM_SECONDARY) ||
		    !F_ISSET(dbp->s_primary, DB_AM_RECNUM)))
			goto err;
		break;
	case DB_SET_RECNO:
		if (!F_ISSET(dbp, DB_AM_RECNUM))
			goto err;
		if ((ret = __dbt_usercopy(env, key)) != 0)
			goto err;
		break;
	default:
err:		__dbt_userfree(env, key, NULL, data);
		return (__db_ferr(env, "DBcursor->get", 0));
	}

	/* Check for invalid key/data flags. */
	if ((ret = __dbt_ferr(dbp, "key", key, 0)) != 0)
		return (ret);
	if ((ret = __dbt_ferr(dbp, "data", data, 0)) != 0)
		return (ret);

	if (multi) {
		if (!F_ISSET(data, DB_DBT_USERMEM)) {
			__db_errx(env,
	    "DB_MULTIPLE/DB_MULTIPLE_KEY require DB_DBT_USERMEM be set");
			return (EINVAL);
		}
		if (F_ISSET(key, DB_DBT_PARTIAL) ||
		    F_ISSET(data, DB_DBT_PARTIAL)) {
			__db_errx(env,
	    "DB_MULTIPLE/DB_MULTIPLE_KEY do not support DB_DBT_PARTIAL");
			return (EINVAL);
		}
		if (data->ulen < 1024 ||
		    data->ulen < dbp->pgsize || data->ulen % 1024 != 0) {
			__db_errx(env, "%s%s",
			    "DB_MULTIPLE/DB_MULTIPLE_KEY buffers must be ",
			    "aligned, at least page size and multiples of 1KB");
			return (EINVAL);
		}
	}

	/*
	 * The cursor must be initialized for DB_CURRENT, DB_GET_RECNO,
	 * DB_PREV_DUP and DB_NEXT_DUP.  Return EINVAL for an invalid
	 * cursor, otherwise 0.
	 */
	if (!IS_INITIALIZED(dbc) && (flags == DB_CURRENT ||
	    flags == DB_GET_RECNO ||
	    flags == DB_NEXT_DUP || flags == DB_PREV_DUP))
		return (__db_curinval(env));

	/* Check for consistent transaction usage. */
	if (LF_ISSET(DB_RMW) &&
	    (ret = __db_check_txn(dbp, dbc->txn, dbc->locker, 0)) != 0)
		return (ret);

	return (0);
}

/*
 * __db_secondary_close_pp --
 *	DB->close for secondaries
 *
 * PUBLIC: int __db_secondary_close_pp __P((DB *, u_int32_t));
 */
int
__db_secondary_close_pp(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;
	ret = 0;

	/*
	 * As a DB handle destructor, we can't fail.
	 *
	 * !!!
	 * The actual argument checking is simple, do it inline, outside of
	 * the replication block.
	 */
	if (flags != 0 && flags != DB_NOSYNC)
		ret = __db_ferr(env, "DB->close", 0);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (t_ret = __db_rep_enter(dbp, 0, 0, 0)) != 0) {
		handle_check = 0;
		if (ret == 0)
			ret = t_ret;
	}

	if ((t_ret = __db_secondary_close(dbp, flags)) != 0 && ret == 0)
		ret = t_ret;

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __dbc_pget_pp --
 *	DBC->pget pre/post processing.
 *
 * PUBLIC: int __dbc_pget_pp __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_pget_pp(dbc, skey, pkey, data, flags)
	DBC *dbc;
	DBT *skey, *pkey, *data;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ignore_lease, ret;

	dbp = dbc->dbp;
	env = dbp->env;

	ignore_lease = LF_ISSET(DB_IGNORE_LEASE) ? 1 : 0;
	LF_CLR(DB_IGNORE_LEASE);
	if ((ret = __dbc_pget_arg(dbc, pkey, flags)) != 0 ||
	    (ret = __dbc_get_arg(dbc, skey, data, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	ret = __dbc_pget(dbc, skey, pkey, data, flags);
	/*
	 * Check for master leases.
	 */
	if (ret == 0 &&
	    IS_REP_MASTER(env) && IS_USING_LEASES(env) && !ignore_lease)
		ret = __rep_lease_check(env, 1);

	ENV_LEAVE(env, ip);

	__dbt_userfree(env, skey, pkey, data);
	return (ret);
}

/*
 * __dbc_pget_arg --
 *	Check DBC->pget arguments.
 */
static int
__dbc_pget_arg(dbc, pkey, flags)
	DBC *dbc;
	DBT *pkey;
	u_int32_t flags;
{
	DB *dbp;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	env = dbp->env;

	if (!F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env,
		    "DBcursor->pget may only be used on secondary indices");
		return (EINVAL);
	}

	if (LF_ISSET(DB_MULTIPLE | DB_MULTIPLE_KEY)) {
		__db_errx(env,
	"DB_MULTIPLE and DB_MULTIPLE_KEY may not be used on secondary indices");
		return (EINVAL);
	}

	switch (LF_ISSET(DB_OPFLAGS_MASK)) {
	case DB_CONSUME:
	case DB_CONSUME_WAIT:
		/* These flags make no sense on a secondary index. */
		return (__db_ferr(env, "DBcursor->pget", 0));
	case DB_GET_BOTH:
	case DB_GET_BOTH_RANGE:
		/* BOTH is "get both the primary and the secondary". */
		if (pkey == NULL) {
			__db_errx(env,
			    "%s requires both a secondary and a primary key",
			     LF_ISSET(DB_GET_BOTH) ?
			     "DB_GET_BOTH" : "DB_GET_BOTH_RANGE");
			return (EINVAL);
		}
		if ((ret = __dbt_usercopy(env, pkey)) != 0)
			return (ret);
		break;
	default:
		/* __dbc_get_arg will catch the rest. */
		break;
	}

	/*
	 * We allow the pkey field to be NULL, so that we can make the
	 * two-DBT get calls into wrappers for the three-DBT ones.
	 */
	if (pkey != NULL &&
	    (ret = __dbt_ferr(dbp, "primary key", pkey, 0)) != 0)
		return (ret);

	/* But the pkey field can't be NULL if we're doing a DB_GET_BOTH. */
	if (pkey == NULL && (flags & DB_OPFLAGS_MASK) == DB_GET_BOTH) {
		__db_errx(env,
		    "DB_GET_BOTH on a secondary index requires a primary key");
		return (EINVAL);
	}
	return (0);
}

/*
 * __dbc_put_pp --
 *	DBC->put pre/post processing.
 *
 * PUBLIC: int __dbc_put_pp __P((DBC *, DBT *, DBT *, u_int32_t));
 */
int
__dbc_put_pp(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	dbp = dbc->dbp;
	env = dbp->env;

	if ((ret = __dbc_put_arg(dbc, key, data, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, dbc->txn, dbc->locker, 0)) != 0)
		goto err;

	DEBUG_LWRITE(dbc, dbc->txn, "DBcursor->put",
	    flags == DB_KEYFIRST || flags == DB_KEYLAST ||
	    flags == DB_NODUPDATA || flags == DB_UPDATE_SECONDARY ?
	    key : NULL, data, flags);
	ret = __dbc_put(dbc, key, data, flags);

err:	ENV_LEAVE(env, ip);
	__dbt_userfree(env, key, NULL, data);
	return (ret);
}

/*
 * __dbc_put_arg --
 *	Check DBC->put arguments.
 */
static int
__dbc_put_arg(dbc, key, data, flags)
	DBC *dbc;
	DBT *key, *data;
	u_int32_t flags;
{
	DB *dbp;
	ENV *env;
	int key_flags, ret;

	dbp = dbc->dbp;
	env = dbp->env;
	key_flags = 0;

	/* Check for changes to a read-only tree. */
	if (DB_IS_READONLY(dbp))
		return (__db_rdonly(env, "DBcursor->put"));

	/* Check for puts on a secondary. */
	if (F_ISSET(dbp, DB_AM_SECONDARY)) {
		if (flags == DB_UPDATE_SECONDARY)
			flags = 0;
		else {
			__db_errx(env,
			    "DBcursor->put forbidden on secondary indices");
			return (EINVAL);
		}
	}

	if ((ret = __dbt_usercopy(env, data)) != 0)
		return (ret);

	/* Check for invalid function flags. */
	switch (flags) {
	case DB_AFTER:
	case DB_BEFORE:
		switch (dbp->type) {
		case DB_BTREE:
		case DB_HASH:		/* Only with unsorted duplicates. */
			if (!F_ISSET(dbp, DB_AM_DUP))
				goto err;
			if (dbp->dup_compare != NULL)
				goto err;
			break;
		case DB_QUEUE:		/* Not permitted. */
			goto err;
		case DB_RECNO:		/* Only with mutable record numbers. */
			if (!F_ISSET(dbp, DB_AM_RENUMBER))
				goto err;
			key_flags = key == NULL ? 0 : 1;
			break;
		case DB_UNKNOWN:
		default:
			goto err;
		}
		break;
	case DB_CURRENT:
		/*
		 * If there is a comparison function, doing a DB_CURRENT
		 * must not change the part of the data item that is used
		 * for the comparison.
		 */
		break;
	case DB_NODUPDATA:
		if (!F_ISSET(dbp, DB_AM_DUPSORT))
			goto err;
		/* FALLTHROUGH */
	case DB_KEYFIRST:
	case DB_KEYLAST:
	case DB_OVERWRITE_DUP:
		key_flags = 1;
		if ((ret = __dbt_usercopy(env, key)) != 0)
			return (ret);
		break;
	default:
err:		return (__db_ferr(env, "DBcursor->put", 0));
	}

	/*
	 * Check for invalid key/data flags.  The key may reasonably be NULL
	 * if DB_AFTER or DB_BEFORE is set and the application doesn't care
	 * about the returned key, or if the DB_CURRENT flag is set.
	 */
	if (key_flags && (ret = __dbt_ferr(dbp, "key", key, 0)) != 0)
		return (ret);
	if ((ret = __dbt_ferr(dbp, "data", data, 0)) != 0)
		return (ret);

	/*
	 * The key parameter should not be NULL or have the "partial" flag set
	 * in a put call unless the user doesn't care about a key value we'd
	 * return.  The user tells us they don't care about the returned key by
	 * setting the key parameter to NULL or configuring the key DBT to not
	 * return any information.  (Returned keys from a put are always record
	 * numbers, and returning part of a record number  doesn't make sense:
	 * only accept a partial return if the length returned is 0.)
	 */
	if (key_flags && F_ISSET(key, DB_DBT_PARTIAL) && key->dlen != 0)
		return (__db_ferr(env, "key DBT", 0));

	/*
	 * The cursor must be initialized for anything other than DB_KEYFIRST,
	 * DB_KEYLAST or zero: return EINVAL for an invalid cursor, otherwise 0.
	 */
	if (!IS_INITIALIZED(dbc) && flags != 0 && flags != DB_KEYFIRST &&
	    flags != DB_KEYLAST && flags != DB_NODUPDATA &&
	    flags != DB_OVERWRITE_DUP)
		return (__db_curinval(env));

	return (0);
}

/*
 * __dbt_ferr --
 *	Check a DBT for flag errors.
 */
static int
__dbt_ferr(dbp, name, dbt, check_thread)
	const DB *dbp;
	const char *name;
	const DBT *dbt;
	int check_thread;
{
	ENV *env;
	int ret;

	env = dbp->env;

	/*
	 * Check for invalid DBT flags.  We allow any of the flags to be
	 * specified to any DB or DBcursor call so that applications can
	 * set DB_DBT_MALLOC when retrieving a data item from a secondary
	 * database and then specify that same DBT as a key to a primary
	 * database, without having to clear flags.
	 */
	if ((ret = __db_fchk(env, name, dbt->flags, DB_DBT_APPMALLOC |
	    DB_DBT_BULK | DB_DBT_DUPOK | DB_DBT_MALLOC | DB_DBT_REALLOC |
	    DB_DBT_USERCOPY | DB_DBT_USERMEM | DB_DBT_PARTIAL)) != 0)
		return (ret);
	switch (F_ISSET(dbt, DB_DBT_MALLOC | DB_DBT_REALLOC |
	    DB_DBT_USERCOPY | DB_DBT_USERMEM)) {
	case 0:
	case DB_DBT_MALLOC:
	case DB_DBT_REALLOC:
	case DB_DBT_USERCOPY:
	case DB_DBT_USERMEM:
		break;
	default:
		return (__db_ferr(env, name, 1));
	}

	if (F_ISSET(dbt, DB_DBT_BULK) && F_ISSET(dbt, DB_DBT_PARTIAL)) {
		__db_errx(env,
	      "Bulk and partial operations cannot be combined on %s DBT", name);
		return (EINVAL);
	}

	if (check_thread && DB_IS_THREADED(dbp) &&
	    !F_ISSET(dbt, DB_DBT_MALLOC | DB_DBT_REALLOC |
		DB_DBT_USERCOPY | DB_DBT_USERMEM)) {
		__db_errx(env,
		    "DB_THREAD mandates memory allocation flag on %s DBT",
		    name);
		return (EINVAL);
	}
	return (0);
}

/*
 * __db_curinval
 *	Report that a cursor is in an invalid state.
 */
static int
__db_curinval(env)
	const ENV *env;
{
	__db_errx(env,
	    "Cursor position must be set before performing this operation");
	return (EINVAL);
}

/*
 * __db_txn_auto_init --
 *	Handle DB_AUTO_COMMIT initialization.
 *
 * PUBLIC: int __db_txn_auto_init __P((ENV *, DB_THREAD_INFO *, DB_TXN **));
 */
int
__db_txn_auto_init(env, ip, txnidp)
	ENV *env;
	DB_THREAD_INFO *ip;
	DB_TXN **txnidp;
{
	/*
	 * Method calls where applications explicitly specify DB_AUTO_COMMIT
	 * require additional validation: the DB_AUTO_COMMIT flag cannot be
	 * specified if a transaction cookie is also specified, nor can the
	 * flag be specified in a non-transactional environment.
	 */
	if (*txnidp != NULL) {
		__db_errx(env,
    "DB_AUTO_COMMIT may not be specified along with a transaction handle");
		return (EINVAL);
	}

	if (!TXN_ON(env)) {
		__db_errx(env,
    "DB_AUTO_COMMIT may not be specified in non-transactional environment");
		return (EINVAL);
	}

	/*
	 * Our caller checked to see if replication is making a state change.
	 * Don't call the user-level API (which would repeat that check).
	 */
	return (__txn_begin(env, ip, NULL, txnidp, 0));
}

/*
 * __db_txn_auto_resolve --
 *	Resolve local transactions.
 *
 * PUBLIC: int __db_txn_auto_resolve __P((ENV *, DB_TXN *, int, int));
 */
int
__db_txn_auto_resolve(env, txn, nosync, ret)
	ENV *env;
	DB_TXN *txn;
	int nosync, ret;
{
	int t_ret;

	/*
	 * We're resolving a transaction for the user, and must decrement the
	 * replication handle count.  Call the user-level API.
	 */
	if (ret == 0)
		return (__txn_commit(txn, nosync ? DB_TXN_NOSYNC : 0));

	if ((t_ret = __txn_abort(txn)) != 0)
		return (__env_panic(env, t_ret));

	return (ret);
}
