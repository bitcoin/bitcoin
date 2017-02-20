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

/*
 * We need to check sites == nsites, not more than half
 * like we do in __rep_elect and the VOTE2 code.  The
 * reason is that we want to process all the incoming votes
 * and not short-circuit once we reach more than half.  The
 * real winner's vote may be in the last half.
 */
#define	IS_PHASE1_DONE(rep)						\
    ((rep)->sites >= (rep)->nsites && (rep)->w_priority > 0)

#define	I_HAVE_WON(rep, winner)						\
    ((rep)->votes >= (rep)->nvotes && winner == (rep)->eid)

static void __rep_cmp_vote __P((ENV *, REP *, int, DB_LSN *,
    u_int32_t, u_int32_t, u_int32_t, u_int32_t));
static int __rep_elect_init
	       __P((ENV *, u_int32_t, u_int32_t, int *, u_int32_t *));
static int __rep_fire_elected __P((ENV *, REP *, u_int32_t));
static void __rep_elect_master __P((ENV *, REP *));
static int __rep_grow_sites __P((ENV *, u_int32_t));
static int __rep_tally __P((ENV *, REP *, int, u_int32_t *, u_int32_t, int));
static int __rep_wait __P((ENV *, db_timeout_t *, int, u_int32_t, u_int32_t));

/*
 * __rep_elect_pp --
 *	Called after master failure to hold/participate in an election for
 *	a new master.
 *
 * PUBLIC:  int __rep_elect_pp
 * PUBLIC:      __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__rep_elect_pp(dbenv, given_nsites, nvotes, flags)
	DB_ENV *dbenv;
	u_int32_t given_nsites, nvotes;
	u_int32_t flags;
{
	DB_REP *db_rep;
	ENV *env;
	int ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	ret = 0;

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->rep_elect", DB_INIT_REP);

	if (APP_IS_REPMGR(env)) {
		__db_errx(env,
"DB_ENV->rep_elect: cannot call from Replication Manager application");
		return (EINVAL);
	}

	/* We need a transport function because we send messages. */
	if (db_rep->send == NULL) {
		__db_errx(env,
    "DB_ENV->rep_elect: must be called after DB_ENV->rep_set_transport");
		return (EINVAL);
	}

	if (IS_USING_LEASES(env) && given_nsites != 0) {
		__db_errx(env,
	    "DB_ENV->rep_elect: nsites must be zero if leases configured");
		return (EINVAL);
	}

	ret = __rep_elect_int(env, given_nsites, nvotes, flags);

	return (ret);
}

/*
 * __rep_elect_int --
 *	Internal processing to hold/participate in an election for
 *	a new master after master failure.
 *
 * PUBLIC:  int __rep_elect_int
 * PUBLIC:      __P((ENV *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__rep_elect_int(env, given_nsites, nvotes, flags)
	ENV *env;
	u_int32_t given_nsites, nvotes;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	LOG *lp;
	REP *rep;
	int done, elected, full_elect, in_progress, locked, need_req;
	int ret, send_vote, t_ret;
	u_int32_t ack, ctlflags, egen, nsites, orig_tally, priority, realpri;
	u_int32_t repflags, tiebreaker;
	db_timeout_t last_to, timeout, to;

	COMPQUIET(flags, 0);
	COMPQUIET(egen, 0);

	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	elected = 0;

	/*
	 * Specifying 0 for nsites signals us to use the value configured
	 * previously via rep_set_nsites.  Similarly, if the given nvotes is 0,
	 * it asks us to compute the value representing a simple majority.
	 */
	nsites = given_nsites == 0 ? rep->config_nsites : given_nsites;
	ack = nvotes == 0 ? ELECTION_MAJORITY(nsites) : nvotes;
	locked = 0;

	/*
	 * XXX
	 * If users give us less than a majority, they run the risk of
	 * having a network partition.  However, this also allows the
	 * scenario of master/1 client to elect the client.  Allow
	 * sub-majority values, but give a warning.
	 */
	if (ack <= (nsites / 2)) {
		__db_errx(env,
    "DB_ENV->rep_elect:WARNING: nvotes (%d) is sub-majority with nsites (%d)",
		    nvotes, nsites);
	}

	if (nsites < ack) {
		__db_errx(env,
    "DB_ENV->rep_elect: nvotes (%d) is larger than nsites (%d)",
		    ack, nsites);
		return (EINVAL);
	}

	/*
	 * Default to the normal timeout unless the user configured
	 * a full election timeout and we think we need a full election.
	 */
	full_elect = 0;
	timeout = rep->elect_timeout;
	if (!F_ISSET(rep, REP_F_GROUP_ESTD) && rep->full_elect_timeout != 0) {
		full_elect = 1;
		timeout = rep->full_elect_timeout;
	}
	realpri = rep->priority;

	RPRINT(env, DB_VERB_REP_ELECT,
	    (env, "Start election nsites %d, ack %d, priority %d",
	    nsites, ack, realpri));

	/*
	 * Special case when having an election while running with
	 * sites of potentially mixed versions.  We set a bit indicating
	 * we're an electable site, but set our priority to 0.
	 * Old sites will never elect us, with 0 priority, but if all
	 * we have are new sites, then we can elect the best electable
	 * site of the group.
	 *     Thus 'priority' is this special, possibly-fake, effective
	 * priority that we'll use for this election, while 'realpri' is our
	 * real, configured priority, as retrieved from REP region.
	 */
	ctlflags = realpri != 0 ? REPCTL_ELECTABLE : 0;
	ENV_ENTER(env, ip);

	orig_tally = 0;
	if ((ret = __rep_elect_init(env, nsites, ack,
	    &in_progress, &orig_tally)) != 0) {
		if (ret == DB_REP_NEWMASTER)
			ret = 0;
		goto err;
	}
	/*
	 * If another thread is in the middle of an election we
	 * just quietly return and not interfere.
	 */
	if (in_progress)
		goto edone;

	priority = lp->persist.version != DB_LOGVERSION ? 0 : realpri;
