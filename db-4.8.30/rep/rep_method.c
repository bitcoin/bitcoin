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
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int  __rep_abort_prepared __P((ENV *));
static int  __rep_bt_cmp __P((DB *, const DBT *, const DBT *));
static void __rep_config_map __P((ENV *, u_int32_t *, u_int32_t *));
static u_int32_t __rep_conv_vers __P((ENV *, u_int32_t));
static int  __rep_restore_prepared __P((ENV *));

/*
 * __rep_env_create --
 *	Replication-specific initialization of the ENV structure.
 *
 * PUBLIC: int __rep_env_create __P((DB_ENV *));
 */
int
__rep_env_create(dbenv)
	DB_ENV *dbenv;
{
	DB_REP *db_rep;
	ENV *env;
	int ret;

	env = dbenv->env;

	if ((ret = __os_calloc(env, 1, sizeof(DB_REP), &db_rep)) != 0)
		return (ret);

	db_rep->eid = DB_EID_INVALID;
	db_rep->bytes = REP_DEFAULT_THROTTLE;
	DB_TIMEOUT_TO_TIMESPEC(DB_REP_REQUEST_GAP, &db_rep->request_gap);
	DB_TIMEOUT_TO_TIMESPEC(DB_REP_MAX_GAP, &db_rep->max_gap);
	db_rep->elect_timeout = 2 * US_PER_SEC;			/*  2 seconds */
	db_rep->chkpt_delay = 30 * US_PER_SEC;			/* 30 seconds */
	db_rep->my_priority = DB_REP_DEFAULT_PRIORITY;
	/*
	 * Make no clock skew the default.  Setting both fields
	 * to the same non-zero value means no skew.
	 */
	db_rep->clock_skew = 1;
	db_rep->clock_base = 1;

#ifdef HAVE_REPLICATION_THREADS
	if ((ret = __repmgr_env_create(env, db_rep)) != 0) {
		__os_free(env, db_rep);
		return (ret);
	}
#endif

	env->rep_handle = db_rep;
	return (0);
}

/*
 * __rep_env_destroy --
 *	Replication-specific destruction of the ENV structure.
 *
 * PUBLIC: void __rep_env_destroy __P((DB_ENV *));
 */
void
__rep_env_destroy(dbenv)
	DB_ENV *dbenv;
{
	ENV *env;

	env = dbenv->env;

	if (env->rep_handle != NULL) {
#ifdef HAVE_REPLICATION_THREADS
		__repmgr_env_destroy(env, env->rep_handle);
#endif
		__os_free(env, env->rep_handle);
		env->rep_handle = NULL;
	}
}

/*
 * __rep_get_config --
 *	Return the replication subsystem configuration.
 *
 * PUBLIC: int __rep_get_config __P((DB_ENV *, u_int32_t, int *));
 */
int
__rep_get_config(dbenv, which, onp)
	DB_ENV *dbenv;
	u_int32_t which;
	int *onp;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;
	u_int32_t mapped;

	env = dbenv->env;

#undef	OK_FLAGS
#define	OK_FLAGS							\
    (DB_REP_CONF_BULK | DB_REP_CONF_DELAYCLIENT | DB_REP_CONF_INMEM |	\
    DB_REP_CONF_LEASE | DB_REP_CONF_NOAUTOINIT | DB_REP_CONF_NOWAIT |	\
    DB_REPMGR_CONF_2SITE_STRICT)

	if (FLD_ISSET(which, ~OK_FLAGS))
		return (__db_ferr(env, "DB_ENV->rep_get_config", 0));

	db_rep = env->rep_handle;
	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_config", DB_INIT_REP);

	mapped = 0;
	__rep_config_map(env, &which, &mapped);
	if (REP_ON(env)) {
		rep = db_rep->region;
		if (FLD_ISSET(rep->config, mapped))
			*onp = 1;
		else
			*onp = 0;
	} else {
		if (FLD_ISSET(db_rep->config, mapped))
			*onp = 1;
		else
			*onp = 0;
	}
	return (0);
}

/*
 * __rep_set_config --
 *	Configure the replication subsystem.
 *
 * PUBLIC: int __rep_set_config __P((DB_ENV *, u_int32_t, int));
 */
int
__rep_set_config(dbenv, which, on)
	DB_ENV *dbenv;
	u_int32_t which;
	int on;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	LOG *lp;
	REP *rep;
	REP_BULK bulk;
	u_int32_t mapped, orig;
	int ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	ret = 0;

#undef	OK_FLAGS
#define	OK_FLAGS							\
    (DB_REP_CONF_BULK | DB_REP_CONF_DELAYCLIENT | DB_REP_CONF_INMEM |	\
    DB_REP_CONF_LEASE | DB_REP_CONF_NOAUTOINIT | DB_REP_CONF_NOWAIT |	\
    DB_REPMGR_CONF_2SITE_STRICT)

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_config", DB_INIT_REP);

	if (FLD_ISSET(which, ~OK_FLAGS))
		return (__db_ferr(env, "DB_ENV->rep_set_config", 0));

	mapped = 0;
	__rep_config_map(env, &which, &mapped);

	if (APP_IS_BASEAPI(env) && FLD_ISSET(mapped, REP_C_2SITE_STRICT)) {
		__db_errx(env, "%s %s", "DB_ENV->rep_set_config:",
"cannot configure 2SITE_STRICT from base replication application");
		return (EINVAL);
	}

	if (REP_ON(env)) {
		ENV_ENTER(env, ip);

		rep = db_rep->region;
		/*
		 * In-memory replication must be called before calling
		 * env->open.  If it is turned on and off before env->open,
		 * it doesn't matter.  Any attempt to turn it on or off after
		 * env->open is intercepted by this error.
		 */
		if (FLD_ISSET(mapped, REP_C_INMEM)) {
			__db_errx(env, "%s %s", "DB_ENV->rep_set_config:",
	"in-memory replication must be configured before DB_ENV->open");
			return (EINVAL);
		}
		/*
		 * Leases must be turned on before calling rep_start.
		 * Leases can never be turned off once they're turned on.
		 */
		if (FLD_ISSET(mapped, REP_C_LEASE)) {
			if (F_ISSET(rep, REP_F_START_CALLED)) {
				__db_errx(env,
"DB_ENV->rep_set_config: leases must be configured before DB_ENV->rep_start");
				ret = EINVAL;
			}
			if (on == 0) {
				__db_errx(env,
	"DB_ENV->rep_set_config: leases cannot be turned off");
				ret = EINVAL;
			}
			if (ret != 0)
				return (ret);
		}
		MUTEX_LOCK(env, rep->mtx_clientdb);
		REP_SYSTEM_LOCK(env);
		orig = rep->config;
		if (on)
			FLD_SET(rep->config, mapped);
		else
			FLD_CLR(rep->config, mapped);

		/*
		 * Bulk transfer requires special processing if it is getting
		 * toggled.
		 */
		dblp = env->lg_handle;
		lp = dblp->reginfo.primary;
		if (FLD_ISSET(rep->config, REP_C_BULK) &&
		    !FLD_ISSET(orig, REP_C_BULK))
			db_rep->bulk = R_ADDR(&dblp->reginfo, lp->bulk_buf);
		REP_SYSTEM_UNLOCK(env);

		/*
		 * If turning bulk off and it was on, send out whatever is in
		 * the buffer already.
		 */
		if (FLD_ISSET(orig, REP_C_BULK) &&
		    !FLD_ISSET(rep->config, REP_C_BULK) && lp->bulk_off != 0) {
			memset(&bulk, 0, sizeof(bulk));
			if (db_rep->bulk == NULL)
				bulk.addr =
				    R_ADDR(&dblp->reginfo, lp->bulk_buf);
			else
				bulk.addr = db_rep->bulk;
			bulk.offp = &lp->bulk_off;
			bulk.len = lp->bulk_len;
			bulk.type = REP_BULK_LOG;
			bulk.eid = DB_EID_BROADCAST;
			bulk.flagsp = &lp->bulk_flags;
			ret = __rep_send_bulk(env, &bulk, 0);
		}
		MUTEX_UNLOCK(env, rep->mtx_clientdb);

		ENV_LEAVE(env, ip);
	} else {
		if (on)
			FLD_SET(db_rep->config, mapped);
		else
			FLD_CLR(db_rep->config, mapped);
	}
	/* Configuring 2SITE_STRICT makes this a repmgr application */
	if (ret == 0 && FLD_ISSET(mapped, REP_C_2SITE_STRICT))
		APP_SET_REPMGR(env);
	return (ret);
}

