/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1995, 1996
 *	The President and Fellows of Harvard University.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
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
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

/*
 * __txn_checkpoint_pp --
 *	ENV->txn_checkpoint pre/post processing.
 *
 * PUBLIC: int __txn_checkpoint_pp
 * PUBLIC:     __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__txn_checkpoint_pp(dbenv, kbytes, minutes, flags)
	DB_ENV *dbenv;
	u_int32_t kbytes, minutes, flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->tx_handle, "txn_checkpoint", DB_INIT_TXN);

	/*
	 * On a replication client, all transactions are read-only; therefore,
	 * a checkpoint is a null-op.
	 *
	 * We permit txn_checkpoint, instead of just rendering it illegal,
	 * so that an application can just let a checkpoint thread continue
	 * to operate as it gets promoted or demoted between being a
	 * master and a client.
	 */
	if (IS_REP_CLIENT(env))
		return (0);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env,
	    (__txn_checkpoint(env, kbytes, minutes, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __txn_checkpoint --
 *	ENV->txn_checkpoint.
 *
 * PUBLIC: int __txn_checkpoint
 * PUBLIC:	__P((ENV *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__txn_checkpoint(env, kbytes, minutes, flags)
	ENV *env;
	u_int32_t kbytes, minutes, flags;
{
	DB_LSN ckp_lsn, last_ckp;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	REGENV *renv;
	REGINFO *infop;
	time_t last_ckp_time, now;
	u_int32_t bytes, id, logflags, mbytes, op;
	int ret;

	ret = 0;

	/*
	 * A client will only call through here during recovery,
	 * so just sync the Mpool and go home.  We want to be sure
	 * that since queue meta pages are not rolled back that they
	 * are clean in the cache prior to any transaction log
	 * truncation due to syncup.
	 */
	if (IS_REP_CLIENT(env)) {
		if (MPOOL_ON(env) &&
		    (ret = __memp_sync(env, DB_SYNC_CHECKPOINT, NULL)) != 0) {
			__db_err(env, ret,
		    "txn_checkpoint: failed to flush the buffer cache");
			return (ret);
		}
		return (0);
	}

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;
	infop = env->reginfo;
	renv = infop->primary;
	/*
	 * No mutex is needed as envid is read-only once it is set.
	 */
	id = renv->envid;

	/*
	 * The checkpoint LSN is an LSN such that all transactions begun before
	 * it are complete.  Our first guess (corrected below based on the list
	 * of active transactions) is the last-written LSN.
	 */
	if ((ret = __log_current_lsn(env, &ckp_lsn, &mbytes, &bytes)) != 0)
		return (ret);

	if (!LF_ISSET(DB_FORCE)) {
		/* Don't checkpoint a quiescent database. */
		if (bytes == 0 && mbytes == 0)
			return (0);

		/*
		 * If either kbytes or minutes is non-zero, then only take the
		 * checkpoint if more than "minutes" minutes have passed or if
		 * more than "kbytes" of log data have been written since the
		 * last checkpoint.
		 */
		if (kbytes != 0 &&
		    mbytes * 1024 + bytes / 1024 >= (u_int32_t)kbytes)
			goto do_ckp;

		if (minutes != 0) {
			(void)time(&now);

			TXN_SYSTEM_LOCK(env);
			last_ckp_time = region->time_ckp;
			TXN_SYSTEM_UNLOCK(env);

			if (now - last_ckp_time >= (time_t)(minutes * 60))
				goto do_ckp;
		}

		/*
		 * If we checked time and data and didn't go to checkpoint,
		 * we're done.
		 */
		if (minutes != 0 || kbytes != 0)
			return (0);
	}

	/*
	 * We must single thread checkpoints otherwise the chk_lsn may get out
	 * of order.  We need to capture the start of the earliest currently
	 * active transaction (chk_lsn) and then flush all buffers.  While
	 * doing this we we could then be overtaken by another checkpoint that
	 * sees a later chk_lsn but competes first.  An archive process could
	 * then remove a log this checkpoint depends on.
	 */
do_ckp:
	MUTEX_LOCK(env, region->mtx_ckp);
	if ((ret = __txn_getactive(env, &ckp_lsn)) != 0)
		goto err;

	/*
	 * Checkpoints in replication groups can cause performance problems.
	 *
	 * As on the master, checkpoint on the replica requires the cache be
	 * flushed.  The problem occurs when a client has dirty cache pages
	 * to write when the checkpoint record arrives, and the client's PERM
	 * response is necessary in order to meet the system's durability
	 * guarantees.  In this case, the master will have to wait until the
	 * client completes its cache flush and writes the checkpoint record
	 * before subsequent transactions can be committed.  The delay may
	 * cause transactions to timeout waiting on client response, which
	 * can cause nasty ripple effects in the system's overall throughput.
	 * [#15338]
	 *
	 * First, we send a start-sync record when the checkpoint starts so
	 * clients can start flushing their cache in preparation for the
	 * arrival of the checkpoint record.
	 */
	if (LOGGING_ON(env) && IS_REP_MASTER(env)) {
#ifdef HAVE_REPLICATION_THREADS
		/*
		 * If repmgr is configured in the shared environment (which we
		 * know if we have a local host address), but no send() function
		 * configured for this process, assume we have a
		 * replication-unaware process that wants to automatically
		 * participate in replication (i.e., sending replication
		 * messages to clients).
		 */
		if (env->rep_handle->send == NULL &&
		    F_ISSET(env, ENV_THREAD) &&
		    env->rep_handle->region->my_addr.host != INVALID_ROFF &&
		    (ret = __repmgr_autostart(env)) != 0)
			goto err;
#endif
		if (env->rep_handle->send != NULL)
			(void)__rep_send_message(env, DB_EID_BROADCAST,
			    REP_START_SYNC, &ckp_lsn, NULL, 0, 0);
	}

	/* Flush the cache. */
	if (MPOOL_ON(env) &&
	    (ret = __memp_sync_int(
		env, NULL, 0, DB_SYNC_CHECKPOINT, NULL, NULL)) != 0) {
		__db_err(env, ret,
		    "txn_checkpoint: failed to flush the buffer cache");
		goto err;
	}

	/*
	 * The client won't have more dirty pages to flush from its cache than
	 * the master did, but there may be differences between the hardware,
	 * I/O configuration and workload on the master and the client that
	 * can result in the client being unable to finish its cache flush as
	 * fast as the master.  A way to avoid the problem is to pause after
	 * the master completes its checkpoint and before the actual checkpoint
	 * record is logged, giving the replicas additional time to finish.
	 *
	 * !!!
	 * Currently turned off when testing, because it makes the test suite
	 * take a long time to run.
	 */
#ifndef	CONFIG_TEST
	if (LOGGING_ON(env) &&
	    IS_REP_MASTER(env) && env->rep_handle->send != NULL &&
	    !LF_ISSET(DB_CKP_INTERNAL) &&
	    env->rep_handle->region->chkpt_delay != 0)
		__os_yield(env, 0, env->rep_handle->region->chkpt_delay);
#endif

	/*
	 * Because we can't be a replication client here, and because
	 * recovery (somewhat unusually) calls txn_checkpoint and expects
	 * it to write a log message, LOGGING_ON is the correct macro here.
	 */
	if (LOGGING_ON(env)) {
		TXN_SYSTEM_LOCK(env);
		last_ckp = region->last_ckp;
		TXN_SYSTEM_UNLOCK(env);
		/*
		 * Put out records for the open files before we log
		 * the checkpoint.  The records are certain to be at
		 * or after ckp_lsn, but before the checkpoint record
		 * itself, so they're sure to be included if we start
		 * recovery from the ckp_lsn contained in this
		 * checkpoint.
		 */
		logflags = DB_LOG_CHKPNT;
		/*
		 * If this is a normal checkpoint, log files as checkpoints.
		 * If we are recovering, only log as DBREG_RCLOSE if
		 * there are no prepared txns.  Otherwise, it should
		 * stay as DBREG_CHKPNT.
		 */
		op = DBREG_CHKPNT;
		if (!IS_RECOVERING(env))
			logflags |= DB_FLUSH;
		else if (region->stat.st_nrestores == 0)
			op = DBREG_RCLOSE;
		if ((ret = __dbreg_log_files(env, op)) != 0 ||
		    (ret = __txn_ckp_log(env, NULL, &ckp_lsn, logflags,
		    &ckp_lsn, &last_ckp, (int32_t)time(NULL), id, 0)) != 0) {
			__db_err(env, ret,
			    "txn_checkpoint: log failed at LSN [%ld %ld]",
			    (long)ckp_lsn.file, (long)ckp_lsn.offset);
			goto err;
		}

		if ((ret = __txn_updateckp(env, &ckp_lsn)) != 0)
			goto err;
	}

err:	MUTEX_UNLOCK(env, region->mtx_ckp);
	return (ret);
}

/*
 * __txn_getactive --
 *	 Find the oldest active transaction and figure out its "begin" LSN.
 *	 This is the lowest LSN we can checkpoint, since any record written
 *	 after it may be involved in a transaction and may therefore need
 *	 to be undone in the case of an abort.
 *
 *	 We check both the file and offset for 0 since the lsn may be in
 *	 transition.  If it is then we don't care about this txn because it
 *	 must be starting after we set the initial value of lsnp in the caller.
 *	 All txns must initalize their begin_lsn before writing to the log.
 *
 * PUBLIC: int __txn_getactive __P((ENV *, DB_LSN *));
 */
int
__txn_getactive(env, lsnp)
	ENV *env;
	DB_LSN *lsnp;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *td;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	TXN_SYSTEM_LOCK(env);
	SH_TAILQ_FOREACH(td, &region->active_txn, links, __txn_detail)
		if (td->begin_lsn.file != 0 &&
		    td->begin_lsn.offset != 0 &&
		    LOG_COMPARE(&td->begin_lsn, lsnp) < 0)
			*lsnp = td->begin_lsn;
	TXN_SYSTEM_UNLOCK(env);

	return (0);
}

/*
 * __txn_getckp --
 *	Get the LSN of the last transaction checkpoint.
 *
 * PUBLIC: int __txn_getckp __P((ENV *, DB_LSN *));
 */
int
__txn_getckp(env, lsnp)
	ENV *env;
	DB_LSN *lsnp;
{
	DB_LSN lsn;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	TXN_SYSTEM_LOCK(env);
	lsn = region->last_ckp;
	TXN_SYSTEM_UNLOCK(env);

	if (IS_ZERO_LSN(lsn))
		return (DB_NOTFOUND);

	*lsnp = lsn;
	return (0);
}

/*
 * __txn_updateckp --
 *	Update the last_ckp field in the transaction region.  This happens
 * at the end of a normal checkpoint and also when a replication client
 * receives a checkpoint record.
 *
 * PUBLIC: int __txn_updateckp __P((ENV *, DB_LSN *));
 */
int
__txn_updateckp(env, lsnp)
	ENV *env;
	DB_LSN *lsnp;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	/*
	 * We want to make sure last_ckp only moves forward;  since we drop
	 * locks above and in log_put, it's possible for two calls to
	 * __txn_ckp_log to finish in a different order from how they were
	 * called.
	 */
	TXN_SYSTEM_LOCK(env);
	if (LOG_COMPARE(&region->last_ckp, lsnp) < 0) {
		region->last_ckp = *lsnp;
		(void)time(&region->time_ckp);
	}
	TXN_SYSTEM_UNLOCK(env);

	return (0);
}