#ifdef	CONFIG_TEST
	/*
	 * This allows us to unit test the ELECTABLE flag simply by
	 * using the priority values.
	 */
	if (priority > 0 && priority <= 5) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
	   "Artificially setting priority 0 (ELECTABLE) for CONFIG_TEST mode"));
		DB_ASSERT(env, ctlflags == REPCTL_ELECTABLE);
		priority = 0;
	}
#endif
	__os_gettime(env, &rep->etime, 1);
	REP_SYSTEM_LOCK(env);
	/*
	 * If leases are configured, wait for them to expire, and
	 * see if we can discover the master while waiting.
	 */
	if (IS_USING_LEASES(env)) {
		to = __rep_lease_waittime(env);
		if (to != 0) {
			F_SET(rep, REP_F_EPHASE0);
			REP_SYSTEM_UNLOCK(env);
			(void)__rep_send_message(env, DB_EID_BROADCAST,
			    REP_MASTER_REQ, NULL, NULL, 0, 0);
			ret = __rep_wait(env, &to, 0, rep->egen, REP_F_EPHASE0);
			REP_SYSTEM_LOCK(env);
			repflags = rep->flags;
			F_CLR(rep, REP_F_EPHASE0);
			switch (ret) {
			/*
			 * If waiting is successful, our flag is cleared
			 * and the master responded.  We're done.
			 */
			case DB_REP_EGENCHG:
			case 0:
				REP_SYSTEM_UNLOCK(env);
				goto edone;
			/*
			 * If we get a timeout, continue with the election.
			 */
			case DB_TIMEOUT:
				/*
				 * We have waited a full lease timeout.  We
				 * need to check now under lock to verify that
				 * the phase was not over and that the client
				 * did not grant the lease.  If either happened
				 * between the time the wait finished and we
				 * reacquired the mutex, we're done.
				 */
				if (!FLD_ISSET(repflags, REP_F_EPHASE0) ||
				    __rep_islease_granted(env) != 0) {
					ret = 0;
					REP_SYSTEM_UNLOCK(env);
					goto edone;
				}
				F_SET(rep, REP_F_LEASE_EXPIRED);
				break;
			default:
				goto lockdone;
			}
		}
	}
	/*
	 * We need to lockout applying incoming log records during
	 * the election.  We need to use a special rep_lockout_apply
	 * instead of rep_lockout_msg because we do not want to
	 * lockout all incoming messages, like other VOTEs!
	 */
	if ((ret = __rep_lockout_apply(env, rep, 0)) != 0)
		goto lockdone;
	locked = 1;
	last_to = to = timeout;
	REP_SYSTEM_UNLOCK(env);
