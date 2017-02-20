/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

static int __rep_internal_init __P((ENV *, u_int32_t));

/*
 * __rep_verify --
 *	Handle a REP_VERIFY message.
 *
 * PUBLIC: int __rep_verify __P((ENV *, __rep_control_args *, DBT *,
 * PUBLIC:     int, time_t));
 */
int
__rep_verify(env, rp, rec, eid, savetime)
	ENV *env;
	__rep_control_args *rp;
	DBT *rec;
	int eid;
	time_t savetime;
{
	DBT mylog;
	DB_LOG *dblp;
	DB_LOGC *logc;
	DB_LSN lsn, prev_ckp;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	__txn_ckp_args *ckp_args;
	u_int32_t logflag, rectype;
	int master, match, ret, t_ret;

	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	/* Do nothing if VERIFY flag is not set. */
	if (!F_ISSET(rep, REP_F_RECOVER_VERIFY))
		return (ret);

#ifdef DIAGNOSTIC
	/*
	 * We should not ever be in internal init with a lease granted.
	 */
	if (IS_USING_LEASES(env)) {
		REP_SYSTEM_LOCK(env);
		DB_ASSERT(env, __rep_islease_granted(env) == 0);
		REP_SYSTEM_UNLOCK(env);
	}
#endif

	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);
	memset(&mylog, 0, sizeof(mylog));
	/* If verify_lsn of ZERO is passed in, get last log. */
	MUTEX_LOCK(env, rep->mtx_clientdb);
	logflag = IS_ZERO_LSN(lp->verify_lsn) ? DB_LAST : DB_SET;
	prev_ckp = lp->prev_ckp;
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	if ((ret = __logc_get(logc, &rp->lsn, &mylog, logflag)) != 0)
		goto out;
	match = 0;
	if (mylog.size == rec->size &&
	    memcmp(mylog.data, rec->data, rec->size) == 0)
		match = 1;
	/*
	 * If we don't have a match, backup to the previous
	 * identification record and try again.
	 */
	if (match == 0) {
		master = rep->master_id;
		/*
		 * We will eventually roll back over this log record (unless we
		 * ultimately have to give up and do an internal init).  So, if
		 * it was a checkpoint, make sure we don't end up without any
		 * checkpoints left in the entire log.
		 */
		LOGCOPY_32(env, &rectype, mylog.data);
		DB_ASSERT(env, ret == 0);
		if (!lp->db_log_inmemory && rectype == DB___txn_ckp) {
			if ((ret = __txn_ckp_read(env,
			    mylog.data, &ckp_args)) != 0)
				goto out;
			lsn = ckp_args->last_ckp;
			__os_free(env, ckp_args);
			MUTEX_LOCK(env, rep->mtx_clientdb);
			lp->prev_ckp =	lsn;
			MUTEX_UNLOCK(env, rep->mtx_clientdb);
			if (IS_ZERO_LSN(lsn)) {
				/*
				 * No previous checkpoints?  The only way this
				 * is OK is if we have the entire log, all the
				 * way back to file #1.
				 */
				if ((ret = __logc_get(logc,
				    &lsn, &mylog, DB_FIRST)) != 0)
					goto out;
				if (lsn.file != 1) {
					ret = __rep_internal_init(env, 0);
					goto out;
				}

				/* Restore position of log cursor. */
				if ((ret = __logc_get(logc,
				    &rp->lsn, &mylog, DB_SET)) != 0)
					goto out;
			}
		}
		if ((ret = __rep_log_backup(env, rep, logc, &lsn)) == 0) {
			MUTEX_LOCK(env, rep->mtx_clientdb);
			lp->verify_lsn = lsn;
			__os_gettime(env, &lp->rcvd_ts, 1);
			lp->wait_ts = rep->request_gap;
			MUTEX_UNLOCK(env, rep->mtx_clientdb);
			if (master != DB_EID_INVALID)
				eid = master;
			(void)__rep_send_message(env, eid, REP_VERIFY_REQ,
			    &lsn, NULL, 0, DB_REP_ANYWHERE);
		} else if (ret == DB_NOTFOUND) {
			/*
			 * We've either run out of records because
			 * logs have been removed or we've rolled back
			 * all the way to the beginning.
			 */
			ret = __rep_internal_init(env, 0);
		}
	} else {
		/*
		 * We have a match, so we can probably do a simple sync, without
		 * needing internal init.  But first, check for a couple of
		 * special cases.
		 */

		if (!lp->db_log_inmemory && !IS_ZERO_LSN(prev_ckp)) {
			/*
			 * We previously saw a checkpoint, which means we may
			 * now be about to roll back over it and lose it.  Make
			 * sure we'll end up still having at least one other
			 * checkpoint.  (Note that if the current record -- the
			 * one we've just matched -- happens to be a checkpoint,
			 * then it must be the same as the prev_ckp we're now
			 * about to try reading.  Which means we wouldn't really
			 * have to read it.  But checking for that special case
			 * doesn't seem worth the trouble.)
			 */
			if ((ret = __logc_get(logc,
			    &prev_ckp, &mylog, DB_SET)) != 0) {
				if (ret == DB_NOTFOUND)
					ret = __rep_internal_init(env, 0);
				goto out;
			}
			/*
			 * We succeeded reading for the prev_ckp, so it's safe
			 * to fall through to the verify_match.
			 */
		}
		/*
		 * Mixed version internal init doesn't work with 4.4, so we
		 * can't load NIMDBs from a very old-version master.  So, fib to
		 * ourselves that they're already loaded, so that we don't try.
		 */
		if (rep->version == DB_REPVERSION_44)
			F_SET(rep, REP_F_NIMDBS_LOADED);
		if (F_ISSET(rep, REP_F_NIMDBS_LOADED))
			ret = __rep_verify_match(env, &rp->lsn, savetime);
		else {
			/*
			 * Even though we found a match, we haven't yet loaded
			 * any NIMDBs, so we have to do an abbreviated internal
			 * init.  We leave lp->verify_lsn set to the matching
			 * sync point, in case upon eventual examination of the
			 * UPDATE message it turns out there are no NIMDBs
			 * (since we can then skip back to a verify_match
			 * outcome).
			 */
			ret = __rep_internal_init(env, REP_F_ABBREVIATED);
		}
	}

