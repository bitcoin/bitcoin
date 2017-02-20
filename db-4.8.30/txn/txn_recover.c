/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/txn.h"
#include "dbinc/db_page.h"
#include "dbinc/db_dispatch.h"
#include "dbinc/log.h"
#include "dbinc_auto/db_auto.h"
#include "dbinc_auto/crdel_auto.h"
#include "dbinc_auto/db_ext.h"

/*
 * __txn_map_gid
 *	Return the txn that corresponds to this global ID.
 *
 * PUBLIC: int __txn_map_gid __P((ENV *,
 * PUBLIC:     u_int8_t *, TXN_DETAIL **, roff_t *));
 */
int
__txn_map_gid(env, gid, tdp, offp)
	ENV *env;
	u_int8_t *gid;
	TXN_DETAIL **tdp;
	roff_t *offp;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	/*
	 * Search the internal active transaction table to find the
	 * matching xid.  If this is a performance hit, then we
	 * can create a hash table, but I doubt it's worth it.
	 */
	TXN_SYSTEM_LOCK(env);
	SH_TAILQ_FOREACH(*tdp, &region->active_txn, links, __txn_detail)
		if (memcmp(gid, (*tdp)->gid, sizeof((*tdp)->gid)) == 0)
			break;
	TXN_SYSTEM_UNLOCK(env);

	if (*tdp == NULL)
		return (EINVAL);

	*offp = R_OFFSET(&mgr->reginfo, *tdp);
	return (0);
}

/*
 * __txn_recover_pp --
 *	ENV->txn_recover pre/post processing.
 *
 * PUBLIC: int __txn_recover_pp __P((DB_ENV *,
 * PUBLIC:     DB_PREPLIST *, u_int32_t, u_int32_t *, u_int32_t));
 */