restart:
	/* Generate a randomized tiebreaker value. */
	__os_unique_id(env, &tiebreaker);
	LOG_SYSTEM_LOCK(env);
	lsn = lp->lsn;
	LOG_SYSTEM_UNLOCK(env);
	REP_SYSTEM_LOCK(env);

	F_SET(rep, REP_F_EPHASE1 | REP_F_NOARCHIVE);
	F_CLR(rep, REP_F_TALLY);
	/*
	 * We made sure that leases were expired before starting the
	 * election, but an existing master may be slow in responding.
	 * If, during lockout, acquiring mutexes, etc, the client has now
	 * re-granted its lease, we're done - a master exists.
	 */
	if (IS_USING_LEASES(env) &&
	     __rep_islease_granted(env)) {
		ret = 0;
		goto lockdone;
	}

	/*
	 * If we are in the middle of recovering or internal
	 * init, we participate, but we set our priority to 0
	 * and turn off REPCTL_ELECTABLE.  We *cannot* use the
	 * REP_F_RECOVER_MASK macro because we must explicitly
	 * exclude REP_F_RECOVER_VERIFY.  If we are in verify
	 * then that is okay, we can be elected (i.e. we are not
	 * in an inconsistent state).
	 */
	if (F_ISSET(rep, REP_F_READY_API | REP_F_READY_OP |
	    REP_F_RECOVER_LOG | REP_F_RECOVER_PAGE | REP_F_RECOVER_UPDATE)) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
	   "Setting priority 0, unelectable, due to internal init/recovery"));
		priority = 0;
		ctlflags = 0;
	}

	/*
	 * We are about to participate at this egen.  We must
	 * write out the next egen before participating in this one
	 * so that if we crash we can never participate in this egen
	 * again.
	 */
	if ((ret = __rep_write_egen(env, rep, rep->egen + 1)) != 0)
		goto lockdone;

	/* Tally our own vote */
	if (__rep_tally(env, rep, rep->eid, &rep->sites, rep->egen, 1) != 0) {
		ret = EINVAL;
		goto lockdone;
	}
	__rep_cmp_vote(env, rep, rep->eid, &lsn, priority, rep->gen,
	    tiebreaker, ctlflags);

	RPRINT(env, DB_VERB_REP_ELECT, (env, "Beginning an election"));

	/* Now send vote */
	send_vote = DB_EID_INVALID;
	egen = rep->egen;
	done = IS_PHASE1_DONE(rep);
	REP_SYSTEM_UNLOCK(env);
	__rep_send_vote(env, &lsn, nsites, ack, priority, tiebreaker, egen,
	    DB_EID_BROADCAST, REP_VOTE1, ctlflags);
	DB_ENV_TEST_RECOVERY(env, DB_TEST_ELECTVOTE1, ret, NULL);
	if (done) {
		REP_SYSTEM_LOCK(env);
		goto vote;
	}
	last_to = to;
	ret = __rep_wait(env, &to, full_elect, egen, REP_F_EPHASE1);
	switch (ret) {
		case 0:
			/* Check if election complete or phase complete. */
			if (!IN_ELECTION(rep)) {
				RPRINT(env, DB_VERB_REP_ELECT,
				    (env, "Ended election phase 1"));
				goto edone;
			}
			goto phase2;
		case DB_REP_EGENCHG:
			/*
			 * Pick up reducing our timeout where we last
			 * left off.
			 */
			if (to > last_to)
				to = last_to;
			to = (to * 8) / 10;
			RPRINT(env, DB_VERB_REP_ELECT, (env,
"Egen changed while waiting. Now %lu.  New timeout %lu, orig timeout %lu",
			    (u_long)rep->egen, (u_long)to, (u_long)timeout));
			/*
			 * If the egen changed while we were sleeping, that
			 * means we're probably late to the next election,
			 * so we'll backoff our timeout so that we don't get
			 * into an out-of-phase election scenario.
			 *
			 * Backoff to 80% of the current timeout.
			 */
			goto restart;
		case DB_TIMEOUT:
			break;
		default:
			goto err;
	}

	REP_SYSTEM_LOCK(env);
	/*
	 * If we got here, we haven't heard from everyone, but we've
	 * run out of time, so it's time to decide if we have enough
	 * votes to pick a winner and if so, to send out a vote to
	 * the winner.
	 *
	 * Check the state of the world after reacquiring the mutex.
	 * See if the election actually finished anyway.
	 */
	if (!IN_ELECTION(rep)) {
		RPRINT(env, DB_VERB_REP_ELECT,
		    (env, "Ended election after acquiring mutex"));
		ret = 0;
		goto lockdone;
	}
	/*
	 * If our egen changed while we were waiting.  We need to
	 * essentially reinitialize our election.
	 */
	if (egen != rep->egen) {
		REP_SYSTEM_UNLOCK(env);
		RPRINT(env, DB_VERB_REP_ELECT,
		    (env, "Egen changed from %lu to %lu",
		    (u_long)egen, (u_long)rep->egen));
		goto restart;
	}
	if (rep->sites >= rep->nvotes) {
vote:
		/* We think we've seen enough to cast a vote. */
		send_vote = rep->winner;
		/*
		 * See if we won.  This will make sure we
		 * don't count ourselves twice if we're racing
		 * with incoming votes.
		 */
		if (rep->winner == rep->eid) {
			(void)__rep_tally(env, rep, rep->eid, &rep->votes,
			    egen, 2);
			RPRINT(env, DB_VERB_REP_ELECT, (env,
			    "Counted my vote %d", rep->votes));
		}
		F_SET(rep, REP_F_EPHASE2);
		F_CLR(rep, REP_F_EPHASE1);
	}
	REP_SYSTEM_UNLOCK(env);
	if (send_vote == DB_EID_INVALID) {
		/* We do not have enough votes to elect. */
		if (rep->sites >= rep->nvotes)
			__db_errx(env,
	"No electable site found: recvd %d of %d votes from %d sites",
			    rep->sites, rep->nvotes, rep->nsites);
		else
			__db_errx(env,
	"Not enough votes to elect: recvd %d of %d from %d sites",
			    rep->sites, rep->nvotes, rep->nsites);
		ret = DB_REP_UNAVAIL;
		goto err;
	}

	/*
	 * We have seen enough vote1's.  Now we need to wait
	 * for all the vote2's.
	 */
	if (send_vote != rep->eid) {
		RPRINT(env, DB_VERB_REP_ELECT, (env, "Sending vote"));
		__rep_send_vote(env, NULL, 0, 0, 0, 0, egen,
		    send_vote, REP_VOTE2, 0);
		/*
		 * If we are NOT the new master we want to send
		 * our vote to the winner, and wait longer.  The
		 * reason is that the winner may be "behind" us
		 * in the election waiting and if the master is
		 * down, the winner will wait the full timeout
		 * and we want to give the winner enough time to
		 * process all the votes.  Otherwise we could
		 * incorrectly return DB_REP_UNAVAIL and start a
		 * new election before the winner can declare
		 * itself.
		 */
		to = to * 2;
	}

phase2:
	if (I_HAVE_WON(rep, rep->winner)) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Skipping phase2 wait: already got %d votes", rep->votes));
		REP_SYSTEM_LOCK(env);
		goto i_won;
	}
	/*
	 * Don't set last_to to 'to' here because we may have adjusted
	 * it above.  If egen changes we want to pick up reducing the
	 * timeout from the point we were above.
	 */
	ret = __rep_wait(env, &to, full_elect, egen, REP_F_EPHASE2);
	RPRINT(env, DB_VERB_REP_ELECT,
	    (env, "Ended election phase 2 %d", ret));
	switch (ret) {
		case 0:
			goto edone;
		case DB_REP_EGENCHG:
			if (to > last_to)
				to = last_to;
			to = (to * 8) / 10;
			RPRINT(env, DB_VERB_REP_ELECT, (env,
"While waiting egen changed to %lu.  Phase 2 New timeout %lu, orig timeout %lu",
			    (u_long)rep->egen,
			    (u_long)to, (u_long)timeout));
			goto restart;
		case DB_TIMEOUT:
			ret = DB_REP_UNAVAIL;
			break;
		default:
			goto err;
	}
	REP_SYSTEM_LOCK(env);
	/*
	 * Check the state of the world after reacquiring the mutex.
	 * See if the election actually finished anyway.
	 */
	if (!IN_ELECTION(rep)) {
		RPRINT(env, DB_VERB_REP_ELECT,
		    (env, "Ended election phase 2 after acquiring mutex"));
		ret = 0;
		goto lockdone;
	}
	if (egen != rep->egen) {
		REP_SYSTEM_UNLOCK(env);
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Egen ph2 changed from %lu to %lu",
		    (u_long)egen, (u_long)rep->egen));
		goto restart;
	}
	RPRINT(env, DB_VERB_REP_ELECT, (env,
	    "After phase 2: votes %d, nvotes %d, nsites %d",
	    rep->votes, rep->nvotes, rep->nsites));
	if (I_HAVE_WON(rep, rep->winner)) {
i_won:		__rep_elect_master(env, rep);
		ret = 0;
		elected = 1;
	}
	if (0) {
err:		REP_SYSTEM_LOCK(env);
	}