out:	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

static int
__rep_internal_init(env, abbrev)
	ENV *env;
	u_int32_t abbrev;
{
	REP *rep;
	int master, ret;

	rep = env->rep_handle->region;
	REP_SYSTEM_LOCK(env);
#ifdef HAVE_STATISTICS
	if (!abbrev)
		rep->stat.st_outdated++;
#endif

	/*
	 * What we call "abbreviated internal init" is really just NIMDB
	 * materialization, and we always do that even if NOAUTOINIT has been
	 * configured.
	 */
	if (FLD_ISSET(rep->config, REP_C_NOAUTOINIT) && !abbrev)
		ret = DB_REP_JOIN_FAILURE;
	else {
		F_CLR(rep, REP_F_RECOVER_VERIFY);
		F_SET(rep, REP_F_RECOVER_UPDATE);
		if (abbrev) {
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			 "send UPDATE_REQ, merely to check for NIMDB refresh"));
			F_SET(rep, REP_F_ABBREVIATED);
		} else
			F_CLR(rep, REP_F_ABBREVIATED);
		ZERO_LSN(rep->first_lsn);
		ZERO_LSN(rep->ckp_lsn);
		ret = 0;
	}
	master = rep->master_id;
	REP_SYSTEM_UNLOCK(env);
	if (ret == 0 && master != DB_EID_INVALID)
		(void)__rep_send_message(env,
		    master, REP_UPDATE_REQ, NULL, NULL, 0, 0);
	return (ret);
}

/*
 * __rep_verify_fail --
 *	Handle a REP_VERIFY_FAIL message.
 *
 * PUBLIC: int __rep_verify_fail __P((ENV *, __rep_control_args *));
 */
