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
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/qam.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/partition.h"
#include "dbinc/txn.h"

static int __db_cursor_check __P((DB *));

/*
 * __db_truncate_pp
 *	DB->truncate pre/post processing.
 *
 * PUBLIC: int __db_truncate_pp __P((DB *, DB_TXN *, u_int32_t *, u_int32_t));
 */
int
__db_truncate_pp(dbp, txn, countp, flags)
	DB *dbp;
	DB_TXN *txn;
	u_int32_t *countp, flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret, txn_local;

	env = dbp->env;
	handle_check = txn_local = 0;

	STRIP_AUTO_COMMIT(flags);

	/* Check for invalid flags. */
	if (F_ISSET(dbp, DB_AM_SECONDARY)) {
		__db_errx(env, "DB->truncate forbidden on secondary indices");
		return (EINVAL);
	}
	if ((ret = __db_fchk(env, "DB->truncate", flags, 0)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/*
	 * Make sure there are no active cursors on this db.  Since we drop
	 * pages we cannot really adjust cursors.
	 */
	if ((ret = __db_cursor_check(dbp)) != 0) {
		__db_errx(env,
		     "DB->truncate not permitted with active cursors");
		goto err;
	}

#ifdef CONFIG_TEST
	if (IS_REP_MASTER(env))
		DB_TEST_WAIT(env, env->test_check);
#endif
	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check &&
	    (ret = __db_rep_enter(dbp, 1, 0, txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	/*
	 * Check for changes to a read-only database.  This must be after the
	 * replication block so that we cannot race master/client state changes.
	 */
	if (DB_IS_READONLY(dbp)) {
		ret = __db_rdonly(env, "DB->truncate");
		goto err;
	}

	/*
	 * Create local transaction as necessary, check for consistent
	 * transaction usage.
	 */
	if (IS_DB_AUTO_COMMIT(dbp, txn)) {
		if ((ret = __txn_begin(env, ip, NULL, &txn, 0)) != 0)
			goto err;
		txn_local = 1;
	}

	/* Check for consistent transaction usage. */
	if ((ret = __db_check_txn(dbp, txn, DB_LOCK_INVALIDID, 0)) != 0)
		goto err;

	ret = __db_truncate(dbp, ip, txn, countp);

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
 * __db_truncate
 *	DB->truncate.
 *
 * PUBLIC: int __db_truncate __P((DB *, DB_THREAD_INFO *, DB_TXN *,
 * PUBLIC:     u_int32_t *));
 */
int
__db_truncate(dbp, ip, txn, countp)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	u_int32_t *countp;
{
	DB *sdbp;
	DBC *dbc;
	ENV *env;
	u_int32_t scount;
	int ret, t_ret;

	env = dbp->env;
	dbc = NULL;
	ret = 0;

	/*
	 * Run through all secondaries and truncate them first.  The count
	 * returned is the count of the primary only.  QUEUE uses normal
	 * processing to truncate so it will update the secondaries normally.
	 */
	if (dbp->type != DB_QUEUE && DB_IS_PRIMARY(dbp)) {
		if ((ret = __db_s_first(dbp, &sdbp)) != 0)
			return (ret);
		for (; sdbp != NULL && ret == 0; ret = __db_s_next(&sdbp, txn))
			if ((ret = __db_truncate(sdbp, ip, txn, &scount)) != 0)
				break;
		if (sdbp != NULL)
			(void)__db_s_done(sdbp, txn);
		if (ret != 0)
			return (ret);
	}

	DB_TEST_RECOVERY(dbp, DB_TEST_PREDESTROY, ret, NULL);

	/* Acquire a cursor. */
	if ((ret = __db_cursor(dbp, ip, txn, &dbc, 0)) != 0)
		return (ret);

	DEBUG_LWRITE(dbc, txn, "DB->truncate", NULL, NULL, 0);
#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbp))
		ret = __part_truncate(dbc, countp);
	else
#endif
	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		ret = __bam_truncate(dbc, countp);
		break;
	case DB_HASH:
		ret = __ham_truncate(dbc, countp);
		break;
	case DB_QUEUE:
		ret = __qam_truncate(dbc, countp);
		break;
	case DB_UNKNOWN:
	default:
		ret = __db_unknown_type(env, "DB->truncate", dbp->type);
		break;
	}

	/* Discard the cursor. */
	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	DB_TEST_RECOVERY(dbp, DB_TEST_POSTDESTROY, ret, NULL);

DB_TEST_RECOVERY_LABEL

	return (ret);
}

/*
 * __db_cursor_check --
 *	See if there are any active cursors on this db.
 */
static int
__db_cursor_check(dbp)
	DB *dbp;
{
	DB *ldbp;
	DBC *dbc;
	ENV *env;
	int found;

	env = dbp->env;

	MUTEX_LOCK(env, env->mtx_dblist);
	FIND_FIRST_DB_MATCH(env, dbp, ldbp);
	for (found = 0;
	    !found && ldbp != NULL && ldbp->adj_fileid == dbp->adj_fileid;
	    ldbp = TAILQ_NEXT(ldbp, dblistlinks)) {
		MUTEX_LOCK(env, dbp->mutex);
		TAILQ_FOREACH(dbc, &ldbp->active_queue, links)
			if (IS_INITIALIZED(dbc)) {
				found = 1;
				break;
			}
		MUTEX_UNLOCK(env, dbp->mutex);
	}
	MUTEX_UNLOCK(env, env->mtx_dblist);

	return (found ? EINVAL : 0);
}