lockdone:
	/*
	 * If we get here because of a non-election error, then we
	 * did not tally our vote.  The only non-election error is
	 * from elect_init where we were unable to grow_sites.  In
	 * that case we do not want to discard all known election info.
	 */
	if (ret == 0 || ret == DB_REP_UNAVAIL)
		__rep_elect_done(env, rep, 0);
	else if (orig_tally)
		F_SET(rep, orig_tally);

	/*
	 * If the election finished elsewhere, we need to clear
	 * the elect flag anyway.
	 */
	if (0) {
edone:		REP_SYSTEM_LOCK(env);
	}
	F_CLR(rep, REP_F_INREPELECT);
	if (locked) {
		need_req = F_ISSET(rep, REP_F_SKIPPED_APPLY) &&
		    !I_HAVE_WON(rep, rep->winner);
		F_CLR(rep, REP_F_READY_APPLY | REP_F_SKIPPED_APPLY);
		REP_SYSTEM_UNLOCK(env);
		/*
		 * If we skipped any log records, request them now.
		 */
		if (need_req && (t_ret = __rep_resend_req(env, 0)) != 0 &&
		    ret == 0)
			ret = t_ret;
	} else
		REP_SYSTEM_UNLOCK(env);

	if (elected)
		ret = __rep_fire_elected(env, rep, egen);

	RPRINT(env, DB_VERB_REP_ELECT, (env,
	    "Ended election with %d, sites %d, egen %lu, flags 0x%lx",
	    ret, rep->sites, (u_long)rep->egen, (u_long)rep->flags));

DB_TEST_RECOVERY_LABEL
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __rep_vote1 --
 *	Handle incoming vote1 message on a client.
 *
 * PUBLIC: int __rep_vote1 __P((ENV *, __rep_control_args *, DBT *, int));
 */
