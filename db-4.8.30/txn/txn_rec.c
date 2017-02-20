/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1996
 *	The President and Fellows of Harvard University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/lock.h"
#include "dbinc/txn.h"
#include "dbinc/db_am.h"

/*
 * PUBLIC: int __txn_regop_recover
 * PUBLIC:    __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 *
 * These records are only ever written for commits.  Normally, we redo any
 * committed transaction, however if we are doing recovery to a timestamp, then
 * we may treat transactions that committed after the timestamp as aborted.
 */
int
__txn_regop_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_regop_args *argp;
	DB_TXNHEAD *headp;
	int ret;
	u_int32_t status;

#ifdef DEBUG_RECOVER
	(void)__txn_regop_print(env, dbtp, lsnp, op, info);
#endif

	if ((ret = __txn_regop_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	headp = info;
	/*
	 * We are only ever called during FORWARD_ROLL or BACKWARD_ROLL.
	 * We check for the former explicitly and the last two clauses
	 * apply to the BACKWARD_ROLL case.
	 */

	if (op == DB_TXN_FORWARD_ROLL) {
		/*
		 * If this was a 2-phase-commit transaction, then it
		 * might already have been removed from the list, and
		 * that's OK.  Ignore the return code from remove.
		 */
		if ((ret = __db_txnlist_remove(env,
		    info, argp->txnp->txnid)) != DB_NOTFOUND && ret != 0)
			goto err;
	} else if ((env->dbenv->tx_timestamp != 0 &&
	    argp->timestamp > (int32_t)env->dbenv->tx_timestamp) ||
	    (!IS_ZERO_LSN(headp->trunc_lsn) &&
	    LOG_COMPARE(&headp->trunc_lsn, lsnp) < 0)) {
		/*
		 * We failed either the timestamp check or the trunc_lsn check,
		 * so we treat this as an abort even if it was a commit record.
		 */
		if ((ret = __db_txnlist_update(env, info,
		    argp->txnp->txnid, TXN_ABORT, NULL, &status, 1)) != 0)
			goto err;
		else if (status != TXN_IGNORE && status != TXN_OK)
			goto err;
	} else {
		/* This is a normal commit; mark it appropriately. */
		if ((ret = __db_txnlist_update(env,
		    info, argp->txnp->txnid, argp->opcode, lsnp,
		    &status, 0)) == DB_NOTFOUND) {
			if ((ret = __db_txnlist_add(env,
			    info, argp->txnp->txnid,
			    argp->opcode == TXN_ABORT ?
			    TXN_IGNORE : argp->opcode, lsnp)) != 0)
				goto err;
		} else if (ret != 0 ||
		    (status != TXN_IGNORE && status != TXN_OK))
			goto err;
	}

	if (ret == 0)
		*lsnp = argp->prev_lsn;

	if (0) {
err:		__db_errx(env,
		    "txnid %lx commit record found, already on commit list",
		    (u_long)argp->txnp->txnid);
		ret = EINVAL;
	}
	__os_free(env, argp);

	return (ret);
}

/*
 * PUBLIC: int __txn_prepare_recover
 * PUBLIC:    __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 *
 * These records are only ever written for prepares.
 */
int
__txn_prepare_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_prepare_args *argp;
	DBT *lock_dbt;
	DB_TXNHEAD *headp;
	DB_LOCKTAB *lt;
	u_int32_t status;
	int ret;

#ifdef DEBUG_RECOVER
	(void)__txn_prepare_print(env, dbtp, lsnp, op, info);
#endif

	if ((ret = __txn_prepare_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	if (argp->opcode != TXN_PREPARE && argp->opcode != TXN_ABORT) {
		ret = EINVAL;
		goto err;
	}
	headp = info;

	/*
	 * The return value here is either a DB_NOTFOUND or it is
	 * the transaction status from the list.  It is not a normal
	 * error return, so we must make sure that in each of the
	 * cases below, we overwrite the ret value so we return
	 * appropriately.
	 */
	ret = __db_txnlist_find(env, info, argp->txnp->txnid, &status);

	/*
	 * If we are rolling forward, then an aborted prepare
	 * indicates that this may be the last record we'll see for
	 * this transaction ID, so we should remove it from the list.
	 */

	if (op == DB_TXN_FORWARD_ROLL) {
		if ((ret = __db_txnlist_remove(env,
		    info, argp->txnp->txnid)) != 0)
			goto txn_err;
	} else if (op == DB_TXN_BACKWARD_ROLL && status == TXN_PREPARE) {
		/*
		 * On the backward pass, we have four possibilities:
		 * 1. The transaction is already committed, no-op.
		 * 2. The transaction is already aborted, no-op.
		 * 3. The prepare failed and was aborted, mark as abort.
		 * 4. The transaction is neither committed nor aborted.
		 *	 Treat this like a commit and roll forward so that
		 *	 the transaction can be resurrected in the region.
		 * We handle cases 3 and 4 here; cases 1 and 2
		 * are the final clause below.
		 */
		if (argp->opcode == TXN_ABORT) {
			if ((ret = __db_txnlist_update(env,
			     info, argp->txnp->txnid,
			     TXN_ABORT, NULL, &status, 0)) != 0 &&
			     status != TXN_PREPARE)
				goto txn_err;
			ret = 0;
		}
		/*
		 * This is prepared, but not yet committed transaction.  We
		 * need to add it to the transaction list, so that it gets
		 * rolled forward. We also have to add it to the region's
		 * internal state so it can be properly aborted or committed
		 * after recovery (see txn_recover).
		 */
		else if ((ret = __db_txnlist_remove(env,
		    info, argp->txnp->txnid)) != 0) {
txn_err:		__db_errx(env,
			    "transaction not in list %lx",
			    (u_long)argp->txnp->txnid);
			ret = DB_NOTFOUND;
		} else if (IS_ZERO_LSN(headp->trunc_lsn) ||
		    LOG_COMPARE(&headp->trunc_lsn, lsnp) >= 0) {
			if ((ret = __db_txnlist_add(env,
			   info, argp->txnp->txnid, TXN_COMMIT, lsnp)) == 0) {
				/* Re-acquire the locks for this transaction. */
				lock_dbt = &argp->locks;
				if (LOCKING_ON(env)) {
					lt = env->lk_handle;
					if ((ret = __lock_getlocker(lt,
						argp->txnp->txnid, 1,
						&argp->txnp->locker)) != 0)
						goto err;
					if ((ret = __lock_get_list(env,
					    argp->txnp->locker, 0,
					    DB_LOCK_WRITE, lock_dbt)) != 0)
						goto err;
				}

				ret = __txn_restore_txn(env, lsnp, argp);
			}
		}
	} else
		ret = 0;

	if (ret == 0)
		*lsnp = argp->prev_lsn;

err:	__os_free(env, argp);

	return (ret);
}

/*
 * PUBLIC: int __txn_ckp_recover
 * PUBLIC: __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__txn_ckp_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_ckp_args *argp;
	int ret;

#ifdef DEBUG_RECOVER
	__txn_ckp_print(env, dbtp, lsnp, op, info);
#endif
	if ((ret = __txn_ckp_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	if (op == DB_TXN_BACKWARD_ROLL)
		__db_txnlist_ckp(env, info, lsnp);

	*lsnp = argp->last_ckp;
	__os_free(env, argp);
	return (DB_TXN_CKP);
}

/*
 * __txn_child_recover
 *	Recover a commit record for a child transaction.
 *
 * PUBLIC: int __txn_child_recover
 * PUBLIC:    __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__txn_child_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_child_args *argp;
	u_int32_t c_stat, p_stat, tmpstat;
	int ret, t_ret;

#ifdef DEBUG_RECOVER
	(void)__txn_child_print(env, dbtp, lsnp, op, info);
#endif
	if ((ret = __txn_child_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	/*
	 * This is a record in a PARENT's log trail indicating that a
	 * child committed.  If we are aborting, return the childs last
	 * record's LSN.  If we are in recovery, then if the
	 * parent is committing, we set ourselves up to commit, else
	 * we do nothing.
	 */
	if (op == DB_TXN_ABORT) {
		*lsnp = argp->c_lsn;
		ret = __db_txnlist_lsnadd(env, info, &argp->prev_lsn);
		goto out;
	} else if (op == DB_TXN_BACKWARD_ROLL) {
		/* Child might exist -- look for it. */
		ret = __db_txnlist_find(env, info, argp->child, &c_stat);
		t_ret =
		    __db_txnlist_find(env, info, argp->txnp->txnid, &p_stat);
		if (ret != 0 && ret != DB_NOTFOUND)
			goto out;
		if (t_ret != 0 && t_ret != DB_NOTFOUND) {
			ret = t_ret;
			goto out;
		}
		/*
		 * If the parent is in state COMMIT or IGNORE, then we apply
		 * that to the child, else we need to abort the child.
		 */

		if (ret == DB_NOTFOUND  ||
		    c_stat == TXN_OK || c_stat == TXN_COMMIT) {
			if (t_ret == DB_NOTFOUND ||
			     (p_stat != TXN_COMMIT  && p_stat != TXN_IGNORE))
				c_stat = TXN_ABORT;
			else
				c_stat = p_stat;

			if (ret == DB_NOTFOUND)
				ret = __db_txnlist_add(env,
				     info, argp->child, c_stat, NULL);
			else
				ret = __db_txnlist_update(env, info,
				     argp->child, c_stat, NULL, &tmpstat, 0);
		} else if (c_stat == TXN_EXPECTED) {
			/*
			 * The open after this create succeeded.  If the
			 * parent succeeded, we don't want to redo; if the
			 * parent aborted, we do want to undo.
			 */
			switch (p_stat) {
			case TXN_COMMIT:
			case TXN_IGNORE:
				c_stat = TXN_IGNORE;
				break;
			default:
				c_stat = TXN_ABORT;
			}
			ret = __db_txnlist_update(env,
			    info, argp->child, c_stat, NULL, &tmpstat, 0);
		} else if (c_stat == TXN_UNEXPECTED) {
			/*
			 * The open after this create failed.  If the parent
			 * is rolling forward, we need to roll forward.  If
			 * the parent failed, then we do not want to abort
			 * (because the file may not be the one in which we
			 * are interested).
			 */
			ret = __db_txnlist_update(env, info, argp->child,
			    p_stat == TXN_COMMIT ? TXN_COMMIT : TXN_IGNORE,
			    NULL, &tmpstat, 0);
		}
	} else if (op == DB_TXN_OPENFILES) {
		/*
		 * If we have a partial subtransaction, then the whole
		 * transaction should be ignored.
		 */
		if ((ret = __db_txnlist_find(env,
		    info, argp->child, &c_stat)) == DB_NOTFOUND)
			ret = __db_txnlist_update(env, info,
			     argp->txnp->txnid, TXN_IGNORE,
			     NULL, &p_stat, 1);
	} else if (DB_REDO(op)) {
		/* Forward Roll */
		if ((ret =
		    __db_txnlist_remove(env, info, argp->child)) != 0)
			__db_errx(env,
			    "Transaction not in list %x", argp->child);
	}

	if (ret == 0)
		*lsnp = argp->prev_lsn;

out:	__os_free(env, argp);

	return (ret);
}

/*
 * __txn_restore_txn --
 *	Using only during XA recovery.  If we find any transactions that are
 * prepared, but not yet committed, then we need to restore the transaction's
 * state into the shared region, because the TM is going to issue an abort
 * or commit and we need to respond correctly.
 *
 * lsnp is the LSN of the returned LSN
 * argp is the prepare record (in an appropriate structure)
 *
 * PUBLIC: int __txn_restore_txn __P((ENV *, DB_LSN *, __txn_prepare_args *));
 */
int
__txn_restore_txn(env, lsnp, argp)
	ENV *env;
	DB_LSN *lsnp;
	__txn_prepare_args *argp;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *td;
	int ret;

	if (argp->gid.size == 0)
		return (0);

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;
	TXN_SYSTEM_LOCK(env);

	/* Allocate a new transaction detail structure. */
	if ((ret = __env_alloc(&mgr->reginfo, sizeof(TXN_DETAIL), &td)) != 0) {
		TXN_SYSTEM_UNLOCK(env);
		return (ret);
	}

	/* Place transaction on active transaction list. */
	SH_TAILQ_INSERT_HEAD(&region->active_txn, td, links, __txn_detail);

	td->txnid = argp->txnp->txnid;
	__os_id(env->dbenv, &td->pid, &td->tid);
	td->last_lsn = *lsnp;
	td->begin_lsn = argp->begin_lsn;
	td->parent = INVALID_ROFF;
	td->name = INVALID_ROFF;
	SH_TAILQ_INIT(&td->kids);
	MAX_LSN(td->read_lsn);
	MAX_LSN(td->visible_lsn);
	td->mvcc_ref = 0;
	td->mvcc_mtx = MUTEX_INVALID;
	td->status = TXN_PREPARED;
	td->flags = TXN_DTL_RESTORED;
	memcpy(td->gid, argp->gid.data, argp->gid.size);
	td->nlog_dbs = 0;
	td->nlog_slots = TXN_NSLOTS;
	td->log_dbs = R_OFFSET(&mgr->reginfo, td->slots);

	region->stat.st_nrestores++;
#ifdef HAVE_STATISTICS
	region->stat.st_nactive++;
	if (region->stat.st_nactive > region->stat.st_maxnactive)
		region->stat.st_maxnactive = region->stat.st_nactive;
#endif
	TXN_SYSTEM_UNLOCK(env);
	return (0);
}