static void
__rep_config_map(env, inflagsp, outflagsp)
	ENV *env;
	u_int32_t *inflagsp, *outflagsp;
{
	COMPQUIET(env, NULL);

	if (FLD_ISSET(*inflagsp, DB_REP_CONF_BULK)) {
		FLD_SET(*outflagsp, REP_C_BULK);
		FLD_CLR(*inflagsp, DB_REP_CONF_BULK);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_DELAYCLIENT)) {
		FLD_SET(*outflagsp, REP_C_DELAYCLIENT);
		FLD_CLR(*inflagsp, DB_REP_CONF_DELAYCLIENT);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_INMEM)) {
		FLD_SET(*outflagsp, REP_C_INMEM);
		FLD_CLR(*inflagsp, DB_REP_CONF_INMEM);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_LEASE)) {
		FLD_SET(*outflagsp, REP_C_LEASE);
		FLD_CLR(*inflagsp, DB_REP_CONF_LEASE);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_NOAUTOINIT)) {
		FLD_SET(*outflagsp, REP_C_NOAUTOINIT);
		FLD_CLR(*inflagsp, DB_REP_CONF_NOAUTOINIT);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_NOWAIT)) {
		FLD_SET(*outflagsp, REP_C_NOWAIT);
		FLD_CLR(*inflagsp, DB_REP_CONF_NOWAIT);
	}
	if (FLD_ISSET(*inflagsp, DB_REPMGR_CONF_2SITE_STRICT)) {
		FLD_SET(*outflagsp, REP_C_2SITE_STRICT);
		FLD_CLR(*inflagsp, DB_REPMGR_CONF_2SITE_STRICT);
	}
}

/*
 * __rep_start_pp --
 *	Become a master or client, and start sending messages to participate
 * in the replication environment.  Must be called after the environment
 * is open.
 *
 * PUBLIC: int __rep_start_pp __P((DB_ENV *, DBT *, u_int32_t));
 */
int
__rep_start_pp(dbenv, dbt, flags)
	DB_ENV *dbenv;
	DBT *dbt;
	u_int32_t flags;
{
	DB_REP *db_rep;
	ENV *env;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->rep_start", DB_INIT_REP);

	if (APP_IS_REPMGR(env)) {
		__db_errx(env,
"DB_ENV->rep_start: cannot call from Replication Manager application");
		return (EINVAL);
	}

	switch (LF_ISSET(DB_REP_CLIENT | DB_REP_MASTER)) {
	case DB_REP_CLIENT:
	case DB_REP_MASTER:
		break;
	default:
		__db_errx(env,
	"DB_ENV->rep_start: must specify DB_REP_CLIENT or DB_REP_MASTER");
		return (EINVAL);
	}

	/* We need a transport function because we send messages. */
	if (db_rep->send == NULL) {
		__db_errx(env,
    "DB_ENV->rep_start: must be called after DB_ENV->rep_set_transport");
		return (EINVAL);
	}

	return (__rep_start_int(env, dbt, flags));
}

/*
 * __rep_start_int --
 *	Internal processing to become a master or client and start sending
 * messages to participate in the replication environment.
 *
 * We must protect rep_start_int, which may change the world, with the rest
 * of the DB library.  Each API interface will count itself as it enters
 * the library.  Rep_start_int checks the following:
 *
 * rep->msg_th - this is the count of threads currently in rep_process_message
 * rep->handle_cnt - number of threads actively using a dbp in library.
 * rep->txn_cnt - number of active txns.
 * REP_F_READY_* - Replication flag that indicates that we wish to run
 * recovery, and want to prohibit new transactions from entering and cause
 * existing ones to return immediately (with a DB_LOCK_DEADLOCK error).
 *
 * There is also the renv->rep_timestamp which is updated whenever significant
 * events (i.e., new masters, log rollback, etc).  Upon creation, a handle
 * is associated with the current timestamp.  Each time a handle enters the
 * library it must check if the handle timestamp is the same as the one
 * stored in the replication region.  This prevents the use of handles on
 * clients that reference non-existent files whose creation was backed out
 * during a synchronizing recovery.
 *
 * PUBLIC: int __rep_start_int __P((ENV *, DBT *, u_int32_t));
 */
