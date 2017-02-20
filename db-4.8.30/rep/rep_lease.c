/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2007-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"

static void __rep_find_entry __P((ENV *, REP *, int, REP_LEASE_ENTRY **));

/*
 * __rep_update_grant -
 *      Update a client's lease grant for this perm record
 *	and send the grant to the master.  Caller must
 *	hold the mtx_clientdb mutex.  Timespec given is in
 *	host local format.
 *
 * PUBLIC: int __rep_update_grant __P((ENV *, db_timespec *));
 */
int
__rep_update_grant(env, ts)
	ENV *env;
	db_timespec *ts;
{
	DBT lease_dbt;
	DB_LOG *dblp;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	__rep_grant_info_args gi;
	db_timespec mytime;
	u_int8_t buf[__REP_GRANT_INFO_SIZE];
	int master, ret;
	size_t len;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	timespecclear(&mytime);

	/*
	 * Get current time, and add in the (skewed) lease duration
	 * time to send the grant to the master.
	 */
	__os_gettime(env, &mytime, 1);
	timespecadd(&mytime, &rep->lease_duration);
	REP_SYSTEM_LOCK(env);
	/*
	 * If we are in an election, we cannot grant the lease.
	 * We need to check under the region mutex.
	 */
	if (IN_ELECTION(rep)) {
		REP_SYSTEM_UNLOCK(env);
		return (0);
	}
	if (timespeccmp(&mytime, &rep->grant_expire, >))
		rep->grant_expire = mytime;
	F_CLR(rep, REP_F_LEASE_EXPIRED);
	REP_SYSTEM_UNLOCK(env);

	/*
	 * Send the LEASE_GRANT message with the current lease grant
	 * no matter if we've actually extended the lease or not.
	 */
	gi.msg_sec = (u_int32_t)ts->tv_sec;
	gi.msg_nsec = (u_int32_t)ts->tv_nsec;

	if ((ret = __rep_grant_info_marshal(env, &gi, buf,
	    __REP_GRANT_INFO_SIZE, &len)) != 0)
		return (ret);
	DB_INIT_DBT(lease_dbt, buf, len);
	if ((master = rep->master_id) != DB_EID_INVALID)
		(void)__rep_send_message(env, master, REP_LEASE_GRANT,
		    &lp->max_perm_lsn, &lease_dbt, 0, 0);
	return (0);
}

/*
 * __rep_islease_granted -
 *      Return 0 if this client has no outstanding lease granted.
 *	Return 1 otherwise.
 *	Caller must hold the REP_SYSTEM (region) mutex.
 *
 * PUBLIC: int __rep_islease_granted __P((ENV *));
 */
int
__rep_islease_granted(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	db_timespec mytime;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	/*
	 * Get current time and compare against our granted lease.
	 */
	timespecclear(&mytime);
	__os_gettime(env, &mytime, 1);

	return (timespeccmp(&mytime, &rep->grant_expire, <=) ? 1 : 0);
}

/*
 * __rep_lease_table_alloc -
 *	Allocate the lease table on a master.  Called with rep mutex
 * held.  We need to acquire the env region mutex, so we need to
 * make sure we never acquire those mutexes in the opposite order.
 *
 * PUBLIC: int __rep_lease_table_alloc __P((ENV *, u_int32_t));
 */
int
__rep_lease_table_alloc(env, nsites)
	ENV *env;
	u_int32_t nsites;
{
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	REP_LEASE_ENTRY *le, *table;
	int *lease, ret;
	u_int32_t i;

	rep = env->rep_handle->region;

	infop = env->reginfo;
	renv = infop->primary;
	MUTEX_LOCK(env, renv->mtx_regenv);
	/*
	 * If we have an old table from some other time, free it and
	 * allocate ourselves a new one that is known to be for
	 * the right number of sites.
	 */
	if (rep->lease_off != INVALID_ROFF) {
		__env_alloc_free(infop,
		    R_ADDR(infop, rep->lease_off));
		rep->lease_off = INVALID_ROFF;
	}
	ret = __env_alloc(infop, (size_t)nsites * sizeof(REP_LEASE_ENTRY),
	    &lease);
	MUTEX_UNLOCK(env, renv->mtx_regenv);
	if (ret != 0)
		return (ret);
	else
		rep->lease_off = R_OFFSET(infop, lease);
	table = R_ADDR(infop, rep->lease_off);
	for (i = 0; i < nsites; i++) {
		le = &table[i];
		le->eid = DB_EID_INVALID;
		timespecclear(&le->start_time);
		timespecclear(&le->end_time);
		ZERO_LSN(le->lease_lsn);
	}
	return (0);
}