int
__rep_vote1(env, rp, rec, eid)
	ENV *env;
	__rep_control_args *rp;
	DBT *rec;
	int eid;
{
	DBT data_dbt;
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	REP_OLD_VOTE_INFO *ovi;
	__rep_egen_args egen_arg;
	__rep_vote_info_args tmpvi, *vi;
	u_int32_t egen;
	int elected, inelect, master, ret;
	u_int8_t buf[__REP_MAXMSG_SIZE];
	size_t len;

	COMPQUIET(egen, 0);

	elected = ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	if (F_ISSET(rep, REP_F_MASTER)) {
		RPRINT(env, DB_VERB_REP_ELECT, (env, "Master received vote"));
		LOG_SYSTEM_LOCK(env);
		lsn = lp->lsn;
		LOG_SYSTEM_UNLOCK(env);
		(void)__rep_send_message(env,
		    DB_EID_BROADCAST, REP_NEWMASTER, &lsn, NULL, 0, 0);
		return (ret);
	}

	/*
	 * In 4.7 we changed to having fixed sized u_int32_t's from
	 * non-fixed 'int' fields in the vote structure.
	 */
	if (rp->rep_version < DB_REPVERSION_47) {
		ovi = (REP_OLD_VOTE_INFO *)rec->data;
		tmpvi.egen = ovi->egen;
		tmpvi.nsites = (u_int32_t)ovi->nsites;
		tmpvi.nvotes = (u_int32_t)ovi->nvotes;
		tmpvi.priority = (u_int32_t)ovi->priority;
		tmpvi.tiebreaker = ovi->tiebreaker;
	} else
		if ((ret = __rep_vote_info_unmarshal(env,
		    &tmpvi, rec->data, rec->size, NULL)) != 0)
			return (ret);
	vi = &tmpvi;
	REP_SYSTEM_LOCK(env);

	/*
	 * If we get a vote from a later election gen, we
	 * clear everything from the current one, and we'll
	 * start over by tallying it.  If we get an old vote,
	 * send an ALIVE to the old participant.
	 */
	RPRINT(env, DB_VERB_REP_ELECT,
	    (env, "Received vote1 egen %lu, egen %lu",
	    (u_long)vi->egen, (u_long)rep->egen));
	if (vi->egen < rep->egen) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Received old vote %lu, egen %lu, ignoring vote1",
		    (u_long)vi->egen, (u_long)rep->egen));
		egen_arg.egen = rep->egen;
		REP_SYSTEM_UNLOCK(env);
		if (rep->version < DB_REPVERSION_47)
			DB_INIT_DBT(data_dbt, &egen_arg.egen,
			    sizeof(egen_arg.egen));
		else {
			if ((ret = __rep_egen_marshal(env,
			    &egen_arg, buf, __REP_EGEN_SIZE, &len)) != 0)
				return (ret);
			DB_INIT_DBT(data_dbt, buf, len);
		}
		(void)__rep_send_message(env,
		    eid, REP_ALIVE, &rp->lsn, &data_dbt, 0, 0);
		return (ret);
	}
	inelect = F_ISSET(rep, REP_F_INREPELECT);
	if (vi->egen > rep->egen) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Received VOTE1 from egen %lu, my egen %lu; reset",
		    (u_long)vi->egen, (u_long)rep->egen));
		/*
		 * Record if we're currently in rep_elect.  If so, don't
		 * return HOLDELECTION because the election thread should
		 * notice it.  However, there is a window where the thread
		 * could be at the tail end of processing the previous
		 * election and it would not see this change in egen.
		 */
		__rep_elect_done(env, rep, 0);
		rep->egen = vi->egen;
		F_SET(rep, REP_F_EGENUPDATE);
	}

	/*
	 * If this site (sender of the VOTE1) is the first to the party, simply
	 * initialize values from the message.  Otherwise, see if the site knows
	 * about more sites, and/or requires more votes, than we do.
	 */
	if (!IN_ELECTION_TALLY(rep)) {
		F_SET(rep, REP_F_TALLY);
		rep->nsites = vi->nsites;
		rep->nvotes = vi->nvotes;
	} else {
		if (vi->nsites > rep->nsites)
			rep->nsites = vi->nsites;
		if (vi->nvotes > rep->nvotes)
			rep->nvotes = vi->nvotes;
	}

	/*
	 * We are keeping the vote, let's see if that changes our
	 * count of the number of sites.
	 */
	if (rep->sites + 1 > rep->nsites)
		rep->nsites = rep->sites + 1;
	/*
	 * Ignore vote1's if we're in phase 2.
	 */
	if (F_ISSET(rep, REP_F_EPHASE2)) {
		RPRINT(env, DB_VERB_REP_ELECT,
		    (env, "In phase 2, ignoring vote1"));
		goto err;
	}

	/*
	 * Record this vote.  If we get back non-zero, we
	 * ignore the vote.
	 */
	if ((ret = __rep_tally(env, rep, eid, &rep->sites, vi->egen, 1)) != 0) {
		RPRINT(env, DB_VERB_REP_ELECT,
		    (env, "Tally returned %d, sites %d", ret, rep->sites));
		ret = 0;
		goto err;
	}
	RPRINT(env, DB_VERB_REP_ELECT, (env,
	    "Incoming vote: (eid)%d (pri)%lu %s (gen)%lu (egen)%lu [%lu,%lu]",
	    eid, (u_long)vi->priority,
	    F_ISSET(rp, REPCTL_ELECTABLE) ? "ELECTABLE" : "",
	    (u_long)rp->gen, (u_long)vi->egen,
	    (u_long)rp->lsn.file, (u_long)rp->lsn.offset));
	if (rep->sites > 1)
		RPRINT(env, DB_VERB_REP_ELECT, (env,
	    "Existing vote: (eid)%d (pri)%lu (gen)%lu (sites)%d [%lu,%lu]",
		    rep->winner, (u_long)rep->w_priority,
		    (u_long)rep->w_gen, rep->sites,
		    (u_long)rep->w_lsn.file,
		    (u_long)rep->w_lsn.offset));

	__rep_cmp_vote(env, rep, eid, &rp->lsn, vi->priority,
	    rp->gen, vi->tiebreaker, rp->flags);
	/*
	 * If you get a vote and you're not in an election, we've
	 * already recorded this vote.  But that is all we need
	 * to do.
	 */
	if (!IN_ELECTION(rep)) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Not in election, but received vote1 0x%x", rep->flags));
		/*
		 * If we were in the middle of an election and we ended up
		 * clearing the election out from under the rep_elect caller,
		 * we want to just return here.
		 */
		if (inelect)
			ret = 0;
		else
			ret = DB_REP_HOLDELECTION;
		goto err;
	}

	master = rep->winner;
	lsn = rep->w_lsn;
	if (IS_PHASE1_DONE(rep)) {
		RPRINT(env, DB_VERB_REP_ELECT, (env, "Phase1 election done"));
		RPRINT(env, DB_VERB_REP_ELECT, (env, "Voting for %d%s",
		    master, master == rep->eid ? "(self)" : ""));
		egen = rep->egen;
		F_SET(rep, REP_F_EPHASE2);
		F_CLR(rep, REP_F_EPHASE1);
		if (master == rep->eid) {
			(void)__rep_tally(env, rep, rep->eid,
			    &rep->votes, egen, 2);
			RPRINT(env, DB_VERB_REP_ELECT, (env,
			    "After phase 1 done: counted vote %d of %d",
			    rep->votes, rep->nvotes));
			if (I_HAVE_WON(rep, rep->winner)) {
				__rep_elect_master(env, rep);
				elected = 1;
			}
			goto err;
		}
		REP_SYSTEM_UNLOCK(env);

		/* Vote for someone else. */
		__rep_send_vote(env, NULL, 0, 0, 0, 0, egen,
		    master, REP_VOTE2, 0);
	} else
err:		REP_SYSTEM_UNLOCK(env);
	if (elected)
		ret = __rep_fire_elected(env, rep, egen);
	return (ret);
}

/*
 * __rep_vote2 --
 *	Handle incoming vote2 message on a client.
 *
 * PUBLIC: int __rep_vote2 __P((ENV *, __rep_control_args *, DBT *, int));
 */