int
__rep_start_int(env, dbt, flags)
	ENV *env;
	DBT *dbt;
	u_int32_t flags;
{
	DB *dbp;
	DB_LOG *dblp;
	DB_LOGC *logc;
	DB_LSN lsn, perm_lsn;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	DB_TXNREGION *region;
	LOG *lp;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	db_timeout_t tmp;
	u_int32_t oldvers, pending_event, repflags, role;
	int do_ckp, interrupting, locked, ret, role_chg, start_th, t_ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	infop = env->reginfo;
	renv = infop->primary;
	interrupting = locked = 0;
	pending_event = DB_EVENT_NO_SUCH_EVENT;
	role = LF_ISSET(DB_REP_CLIENT | DB_REP_MASTER);
	start_th = 0;
	do_ckp = 0;

	/*
	 * If we're using master leases, check that all needed
	 * setup has been done, including setting the lease timeout.
	 */
	if (IS_USING_LEASES(env) && rep->lease_timeout == 0) {
		__db_errx(env,
"DB_ENV->rep_start: must call DB_ENV->rep_set_timeout for leases first");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);

	/*
	 * In order to correctly check log files for old versions, we
	 * need to flush the logs.
	 */
	if ((ret = __log_flush(env, NULL)) != 0)
		goto out;

	REP_SYSTEM_LOCK(env);
	/*
	 * We only need one thread to start-up replication, so if
	 * there is another thread in rep_start, we'll let it finish
	 * its work and have this thread simply return.  Similarly,
	 * if a thread is in a critical lockout section we return.
	 */
	if (F_ISSET(rep, REP_F_INREPSTART)) {
		/*
		 * There is already someone in rep_start.  Return.
		 */
		RPRINT(env, DB_VERB_REP_MISC,
		    (env, "Thread already in rep_start"));
		REP_SYSTEM_UNLOCK(env);
		goto out;
	} else {
		F_SET(rep, REP_F_INREPSTART);
		start_th = 1;
	}

	if (F_ISSET(rep, REP_F_READY_MSG)) {
		/*
		 * There is already someone in msg lockout.  Return.
		 */
		RPRINT(env, DB_VERB_REP_MISC,
		    (env, "Thread already in msg lockout"));
		REP_SYSTEM_UNLOCK(env);
		goto out;
	} else if ((ret = __rep_lockout_msg(env, rep, 0)) != 0)
		goto errunlock;

	/*
	 * If we are internal init and we try to become master, reject it.
	 * Our environment databases/logs are in an inconsistent state and
	 * we cannot become master.
	 */
	if (IN_INTERNAL_INIT(rep) && role == DB_REP_MASTER) {
		__db_errx(env,
"DB_ENV->rep_start: Cannot become master during internal init");
		ret = DB_REP_UNAVAIL;
		goto errunlock;
	}

	role_chg = (!F_ISSET(rep, REP_F_MASTER) && role == DB_REP_MASTER) ||
	    (!F_ISSET(rep, REP_F_CLIENT) && role == DB_REP_CLIENT);

	/*
	 * Wait for any active txns or mpool ops to complete, and
	 * prevent any new ones from occurring, only if we're
	 * changing roles.
	 */
	if (role_chg) {
		if ((ret = __rep_lockout_api(env, rep)) != 0)
			goto errunlock;
		locked = 1;
	}

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	if (role == DB_REP_MASTER) {
		if (role_chg) {
			/*
			 * If we were previously a client, it's possible we
			 * could have an interruptible STARTSYNC in progress.
			 * Interrupt it now, so that it doesn't slow down our
			 * transition to master, and because its effects aren't
			 * doing us any good anyway.
			 */
			(void)__memp_set_config(
			    env->dbenv, DB_MEMP_SYNC_INTERRUPT, 1);
			interrupting = 1;

			/*
			 * If we're upgrading from having been a client,
			 * preclose, so that we close our temporary database
			 * and any files we opened while doing a rep_apply.
			 * If we don't we can infinitely leak file ids if
			 * the master crashed with files open (the likely
			 * case).  If we don't close them we can run into
			 * problems if we try to remove that file or long
			 * running applications end up with an unbounded
			 * number of used fileids, each getting written
			 * on checkpoint.  Just close them.
			 * Then invalidate all files open in the logging
			 * region.  These are files open by other processes
			 * attached to the environment.  They must be
			 * closed by the other processes when they notice
			 * the change in role.
			 */
			if ((ret = __rep_preclose(env)) != 0)
				goto errunlock;

			rep->gen++;
			/*
			 * There could have been any number of failed
			 * elections, so jump the gen if we need to now.
			 */
			if (rep->egen > rep->gen)
				rep->gen = rep->egen;
			if (IS_USING_LEASES(env) &&
			    !F_ISSET(rep, REP_F_MASTERELECT)) {
				__db_errx(env,
    "rep_start: Cannot become master without being elected when using leases.");
				ret = EINVAL;
				goto errunlock;
			}
			if (F_ISSET(rep, REP_F_MASTERELECT)) {
				__rep_elect_done(env, rep, 0);
				F_CLR(rep, REP_F_MASTERELECT);
			}
			if (rep->egen <= rep->gen)
				rep->egen = rep->gen + 1;
			RPRINT(env, DB_VERB_REP_MISC, (env,
			    "New master gen %lu, egen %lu",
			    (u_long)rep->gen, (u_long)rep->egen));
			/*
			 * If not running in-memory replication, write
			 * gen file.
			 */
			if (!FLD_ISSET(rep->config, REP_C_INMEM)) {
				if ((ret = __rep_write_gen(env, rep, rep->gen))
				    != 0)
					goto errunlock;
			} else if (!F_ISSET(rep, REP_F_MASTERELECT))
				/*
				 * Help detect if application has
				 * ignored our recommendation against
				 * reappointing same master after a
				 * crash/reboot when running in-memory
				 * replication. Doing this allows a
				 * slight chance of two masters at the
				 * same generation resulting in client
				 * crashes.
				 */
				RPRINT(env, DB_VERB_REP_MISC, (env,
	"Appointed new master while running in-memory replication."));
		}
		/*
		 * Set lease duration assuming clients have faster clock.
		 * Master needs to compensate so that clients do not
		 * expire their grant while the master thinks it is valid.
		 */
		if (IS_USING_LEASES(env) &&
		    (role_chg || !IS_REP_STARTED(env))) {
			/*
			 * If we have already granted our lease, we
			 * cannot become master.
			 */
			if ((ret = __rep_islease_granted(env))) {
				__db_errx(env,
    "rep_start: Cannot become master with outstanding lease granted.");
				ret = EINVAL;
				goto errunlock;
			}
			/*
			 * Set max_perm_lsn to last PERM record on master.
			 */
			if ((ret = __log_cursor(env, &logc)) != 0)
				goto errunlock;
			ret = __rep_log_backup(env, rep, logc, &perm_lsn);
			(void)__logc_close(logc);
			/*
			 * If we found a perm LSN use it.  Otherwise, if
			 * no perm LSN exists, initialize.
			 */
			if (ret == 0)
				lp->max_perm_lsn = perm_lsn;
			else if (ret == DB_NOTFOUND) {
				/*
				 * If we have no perm records, we want to
				 * force (later) a checkpoint to the log.
				 * By doing this now, we avoid a sticky
				 * deadlock with a txn.  We need a perm
				 * record for leases, but if the first perm
				 * record is a txn, that txn cannot commit
				 * without leases refreshed.  A client may
				 * be in internal init and cannot sync up if
				 * it needs to read pages the txn holds write
				 * locks on and we have an impasse.  This
				 * checkpoint will allow leases to be granted
				 * on this perm record first and that does not
				 * need any locks.
				 */
				do_ckp = 1;
				INIT_LSN(lp->max_perm_lsn);
			} else
				goto errunlock;

			/*
			 * Simply compute the larger ratio for the lease.
			 */
			tmp = (db_timeout_t)((double)rep->lease_timeout /
			    ((double)rep->clock_skew /
			    (double)rep->clock_base));
			DB_TIMEOUT_TO_TIMESPEC(tmp, &rep->lease_duration);
			if ((ret = __rep_lease_table_alloc(env,
			    rep->nsites)) != 0)
				goto errunlock;
		}
		rep->master_id = rep->eid;
		STAT(rep->stat.st_master_changes++);

		/*
		 * Clear out almost everything, and then set MASTER.  Leave
		 * READY_* alone in case we did a lockout above;
		 * we'll clear it in a moment (below), once we've written
		 * the txn_recycle into the log.
		 */
		repflags = F_ISSET(rep, REP_F_INREPSTART | REP_F_READY_API |
		    REP_F_READY_MSG | REP_F_READY_OP | REP_F_STICKY_MASK);
#ifdef	DIAGNOSTIC
		if (!F_ISSET(rep, REP_F_GROUP_ESTD))
			RPRINT(env, DB_VERB_REP_MISC, (env,
			    "Establishing group as master."));
#endif
		FLD_SET(repflags, REP_F_MASTER |
		    REP_F_GROUP_ESTD | REP_F_NIMDBS_LOADED);
		rep->flags = repflags;

		/*
		 * We're master.  Set the versions to the current ones.
		 */
		oldvers = lp->persist.version;
		/*
		 * If we're moving forward to the current version, we need
		 * to force the log file to advance and reset the
		 * recovery table since it contains pointers to old
		 * recovery functions.
		 */
		RPRINT(env, DB_VERB_REP_MISC, (env,
		    "rep_start: Old log version was %lu", (u_long)oldvers));
		if (lp->persist.version != DB_LOGVERSION) {
			if ((ret = __env_init_rec(env, DB_LOGVERSION)) != 0)
				goto errunlock;
		}
		rep->version = DB_REPVERSION;
		F_CLR(rep, REP_F_READY_MSG);
		REP_SYSTEM_UNLOCK(env);
		LOG_SYSTEM_LOCK(env);
		lsn = lp->lsn;
		LOG_SYSTEM_UNLOCK(env);

		/*
		 * Send the NEWMASTER message first so that clients know
		 * subsequent messages are coming from the right master.
		 * We need to perform all actions below no matter what
		 * regarding errors.
		 */
		(void)__rep_send_message(env,
		    DB_EID_BROADCAST, REP_NEWMASTER, &lsn, NULL, 0, 0);
		ret = 0;
		if (role_chg) {
			pending_event = DB_EVENT_REP_MASTER;
			/*
			 * If prepared transactions have not been restored
			 * look to see if there are any.  If there are,
			 * then mark the open files, otherwise close them.
			 */
			region = env->tx_handle->reginfo.primary;
			if (region->stat.st_nrestores == 0 &&
			    (t_ret = __rep_restore_prepared(env)) != 0 &&
			    ret == 0)
				ret = t_ret;
			if (region->stat.st_nrestores != 0) {
			    if ((t_ret = __dbreg_mark_restored(env)) != 0 &&
				    ret == 0)
					ret = t_ret;
			} else {
				ret = __dbreg_invalidate_files(env, 0);
				if ((t_ret = __rep_closefiles(env)) != 0 &&
				    ret == 0)
					ret = t_ret;
			}
			if ((t_ret = __txn_recycle_id(env)) != 0 && ret == 0)
				ret = t_ret;
			REP_SYSTEM_LOCK(env);
			F_CLR(rep, REP_F_READY_API | REP_F_READY_OP);
			locked = 0;
			REP_SYSTEM_UNLOCK(env);
			(void)__memp_set_config(
			    env->dbenv, DB_MEMP_SYNC_INTERRUPT, 0);
			interrupting = 0;
			/*
			 * Force a checkpoint if this new master has no
			 * perm record yet.
			 */
			if (ret == 0 && do_ckp)
				ret = __txn_checkpoint(env, 0, 0,
				    DB_CKP_INTERNAL | DB_FORCE);
		}
	} else {
		if (role_chg)
			rep->master_id = DB_EID_INVALID;
		/*
		 * Zero out "everything" except recovery and tally flags.
		 */
		repflags = F_ISSET(rep,
		    REP_F_INREPSTART | REP_F_NOARCHIVE | REP_F_READY_MSG |
		    REP_F_RECOVER_MASK | REP_F_TALLY | REP_F_STICKY_MASK);
		FLD_SET(repflags, REP_F_CLIENT);
		if (role_chg) {
			if ((ret = __log_get_oldversion(env, &oldvers)) != 0)
				goto errunlock;
			RPRINT(env, DB_VERB_REP_MISC, (env,
			    "rep_start: Found old version log %d", oldvers));
			if (oldvers >= DB_LOGVERSION_MIN) {
				__log_set_version(env, oldvers);
				oldvers = __rep_conv_vers(env, oldvers);
				DB_ASSERT(
				    env, oldvers != DB_REPVERSION_INVALID);
				rep->version = oldvers;
			}
		}
		rep->flags = repflags;
		/*
		 * On a client, compute the lease duration on the
		 * assumption that the client has a fast clock.
		 * Expire any existing leases we might have held as
		 * a master.
		 */
		if (IS_USING_LEASES(env) &&
		    (role_chg || !IS_REP_STARTED(env))) {
			if ((ret = __rep_lease_expire(env)) != 0)
				goto errunlock;
			/*
			 * Since the master is also compensating on its
			 * side as well, we're being doubly conservative
			 * to compensate on the client side.  Theoretically,
			 * this compensation is not necessary, as it is
			 * effectively doubling the skew compensation.
			 * But we are making guarantees based on time and
			 * skews across machines.  So we are being extra
			 * cautious.
			 */
			tmp = (db_timeout_t)((double)rep->lease_timeout *
			    ((double)rep->clock_skew /
			    (double)rep->clock_base));
			DB_TIMEOUT_TO_TIMESPEC(tmp, &rep->lease_duration);
			if (rep->lease_off != INVALID_ROFF) {
				MUTEX_LOCK(env, renv->mtx_regenv);
				__env_alloc_free(infop,
				    R_ADDR(infop, rep->lease_off));
				MUTEX_UNLOCK(env, renv->mtx_regenv);
				rep->lease_off = INVALID_ROFF;
			}
		}
		REP_SYSTEM_UNLOCK(env);

		/*
		 * Abort any prepared transactions that were restored
		 * by recovery.  We won't be able to create any txns of
		 * our own until they're resolved, but we can't resolve
		 * them ourselves;  the master has to.  If any get
		 * resolved as commits, we'll redo them when commit
		 * records come in.  Aborts will simply be ignored.
		 */
		if ((ret = __rep_abort_prepared(env)) != 0)
			goto errlock;

		/*
		 * If we're changing roles we need to init the db.
		 */
		if (role_chg) {
			if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
				goto errlock;
			/*
			 * Ignore errors, because if the file doesn't exist,
			 * this is perfectly OK.
			 */
			MUTEX_LOCK(env, rep->mtx_clientdb);
			(void)__db_remove(dbp, ip, NULL, REPDBNAME,
			    NULL, DB_FORCE);
			MUTEX_UNLOCK(env, rep->mtx_clientdb);
			/*
			 * Set pending_event after calls that can fail.
			 */
			pending_event = DB_EVENT_REP_CLIENT;
		}
		REP_SYSTEM_LOCK(env);
		F_CLR(rep, REP_F_READY_MSG);
		if (locked) {
			F_CLR(rep, REP_F_READY_API | REP_F_READY_OP);
			locked = 0;
		}
		REP_SYSTEM_UNLOCK(env);

		if ((role_chg || rep->master_id == DB_EID_INVALID) &&
		    F_ISSET(env, ENV_PRIVATE))
			/*
			 * If we think we're a new client, and we have a
			 * private env, set our gen number down to 0.
			 * Otherwise, we can restart and think
			 * we're ready to accept a new record (because our
			 * gen is okay), but really this client needs to
			 * sync with the master.
			 */
			rep->gen = 0;

		/*
		 * Announce ourselves and send out our data.
		 */
		if ((ret = __dbt_usercopy(env, dbt)) != 0)
			goto out;
		(void)__rep_send_message(env,
		    DB_EID_BROADCAST, REP_NEWCLIENT, NULL, dbt, 0, 0);
	}

	if (0) {
		/*
		 * We have separate labels for errors.  If we're returning an
		 * error before we've set REP_F_READY_MSG, we use 'err'.  If
		 * we are erroring while holding the region mutex, then we use
		 * 'errunlock' label.  If we error without holding the rep
		 * mutex we must use 'errlock'.
		 */
errlock:	REP_SYSTEM_LOCK(env);
errunlock:	F_CLR(rep, REP_F_READY_MSG);
		if (locked)
			F_CLR(rep, REP_F_READY_API | REP_F_READY_OP);
		if (interrupting)
			(void)__memp_set_config(
			    env->dbenv, DB_MEMP_SYNC_INTERRUPT, 0);
		REP_SYSTEM_UNLOCK(env);
	}
out:
	if (ret == 0) {
		REP_SYSTEM_LOCK(env);
		F_SET(rep, REP_F_START_CALLED);
		REP_SYSTEM_UNLOCK(env);
	}
	if (start_th) {
		REP_SYSTEM_LOCK(env);
		F_CLR(rep, REP_F_INREPSTART);
		REP_SYSTEM_UNLOCK(env);
	}
	if (pending_event != DB_EVENT_NO_SUCH_EVENT)
		__rep_fire_event(env, pending_event, NULL);
	__dbt_userfree(env, dbt, NULL, NULL);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __rep_client_dbinit --
 *
 * Initialize the LSN database on the client side.  This is called from the
 * client initialization code.  The startup flag value indicates if
 * this is the first thread/process starting up and therefore should create
 * the LSN database.  This routine must be called once by each process acting
 * as a client.
 *
 * Assumes caller holds appropriate mutex.
 *
 * PUBLIC: int __rep_client_dbinit __P((ENV *, int, repdb_t));
 */
int
__rep_client_dbinit(env, startup, which)
	ENV *env;
	int startup;
	repdb_t which;
{
	DB *dbp, **rdbpp;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	REP *rep;
	int ret, t_ret;
	u_int32_t flags;
	const char *fname, *name, *subdb;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	dbp = NULL;

	if (which == REP_DB) {
		name = REPDBNAME;
		rdbpp = &db_rep->rep_db;
	} else {
		name = REPPAGENAME;
		rdbpp = &rep->file_dbp;
	}
	/* Check if this has already been called on this environment. */
	if (*rdbpp != NULL)
		return (0);

	ENV_GET_THREAD_INFO(env, ip);

	/* Set up arguments for __db_remove and __db_open calls. */
	fname = name;
	subdb = NULL;
	if (FLD_ISSET(rep->config, REP_C_INMEM)) {
		fname = NULL;
		subdb = name;
	}

	if (startup) {
		if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
			goto err;
		/*
		 * Prevent in-memory database remove from writing to
		 * non-existent logs.
		 */
		if (FLD_ISSET(rep->config, REP_C_INMEM))
			(void)__db_set_flags(dbp, DB_TXN_NOT_DURABLE);
		/*
		 * Ignore errors, because if the file doesn't exist, this
		 * is perfectly OK.
		 */
		(void)__db_remove(dbp, ip, NULL, fname, subdb, DB_FORCE);
	}

	if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
		goto err;
	if (which == REP_DB &&
	    (ret = __bam_set_bt_compare(dbp, __rep_bt_cmp)) != 0)
		goto err;

	/* Don't write log records on the client. */
	if ((ret = __db_set_flags(dbp, DB_TXN_NOT_DURABLE)) != 0)
		goto err;

	flags = DB_NO_AUTO_COMMIT | DB_CREATE |
	    (F_ISSET(env, ENV_THREAD) ? DB_THREAD : 0);

	if ((ret = __db_open(dbp, ip, NULL, fname, subdb,
	    (which == REP_DB ? DB_BTREE : DB_RECNO),
	    flags, 0, PGNO_BASE_MD)) != 0)
		goto err;

	*rdbpp = dbp;

	if (0) {
err:		if (dbp != NULL &&
		    (t_ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
			ret = t_ret;
		*rdbpp = NULL;
	}

	return (ret);
}

/*
 * __rep_bt_cmp --
 *
 * Comparison function for the LSN table.  We use the entire control
 * structure as a key (for simplicity, so we don't have to merge the
 * other fields in the control with the data field), but really only
 * care about the LSNs.
 */
static int
__rep_bt_cmp(dbp, dbt1, dbt2)
	DB *dbp;
	const DBT *dbt1, *dbt2;
{
	DB_LSN lsn1, lsn2;
	__rep_control_args *rp1, *rp2;

	COMPQUIET(dbp, NULL);

	rp1 = dbt1->data;
	rp2 = dbt2->data;

	(void)__ua_memcpy(&lsn1, &rp1->lsn, sizeof(DB_LSN));
	(void)__ua_memcpy(&lsn2, &rp2->lsn, sizeof(DB_LSN));

	if (lsn1.file > lsn2.file)
		return (1);

	if (lsn1.file < lsn2.file)
		return (-1);

	if (lsn1.offset > lsn2.offset)
		return (1);

	if (lsn1.offset < lsn2.offset)
		return (-1);

	return (0);
}

/*
 * __rep_abort_prepared --
 *	Abort any prepared transactions that recovery restored.
 *
 *	This is used by clients that have just run recovery, since
 * they cannot/should not call txn_recover and handle prepared transactions
 * themselves.
 */
static int
__rep_abort_prepared(env)
	ENV *env;
{
#define	PREPLISTSIZE	50
	DB_LOG *dblp;
	DB_PREPLIST prep[PREPLISTSIZE], *p;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	LOG *lp;
	int ret;
	u_int32_t count, i;
	u_int32_t op;

	mgr = env->tx_handle;
	region = mgr->reginfo.primary;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	if (region->stat.st_nrestores == 0)
		return (0);

	op = DB_FIRST;
	do {
		if ((ret = __txn_recover(env,
		    prep, PREPLISTSIZE, &count, op)) != 0)
			return (ret);
		for (i = 0; i < count; i++) {
			p = &prep[i];
			if ((ret = __txn_abort(p->txn)) != 0)
				return (ret);
			env->rep_handle->region->op_cnt--;
			env->rep_handle->region->max_prep_lsn = lp->lsn;
			region->stat.st_nrestores--;
		}
		op = DB_NEXT;
	} while (count == PREPLISTSIZE);

	return (0);
}

/*
 * __rep_restore_prepared --
 *	Restore to a prepared state any prepared but not yet committed
 * transactions.
 *
 *	This performs, in effect, a "mini-recovery";  it is called from
 * __rep_start by newly upgraded masters.  There may be transactions that an
 * old master prepared but did not resolve, which we need to restore to an
 * active state.
 */
static int
__rep_restore_prepared(env)
	ENV *env;
{
	DBT rec;
	DB_LOGC *logc;
	DB_LSN ckp_lsn, lsn;
	DB_REP *db_rep;
	DB_TXNHEAD *txninfo;
	REP *rep;
	__txn_ckp_args *ckp_args;
	__txn_regop_args *regop_args;
	__txn_prepare_args *prep_args;
	int ret, t_ret;
	u_int32_t hi_txn, low_txn, rectype, status, txnid, txnop;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	if (IS_ZERO_LSN(rep->max_prep_lsn)) {
		RPRINT(env, DB_VERB_REP_MISC,
		    (env, "restore_prep: No prepares. Skip."));
		return (0);
	}
	txninfo = NULL;
	ckp_args = NULL;
	prep_args = NULL;
	regop_args = NULL;
	ZERO_LSN(ckp_lsn);
	ZERO_LSN(lsn);

	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);

	/*
	 * Get our first LSN to see if the prepared LSN is still
	 * available.  If so, it might be unresolved.  If not,
	 * then it is guaranteed to be resolved.
	 */
	memset(&rec, 0, sizeof(DBT));
	if ((ret = __logc_get(logc, &lsn, &rec, DB_FIRST)) != 0)  {
		__db_errx(env, "First record not found");
		goto err;
	}
	/*
	 * If the max_prep_lsn is no longer available, we're sure
	 * that txn has been resolved.  We're done.
	 */
	if (rep->max_prep_lsn.file < lsn.file) {
		RPRINT(env, DB_VERB_REP_MISC,
		    (env, "restore_prep: Prepare resolved. Skip"));
		ZERO_LSN(rep->max_prep_lsn);
		goto done;
	}
	/*
	 * We need to consider the set of records between the most recent
	 * checkpoint LSN and the end of the log;  any txn in that
	 * range, and only txns in that range, could still have been
	 * active, and thus prepared but not yet committed (PBNYC),
	 * when the old master died.
	 *
	 * Find the most recent checkpoint LSN, and get the record there.
	 * If there is no checkpoint in the log, start off by getting
	 * the very first record in the log instead.
	 */
	if ((ret = __txn_getckp(env, &lsn)) == 0) {
		if ((ret = __logc_get(logc, &lsn, &rec, DB_SET)) != 0)  {
			__db_errx(env,
			    "Checkpoint record at LSN [%lu][%lu] not found",
			    (u_long)lsn.file, (u_long)lsn.offset);
			goto err;
		}

		if ((ret = __txn_ckp_read(
		    env, rec.data, &ckp_args)) == 0) {
			ckp_lsn = ckp_args->ckp_lsn;
			__os_free(env, ckp_args);
		}
		if (ret != 0) {
			__db_errx(env,
			    "Invalid checkpoint record at [%lu][%lu]",
			    (u_long)lsn.file, (u_long)lsn.offset);
			goto err;
		}

		if ((ret = __logc_get(logc, &ckp_lsn, &rec, DB_SET)) != 0) {
			__db_errx(env,
			    "Checkpoint LSN record [%lu][%lu] not found",
			    (u_long)ckp_lsn.file, (u_long)ckp_lsn.offset);
			goto err;
		}
	} else if ((ret = __logc_get(logc, &lsn, &rec, DB_FIRST)) != 0) {
		if (ret == DB_NOTFOUND) {
			/* An empty log means no PBNYC txns. */
			ret = 0;
			goto done;
		}
		__db_errx(env, "Attempt to get first log record failed");
		goto err;
	}

	/*
	 * We use the same txnlist infrastructure that recovery does;
	 * it demands an estimate of the high and low txnids for
	 * initialization.
	 *
	 * First, the low txnid.
	 */
	do {
		/* txnid is after rectype, which is a u_int32. */
		LOGCOPY_32(env, &low_txn,
		    (u_int8_t *)rec.data + sizeof(u_int32_t));
		if (low_txn != 0)
			break;
	} while ((ret = __logc_get(logc, &lsn, &rec, DB_NEXT)) == 0);

	/* If there are no txns, there are no PBNYC txns. */
	if (ret == DB_NOTFOUND) {
		ret = 0;
		goto done;
	} else if (ret != 0)
		goto err;

	/* Now, the high txnid. */
	if ((ret = __logc_get(logc, &lsn, &rec, DB_LAST)) != 0) {
		/*
		 * Note that DB_NOTFOUND is unacceptable here because we
		 * had to have looked at some log record to get this far.
		 */
		__db_errx(env, "Final log record not found");
		goto err;
	}
	do {
		/* txnid is after rectype, which is a u_int32. */
		LOGCOPY_32(env, &hi_txn,
		    (u_int8_t *)rec.data + sizeof(u_int32_t));
		if (hi_txn != 0)
			break;
	} while ((ret = __logc_get(logc, &lsn, &rec, DB_PREV)) == 0);
	if (ret == DB_NOTFOUND) {
		ret = 0;
		goto done;
	} else if (ret != 0)
		goto err;

	/* We have a high and low txnid.  Initialise the txn list. */
	if ((ret = __db_txnlist_init(env,
	    NULL, low_txn, hi_txn, NULL, &txninfo)) != 0)
		goto err;

	/*
	 * Now, walk backward from the end of the log to ckp_lsn.  Any
	 * prepares that we hit without first hitting a commit or
	 * abort belong to PBNYC txns, and we need to apply them and
	 * restore them to a prepared state.
	 *
	 * Note that we wind up applying transactions out of order.
	 * Since all PBNYC txns still held locks on the old master and
	 * were isolated, this should be safe.
	 */
	F_SET(env->lg_handle, DBLOG_RECOVER);
	for (ret = __logc_get(logc, &lsn, &rec, DB_LAST);
	    ret == 0 && LOG_COMPARE(&lsn, &ckp_lsn) > 0;
	    ret = __logc_get(logc, &lsn, &rec, DB_PREV)) {
		LOGCOPY_32(env, &rectype, rec.data);
		switch (rectype) {
		case DB___txn_regop:
			/*
			 * It's a commit or abort--but we don't care
			 * which!  Just add it to the list of txns
			 * that are resolved.
			 */
			if ((ret = __txn_regop_read(
			    env, rec.data, &regop_args)) != 0)
				goto err;
			txnid = regop_args->txnp->txnid;
			txnop = regop_args->opcode;
			__os_free(env, regop_args);

			ret = __db_txnlist_find(env,
			    txninfo, txnid, &status);
			if (ret == DB_NOTFOUND)
				ret = __db_txnlist_add(env, txninfo,
				    txnid, txnop, &lsn);
			else if (ret != 0)
				goto err;
			break;
		case DB___txn_prepare:
			/*
			 * It's a prepare.  If its not aborted and
			 * we haven't put the txn on our list yet, it
			 * hasn't been resolved, so apply and restore it.
			 */
			if ((ret = __txn_prepare_read(
			    env, rec.data, &prep_args)) != 0)
				goto err;
			ret = __db_txnlist_find(env, txninfo,
			    prep_args->txnp->txnid, &status);
			if (ret == DB_NOTFOUND) {
				if (prep_args->opcode == TXN_ABORT)
					ret = __db_txnlist_add(env, txninfo,
					    prep_args->txnp->txnid,
					    prep_args->opcode, &lsn);
				else if ((ret =
				    __rep_process_txn(env, &rec)) == 0) {
					/*
					 * We are guaranteed to be single
					 * threaded here.  We need to
					 * account for this newly
					 * instantiated txn in the op_cnt
					 * so that it is counted when it is
					 * resolved.
					 */
					rep->op_cnt++;
					ret = __txn_restore_txn(env,
					    &lsn, prep_args);
				}
			} else if (ret != 0)
				goto err;
			__os_free(env, prep_args);
			break;
		default:
			continue;
		}
	}

	/* It's not an error to have hit the beginning of the log. */
	if (ret == DB_NOTFOUND)
		ret = 0;

done:
err:	t_ret = __logc_close(logc);
	F_CLR(env->lg_handle, DBLOG_RECOVER);

	if (txninfo != NULL)
		__db_txnlist_end(env, txninfo);

	return (ret == 0 ? t_ret : ret);
}

/*
 * __rep_get_limit --
 *	Get the limit on the amount of data that will be sent during a single
 * invocation of __rep_process_message.
 *
 * PUBLIC: int __rep_get_limit __P((DB_ENV *, u_int32_t *, u_int32_t *));
 */
int
__rep_get_limit(dbenv, gbytesp, bytesp)
	DB_ENV *dbenv;
	u_int32_t *gbytesp, *bytesp;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_limit", DB_INIT_REP);

	if (REP_ON(env)) {
		rep = db_rep->region;
		ENV_ENTER(env, ip);
		REP_SYSTEM_LOCK(env);
		if (gbytesp != NULL)
			*gbytesp = rep->gbytes;
		if (bytesp != NULL)
			*bytesp = rep->bytes;
		REP_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		if (gbytesp != NULL)
			*gbytesp = db_rep->gbytes;
		if (bytesp != NULL)
			*bytesp = db_rep->bytes;
	}

	return (0);
}

/*
 * __rep_set_limit --
 *	Set a limit on the amount of data that will be sent during a single
 * invocation of __rep_process_message.
 *
 * PUBLIC: int __rep_set_limit __P((DB_ENV *, u_int32_t, u_int32_t));
 */
int
__rep_set_limit(dbenv, gbytes, bytes)
	DB_ENV *dbenv;
	u_int32_t gbytes, bytes;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_limit", DB_INIT_REP);

	if (bytes > GIGABYTE) {
		gbytes += bytes / GIGABYTE;
		bytes = bytes % GIGABYTE;
	}

	if (REP_ON(env)) {
		rep = db_rep->region;
		ENV_ENTER(env, ip);
		REP_SYSTEM_LOCK(env);
		rep->gbytes = gbytes;
		rep->bytes = bytes;
		REP_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		db_rep->gbytes = gbytes;
		db_rep->bytes = bytes;
	}

	return (0);
}