/*
 * __rep_lease_grant -
 *	Handle incoming REP_LEASE_GRANT message on a master.
 *
 * PUBLIC: int __rep_lease_grant __P((ENV *, __rep_control_args *, DBT *, int));
 */
int
__rep_lease_grant(env, rp, rec, eid)
	ENV *env;
	__rep_control_args *rp;
	DBT *rec;
	int eid;
{
	DB_REP *db_rep;
	REP *rep;
	__rep_grant_info_args gi;
	REP_LEASE_ENTRY *le;
	db_timespec msg_time;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	if ((ret = __rep_grant_info_unmarshal(env,
	    &gi, rec->data, rec->size, NULL)) != 0)
		return (ret);
	timespecset(&msg_time, gi.msg_sec, gi.msg_nsec);
	le = NULL;

	/*
	 * Get current time, and add in the (skewed) lease duration
	 * time to send the grant to the master.
	 */
	REP_SYSTEM_LOCK(env);
	__rep_find_entry(env, rep, eid, &le);
	/*
	 * We either get back this site's entry, or an empty entry
	 * that we need to initialize.
	 */
	DB_ASSERT(env, le != NULL);
	/*
	 * Update the entry if it is an empty entry or if the new
	 * lease grant is a later start time than the current one.
	 */
	RPRINT(env, DB_VERB_REP_LEASE,
	    (env, "lease_grant: grant msg time %lu %lu",
	    (u_long)msg_time.tv_sec, (u_long)msg_time.tv_nsec));
	if (le->eid == DB_EID_INVALID ||
	    timespeccmp(&msg_time, &le->start_time, >)) {
		le->eid = eid;
		le->start_time = msg_time;
		le->end_time = le->start_time;
		timespecadd(&le->end_time, &rep->lease_duration);
		RPRINT(env, DB_VERB_REP_LEASE, (env,
    "lease_grant: eid %d, start %lu %lu, end %lu %lu, duration %lu %lu",
    le->eid, (u_long)le->start_time.tv_sec, (u_long)le->start_time.tv_nsec,
    (u_long)le->end_time.tv_sec, (u_long)le->end_time.tv_nsec,
    (u_long)rep->lease_duration.tv_sec, (u_long)rep->lease_duration.tv_nsec));
		/*
		 * XXX Is this really true?  Could we have a lagging
		 * record that has a later start time, but smaller
		 * LSN than we have previously seen??
		 */
		DB_ASSERT(env, LOG_COMPARE(&rp->lsn, &le->lease_lsn) >= 0);
		le->lease_lsn = rp->lsn;
	}
	REP_SYSTEM_UNLOCK(env);
	return (0);
}

/*
 * Find the entry for the given EID.  Or the first empty one.
 */
static void
__rep_find_entry(env, rep, eid, lep)
	ENV *env;
	REP *rep;
	int eid;
	REP_LEASE_ENTRY **lep;
{
	REGINFO *infop;
	REP_LEASE_ENTRY *le, *table;
	u_int32_t i;

	infop = env->reginfo;
	table = R_ADDR(infop, rep->lease_off);

	for (i = 0; i < rep->nsites; i++) {
		le = &table[i];
		/*
		 * Find either the one that matches the client's
		 * EID or the first empty one.
		 */
		if (le->eid == eid || le->eid == DB_EID_INVALID) {
			*lep = le;
			return;
		}
	}
	return;
}