int
__rep_vote2(env, rp, rec, eid)
	ENV *env;
	__rep_control_args *rp;
	DBT *rec;
	int eid;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	REP_OLD_VOTE_INFO *ovi;
	__rep_vote_info_args tmpvi, *vi;
	u_int32_t egen;
	int ret;

	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	RPRINT(env, DB_VERB_REP_ELECT, (env, "We received a vote%s",
	    F_ISSET(rep, REP_F_MASTER) ? " (master)" : ""));
	if (F_ISSET(rep, REP_F_MASTER)) {
		LOG_SYSTEM_LOCK(env);
		lsn = lp->lsn;
		LOG_SYSTEM_UNLOCK(env);
		STAT(rep->stat.st_elections_won++);
		(void)__rep_send_message(env,
		    DB_EID_BROADCAST, REP_NEWMASTER, &lsn, NULL, 0, 0);
		if (IS_USING_LEASES(env))
			ret = __rep_lease_refresh(env);
		return (ret);
	}

	REP_SYSTEM_LOCK(env);
	egen = rep->egen;

	/*
	 * We might be the last to the party and we haven't had
	 * time to tally all the vote1's, but others have and
	 * decided we're the winner.  So, if we're in the process
	 * of tallying sites, keep the vote so that when our
	 * election thread catches up we'll have the votes we
	 * already received.
	 */
	/*
	 * In 4.7 we changed to having fixed sized u_int32_t's from
	 * non-fixed 'int' fields in the vote structure.
	 */
	if (rp->rep_version < DB_REPVERSION_47) {
		ovi = (REP_OLD_VOTE_INFO *)rec->data;
		tmpvi.egen = ovi->egen;
		tmpvi.nsites = (u_int32_t)ovi->nsites;
		tmpvi.nvotes = (u_int32_t)ovi->nvotes;
		tmpvi.priority = (u_int32_t)ovi->priority;
		tmpvi.tiebreaker = ovi->tiebreaker;
	} else
		if ((ret = __rep_vote_info_unmarshal(env,
		    &tmpvi, rec->data, rec->size, NULL)) != 0)
			return (ret);
	vi = &tmpvi;
	if (!IN_ELECTION_TALLY(rep) && vi->egen >= rep->egen) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Not in election gen %lu, at %lu, got vote",
		    (u_long)vi->egen, (u_long)rep->egen));
		ret = DB_REP_HOLDELECTION;
		goto err;
	}

	/*
	 * Record this vote.  In a VOTE2, the only valid entry
	 * in the vote information is the election generation.
	 *
	 * There are several things which can go wrong that we
	 * need to account for:
	 * 1. If we receive a latent VOTE2 from an earlier election,
	 * we want to ignore it.
	 * 2. If we receive a VOTE2 from a site from which we never
	 * received a VOTE1, we want to record it, because we simply
	 * may be processing messages out of order or its vote1 got lost,
	 * but that site got all the votes it needed to send it.
	 * 3. If we have received a duplicate VOTE2 from this election
	 * from the same site we want to ignore it.
	 * 4. If this is from the current election and someone is
	 * really voting for us, then we finally get to record it.
	 */
	/*
	 * Case 1.
	 */
	if (vi->egen != rep->egen) {
		RPRINT(env, DB_VERB_REP_ELECT,
		    (env, "Bad vote egen %lu.  Mine %lu",
		    (u_long)vi->egen, (u_long)rep->egen));
		ret = 0;
		goto err;
	}

	/*
	 * __rep_tally takes care of cases 2, 3 and 4.
	 */
	if ((ret = __rep_tally(env, rep, eid, &rep->votes, vi->egen, 2)) != 0) {
		ret = 0;
		goto err;
	}
	RPRINT(env, DB_VERB_REP_ELECT, (env, "Counted vote %d of %d",
	    rep->votes, rep->nvotes));
	if (I_HAVE_WON(rep, rep->winner)) {
		__rep_elect_master(env, rep);
		ret = DB_REP_NEWMASTER;
	}

err:	REP_SYSTEM_UNLOCK(env);
	if (ret == DB_REP_NEWMASTER)
		ret = __rep_fire_elected(env, rep, egen);
	return (ret);
}

/*
 * __rep_tally --
 *	Handle incoming vote message on a client.  Called with the db_rep
 *	mutex held.  This function will return 0 if we successfully tally
 *	the vote and non-zero if the vote is ignored.  This will record
 *	both VOTE1 and VOTE2 records, depending on which region offset the
 *	caller passed in.
 */
static int
__rep_tally(env, rep, eid, countp, egen, phase)
	ENV *env;
	REP *rep;
	int eid;
	u_int32_t *countp;
	u_int32_t egen;
	int phase;
{
	REP_VTALLY *tally, *vtp;
	u_int32_t i, max_sites;
	int ret;

	/*
	 * The counts are indices, and therefore 0-based.
	 */
	if ((*countp + 1) > rep->nsites)
		max_sites = (*countp + 1);
	else
		max_sites = rep->nsites;
	if (max_sites > rep->asites &&
	    (ret = __rep_grow_sites(env, max_sites)) != 0) {
		RPRINT(env, DB_VERB_REP_ELECT, (env,
		    "Grow sites returned error %d", ret));
		return (ret);
	}
	if (phase == 1)
		tally = R_ADDR(env->reginfo, rep->tally_off);
	else
		tally = R_ADDR(env->reginfo, rep->v2tally_off);
	vtp = &tally[0];
	for (i = 0; i < *countp;) {
		/*
		 * Ignore votes from earlier elections (i.e. we've heard
		 * from this site in this election, but its vote from an
		 * earlier election got delayed and we received it now).
		 * However, if we happened to hear from an earlier vote
		 * and we recorded it and we're now hearing from a later
		 * election we want to keep the updated one.  Note that
		 * updating the entry will not increase the count.
		 * Also ignore votes that are duplicates.
		 */
		if (vtp->eid == eid) {
			RPRINT(env, DB_VERB_REP_ELECT, (env,
			    "Tally found[%d] (%d, %lu), this vote (%d, %lu)",
				    i, vtp->eid, (u_long)vtp->egen,
				    eid, (u_long)egen));
			if (vtp->egen >= egen)
				return (1);
			else {
				vtp->egen = egen;
				return (0);
			}
		}
		i++;
		vtp = &tally[i];
	}

	/*
	 * If we get here, we have a new voter we haven't seen before.  Tally
	 * this vote.
	 */
	RPRINT(env, DB_VERB_REP_ELECT, (env, "Tallying VOTE%d[%d] (%d, %lu)",
	    phase, i, eid, (u_long)egen));

	vtp->eid = eid;
	vtp->egen = egen;
	(*countp)++;
	return (0);
}

/*
 * __rep_cmp_vote --
 *	Compare incoming vote1 message on a client.  Called with the db_rep
 *	mutex held.
 *
 */