/*
 * PUBLIC: int __rep_set_nsites __P((DB_ENV *, u_int32_t));
 */
int
__rep_set_nsites(dbenv, n)
	DB_ENV *dbenv;
	u_int32_t n;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_nsites", DB_INIT_REP);

	if (IS_USING_LEASES(env) && IS_REP_STARTED(env)) {
		__db_errx(env,
	"DB_ENV->rep_set_nsites: must be called before DB_ENV->rep_start");
		return (EINVAL);
	}

	if (REP_ON(env)) {
		rep = db_rep->region;
		rep->config_nsites = n;
	} else
		db_rep->config_nsites = n;
	return (0);
}

/*
 * PUBLIC: int __rep_get_nsites __P((DB_ENV *, u_int32_t *));
 */
int
__rep_get_nsites(dbenv, n)
	DB_ENV *dbenv;
	u_int32_t *n;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_nsites", DB_INIT_REP);

	if (REP_ON(env)) {
		rep = db_rep->region;
		*n = rep->config_nsites;
	} else
		*n = db_rep->config_nsites;

	return (0);
}

/*
 * PUBLIC: int __rep_set_priority __P((DB_ENV *, u_int32_t));
 */
int
__rep_set_priority(dbenv, priority)
	DB_ENV *dbenv;
	u_int32_t priority;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_priority", DB_INIT_REP);

	if (REP_ON(env)) {
		rep = db_rep->region;
		rep->priority = priority;
	} else
		db_rep->my_priority = priority;
	return (0);
}