/*
 * __rep_lease_check -
 *      Return 0 if this master holds valid leases and can confirm
 *	its mastership.  If leases are expired, an attempt is made
 *	to refresh the leases.  If that fails, then return the
 *	DB_REP_LEASE_EXPIRED error to the user.  No mutexes held.
 *
 * PUBLIC: int __rep_lease_check __P((ENV *, int));
 */
int
__rep_lease_check(env, refresh)
	ENV *env;
	int refresh;
{
	DB_LOG *dblp;
	DB_LSN lease_lsn;
	DB_REP *db_rep;
	LOG *lp;
	REGINFO *infop;
	REP *rep;
	REP_LEASE_ENTRY *le, *table;
	db_timespec curtime;
	int ret, tries;
	u_int32_t i, min_leases, valid_leases;

	infop = env->reginfo;
	tries = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	LOG_SYSTEM_LOCK(env);
	lease_lsn = lp->max_perm_lsn;
	LOG_SYSTEM_UNLOCK(env);

retry:
	REP_SYSTEM_LOCK(env);
	min_leases = rep->nsites / 2;
	ret = 0;
	__os_gettime(env, &curtime, 1);
	RPRINT(env, DB_VERB_REP_LEASE, (env,
	"lease_check: try %d min_leases %lu curtime %lu %lu, maxLSN [%lu][%lu]",
	    tries,
	    (u_long)min_leases, (u_long)curtime.tv_sec,
	    (u_long)curtime.tv_nsec,
	    (u_long)lease_lsn.file,
	    (u_long)lease_lsn.offset));
	table = R_ADDR(infop, rep->lease_off);
	for (i = 0, valid_leases = 0;
	    i < rep->nsites && valid_leases < min_leases; i++) {
		le = &table[i];
		/*
		 * Count this lease as valid if:
		 * - It is a valid entry (has an EID).
		 * - The lease has not expired.
		 * - The LSN is up to date.
		 */
		if (le->eid != DB_EID_INVALID) {
			RPRINT(env, DB_VERB_REP_LEASE, (env,
		    "lease_check: valid %lu eid %d, lease_lsn [%lu][%lu]",
			    (u_long)valid_leases, le->eid,
			    (u_long)le->lease_lsn.file,
			    (u_long)le->lease_lsn.offset));
			RPRINT(env, DB_VERB_REP_LEASE,
			    (env, "lease_check: endtime %lu %lu",
			    (u_long)le->end_time.tv_sec,
			    (u_long)le->end_time.tv_nsec));
		}
		if (le->eid != DB_EID_INVALID &&
		    timespeccmp(&le->end_time, &curtime, >=) &&
		    LOG_COMPARE(&le->lease_lsn, &lease_lsn) >= 0)
			valid_leases++;
	}
	REP_SYSTEM_UNLOCK(env);

	/*
	 * Now see if we have enough.
	 */
	RPRINT(env, DB_VERB_REP_LEASE, (env, "valid %lu, min %lu",
	    (u_long)valid_leases, (u_long)min_leases));
	if (valid_leases < min_leases) {
		if (!refresh)
			ret = DB_REP_LEASE_EXPIRED;
		else {
			/*
			 * If we are successful, we need to recheck the leases
			 * because the lease grant messages may have raced with
			 * the PERM acknowledgement.  Give the grant messages
			 * a chance to arrive and be processed.
			 */
			if ((ret = __rep_lease_refresh(env)) == 0) {
				if (tries <= LEASE_REFRESH_TRIES) {
					/*
					 * If we were successful sending, but
					 * not in racing the message threads,
					 * then yield the processor so that
					 * the message threads get a chance
					 * to run.
					 */
					if (tries > 0)
						__os_yield(env, 1, 0);
					tries++;
					goto retry;
				} else
					ret = DB_REP_LEASE_EXPIRED;
			}
		}
	}

	if (ret == DB_REP_LEASE_EXPIRED)
		RPRINT(env, DB_VERB_REP_LEASE, (env,
		    "lease_check: Expired.  Only %lu valid",
		    (u_long)valid_leases));
	return (ret);
}