int
__rep_verify_fail(env, rp)
	ENV *env;
	__rep_control_args *rp;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	int clnt_lock_held, lockout, master, ret;

	clnt_lock_held = lockout = 0;
	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	/*
	 * If any recovery flags are set, but not LOG or VERIFY,
	 * then we ignore this message.  We are already
	 * in the middle of updating.
	 */
	if (F_ISSET(rep, REP_F_RECOVER_MASK) &&
	    !F_ISSET(rep, REP_F_RECOVER_LOG | REP_F_RECOVER_VERIFY))
		return (0);
	REP_SYSTEM_LOCK(env);
	/*
	 * We should not ever be in internal init with a lease granted.
	 */
	DB_ASSERT(env,
	    !IS_USING_LEASES(env) || __rep_islease_granted(env) == 0);

	/*
	 * Clean up old internal init in progress if:
	 * REP_C_NOAUTOINIT is not configured and
	 * we are recovering LOG and this LSN is in the range we need.
	 */
	if (F_ISSET(rep, REP_F_RECOVER_LOG) &&
	    LOG_COMPARE(&rep->first_lsn, &rp->lsn) <= 0 &&
	    LOG_COMPARE(&rep->last_lsn, &rp->lsn) >= 0) {
		/*
		 * Already locking out messages, give up.
		 */
		if (F_ISSET(rep, REP_F_READY_MSG))
			goto unlock;

		/*
		 * Lock out other messages to prevent race conditions.
		 */
		if ((ret = __rep_lockout_msg(env, rep, 1)) != 0)
			goto unlock;
		lockout = 1;

		/*
		 * Clean up internal init if one was in progress.
		 */
		if (F_ISSET(rep, REP_F_READY_API | REP_F_READY_OP)) {
			RPRINT(env, DB_VERB_REP_SYNC, (env,
    "VERIFY_FAIL is cleaning up old internal init for missing log"));
			if ((ret =
			    __rep_init_cleanup(env, rep, DB_FORCE)) != 0) {
				RPRINT(env, DB_VERB_REP_SYNC, (env,
    "VERIFY_FAIL error cleaning up internal init for missing log: %d", ret));
				goto msglck;
			}
			F_CLR(rep, REP_F_RECOVER_MASK);
		}
		F_CLR(rep, REP_F_READY_MSG);
		lockout = 0;
	}

	REP_SYSTEM_UNLOCK(env);
	MUTEX_LOCK(env, rep->mtx_clientdb);
	clnt_lock_held = 1;
	REP_SYSTEM_LOCK(env);
	/*
	 * Commence an internal init if:
	 * We are in VERIFY state and the failing LSN is the one we
	 * were verifying or
	 * we're recovering LOG and this LSN is in the range we need or
	 * we are in normal state (no recovery flags set) and
	 * the failing LSN is the one we're ready for.
	 *
	 * We don't want an old or delayed VERIFY_FAIL message to throw us
	 * into internal initialization when we shouldn't be.
	 */
	if (((F_ISSET(rep, REP_F_RECOVER_VERIFY)) &&
	    LOG_COMPARE(&rp->lsn, &lp->verify_lsn) == 0) ||
	    (F_ISSET(rep, REP_F_RECOVER_LOG) &&
	    LOG_COMPARE(&rep->first_lsn, &rp->lsn) <= 0 &&
	    LOG_COMPARE(&rep->last_lsn, &rp->lsn) >= 0) ||
	    (F_ISSET(rep, REP_F_RECOVER_MASK) == 0 &&
	    LOG_COMPARE(&rp->lsn, &lp->ready_lsn) >= 0)) {
		/*
		 * Update stats.
		 */
		STAT(rep->stat.st_outdated++);

		/*
		 * If REP_C_NOAUTOINIT is configured, return
		 * DB_REP_JOIN_FAILURE instead of doing internal init.
		 */
		if (FLD_ISSET(rep->config, REP_C_NOAUTOINIT)) {
			ret = DB_REP_JOIN_FAILURE;
			goto unlock;
		}

		/*
		 * Do the internal init.
		 */
		F_CLR(rep, REP_F_RECOVER_VERIFY);
		F_SET(rep, REP_F_RECOVER_UPDATE);
		ZERO_LSN(rep->first_lsn);
		ZERO_LSN(rep->ckp_lsn);
		lp->wait_ts = rep->request_gap;
		master = rep->master_id;
		REP_SYSTEM_UNLOCK(env);
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		if (master != DB_EID_INVALID)
			(void)__rep_send_message(env,
			    master, REP_UPDATE_REQ, NULL, NULL, 0, 0);
	} else {
		/*
		 * Otherwise ignore this message.
		 */
msglck:		if (lockout)
			F_CLR(rep, REP_F_READY_MSG);
unlock:		REP_SYSTEM_UNLOCK(env);
		if (clnt_lock_held)
			MUTEX_UNLOCK(env, rep->mtx_clientdb);
	}
	return (ret);
}