/*
 * PUBLIC: int __rep_get_priority __P((DB_ENV *, u_int32_t *));
 */
int
__rep_get_priority(dbenv, priority)
	DB_ENV *dbenv;
	u_int32_t *priority;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_priority", DB_INIT_REP);

	if (REP_ON(env)) {
		rep = db_rep->region;
		*priority = rep->priority;
	} else
		*priority = db_rep->my_priority;
	return (0);
}

/*
 * PUBLIC: int __rep_set_timeout __P((DB_ENV *, int, db_timeout_t));
 */
int
__rep_set_timeout(dbenv, which, timeout)
	DB_ENV *dbenv;
	int which;
	db_timeout_t timeout;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;
	int repmgr_timeout, ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	ret = 0;
	repmgr_timeout = 0;

	if (which == DB_REP_ACK_TIMEOUT || which == DB_REP_CONNECTION_RETRY ||
	    which == DB_REP_ELECTION_RETRY ||
	    which == DB_REP_HEARTBEAT_MONITOR ||
	    which == DB_REP_HEARTBEAT_SEND)
		repmgr_timeout = 1;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_timeout", DB_INIT_REP);

	if (APP_IS_BASEAPI(env) && repmgr_timeout) {
		__db_errx(env, "%s %s", "DB_ENV->rep_set_timeout:",
"cannot set Replication Manager timeout from base replication application");
		return (EINVAL);
	}
	if (which == DB_REP_LEASE_TIMEOUT && IS_REP_STARTED(env)) {
		ret = EINVAL;
		__db_errx(env, "%s %s", "DB_ENV->rep_set_timeout:",
"lease timeout must be set before DB_ENV->rep_start.");
		return (EINVAL);
	}

	switch (which) {
	case DB_REP_CHECKPOINT_DELAY:
		if (REP_ON(env))
			rep->chkpt_delay = timeout;
		else
			db_rep->chkpt_delay = timeout;
		break;
	case DB_REP_ELECTION_TIMEOUT:
		if (REP_ON(env))
			rep->elect_timeout = timeout;
		else
			db_rep->elect_timeout = timeout;
		break;
	case DB_REP_FULL_ELECTION_TIMEOUT:
		if (REP_ON(env))
			rep->full_elect_timeout = timeout;
		else
			db_rep->full_elect_timeout = timeout;
		break;
	case DB_REP_LEASE_TIMEOUT:
		if (REP_ON(env))
			rep->lease_timeout = timeout;
		else
			db_rep->lease_timeout = timeout;
		break;
#ifdef HAVE_REPLICATION_THREADS
	case DB_REP_ACK_TIMEOUT:
		db_rep->ack_timeout = timeout;
		break;
	case DB_REP_CONNECTION_RETRY:
		db_rep->connection_retry_wait = timeout;
		break;
	case DB_REP_ELECTION_RETRY:
		db_rep->election_retry_wait = timeout;
		break;
	case DB_REP_HEARTBEAT_MONITOR:
		db_rep->heartbeat_monitor_timeout = timeout;
		break;
	case DB_REP_HEARTBEAT_SEND:
		db_rep->heartbeat_frequency = timeout;
		break;
#endif
	default:
		__db_errx(env,
		    "Unknown timeout type argument to DB_ENV->rep_set_timeout");
		ret = EINVAL;
	}

	/* Setting a repmgr timeout makes this a repmgr application */
	if (ret == 0 && repmgr_timeout)
		APP_SET_REPMGR(env);
	return (ret);
}