static void
__rep_cmp_vote(env, rep, eid, lsnp, priority, gen, tiebreaker, flags)
	ENV *env;
	REP *rep;
	int eid;
	DB_LSN *lsnp;
	u_int32_t priority;
	u_int32_t flags, gen, tiebreaker;
{
	int cmp;

	cmp = LOG_COMPARE(lsnp, &rep->w_lsn);
	/*
	 * If we've seen more than one, compare us to the best so far.
	 * If we're the first, make ourselves the winner to start.
	 */
	if (rep->sites > 1 &&
	    (priority != 0 || LF_ISSET(REPCTL_ELECTABLE))) {
		/*
		 * Special case, if we have a mixed version group of sites,
		 * we set priority to 0, but set the ELECTABLE flag so that
		 * all sites talking at lower versions can correctly elect.
		 * If a non-zero priority comes in and current winner is
		 * zero priority (but was electable), then the non-zero
		 * site takes precedence no matter what its LSN is.
		 *
		 * Then LSN is determinant only if we're comparing
		 * like-styled version/priorities.  I.e. both with
		 * 0/ELECTABLE priority or both with non-zero priority.
		 * Then actual priority value if LSNs
		 * are equal, then tiebreaker if both are equal.
		 */
		if ((priority != 0 && rep->w_priority == 0) ||
		    (((priority == 0 && rep->w_priority == 0) ||
		     (priority != 0 && rep->w_priority != 0)) && cmp > 0) ||
		    (cmp == 0 && (priority > rep->w_priority ||
		    (priority == rep->w_priority &&
		    (tiebreaker > rep->w_tiebreaker))))) {
			RPRINT(env, DB_VERB_REP_ELECT,
			    (env, "Accepting new vote"));
			rep->winner = eid;
			rep->w_priority = priority;
			rep->w_lsn = *lsnp;
			rep->w_gen = gen;
			rep->w_tiebreaker = tiebreaker;
		}
	} else if (rep->sites == 1) {
		if (priority != 0 || LF_ISSET(REPCTL_ELECTABLE)) {
			/* Make ourselves the winner to start. */
			rep->winner = eid;
			rep->w_priority = priority;
			rep->w_gen = gen;
			rep->w_lsn = *lsnp;
			rep->w_tiebreaker = tiebreaker;
		} else {
			rep->winner = DB_EID_INVALID;
			rep->w_priority = 0;
			rep->w_gen = 0;
			ZERO_LSN(rep->w_lsn);
			rep->w_tiebreaker = 0;
		}
	}
}

/*
 * __rep_elect_init
 *	Initialize an election.  Sets beginp non-zero if the election is
 * already in progress; makes it 0 otherwise.
 */
static int
__rep_elect_init(env, nsites, nvotes, beginp, otally)
	ENV *env;
	u_int32_t nsites, nvotes;
	int *beginp;
	u_int32_t *otally;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	ret = 0;

	/* We may miscount, as we don't hold the replication mutex here. */
	STAT(rep->stat.st_elections++);

	/* If we are already master; simply broadcast that fact and return. */
	if (F_ISSET(rep, REP_F_MASTER)) {
		dblp = env->lg_handle;
		lp = dblp->reginfo.primary;
		LOG_SYSTEM_LOCK(env);
		lsn = lp->lsn;
		LOG_SYSTEM_UNLOCK(env);
		(void)__rep_send_message(env,
		    DB_EID_BROADCAST, REP_NEWMASTER, &lsn, NULL, 0, 0);
		if (IS_USING_LEASES(env))
			ret = __rep_lease_refresh(env);
		STAT(rep->stat.st_elections_won++);
		return (DB_REP_NEWMASTER);
	}

	REP_SYSTEM_LOCK(env);
	if (otally != NULL)
		*otally = F_ISSET(rep, REP_F_TALLY);
	*beginp = IN_ELECTION(rep) || F_ISSET(rep, REP_F_INREPELECT);
	if (!*beginp) {
		/*
		 * Make sure that we always initialize all the election fields
		 * before putting ourselves in an election state.  That means
		 * issuing calls that can fail (allocation) before setting all
		 * the variables.
		 */
		if (nsites > rep->asites &&
		    (ret = __rep_grow_sites(env, nsites)) != 0)
			goto err;
		DB_ENV_TEST_RECOVERY(env, DB_TEST_ELECTINIT, ret, NULL);
		F_SET(rep, REP_F_INREPELECT);
		F_CLR(rep, REP_F_EGENUPDATE);
		/*
		 * If we're the first to the party, we simply set initial
		 * values: pre-existing values would be left over from previous
		 * election.
		 */
		if (!IN_ELECTION_TALLY(rep)) {
			rep->nsites = nsites;
			rep->nvotes = nvotes;
		} else {
			if (nsites > rep->nsites)
				rep->nsites = nsites;
			if (nvotes > rep->nvotes)
				rep->nvotes = nvotes;
		}
	}
DB_TEST_RECOVERY_LABEL
err:	REP_SYSTEM_UNLOCK(env);
	return (ret);
}

/*
 * __rep_elect_master
 *	Set up for new master from election.  Must be called with
 *	the replication region mutex held.
 */
static void
__rep_elect_master(env, rep)
	ENV *env;
	REP *rep;
{
	if (F_ISSET(rep, REP_F_MASTERELECT | REP_F_MASTER)) {
		/* We've been through here already; avoid double counting. */
		return;
	}

	F_SET(rep, REP_F_MASTERELECT);
	STAT(rep->stat.st_elections_won++);

	RPRINT(env, DB_VERB_REP_ELECT, (env,
	    "Got enough votes to win; election done; winner is %d, gen %lu",
	    rep->master_id, (u_long)rep->gen));
}

static int
__rep_fire_elected(env, rep, egen)
	ENV *env;
	REP *rep;
	u_int32_t egen;
{
	REP_EVENT_LOCK(env);
	if (rep->notified_egen < egen) {
		__rep_fire_event(env, DB_EVENT_REP_ELECTED, NULL);
		rep->notified_egen = egen;
	}
	REP_EVENT_UNLOCK(env);
	return (0);
}