/*
 * __rep_lease_refresh -
 *	Find the last permanent record and send that out so that it
 *	forces clients to grant their leases.
 *
 *	If there is no permanent record, this function cannot refresh
 *	leases.  That should not happen because the master should write
 *	a checkpoint when it starts, if there is no other perm record.
 *
 * PUBLIC: int __rep_lease_refresh __P((ENV *));
 */
int
__rep_lease_refresh(env)
	ENV *env;
{
	DBT rec;
	DB_LOGC *logc;
	DB_LSN lsn;
	DB_REP *db_rep;
	REP *rep;
	int ret, t_ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);

	memset(&rec, 0, sizeof(rec));
	memset(&lsn, 0, sizeof(lsn));
	/*
	 * Use __rep_log_backup to find the last PERM record.
	 */
	if ((ret = __rep_log_backup(env, rep, logc, &lsn)) != 0) {
		/*
		 * If there is no PERM record, then we get DB_NOTFOUND.
		 */
		if (ret == DB_NOTFOUND)
			ret = 0;
		goto err;
	}

	if ((ret = __logc_get(logc, &lsn, &rec, DB_CURRENT)) != 0)
		goto err;

	(void)__rep_send_message(env, DB_EID_BROADCAST, REP_LOG, &lsn,
	    &rec, REPCTL_PERM, 0);

err:	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_lease_expire -
 *	Proactively expire all leases granted to us.
 * Assume the caller holds the REP_SYSTEM (region) mutex.
 *
 * PUBLIC: int __rep_lease_expire __P((ENV *));
 */
int
__rep_lease_expire(env)
	ENV *env;
{
	DB_REP *db_rep;
	REGINFO *infop;
	REP *rep;
	REP_LEASE_ENTRY *le, *table;
	int ret;
	u_int32_t i;

	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	infop = env->reginfo;

	if (rep->lease_off != INVALID_ROFF) {
		table = R_ADDR(infop, rep->lease_off);
		/*
		 * Expire all leases forcibly.  We are guaranteed that the
		 * start_time for all leases are not in the future.  Therefore,
		 * set the end_time to the start_time.
		 */
		for (i = 0; i < rep->nsites; i++) {
			le = &table[i];
			le->end_time = le->start_time;
		}
	}
	return (ret);
}

/*
 * __rep_lease_waittime -
 *	Return the amount of time remaining on a granted lease.
 * Assume the caller holds the REP_SYSTEM (region) mutex.
 *
 * PUBLIC: db_timeout_t __rep_lease_waittime __P((ENV *));
 */
db_timeout_t
__rep_lease_waittime(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	db_timespec exptime, mytime;
	db_timeout_t to;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	exptime = rep->grant_expire;
	to = 0;
	/*
	 * If the lease has never been granted, we must wait a full
	 * lease timeout because we could be freshly rebooted after
	 * a crash and a lease could be granted from a previous
	 * incarnation of this client.  However, if the lease has never
	 * been granted, and this client has already waited a full
	 * lease timeout, we know our lease cannot be granted and there
	 * is no need to wait again.
	 */
	RPRINT(env, DB_VERB_REP_LEASE, (env,
    "wait_time: grant_expire %lu %lu lease_to %lu",
	    (u_long)exptime.tv_sec, (u_long)exptime.tv_nsec,
	    (u_long)rep->lease_timeout));
	if (!timespecisset(&exptime)) {
		if (!F_ISSET(rep, REP_F_LEASE_EXPIRED))
			to = rep->lease_timeout;
	} else {
		__os_gettime(env, &mytime, 1);
		RPRINT(env, DB_VERB_REP_LEASE, (env,
    "wait_time: mytime %lu %lu, grant_expire %lu %lu",
		    (u_long)mytime.tv_sec, (u_long)mytime.tv_nsec,
		    (u_long)exptime.tv_sec, (u_long)exptime.tv_nsec));
		if (timespeccmp(&mytime, &exptime, <=)) {
			/*
			 * If the current time is before the grant expiration
			 * compute the difference and return remaining grant
			 * time.
			 */
			timespecsub(&exptime, &mytime);
			DB_TIMESPEC_TO_TIMEOUT(to, &exptime, 1);
		}
	}
	return (to);
}