/*
 * PUBLIC: int __rep_get_timeout __P((DB_ENV *, int, db_timeout_t *));
 */
int
__rep_get_timeout(dbenv, which, timeout)
	DB_ENV *dbenv;
	int which;
	db_timeout_t *timeout;
{
	DB_REP *db_rep;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;
	rep = db_rep->region;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_timeout", DB_INIT_REP);

	switch (which) {
	case DB_REP_CHECKPOINT_DELAY:
		*timeout = REP_ON(env) ?
		    rep->chkpt_delay : db_rep->chkpt_delay;
		break;
	case DB_REP_ELECTION_TIMEOUT:
		*timeout = REP_ON(env) ?
		    rep->elect_timeout : db_rep->elect_timeout;
		break;
	case DB_REP_FULL_ELECTION_TIMEOUT:
		*timeout = REP_ON(env) ?
		    rep->full_elect_timeout : db_rep->full_elect_timeout;
		break;
	case DB_REP_LEASE_TIMEOUT:
		*timeout = REP_ON(env) ?
		    rep->lease_timeout : db_rep->lease_timeout;
		break;
#ifdef HAVE_REPLICATION_THREADS
	case DB_REP_ACK_TIMEOUT:
		*timeout = db_rep->ack_timeout;
		break;
	case DB_REP_CONNECTION_RETRY:
		*timeout = db_rep->connection_retry_wait;
		break;
	case DB_REP_ELECTION_RETRY:
		*timeout = db_rep->election_retry_wait;
		break;
	case DB_REP_HEARTBEAT_MONITOR:
		*timeout = db_rep->heartbeat_monitor_timeout;
		break;
	case DB_REP_HEARTBEAT_SEND:
		*timeout = db_rep->heartbeat_frequency;
		break;
#endif
	default:
		__db_errx(env,
		    "unknown timeout type argument to DB_ENV->rep_get_timeout");
		return (EINVAL);
	}

	return (0);
}