/*
 * __txn_recycle_recover --
 *	Recovery function for recycle.
 *
 * PUBLIC: int __txn_recycle_recover
 * PUBLIC:   __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__txn_recycle_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_recycle_args *argp;
	int ret;

#ifdef DEBUG_RECOVER
	(void)__txn_child_print(env, dbtp, lsnp, op, info);
#endif
	if ((ret = __txn_recycle_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	COMPQUIET(lsnp, NULL);

	if ((ret = __db_txnlist_gen(env, info,
	    DB_UNDO(op) ? -1 : 1, argp->min, argp->max)) != 0)
		return (ret);

	__os_free(env, argp);

	return (0);
}

/*
 * PUBLIC: int __txn_regop_42_recover
 * PUBLIC:    __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 *
 * These records are only ever written for commits.  Normally, we redo any
 * committed transaction, however if we are doing recovery to a timestamp, then
 * we may treat transactions that committed after the timestamp as aborted.
 */
int
__txn_regop_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_regop_42_args *argp;
	DB_TXNHEAD *headp;
	u_int32_t status;
	int ret;

#ifdef DEBUG_RECOVER
	(void)__txn_regop_42_print(env, dbtp, lsnp, op, info);
#endif

	if ((ret = __txn_regop_42_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	headp = info;
	/*
	 * We are only ever called during FORWARD_ROLL or BACKWARD_ROLL.
	 * We check for the former explicitly and the last two clauses
	 * apply to the BACKWARD_ROLL case.
	 */

	if (op == DB_TXN_FORWARD_ROLL) {
		/*
		 * If this was a 2-phase-commit transaction, then it
		 * might already have been removed from the list, and
		 * that's OK.  Ignore the return code from remove.
		 */
		if ((ret = __db_txnlist_remove(env,
		    info, argp->txnp->txnid)) != DB_NOTFOUND && ret != 0)
			goto err;
	} else if ((env->dbenv->tx_timestamp != 0 &&
	    argp->timestamp > (int32_t)env->dbenv->tx_timestamp) ||
	    (!IS_ZERO_LSN(headp->trunc_lsn) &&
	    LOG_COMPARE(&headp->trunc_lsn, lsnp) < 0)) {
		/*
		 * We failed either the timestamp check or the trunc_lsn check,
		 * so we treat this as an abort even if it was a commit record.
		 */
		if ((ret = __db_txnlist_update(env, info,
		    argp->txnp->txnid, TXN_ABORT, NULL, &status, 1)) != 0)
			goto err;
		else if (status != TXN_IGNORE && status != TXN_OK)
			goto err;
	} else {
		/* This is a normal commit; mark it appropriately. */
		if ((ret = __db_txnlist_update(env,
		    info, argp->txnp->txnid, argp->opcode, lsnp,
		    &status, 0)) == DB_NOTFOUND) {
			if ((ret = __db_txnlist_add(env,
			    info, argp->txnp->txnid,
			    argp->opcode == TXN_ABORT ?
			    TXN_IGNORE : argp->opcode, lsnp)) != 0)
				goto err;
		} else if (ret != 0 ||
		    (status != TXN_IGNORE && status != TXN_OK))
			goto err;
	}

	if (ret == 0)
		*lsnp = argp->prev_lsn;

	if (0) {
err:		__db_errx(env,
		    "txnid %lx commit record found, already on commit list",
		    (u_long)argp->txnp->txnid);
		ret = EINVAL;
	}
	__os_free(env, argp);

	return (ret);
}

/*
 * PUBLIC: int __txn_ckp_42_recover
 * PUBLIC: __P((ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__txn_ckp_42_recover(env, dbtp, lsnp, op, info)
	ENV *env;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__txn_ckp_42_args *argp;
	int ret;

#ifdef DEBUG_RECOVER
	__txn_ckp_42_print(env, dbtp, lsnp, op, info);
#endif
	if ((ret = __txn_ckp_42_read(env, dbtp->data, &argp)) != 0)
		return (ret);

	if (op == DB_TXN_BACKWARD_ROLL)
		__db_txnlist_ckp(env, info, lsnp);

	*lsnp = argp->last_ckp;
	__os_free(env, argp);
	return (DB_TXN_CKP);
}