/*
 * __rep_verify_req --
 *	Handle a REP_VERIFY_REQ message.
 *
 * PUBLIC: int __rep_verify_req __P((ENV *, __rep_control_args *, int));
 */
int
__rep_verify_req(env, rp, eid)
	ENV *env;
	__rep_control_args *rp;
	int eid;
{
	DBT *d, data_dbt;
	DB_LOGC *logc;
	DB_REP *db_rep;
	REP *rep;
	u_int32_t type;
	int old, ret;

	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;

	type = REP_VERIFY;
	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);
	d = &data_dbt;
	memset(d, 0, sizeof(data_dbt));
	F_SET(logc, DB_LOG_SILENT_ERR);
	ret = __logc_get(logc, &rp->lsn, d, DB_SET);
	/*
	 * If the LSN was invalid, then we might get a DB_NOTFOUND
	 * we might get an EIO, we could get anything.
	 * If we get a DB_NOTFOUND, then there is a chance that
	 * the LSN comes before the first file present in which
	 * case we need to return a fail so that the client can
	 * perform an internal init or return a REP_JOIN_FAILURE.
	 *
	 * If we're a client servicing this request and we get a
	 * NOTFOUND, return it so the caller can rerequest from
	 * a better source.
	 */
	if (ret == DB_NOTFOUND) {
		if (F_ISSET(rep, REP_F_CLIENT)) {
			(void)__logc_close(logc);
			return (DB_NOTFOUND);
		}
		if (__log_is_outdated(env, rp->lsn.file, &old) == 0 &&
		    old != 0)
			type = REP_VERIFY_FAIL;
	}

	if (ret != 0)
		d = NULL;

	(void)__rep_send_message(env, eid, type, &rp->lsn, d, 0, 0);
	return (__logc_close(logc));
}

/*
 * PUBLIC: int __rep_dorecovery __P((ENV *, DB_LSN *, DB_LSN *));
 */
int
__rep_dorecovery(env, lsnp, trunclsnp)
	ENV *env;
	DB_LSN *lsnp, *trunclsnp;
{
	DBT mylog;
	DB_LOGC *logc;
	DB_LSN last_ckp, lsn;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	REP *rep;
	int ret, skip_rec, t_ret, update;
	u_int32_t rectype, opcode;
	__txn_regop_args *txnrec;
	__txn_regop_42_args *txn42rec;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	ENV_GET_THREAD_INFO(env, ip);

	/* Figure out if we are backing out any committed transactions. */
	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);

	memset(&mylog, 0, sizeof(mylog));
	if (F_ISSET(rep, REP_F_RECOVER_LOG)) {
		/*
		 * Internal init can never skip recovery.
		 * Internal init must always update the timestamp and
		 * force dead handles.
		 */
		skip_rec = 0;
		update = 1;
	} else {
		skip_rec = 1;
		update = 0;
	}
	while (update == 0 &&
	    (ret = __logc_get(logc, &lsn, &mylog, DB_PREV)) == 0 &&
	    LOG_COMPARE(&lsn, lsnp) > 0) {
		LOGCOPY_32(env, &rectype, mylog.data);
		/*
		 * Find out if we can skip recovery completely.  If we
		 * are backing up over any record a client usually
		 * cares about, we must run recovery.
		 *
		 * Skipping sync-up recovery can be pretty scary!
		 * Here's why we can do it:
		 * If a master downgraded to client and is now running
		 * sync-up to a new master, that old master must have
		 * waited for any outstanding txns to resolve before
		 * becoming a client.  Also we are in lockout so there
		 * can be no other operations right now.
		 *
		 * If the client wrote a commit record to the log, but
		 * was descheduled before processing the txn, and then
		 * a new master was found, we must've let the txn get
		 * processed because right now we are the only message
		 * thread allowed to be running.
		 */
		DB_ASSERT(env, rep->op_cnt == 0);
		DB_ASSERT(env, rep->msg_th == 1);
		if (rectype == DB___txn_regop || rectype == DB___txn_ckp ||
		    rectype == DB___dbreg_register)
			skip_rec = 0;
		if (rectype == DB___txn_regop) {
			if (rep->version >= DB_REPVERSION_44) {
				if ((ret = __txn_regop_read(
				    env, mylog.data, &txnrec)) != 0)
					goto err;
				opcode = txnrec->opcode;
				__os_free(env, txnrec);
			} else {
				if ((ret = __txn_regop_42_read(
				    env, mylog.data, &txn42rec)) != 0)
					goto err;
				opcode = txn42rec->opcode;
				__os_free(env, txn42rec);
			}
			if (opcode != TXN_ABORT)
				update = 1;
		}
	}
	/*
	 * Handle if the logc_get fails.
	 */
	if (ret != 0)
		goto err;

	/*
	 * If we successfully run recovery, we've opened all the necessary
	 * files.  We are guaranteed to be single-threaded here, so no mutex
	 * is necessary.
	 */
	if (skip_rec) {
		if ((ret = __log_get_stable_lsn(env, &last_ckp)) != 0) {
			if (ret != DB_NOTFOUND)
				goto err;
			ZERO_LSN(last_ckp);
		}
		RPRINT(env, DB_VERB_REP_SYNC, (env,
    "Skip sync-up rec.  Truncate log to [%lu][%lu], ckp [%lu][%lu]",
    (u_long)lsnp->file, (u_long)lsnp->offset,
    (u_long)last_ckp.file, (u_long)last_ckp.offset));
		ret = __log_vtruncate(env, lsnp, &last_ckp, trunclsnp);
	} else
		ret = __db_apprec(env, ip, lsnp, trunclsnp, update, 0);

	if (ret != 0)
		goto err;
	F_SET(db_rep, DBREP_OPENFILES);