/*
 * __rep_get_request --
 *	Get the minimum and maximum number of log records that we wait
 *	before retransmitting.
 *
 * PUBLIC: int __rep_get_request
 * PUBLIC:     __P((DB_ENV *, db_timeout_t *, db_timeout_t *));
 */
int
__rep_get_request(dbenv, minp, maxp)
	DB_ENV *dbenv;
	db_timeout_t *minp, *maxp;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_request", DB_INIT_REP);

	if (REP_ON(env)) {
		rep = db_rep->region;
		ENV_ENTER(env, ip);
		/*
		 * We acquire the mtx_region or mtx_clientdb mutexes as needed.
		 */
		REP_SYSTEM_LOCK(env);
		if (minp != NULL)
			DB_TIMESPEC_TO_TIMEOUT((*minp), &rep->request_gap, 0);
		if (maxp != NULL)
			DB_TIMESPEC_TO_TIMEOUT((*maxp), &rep->max_gap, 0);
		REP_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		if (minp != NULL)
			DB_TIMESPEC_TO_TIMEOUT((*minp),
			    &db_rep->request_gap, 0);
		if (maxp != NULL)
			DB_TIMESPEC_TO_TIMEOUT((*maxp), &db_rep->max_gap, 0);
	}

	return (0);
}

/*
 * __rep_set_request --
 *	Set the minimum and maximum number of log records that we wait
 *	before retransmitting.
 *
 * PUBLIC: int __rep_set_request __P((DB_ENV *, db_timeout_t, db_timeout_t));
 */
int
__rep_set_request(dbenv, min, max)
	DB_ENV *dbenv;
	db_timeout_t min, max;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	LOG *lp;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_request", DB_INIT_REP);

	if (min == 0 || max < min) {
		__db_errx(env,
		    "DB_ENV->rep_set_request: Invalid min or max values");
		return (EINVAL);
	}
	if (REP_ON(env)) {
		rep = db_rep->region;
		ENV_ENTER(env, ip);
		/*
		 * We acquire the mtx_region or mtx_clientdb mutexes as needed.
		 */
		REP_SYSTEM_LOCK(env);
		DB_TIMEOUT_TO_TIMESPEC(min, &rep->request_gap);
		DB_TIMEOUT_TO_TIMESPEC(max, &rep->max_gap);
		REP_SYSTEM_UNLOCK(env);

		MUTEX_LOCK(env, rep->mtx_clientdb);
		dblp = env->lg_handle;
		if (dblp != NULL && (lp = dblp->reginfo.primary) != NULL) {
			DB_TIMEOUT_TO_TIMESPEC(min, &lp->wait_ts);
		}
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		ENV_LEAVE(env, ip);
	} else {
		DB_TIMEOUT_TO_TIMESPEC(min, &db_rep->request_gap);
		DB_TIMEOUT_TO_TIMESPEC(max, &db_rep->max_gap);
	}

	return (0);
}

/*
 * __rep_set_transport_pp --
 *	Set the transport function for replication.
 *
 * PUBLIC: int __rep_set_transport_pp __P((DB_ENV *, int,
 * PUBLIC:     int (*)(DB_ENV *, const DBT *, const DBT *, const DB_LSN *,
 * PUBLIC:     int, u_int32_t)));
 */
int
__rep_set_transport_pp(dbenv, eid, f_send)
	DB_ENV *dbenv;
	int eid;
	int (*f_send) __P((DB_ENV *,
	    const DBT *, const DBT *, const DB_LSN *, int, u_int32_t));
{
	DB_REP *db_rep;
	ENV *env;
	int ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	ret = 0;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_transport", DB_INIT_REP);

	if (APP_IS_REPMGR(env)) {
		__db_errx(env,
"DB_ENV->rep_set_transport: cannot call from Replication Manager application");
		return (EINVAL);
	}

	if (f_send == NULL) {
		__db_errx(env,
		    "DB_ENV->rep_set_transport: no send function specified");
		return (EINVAL);
	}

	if (eid < 0) {
		__db_errx(env,
	"DB_ENV->rep_set_transport: eid must be greater than or equal to 0");
		return (EINVAL);
	}

	if ((ret = __rep_set_transport_int(env, eid, f_send)) == 0)
		/*
		 * Setting a non-repmgr send function makes this a base API
		 * application.
		 */
		APP_SET_BASEAPI(env);

	return (ret);
}

/*
 * __rep_set_transport_int --
 *	Set the internal values for the transport function for replication.
 *
 * PUBLIC: int __rep_set_transport_int __P((ENV *, int,
 * PUBLIC:     int (*)(DB_ENV *, const DBT *, const DBT *, const DB_LSN *,
 * PUBLIC:     int, u_int32_t)));
 */