/*
 * Compute a sleep interval.  Set it to the smaller of .5s or
 * timeout/10, making sure we sleep at least 1usec if timeout < 10.
 */
#define	SLEEPTIME(timeout)					\
	(timeout > 5000000) ? 500000 : ((timeout >= 10) ? timeout / 10 : 1);

static int
__rep_wait(env, timeoutp, full_elect, egen, flags)
	ENV *env;
	db_timeout_t *timeoutp;
	int full_elect;
	u_int32_t egen, flags;
{
	DB_REP *db_rep;
	REP *rep;
	int done, echg, phase_over, ret;
	u_int32_t sleeptime, sleeptotal, timeout;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	done = echg = phase_over = ret = 0;

	timeout = *timeoutp;
	/*
	 * The user specifies an overall timeout function, but checking
	 * is cheap and the timeout may be a generous upper bound.
	 * Sleep repeatedly for the smaller of .5s and timeout/10.
	 */
	sleeptime = SLEEPTIME(timeout);
	sleeptotal = 0;
	while (sleeptotal < timeout) {
		__os_yield(env, 0, sleeptime);
		sleeptotal += sleeptime;
		REP_SYSTEM_LOCK(env);
		/*
		 * Check if group membership changed while we were
		 * sleeping.  Specifically we're trying for a full
		 * election and someone is telling us we're joining
		 * a previously established replication group.
		 */
		if (full_elect && F_ISSET(rep, REP_F_GROUP_ESTD)) {
			*timeoutp = rep->elect_timeout;
			timeout = *timeoutp;
			/*
			 * We adjusted timeout, if we've already waited
			 * that long, then return as though this phase
			 * timed out.  However, we want to give other
			 * changes a chance to return, so if we both
			 * found a group and found a new egen, we
			 * override this return with the egen information.
			 * If we found a group and our election finished
			 * then we want to return the election completion.
			 */
			if (sleeptotal >= timeout) {
				done = 1;
				ret = DB_TIMEOUT;
			} else
				sleeptime = SLEEPTIME(timeout);
		}

		echg = egen != rep->egen;
		phase_over = !F_ISSET(rep, flags);

		/*
		 * Since we're not clearing out master_id any more,
		 * we need to do more to detect the difference between
		 * a new master getting elected and egen changing,
		 * or a new election starting because the old one
		 * timed out at another site (which easily happens
		 * when sites have very different timeout settings).
		 *
		 * Detect this by:
		 * If my phase was over, egen has changed but
		 * there are still election flags set, or we're
		 * told our egen was out of date and updated
		 * then return DB_REP_EGENCHG.
		 *
		 * Phase 0 doesn't care about egen, only the phase over.
		 *
		 * Otherwise, if my phase is over I want to
		 * set my idea of the master and return.
		 */
		if (phase_over && echg &&
		    flags != REP_F_EPHASE0 &&
		    (IN_ELECTION_TALLY(rep) ||
		    F_ISSET(rep, REP_F_EGENUPDATE))) {
			done = 1;
			F_CLR(rep, REP_F_EGENUPDATE);
			ret = DB_REP_EGENCHG;
		} else if (phase_over) {
			done = 1;
			ret = 0;
		}
		REP_SYSTEM_UNLOCK(env);

		if (done)
			return (ret);
	}
	return (DB_TIMEOUT);
}

/*
 * __rep_grow_sites --
 *	Called to allocate more space in the election tally information.
 * Called with the rep mutex held.  We need to call the region mutex, so
 * we need to make sure that we *never* acquire those mutexes in the
 * opposite order.
 */
static int
__rep_grow_sites(env, nsites)
	ENV *env;
	u_int32_t nsites;
{
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	int ret, *tally;
	u_int32_t nalloc;

	rep = env->rep_handle->region;

	/*
	 * Allocate either twice the current allocation or nsites,
	 * whichever is more.
	 */
	nalloc = 2 * rep->asites;
	if (nalloc < nsites)
		nalloc = nsites;

	infop = env->reginfo;
	renv = infop->primary;
	MUTEX_LOCK(env, renv->mtx_regenv);

	/*
	 * We allocate 2 tally regions, one for tallying VOTE1's and
	 * one for VOTE2's.  Always grow them in tandem, because if we
	 * get more VOTE1's we'll always expect more VOTE2's then too.
	 */
	if ((ret = __env_alloc(infop,
	    (size_t)nalloc * sizeof(REP_VTALLY), &tally)) == 0) {
		if (rep->tally_off != INVALID_ROFF)
			 __env_alloc_free(
			     infop, R_ADDR(infop, rep->tally_off));
		rep->tally_off = R_OFFSET(infop, tally);
		if ((ret = __env_alloc(infop,
		    (size_t)nalloc * sizeof(REP_VTALLY), &tally)) == 0) {
			/* Success */
			if (rep->v2tally_off != INVALID_ROFF)
				 __env_alloc_free(infop,
				    R_ADDR(infop, rep->v2tally_off));
			rep->v2tally_off = R_OFFSET(infop, tally);
			rep->asites = nalloc;
			rep->nsites = nsites;
		} else {
			/*
			 * We were unable to allocate both.  So, we must
			 * free the first one and reinitialize.  If
			 * v2tally_off is valid, it is from an old
			 * allocation and we are clearing it all out due
			 * to the error.
			 */
			if (rep->v2tally_off != INVALID_ROFF)
				 __env_alloc_free(infop,
				    R_ADDR(infop, rep->v2tally_off));
			__env_alloc_free(infop,
			    R_ADDR(infop, rep->tally_off));
			rep->v2tally_off = rep->tally_off = INVALID_ROFF;
			rep->asites = 0;
			rep->nsites = 0;
		}
	}
	MUTEX_UNLOCK(env, renv->mtx_regenv);
	return (ret);
}