int
__txn_recover_pp(dbenv, preplist, count, retp, flags)
	DB_ENV *dbenv;
	DB_PREPLIST *preplist;
	u_int32_t count, *retp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(
	    env, env->tx_handle, "txn_recover", DB_INIT_TXN);

	if (F_ISSET((DB_TXNREGION *)env->tx_handle->reginfo.primary,
	    TXN_IN_RECOVERY)) {
		__db_errx(env, "operation not permitted while in recovery");
		return (EINVAL);
	}

	if (flags != DB_FIRST && flags != DB_NEXT)
		return (__db_ferr(env, "DB_ENV->txn_recover", 0));

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env,
	    (__txn_recover(env, preplist, count, retp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_recover --
 *	ENV->txn_recover.
 *
 * PUBLIC: int __txn_recover __P((ENV *,
 * PUBLIC:         DB_PREPLIST *, u_int32_t, u_int32_t *, u_int32_t));
 */
int
__txn_recover(env, txns, count, retp, flags)
	ENV *env;
	DB_PREPLIST *txns;
	u_int32_t  count, *retp;
	u_int32_t flags;
{
	DB_LSN min;
	DB_PREPLIST *prepp;
	DB_THREAD_INFO *ip;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *td;
	u_int32_t i;
	int restored, ret;

	*retp = 0;

	MAX_LSN(min);
	prepp = txns;
	restored = ret = 0;

	DB_ASSERT(env, txns != NULL);
	/*
	 * If we are starting a scan, then we traverse the active transaction
	 * list once making sure that all transactions are marked as not having
	 * been collected.  Then on each pass, we mark the ones we collected
	 * so that if we cannot collect them all at once, we can finish up
	 * next time with a continue.
	 */

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	/*
	 * During this pass we need to figure out if we are going to need
	 * to open files.  We need to open files if we've never collected
	 * before (in which case, none of the COLLECTED bits will be set)
	 * and the ones that we are collecting are restored (if they aren't
	 * restored, then we never crashed; just the main server did).
	 */
	TXN_SYSTEM_LOCK(env);

	/* Now begin collecting active transactions. */
	for (td = SH_TAILQ_FIRST(&region->active_txn, __txn_detail);
	    td != NULL && *retp < count;
	    td = SH_TAILQ_NEXT(td, links, __txn_detail)) {
		if (td->status != TXN_PREPARED ||
		    (flags != DB_FIRST && F_ISSET(td, TXN_DTL_COLLECTED)))
			continue;

		if (F_ISSET(td, TXN_DTL_RESTORED))
			restored = 1;

		if ((ret = __os_calloc(env,
		    1, sizeof(DB_TXN), &prepp->txn)) != 0) {
			TXN_SYSTEM_UNLOCK(env);
			goto err;
		}
		if ((ret = __txn_continue(env, prepp->txn, td)) != 0)
			goto err;
		F_SET(prepp->txn, TXN_MALLOC);
		if (F_ISSET(env->dbenv, DB_ENV_TXN_NOSYNC))
			F_SET(prepp->txn, TXN_NOSYNC);
		else if (F_ISSET(env->dbenv, DB_ENV_TXN_WRITE_NOSYNC))
			F_SET(prepp->txn, TXN_WRITE_NOSYNC);
		else
			F_SET(prepp->txn, TXN_SYNC);
		memcpy(prepp->gid, td->gid, sizeof(td->gid));
		prepp++;

		if (!IS_ZERO_LSN(td->begin_lsn) &&
		    LOG_COMPARE(&td->begin_lsn, &min) < 0)
			min = td->begin_lsn;

		(*retp)++;
		F_SET(td, TXN_DTL_COLLECTED);
	}
	if (flags == DB_FIRST)
		for (; td != NULL; td = SH_TAILQ_NEXT(td, links, __txn_detail))
			F_CLR(td, TXN_DTL_COLLECTED);
	TXN_SYSTEM_UNLOCK(env);

	/*
	 * Now link all the transactions into the transaction manager's list.
	 */
	if (*retp != 0) {
		MUTEX_LOCK(env, mgr->mutex);
		for (i = 0; i < *retp; i++)
			TAILQ_INSERT_TAIL(&mgr->txn_chain, txns[i].txn, links);
		MUTEX_UNLOCK(env, mgr->mutex);

		/*
		 * If we are restoring, update our count of outstanding
		 * transactions.
		 */
		if (REP_ON(env)) {
			REP_SYSTEM_LOCK(env);
			env->rep_handle->region->op_cnt += (u_long)*retp;
			REP_SYSTEM_UNLOCK(env);
		}

	}
	/*
	 * If recovery already opened the files for us, don't
	 * do it here.
	 */
	if (restored != 0 && flags == DB_FIRST &&
	    !F_ISSET(env->lg_handle, DBLOG_OPENFILES)) {
		ENV_GET_THREAD_INFO(env, ip);
		ret = __txn_openfiles(env, ip, &min, 0);
	}

	if (0) {
err:		TXN_SYSTEM_UNLOCK(env);
	}
	return (ret);
}

/*
 * __txn_openfiles --
 *	Call env_openfiles.
 *
 * PUBLIC: int __txn_openfiles __P((ENV *, DB_THREAD_INFO *, DB_LSN *, int));
 */
int
__txn_openfiles(env, ip, min, force)
	ENV *env;
	DB_THREAD_INFO *ip;
	DB_LSN *min;
	int force;
{
	DBT data;
	DB_LOGC *logc;
	DB_LSN open_lsn;
	DB_TXNHEAD *txninfo;
	__txn_ckp_args *ckp_args;
	int ret, t_ret;

	/*
	 * Figure out the last checkpoint before the smallest
	 * start_lsn in the region.
	 */
	logc = NULL;
	if ((ret = __log_cursor(env, &logc)) != 0)
		goto err;

	memset(&data, 0, sizeof(data));
	if ((ret = __txn_getckp(env, &open_lsn)) == 0)
		while (!IS_ZERO_LSN(open_lsn) && (ret =
		    __logc_get(logc, &open_lsn, &data, DB_SET)) == 0 &&
		    (force ||
		    (min != NULL && LOG_COMPARE(min, &open_lsn) < 0))) {
			/* Format the log record. */
			if ((ret = __txn_ckp_read(
			    env, data.data, &ckp_args)) != 0) {
				__db_errx(env,
			    "Invalid checkpoint record at [%lu][%lu]",
				    (u_long)open_lsn.file,
				    (u_long)open_lsn.offset);
				goto err;
			}
			/*
			 * If force is set, then we're forcing ourselves
			 * to go back far enough to open files.
			 * Use ckp_lsn and then break out of the loop.
			 */
			open_lsn = force ? ckp_args->ckp_lsn :
			    ckp_args->last_ckp;
			__os_free(env, ckp_args);
			if (force) {
				if ((ret = __logc_get(logc, &open_lsn,
				    &data, DB_SET)) != 0)
					goto err;
				break;
			}
		}

	/*
	 * There are several ways by which we may have gotten here.
	 * - We got a DB_NOTFOUND -- we need to read the first
	 *	log record.
	 * - We found a checkpoint before min.  We're done.
	 * - We found a checkpoint after min who's last_ckp is 0.  We
	 *	need to start at the beginning of the log.
	 * - We are forcing an openfiles and we have our ckp_lsn.
	 */
	if ((ret == DB_NOTFOUND || IS_ZERO_LSN(open_lsn)) && (ret =
	    __logc_get(logc, &open_lsn, &data, DB_FIRST)) != 0) {
		__db_errx(env, "No log records");
		goto err;
	}

	if ((ret = __db_txnlist_init(env, ip, 0, 0, NULL, &txninfo)) != 0)
		goto err;
	ret = __env_openfiles(
	    env, logc, txninfo, &data, &open_lsn, NULL, (double)0, 0);
	if (txninfo != NULL)
		__db_txnlist_end(env, txninfo);

err:
	if (logc != NULL && (t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}