int
__rep_set_transport_int(env, eid, f_send)
	ENV *env;
	int eid;
	int (*f_send) __P((DB_ENV *,
	    const DBT *, const DBT *, const DB_LSN *, int, u_int32_t));
{
	DB_REP *db_rep;
	REP *rep;

	db_rep = env->rep_handle;
	db_rep->send = f_send;
	if (REP_ON(env)) {
		rep = db_rep->region;
		rep->eid = eid;
	} else
		db_rep->eid = eid;
	return (0);
}

/*
 * PUBLIC: int __rep_get_clockskew __P((DB_ENV *, u_int32_t *, u_int32_t *));
 */
int
__rep_get_clockskew(dbenv, fast_clockp, slow_clockp)
	DB_ENV *dbenv;
	u_int32_t *fast_clockp, *slow_clockp;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REP *rep;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_get_clockskew", DB_INIT_REP);

	if (REP_ON(env)) {
		rep = db_rep->region;
		ENV_ENTER(env, ip);
		REP_SYSTEM_LOCK(env);
		*fast_clockp = rep->clock_skew;
		*slow_clockp = rep->clock_base;
		REP_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		*fast_clockp = db_rep->clock_skew;
		*slow_clockp = db_rep->clock_base;
	}

	return (0);
}

/*
 * PUBLIC: int __rep_set_clockskew __P((DB_ENV *, u_int32_t, u_int32_t));
 */
int
__rep_set_clockskew(dbenv, fast_clock, slow_clock)
	DB_ENV *dbenv;
	u_int32_t fast_clock, slow_clock;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REP *rep;
	int ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	ret = 0;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->rep_set_clockskew", DB_INIT_REP);

	/*
	 * Check for valid values.  The fast clock should be a larger
	 * number than the slow clock.  We use the slow clock value as
	 * our base for adjustment - therefore, a 2% difference should
	 * be fast == 102, slow == 100.  Check for values being 0.  If
	 * they are, then set them both to 1 internally.
	 *
	 * We will use these numbers to compute the larger ratio to be
	 * most conservative about the user's intention.
	 */
	if (fast_clock == 0 || slow_clock == 0) {
		/*
		 * If one value is zero, reject if both aren't zero.
		 */
		if (slow_clock != 0 || fast_clock != 0) {
			__db_errx(env,
"DB_ENV->rep_set_clockskew: Zero only valid for when used for both arguments");
			return (EINVAL);
		}
		fast_clock = 1;
		slow_clock = 1;
	}
	if (fast_clock < slow_clock) {
		__db_errx(env,
"DB_ENV->rep_set_clockskew: slow_clock value is larger than fast_clock_value");
		return (EINVAL);
	}
	if (REP_ON(env)) {
		rep = db_rep->region;
		if (IS_REP_STARTED(env)) {
			__db_errx(env,
	"DB_ENV->rep_set_clockskew: must be called before DB_ENV->rep_start");
			return (EINVAL);
		}
		ENV_ENTER(env, ip);
		REP_SYSTEM_LOCK(env);
		rep->clock_skew = fast_clock;
		rep->clock_base = slow_clock;
		REP_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		db_rep->clock_skew = fast_clock;
		db_rep->clock_base = slow_clock;
	}
	return (ret);
}

/*
 * __rep_flush --
 *	Re-push the last log record to all clients, in case they've lost
 *	messages and don't know it.
 *
 * PUBLIC: int __rep_flush __P((DB_ENV *));
 */
int
__rep_flush(dbenv)
	DB_ENV *dbenv;
{
	DBT rec;
	DB_LOGC *logc;
	DB_LSN lsn;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret, t_ret;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->rep_flush", DB_INIT_REP);

	/* We need a transport function because we send messages. */
	if (db_rep->send == NULL) {
		__db_errx(env,
    "DB_ENV->rep_flush: must be called after DB_ENV->rep_set_transport");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);

	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);

	memset(&rec, 0, sizeof(rec));
	memset(&lsn, 0, sizeof(lsn));

	if ((ret = __logc_get(logc, &lsn, &rec, DB_LAST)) != 0)
		goto err;

	(void)__rep_send_message(env,
	    DB_EID_BROADCAST, REP_LOG, &lsn, &rec, 0, 0);

err:	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __rep_sync --
 *	Force a synchronization to occur between this client and the master.
 *	This is the other half of configuring DELAYCLIENT.
 *
 * PUBLIC: int __rep_sync __P((DB_ENV *, u_int32_t));
 */
int
__rep_sync(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	LOG *lp;
	REP *rep;
	int master, ret;
	u_int32_t repflags, type;

	env = dbenv->env;
	db_rep = env->rep_handle;

	COMPQUIET(flags, 0);

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->rep_sync", DB_INIT_REP);

	/* We need a transport function because we send messages. */
	if (db_rep->send == NULL) {
		__db_errx(env,
    "DB_ENV->rep_sync: must be called after DB_ENV->rep_set_transport");
		return (EINVAL);
	}

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	rep = db_rep->region;
	ret = 0;

	ENV_ENTER(env, ip);

	/*
	 * Simple cases.  If we're not in the DELAY state we have nothing
	 * to do.  If we don't know who the master is, send a MASTER_REQ.
	 */
	MUTEX_LOCK(env, rep->mtx_clientdb);
	lsn = lp->verify_lsn;
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	REP_SYSTEM_LOCK(env);
	master = rep->master_id;
	if (master == DB_EID_INVALID) {
		REP_SYSTEM_UNLOCK(env);
		(void)__rep_send_message(env, DB_EID_BROADCAST,
		    REP_MASTER_REQ, NULL, NULL, 0, 0);
		goto out;
	}
	/*
	 * We want to hold the rep mutex to test and then clear the
	 * DELAY flag.  Racing threads in here could otherwise result
	 * in dual data streams.
	 */
	if (!F_ISSET(rep, REP_F_DELAY)) {
		REP_SYSTEM_UNLOCK(env);
		goto out;
	}

	DB_ASSERT(env,
	    !IS_USING_LEASES(env) || __rep_islease_granted(env) == 0);

	/*
	 * If we get here, we clear the delay flag and kick off a
	 * synchronization.  From this point forward, we will
	 * synchronize until the next time the master changes.
	 */
	F_CLR(rep, REP_F_DELAY);
	if (IS_ZERO_LSN(lsn) && FLD_ISSET(rep->config, REP_C_NOAUTOINIT)) {
		F_CLR(rep, REP_F_NOARCHIVE | REP_F_RECOVER_MASK);
		ret = DB_REP_JOIN_FAILURE;
		REP_SYSTEM_UNLOCK(env);
		goto out;
	}
	REP_SYSTEM_UNLOCK(env);
	/*
	 * When we set REP_F_DELAY, we set verify_lsn to the real verify lsn if
	 * we need to verify, or we zeroed it out if this is a client that needs
	 * internal init.  So, send the type of message now that
	 * __rep_new_master delayed sending.
	 */
	if (IS_ZERO_LSN(lsn)) {
		DB_ASSERT(env, F_ISSET(rep, REP_F_RECOVER_UPDATE));
		type = REP_UPDATE_REQ;
		repflags = 0;
	} else {
		DB_ASSERT(env, F_ISSET(rep, REP_F_RECOVER_VERIFY));
		type = REP_VERIFY_REQ;
		repflags = DB_REP_ANYWHERE;
	}
	(void)__rep_send_message(env, master, type, &lsn, NULL, 0, repflags);

out:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __rep_conv_vers --
 *	Convert from a log version to the replication message version
 *	that release used.
 */
static u_int32_t
__rep_conv_vers(env, log_ver)
	ENV *env;
	u_int32_t log_ver;
{
	COMPQUIET(env, NULL);

	/*
	 * We can't use a switch statement, some of the DB_LOGVERSION_XX
	 * constants are the same
	 */
	if (log_ver == DB_LOGVERSION)
		return (DB_REPVERSION);
	if (log_ver == DB_LOGVERSION_44)
		return (DB_REPVERSION_44);
	if (log_ver == DB_LOGVERSION_45)
		return (DB_REPVERSION_45);
	if (log_ver == DB_LOGVERSION_46)
		return (DB_REPVERSION_46);
	if (log_ver == DB_LOGVERSION_47)
		return (DB_REPVERSION_47);
	return (DB_REPVERSION_INVALID);
}
