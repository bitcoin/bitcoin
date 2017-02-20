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
#include "dbinc/db_am.h"
#include "dbinc/log.h"

#ifdef HAVE_STATISTICS
static int __rep_print_all __P((ENV *, u_int32_t));
static int __rep_print_stats __P((ENV *, u_int32_t));
static int __rep_stat __P((ENV *, DB_REP_STAT **, u_int32_t));

/*
 * __rep_stat_pp --
 *	ENV->rep_stat pre/post processing.
 *
 * PUBLIC: int __rep_stat_pp __P((DB_ENV *, DB_REP_STAT **, u_int32_t));
 */
int
__rep_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_REP_STAT **statp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->rep_stat", DB_INIT_REP);

	if ((ret = __db_fchk(env,
	    "DB_ENV->rep_stat", flags, DB_STAT_CLEAR)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	ret = __rep_stat(env, statp, flags);
	ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __rep_stat --
 *	ENV->rep_stat.
 */
static int
__rep_stat(env, statp, flags)
	ENV *env;
	DB_REP_STAT **statp;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	DB_REP_STAT *stats;
	LOG *lp;
	REP *rep;
	u_int32_t startupdone;
	uintmax_t queued;
	int dolock, ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	*statp = NULL;

	/* Allocate a stat struct to return to the user. */
	if ((ret = __os_umalloc(env, sizeof(DB_REP_STAT), &stats)) != 0)
		return (ret);

	/*
	 * Read without holding the lock.  If we are in client recovery, we
	 * copy just the stats struct so we won't block.  We only copy out
	 * those stats that don't require acquiring any mutex.
	 */
	dolock = FLD_ISSET(rep->flags, REP_F_RECOVER_MASK) ? 0 : 1;
	memcpy(stats, &rep->stat, sizeof(*stats));

	/* Copy out election stats. */
	if (F_ISSET(rep, REP_F_EPHASE1))
		stats->st_election_status = 1;
	else if (F_ISSET(rep, REP_F_EPHASE2))
		stats->st_election_status = 2;

	stats->st_election_nsites = rep->sites;
	stats->st_election_cur_winner = rep->winner;
	stats->st_election_priority = rep->w_priority;
	stats->st_election_gen = rep->w_gen;
	stats->st_election_lsn = rep->w_lsn;
	stats->st_election_votes = rep->votes;
	stats->st_election_nvotes = rep->nvotes;
	stats->st_election_tiebreaker = rep->w_tiebreaker;

	/* Copy out other info that's protected by the rep mutex. */
	stats->st_env_id = rep->eid;
	stats->st_env_priority = rep->priority;
	stats->st_nsites = rep->nsites;
	stats->st_master = rep->master_id;
	stats->st_gen = rep->gen;
	stats->st_egen = rep->egen;

	if (F_ISSET(rep, REP_F_MASTER))
		stats->st_status = DB_REP_MASTER;
	else if (F_ISSET(rep, REP_F_CLIENT))
		stats->st_status = DB_REP_CLIENT;
	else
		stats->st_status = 0;

	if (LF_ISSET(DB_STAT_CLEAR)) {
		queued = rep->stat.st_log_queued;
		startupdone = rep->stat.st_startup_complete;
		memset(&rep->stat, 0, sizeof(rep->stat));
		rep->stat.st_log_queued = rep->stat.st_log_queued_total =
		    rep->stat.st_log_queued_max = queued;
		rep->stat.st_startup_complete = startupdone;
	}

	/*
	 * Log-related replication info is stored in the log system and
	 * protected by the log region lock.
	 */
	if (dolock)
		MUTEX_LOCK(env, rep->mtx_clientdb);
	if (F_ISSET(rep, REP_F_CLIENT)) {
		stats->st_next_lsn = lp->ready_lsn;
		stats->st_waiting_lsn = lp->waiting_lsn;
		stats->st_next_pg = rep->ready_pg;
		stats->st_waiting_pg = rep->waiting_pg;
		stats->st_max_lease_sec = (u_int32_t)lp->max_lease_ts.tv_sec;
		stats->st_max_lease_usec = (u_int32_t)
		    (lp->max_lease_ts.tv_nsec / NS_PER_US);
	} else {
		if (F_ISSET(rep, REP_F_MASTER)) {
			LOG_SYSTEM_LOCK(env);
			stats->st_next_lsn = lp->lsn;
			LOG_SYSTEM_UNLOCK(env);
		} else
			ZERO_LSN(stats->st_next_lsn);
		ZERO_LSN(stats->st_waiting_lsn);
		stats->st_max_lease_sec = 0;
		stats->st_max_lease_usec = 0;
	}
	stats->st_max_perm_lsn = lp->max_perm_lsn;
	if (dolock)
		MUTEX_UNLOCK(env, rep->mtx_clientdb);

	*statp = stats;
	return (0);
}

/*
 * __rep_stat_print_pp --
 *	ENV->rep_stat_print pre/post processing.
 *
 * PUBLIC: int __rep_stat_print_pp __P((DB_ENV *, u_int32_t));
 */
int
__rep_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->rep_stat_print", DB_INIT_REP);

	if ((ret = __db_fchk(env, "DB_ENV->rep_stat_print",
	    flags, DB_STAT_ALL | DB_STAT_CLEAR)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	ret = __rep_stat_print(env, flags);
	ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __rep_stat_print --
 *	ENV->rep_stat_print method.
 *
 * PUBLIC: int __rep_stat_print __P((ENV *, u_int32_t));
 */
int
__rep_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	u_int32_t orig_flags;
	int ret;

	orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR | DB_STAT_SUBSYSTEM);
	if (flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __rep_print_stats(env, orig_flags);
		if (flags == 0 || ret != 0)
			return (ret);
	}

	if (LF_ISSET(DB_STAT_ALL) &&
	    (ret = __rep_print_all(env, orig_flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __rep_print_stats --
 *	Print out default statistics.
 */
static int
__rep_print_stats(env, flags)
	ENV *env;
	u_int32_t flags;
{
	DB_REP_STAT *sp;
	int is_client, ret;
	char *p;

	if ((ret = __rep_stat(env, &sp, flags)) != 0)
		return (ret);

	if (LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default replication region information:");
	is_client = 0;
	switch (sp->st_status) {
	case DB_REP_MASTER:
		__db_msg(env,
		    "Environment configured as a replication master");
		break;
	case DB_REP_CLIENT:
		__db_msg(env,
		    "Environment configured as a replication client");
		is_client = 1;
		break;
	default:
		__db_msg(env,
		    "Environment not configured for replication");
		break;
	}

	__db_msg(env, "%lu/%lu\t%s",
	    (u_long)sp->st_next_lsn.file, (u_long)sp->st_next_lsn.offset,
	    is_client ? "Next LSN expected" : "Next LSN to be used");
	__db_msg(env, "%lu/%lu\t%s",
	    (u_long)sp->st_waiting_lsn.file, (u_long)sp->st_waiting_lsn.offset,
	    sp->st_waiting_lsn.file == 0 ?
	    "Not waiting for any missed log records" :
	    "LSN of first log record we have after missed log records");
	__db_msg(env, "%lu/%lu\t%s",
	    (u_long)sp->st_max_perm_lsn.file,
	    (u_long)sp->st_max_perm_lsn.offset,
	    sp->st_max_perm_lsn.file == 0 ?
	    "No maximum permanent LSN" :
	    "Maximum permanent LSN");

	__db_dl(env, "Next page number expected", (u_long)sp->st_next_pg);
	p = sp->st_waiting_pg == PGNO_INVALID ?
	    "Not waiting for any missed pages" :
	    "Page number of first page we have after missed pages";
	__db_msg(env, "%lu\t%s", (u_long)sp->st_waiting_pg, p);
	__db_dl(env,
     "Number of duplicate master conditions originally detected at this site",
	    (u_long)sp->st_dupmasters);
	if (sp->st_env_id != DB_EID_INVALID)
		__db_dl(env, "Current environment ID", (u_long)sp->st_env_id);
	else
		__db_msg(env, "No current environment ID");
	__db_dl(env,
	    "Current environment priority", (u_long)sp->st_env_priority);
	__db_dl(env, "Current generation number", (u_long)sp->st_gen);
	__db_dl(env,
	    "Election generation number for the current or next election",
	    (u_long)sp->st_egen);
	__db_dl(env, "Number of duplicate log records received",
	    (u_long)sp->st_log_duplicated);
	__db_dl(env, "Number of log records currently queued",
	    (u_long)sp->st_log_queued);
	__db_dl(env, "Maximum number of log records ever queued at once",
	    (u_long)sp->st_log_queued_max);
	__db_dl(env, "Total number of log records queued",
	    (u_long)sp->st_log_queued_total);
	__db_dl(env,
	    "Number of log records received and appended to the log",
	    (u_long)sp->st_log_records);
	__db_dl(env, "Number of log records missed and requested",
	    (u_long)sp->st_log_requested);
	if (sp->st_master != DB_EID_INVALID)
		__db_dl(env, "Current master ID", (u_long)sp->st_master);
	else
		__db_msg(env, "No current master ID");
	__db_dl(env, "Number of times the master has changed",
	    (u_long)sp->st_master_changes);
	__db_dl(env,
	    "Number of messages received with a bad generation number",
	    (u_long)sp->st_msgs_badgen);
	__db_dl(env, "Number of messages received and processed",
	    (u_long)sp->st_msgs_processed);
	__db_dl(env, "Number of messages ignored due to pending recovery",
	    (u_long)sp->st_msgs_recover);
	__db_dl(env, "Number of failed message sends",
	    (u_long)sp->st_msgs_send_failures);
	__db_dl(env, "Number of messages sent", (u_long)sp->st_msgs_sent);
	__db_dl(env,
	    "Number of new site messages received", (u_long)sp->st_newsites);
	__db_dl(env,
	    "Number of environments believed to be in the replication group",
	    (u_long)sp->st_nsites);
	__db_dl(env, "Transmission limited", (u_long)sp->st_nthrottles);
	__db_dl(env, "Number of outdated conditions detected",
	    (u_long)sp->st_outdated);
	__db_dl(env, "Number of duplicate page records received",
	    (u_long)sp->st_pg_duplicated);
	__db_dl(env, "Number of page records received and added to databases",
	    (u_long)sp->st_pg_records);
	__db_dl(env, "Number of page records missed and requested",
	    (u_long)sp->st_pg_requested);
	if (sp->st_startup_complete == 0)
		__db_msg(env, "Startup incomplete");
	else
		__db_msg(env, "Startup complete");
	__db_dl(env,
	    "Number of transactions applied", (u_long)sp->st_txns_applied);

	__db_dl(env, "Number of startsync messages delayed",
	    (u_long)sp->st_startsync_delayed);

	__db_dl(env, "Number of elections held", (u_long)sp->st_elections);
	__db_dl(env,
	    "Number of elections won", (u_long)sp->st_elections_won);

	if (sp->st_election_status == 0) {
		__db_msg(env, "No election in progress");
		if (sp->st_election_sec > 0 || sp->st_election_usec > 0)
			__db_msg(env,
			    "%lu.%.6lu\tDuration of last election (seconds)",
			    (u_long)sp->st_election_sec,
			    (u_long)sp->st_election_usec);
	} else {
		__db_dl(env, "Current election phase",
		    (u_long)sp->st_election_status);
		__db_dl(env,
    "Environment ID of the winner of the current or last election",
		    (u_long)sp->st_election_cur_winner);
		__db_dl(env,
    "Master generation number of the winner of the current or last election",
		    (u_long)sp->st_election_gen);
		__db_msg(env,
    "%lu/%lu\tMaximum LSN of the winner of the current or last election",
		    (u_long)sp->st_election_lsn.file,
		    (u_long)sp->st_election_lsn.offset);
		__db_dl(env,
    "Number of sites responding to this site during the current election",
		    (u_long)sp->st_election_nsites);
		__db_dl(env,
    "Number of votes required in the current or last election",
		    (u_long)sp->st_election_nvotes);
		__db_dl(env,
		    "Priority of the winner of the current or last election",
		    (u_long)sp->st_election_priority);
		__db_dl(env,
    "Tiebreaker value of the winner of the current or last election",
		    (u_long)sp->st_election_tiebreaker);
		__db_dl(env,
		    "Number of votes received during the current election",
		    (u_long)sp->st_election_votes);
	}
	__db_dl(env, "Number of bulk buffer sends triggered by full buffer",
	    (u_long)sp->st_bulk_fills);
	__db_dl(env, "Number of single records exceeding bulk buffer size",
	    (u_long)sp->st_bulk_overflows);
	__db_dl(env, "Number of records added to a bulk buffer",
	    (u_long)sp->st_bulk_records);
	__db_dl(env, "Number of bulk buffers sent",
	    (u_long)sp->st_bulk_transfers);
	__db_dl(env, "Number of re-request messages received",
	    (u_long)sp->st_client_rerequests);
	__db_dl(env,
	    "Number of request messages this client failed to process",
	    (u_long)sp->st_client_svc_miss);
	__db_dl(env, "Number of request messages received by this client",
	    (u_long)sp->st_client_svc_req);
	if (sp->st_max_lease_sec > 0 || sp->st_max_lease_usec > 0)
		__db_msg(env,
		    "%lu.%.6lu\tDuration of maximum lease (seconds)",
		    (u_long)sp->st_max_lease_sec,
		    (u_long)sp->st_max_lease_usec);

	__os_ufree(env, sp);

	return (0);
}

/*
 * __rep_print_all --
 *	Display debugging replication region statistics.
 */
static int
__rep_print_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	static const FN rep_fn[] = {
		{ REP_F_ABBREVIATED,	"REP_F_ABBREVIATED" },
		{ REP_F_APP_BASEAPI,	"REP_F_APP_BASEAPI" },
		{ REP_F_APP_REPMGR,	"REP_F_APP_REPMGR" },
		{ REP_F_CLIENT,		"REP_F_CLIENT" },
		{ REP_F_DELAY,		"REP_F_DELAY" },
		{ REP_F_EGENUPDATE,	"REP_F_EGENUPDATE" },
		{ REP_F_EPHASE0,	"REP_F_EPHASE0" },
		{ REP_F_EPHASE1,	"REP_F_EPHASE1" },
		{ REP_F_EPHASE2,	"REP_F_EPHASE2" },
		{ REP_F_GROUP_ESTD,	"REP_F_GROUP_ESTD" },
		{ REP_F_INREPELECT,	"REP_F_INREPELECT" },
		{ REP_F_INREPSTART,	"REP_F_INREPSTART" },
		{ REP_F_LEASE_EXPIRED,	"REP_F_LEASE_EXPIRED" },
		{ REP_F_MASTER,		"REP_F_MASTER" },
		{ REP_F_MASTERELECT,	"REP_F_MASTERELECT" },
		{ REP_F_NEWFILE,	"REP_F_NEWFILE" },
		{ REP_F_NIMDBS_LOADED,	"REP_F_NIMDBS_LOADED" },
		{ REP_F_NOARCHIVE,	"REP_F_NOARCHIVE" },
		{ REP_F_READY_API,	"REP_F_READY_API" },
		{ REP_F_READY_APPLY,	"REP_F_READY_APPLY" },
		{ REP_F_READY_MSG,	"REP_F_READY_MSG" },
		{ REP_F_READY_OP,	"REP_F_READY_OP" },
		{ REP_F_RECOVER_LOG,	"REP_F_RECOVER_LOG" },
		{ REP_F_RECOVER_PAGE,	"REP_F_RECOVER_PAGE" },
		{ REP_F_RECOVER_UPDATE,	"REP_F_RECOVER_UPDATE" },
		{ REP_F_RECOVER_VERIFY,	"REP_F_RECOVER_VERIFY" },
		{ REP_F_SKIPPED_APPLY,	"REP_F_SKIPPED_APPLY" },
		{ REP_F_START_CALLED,	"REP_F_START_CALLED" },
		{ REP_F_TALLY,		"REP_F_TALLY" },
		{ 0,			NULL }
	};
	static const FN dbrep_fn[] = {
		{ DBREP_APP_BASEAPI,	"DBREP_APP_BASEAPI" },
		{ DBREP_APP_REPMGR,	"DBREP_APP_REPMGR" },
		{ DBREP_OPENFILES,	"DBREP_OPENFILES" },
		{ 0,			NULL }
	};
	DB_LOG *dblp;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	LOG *lp;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	char time_buf[CTIME_BUFLEN];

	db_rep = env->rep_handle;
	rep = db_rep->region;
	infop = env->reginfo;
	renv = infop->primary;
	ENV_ENTER(env, ip);

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_REP handle information:");

	if (db_rep->rep_db == NULL)
		STAT_ISSET("Bookkeeping database", db_rep->rep_db);
	else
		(void)__db_stat_print(db_rep->rep_db, ip, flags);

	__db_prflags(env, NULL, db_rep->flags, dbrep_fn, NULL, "\tFlags");

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "REP handle information:");
	__mutex_print_debug_single(env,
	    "Replication region mutex", rep->mtx_region, flags);
	__mutex_print_debug_single(env,
	    "Bookkeeping database mutex", rep->mtx_clientdb, flags);

	STAT_LONG("Environment ID", rep->eid);
	STAT_LONG("Master environment ID", rep->master_id);
	STAT_ULONG("Election generation", rep->egen);
	STAT_ULONG("Election generation number", rep->gen);
	STAT_LONG("Space allocated for sites", rep->asites);
	STAT_LONG("Sites in group", rep->nsites);
	STAT_LONG("Votes needed for election", rep->nvotes);
	STAT_LONG("Priority in election", rep->priority);
	__db_dlbytes(env, "Limit on data sent in a single call",
	    rep->gbytes, (u_long)0, rep->bytes);
	STAT_LONG("Request gap seconds", rep->request_gap.tv_sec);
	STAT_LONG("Request gap microseconds",
	    rep->request_gap.tv_nsec / NS_PER_US);
	STAT_LONG("Maximum gap seconds", rep->max_gap.tv_sec);
	STAT_LONG("Maximum gap microseconds",
	    rep->max_gap.tv_nsec / NS_PER_US);

	STAT_ULONG("Callers in rep_proc_msg", rep->msg_th);
	STAT_ULONG("Library handle count", rep->handle_cnt);
	STAT_ULONG("Multi-step operation count", rep->op_cnt);
	__db_msg(env, "%.24s\tRecovery timestamp",
	    renv->rep_timestamp == 0 ?
	    "0" : __os_ctime(&renv->rep_timestamp, time_buf));

	STAT_LONG("Sites heard from", rep->sites);
	STAT_LONG("Current winner", rep->winner);
	STAT_LONG("Winner priority", rep->w_priority);
	STAT_ULONG("Winner generation", rep->w_gen);
	STAT_LSN("Winner LSN", &rep->w_lsn);
	STAT_LONG("Winner tiebreaker", rep->w_tiebreaker);
	STAT_LONG("Votes for this site", rep->votes);

	__db_prflags(env, NULL, rep->flags, rep_fn, NULL, "\tFlags");

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "LOG replication information:");
	MUTEX_LOCK(env, rep->mtx_clientdb);
	dblp = env->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;
	STAT_LSN("First log record after a gap", &lp->waiting_lsn);
	STAT_LSN("Maximum permanent LSN processed", &lp->max_perm_lsn);
	STAT_LSN("LSN waiting to verify", &lp->verify_lsn);
	STAT_LSN("Maximum LSN requested", &lp->max_wait_lsn);
	STAT_LONG("Time to wait before requesting seconds", lp->wait_ts.tv_sec);
	STAT_LONG("Time to wait before requesting microseconds",
	    lp->wait_ts.tv_nsec / NS_PER_US);
	STAT_LSN("Next LSN expected", &lp->ready_lsn);
	STAT_LONG("Maximum lease timestamp seconds", lp->max_lease_ts.tv_sec);
	STAT_LONG("Maximum lease timestamp microseconds",
	    lp->max_lease_ts.tv_nsec / NS_PER_US);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	ENV_LEAVE(env, ip);

	return (0);
}

#else /* !HAVE_STATISTICS */

int
__rep_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_REP_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}

int
__rep_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}
#endif