err:	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __rep_verify_match --
 *	We have just received a matching log record during verification.
 * Figure out if we're going to need to run recovery. If so, wait until
 * everything else has exited the library.  If not, set up the world
 * correctly and move forward.
 *
 * PUBLIC: int __rep_verify_match __P((ENV *, DB_LSN *, time_t));
 */
int
__rep_verify_match(env, reclsnp, savetime)
	ENV *env;
	DB_LSN *reclsnp;
	time_t savetime;
{
	DB_LOG *dblp;
	DB_LSN trunclsn;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	LOG *lp;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	int done, master, ret;
	u_int32_t unused;

	dblp = env->lg_handle;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	lp = dblp->reginfo.primary;
	ret = 0;
	infop = env->reginfo;
	renv = infop->primary;
	ENV_GET_THREAD_INFO(env, ip);

	/*
	 * Check if the savetime is different than our current time stamp.
	 * If it is, then we're racing with another thread trying to recover
	 * and we lost.  We must give up.
	 */
	MUTEX_LOCK(env, rep->mtx_clientdb);
	done = savetime != renv->rep_timestamp;
	if (done) {
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		return (0);
	}
	ZERO_LSN(lp->verify_lsn);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);

	/*
	 * Make sure the world hasn't changed while we tried to get
	 * the lock.  If it hasn't then it's time for us to kick all
	 * operations out of DB and run recovery.
	 */
	REP_SYSTEM_LOCK(env);
	if (F_ISSET(rep, REP_F_READY_MSG) ||
	    (!F_ISSET(rep, REP_F_RECOVER_LOG) &&
	    F_ISSET(rep, REP_F_READY_API | REP_F_READY_OP))) {
		/*
		 * We lost.  The world changed and we should do nothing.
		 */
		STAT(rep->stat.st_msgs_recover++);
		goto errunlock;
	}

	/*
	 * Lockout all message threads but ourselves.
	 */
	if ((ret = __rep_lockout_msg(env, rep, 1)) != 0)
		goto errunlock;

	/*
	 * Lockout the API and wait for operations to complete.
	 */
	if ((ret = __rep_lockout_api(env, rep)) != 0)
		goto errunlock;

	/* OK, everyone is out, we can now run recovery. */
	REP_SYSTEM_UNLOCK(env);

	if ((ret = __rep_dorecovery(env, reclsnp, &trunclsn)) != 0 ||
	    (ret = __rep_remove_init_file(env)) != 0) {
		REP_SYSTEM_LOCK(env);
		F_CLR(rep, REP_F_READY_API | REP_F_READY_MSG | REP_F_READY_OP);
		goto errunlock;
	}

	/*
	 * The log has been truncated (either directly by us or by __db_apprec)
	 * We want to make sure we're waiting for the LSN at the new end-of-log,
	 * not some later point.
	 */
	MUTEX_LOCK(env, rep->mtx_clientdb);
	lp->ready_lsn = trunclsn;
	ZERO_LSN(lp->waiting_lsn);
	ZERO_LSN(lp->max_wait_lsn);
	lp->max_perm_lsn = *reclsnp;
	lp->wait_ts = rep->request_gap;
	__os_gettime(env, &lp->rcvd_ts, 1);
	ZERO_LSN(lp->verify_lsn);
	ZERO_LSN(lp->prev_ckp);

	/*
	 * Discard any log records we have queued;  we're about to re-request
	 * them, and can't trust the ones in the queue.  We need to set the
	 * DB_AM_RECOVER bit in this handle, so that the operation doesn't
	 * deadlock.
	 */
	if (db_rep->rep_db == NULL &&
	    (ret = __rep_client_dbinit(env, 0, REP_DB)) != 0) {
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		goto out;
	}

	F_SET(db_rep->rep_db, DB_AM_RECOVER);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	ret = __db_truncate(db_rep->rep_db, ip, NULL, &unused);
	MUTEX_LOCK(env, rep->mtx_clientdb);
	F_CLR(db_rep->rep_db, DB_AM_RECOVER);

	REP_SYSTEM_LOCK(env);
	rep->stat.st_log_queued = 0;
	F_CLR(rep, REP_F_NOARCHIVE | REP_F_RECOVER_MASK | REP_F_READY_MSG);
	if (ret != 0)
		goto errunlock2;

	/*
	 * If the master_id is invalid, this means that since
	 * the last record was sent, something happened to the
	 * master and we may not have a master to request
	 * things of.
	 *
	 * This is not an error;  when we find a new master,
	 * we'll re-negotiate where the end of the log is and
	 * try to bring ourselves up to date again anyway.
	 */
	master = rep->master_id;
	REP_SYSTEM_UNLOCK(env);
	if (master == DB_EID_INVALID) {
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		ret = 0;
	} else {
		/*
		 * We're making an ALL_REQ.  But now that we've
		 * cleared the flags, we're likely receiving new
		 * log records from the master, resulting in a gap
		 * immediately.  So to avoid multiple data streams,
		 * set the wait_ts value high now to give the master
		 * a chance to start sending us these records before
		 * the gap code re-requests the same gap.  Wait_recs
		 * will get reset once we start receiving these
		 * records.
		 */
		lp->wait_ts = rep->max_gap;
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		(void)__rep_send_message(env,
		    master, REP_ALL_REQ, reclsnp, NULL, 0, DB_REP_ANYWHERE);
	}
	if (0) {
errunlock2:	MUTEX_UNLOCK(env, rep->mtx_clientdb);
errunlock:	REP_SYSTEM_UNLOCK(env);
	}
out:	return (ret);
}

/*
 * __rep_log_backup --
 *
 * In the verify handshake, we walk backward looking for
 * identification records.  Those are the only record types
 * we verify and match on.
 *
 * PUBLIC: int __rep_log_backup __P((ENV *, REP *, DB_LOGC *, DB_LSN *));
 */
int
__rep_log_backup(env, rep, logc, lsn)
	ENV *env;
	REP *rep;
	DB_LOGC *logc;
	DB_LSN *lsn;
{
	DBT mylog;
	u_int32_t rectype;
	int ret;

	ret = 0;
	memset(&mylog, 0, sizeof(mylog));
	while ((ret = __logc_get(logc, lsn, &mylog, DB_PREV)) == 0) {
		/*
		 * Determine what we look for based on version number.
		 * Due to the contents of records changing between
		 * versions we have to match based on criteria of that
		 * particular version.
		 */
		LOGCOPY_32(env, &rectype, mylog.data);
		/*
		 * In 4.4 and beyond we match checkpoint and commit.
		 */
		if (rep->version >= DB_REPVERSION_44 &&
		    (rectype == DB___txn_ckp || rectype == DB___txn_regop))
			break;
	}
	return (ret);
}
