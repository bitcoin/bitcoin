/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#define	__INCLUDE_NETWORKING	1
#include "db_int.h"
#include "dbinc/mp.h"

/*
 * The functions in this module implement a simple wire protocol for
 * transmitting messages, both replication messages and our own internal control
 * messages.  The protocol is as follows:
 *
 *      1 byte          - message type  (defined in repmgr.h)
 *      4 bytes         - size of control
 *      4 bytes         - size of rec
 *      ? bytes         - control
 *      ? bytes         - rec
 *
 * where both sizes are 32-bit binary integers in network byte order.
 * Either control or rec can have zero length, but even in this case the
 * 4-byte length will be present.
 *     Putting both lengths right up at the front allows us to read in fewer
 * phases, and allows us to allocate buffer space for both parts (plus a wrapper
 * struct) at once.
 */

/*
 * In sending a message, we first try to send it in-line, in the sending thread,
 * and without first copying the message, by using scatter/gather I/O, using
 * iovecs to point to the various pieces of the message.  If that all works
 * without blocking, that's optimal.
 *     If we find that, for a particular connection, we can't send without
 * blocking, then we must copy the message for sending later in the select()
 * thread.  In the course of doing that, we might as well "flatten" the message,
 * forming one single buffer, to simplify life.  Not only that, once we've gone
 * to the trouble of doing that, other sites to which we also want to send the
 * message (in the case of a broadcast), may as well take advantage of the
 * simplified structure also.
 *     The sending_msg structure below holds it all.  Note that this structure,
 * and the "flat_msg" structure, are allocated separately, because (1) the
 * flat_msg version is usually not needed; and (2) when a flat_msg is needed, it
 * will need to live longer than the wrapping sending_msg structure.
 *     Note that, for the broadcast case, where we're going to use this
 * repeatedly, the iovecs is a template that must be copied, since in normal use
 * the iovecs pointers and lengths get adjusted after every partial write.
 */
struct sending_msg {
	REPMGR_IOVECS iovecs;
	u_int8_t type;
	u_int32_t control_size_buf, rec_size_buf;
	REPMGR_FLAT *fmsg;
};

static int final_cleanup __P((ENV *, REPMGR_CONNECTION *, void *));
static int flatten __P((ENV *, struct sending_msg *));
static void remove_connection __P((ENV *, REPMGR_CONNECTION *));
static int __repmgr_close_connection __P((ENV *, REPMGR_CONNECTION *));
static int __repmgr_destroy_connection __P((ENV *, REPMGR_CONNECTION *));
static void setup_sending_msg
    __P((struct sending_msg *, u_int, const DBT *, const DBT *));
static int __repmgr_send_internal
    __P((ENV *, REPMGR_CONNECTION *, struct sending_msg *, int));
static int enqueue_msg
    __P((ENV *, REPMGR_CONNECTION *, struct sending_msg *, size_t));
static REPMGR_SITE *__repmgr_available_site __P((ENV *, int));

/*
 * __repmgr_send --
 *	The send function for DB_ENV->rep_set_transport.
 *
 * PUBLIC: int __repmgr_send __P((DB_ENV *, const DBT *, const DBT *,
 * PUBLIC:     const DB_LSN *, int, u_int32_t));
 */
int
__repmgr_send(dbenv, control, rec, lsnp, eid, flags)
	DB_ENV *dbenv;
	const DBT *control, *rec;
	const DB_LSN *lsnp;
	int eid;
	u_int32_t flags;
{
	DB_REP *db_rep;
	REP *rep;
	ENV *env;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *site;
	u_int available, nclients, needed, npeers_sent, nsites_sent;
	int ret, t_ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	rep = db_rep->region;
	ret = 0;

	LOCK_MUTEX(db_rep->mutex);

	/*
	 * If we're already "finished", we can't send anything.  This covers the
	 * case where a bulk buffer is flushed at env close, or perhaps an
	 * unexpected __repmgr_thread_failure.
	 */ 
	if (db_rep->finished) {
		ret = DB_REP_UNAVAIL;
		goto out;
	}
	
	/*
	 * Check whether we need to refresh our site address information with
	 * more recent updates from shared memory.
	 */
	if (rep->siteaddr_seq > db_rep->siteaddr_seq &&
	    (ret = __repmgr_sync_siteaddr(env)) != 0)
		goto out;

	if (eid == DB_EID_BROADCAST) {
		if ((ret = __repmgr_send_broadcast(env, REPMGR_REP_MESSAGE,
		    control, rec, &nsites_sent, &npeers_sent)) != 0)
			goto out;
	} else {
		DB_ASSERT(env, IS_KNOWN_REMOTE_SITE(eid));

		/*
		 * If this is a request that can be sent anywhere, then see if
		 * we can send it to our peer (to save load on the master), but
		 * not if it's a rerequest, 'cuz that likely means we tried this
		 * already and failed.
		 */
		if ((flags & (DB_REP_ANYWHERE | DB_REP_REREQUEST)) ==
		    DB_REP_ANYWHERE &&
		    IS_VALID_EID(db_rep->peer) &&
		    (site = __repmgr_available_site(env, db_rep->peer)) !=
		    NULL) {
			RPRINT(env, DB_VERB_REPMGR_MISC,
			    (env, "sending request to peer"));
		} else if ((site = __repmgr_available_site(env, eid)) ==
		    NULL) {
			RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			    "ignoring message sent to unavailable site"));
			ret = DB_REP_UNAVAIL;
			goto out;
		}

		conn = site->ref.conn;
		/* Pass the "blockable" argument as TRUE. */
		if ((ret = __repmgr_send_one(env, conn, REPMGR_REP_MESSAGE,
		    control, rec, TRUE)) == DB_REP_UNAVAIL &&
		    (t_ret = __repmgr_bust_connection(env, conn)) != 0)
			ret = t_ret;
		if (ret != 0)
			goto out;

		nsites_sent = 1;
		npeers_sent = site->priority > 0 ? 1 : 0;
	}
	/*
	 * Right now, nsites and npeers represent the (maximum) number of sites
	 * we've attempted to begin sending the message to.  Of course we
	 * haven't really received any ack's yet.  But since we've only sent to
	 * nsites/npeers other sites, that's the maximum number of ack's we
	 * could possibly expect.  If even that number fails to satisfy our PERM
	 * policy, there's no point waiting for something that will never
	 * happen.
	 */
	if (LF_ISSET(DB_REP_PERMANENT)) {
		/* Number of sites in the group besides myself. */
		nclients = __repmgr_get_nsites(db_rep) - 1;

		switch (db_rep->perm_policy) {
		case DB_REPMGR_ACKS_NONE:
			needed = 0;
			COMPQUIET(available, 0);
			break;

		case DB_REPMGR_ACKS_ONE:
			needed = 1;
			available = nsites_sent;
			break;

		case DB_REPMGR_ACKS_ALL:
			/* Number of sites in the group besides myself. */
			needed = nclients;
			available = nsites_sent;
			break;

		case DB_REPMGR_ACKS_ONE_PEER:
			needed = 1;
			available = npeers_sent;
			break;

		case DB_REPMGR_ACKS_ALL_PEERS:
			/*
			 * Too hard to figure out "needed", since we're not
			 * keeping track of how many peers we have; so just skip
			 * the optimization in this case.
			 */
			needed = 1;
			available = npeers_sent;
			break;

		case DB_REPMGR_ACKS_QUORUM:
			/*
			 * The minimum number of acks necessary to ensure that
			 * the transaction is durable if an election is held.
			 * (See note below at __repmgr_is_permanent, regarding
			 * the surprising inter-relationship between
			 * 2SITE_STRICT and QUORUM.)
			 */
			if (nclients > 1 ||
			    FLD_ISSET(db_rep->region->config,
			    REP_C_2SITE_STRICT))
				needed = nclients / 2;
			else
				needed = 1;
			available = npeers_sent;
			break;

		default:
			COMPQUIET(available, 0);
			COMPQUIET(needed, 0);
			(void)__db_unknown_path(env, "__repmgr_send");
			break;
		}
		if (needed == 0)
			goto out;
		if (available < needed) {
			ret = DB_REP_UNAVAIL;
			goto out;
		}
		/* In ALL_PEERS case, display of "needed" might be confusing. */
		RPRINT(env, DB_VERB_REPMGR_MISC, (env,
		    "will await acknowledgement: need %u", needed));
		ret = __repmgr_await_ack(env, lsnp);
	}

out:	UNLOCK_MUTEX(db_rep->mutex);
	if (ret != 0 && LF_ISSET(DB_REP_PERMANENT)) {
		STAT(db_rep->region->mstat.st_perm_failed++);
		DB_EVENT(env, DB_EVENT_REP_PERM_FAILED, NULL);
	}
	return (ret);
}

static REPMGR_SITE *
__repmgr_available_site(env, eid)
	ENV *env;
	int eid;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;

	db_rep = env->rep_handle;
	site = SITE_FROM_EID(eid);
	if (site->state != SITE_CONNECTED)
		return (NULL);

	if (site->ref.conn->state == CONN_READY)
		return (site);
	return (NULL);
}

/*
 * Synchronize our list of sites with new information that has been added to the
 * list in the shared region.
 *
 * PUBLIC: int __repmgr_sync_siteaddr __P((ENV *));
 */
int
__repmgr_sync_siteaddr(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	char *host;
	u_int added;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	ret = 0;

	MUTEX_LOCK(env, rep->mtx_repmgr);

	if (db_rep->my_addr.host == NULL && rep->my_addr.host != INVALID_ROFF) {
		host = R_ADDR(env->reginfo, rep->my_addr.host);
		if ((ret = __repmgr_pack_netaddr(env,
		    host, rep->my_addr.port, NULL, &db_rep->my_addr)) != 0)
			goto out;
	}

	added = db_rep->site_cnt;
	if ((ret = __repmgr_copy_in_added_sites(env)) == 0)
		ret = __repmgr_init_new_sites(env, added, db_rep->site_cnt);

out:
	MUTEX_UNLOCK(env, rep->mtx_repmgr);
	return (ret);
}

/*
 * Sends message to all sites with which we currently have an active
 * connection.  Sets result parameters according to how many sites we attempted
 * to begin sending to, even if we did nothing more than queue it for later
 * delivery.
 *
 * !!!
 * Caller must hold env->mutex.
 * PUBLIC: int __repmgr_send_broadcast __P((ENV *, u_int,
 * PUBLIC:    const DBT *, const DBT *, u_int *, u_int *));
 */
int
__repmgr_send_broadcast(env, type, control, rec, nsitesp, npeersp)
	ENV *env;
	u_int type;
	const DBT *control, *rec;
	u_int *nsitesp, *npeersp;
{
	DB_REP *db_rep;
	struct sending_msg msg;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *site;
	u_int eid, nsites, npeers;
	int ret;

	static const u_int version_max_msg_type[] = {
		0,
		REPMGR_MAX_V1_MSG_TYPE,
		REPMGR_MAX_V2_MSG_TYPE,
		REPMGR_MAX_V3_MSG_TYPE
	};

	db_rep = env->rep_handle;

	/*
	 * Sending a broadcast is quick, because we allow no blocking.  So it
	 * shouldn't much matter.  But just in case, take the timestamp before
	 * sending, so that if anything we err on the side of keeping clients
	 * placated (i.e., possibly sending a heartbeat slightly more frequently
	 * than necessary).
	 */
	__os_gettime(env, &db_rep->last_bcast, 1);

	setup_sending_msg(&msg, type, control, rec);
	nsites = npeers = 0;

	/* Send to (only the main connection with) every site. */
	for (eid = 0; eid < db_rep->site_cnt; eid++) {
		if ((site = __repmgr_available_site(env, (int)eid)) == NULL)
			continue;
		conn = site->ref.conn;

		DB_ASSERT(env, IS_VALID_EID(conn->eid) &&
		    conn->version > 0 &&
		    conn->version <= DB_REPMGR_VERSION);

		/*
		 * Skip if the type of message we're sending is beyond the range
		 * of known message types for this connection's version.
		 *
		 * !!!
		 * Don't be misled by the apparent generality of this simple
		 * test.  It works currently, because the only kinds of messages
		 * that we broadcast are REP_MESSAGE and HEARTBEAT.  But in the
		 * future other kinds of messages might require more intricate
		 * per-connection-version customization (for example,
		 * per-version message format conversion, addition of new
		 * fields, etc.).
		 */
		if (type > version_max_msg_type[conn->version])
			continue;

		/*
		 * Broadcast messages are either application threads committing
		 * transactions, or replication status message that we can
		 * afford to lose.  So don't allow blocking for them (pass
		 * "blockable" argument as FALSE).
		 */
		if ((ret = __repmgr_send_internal(env,
		    conn, &msg, FALSE)) == 0) {
			site = SITE_FROM_EID(conn->eid);
			nsites++;
			if (site->priority > 0)
				npeers++;
		} else if (ret == DB_REP_UNAVAIL) {
			if ((ret = __repmgr_bust_connection(env, conn)) != 0)
				return (ret);
		} else
			return (ret);
	}

	*nsitesp = nsites;
	*npeersp = npeers;
	return (0);
}

/*
 * __repmgr_send_one --
 *	Send a message to a site, or if you can't just yet, make a copy of it
 * and arrange to have it sent later.  'rec' may be NULL, in which case we send
 * a zero length and no data.
 *
 * If we get an error, we take care of cleaning up the connection (calling
 * __repmgr_bust_connection()), so that the caller needn't do so.
 *
 * !!!
 * Note that the mutex should be held through this call.
 * It doubles as a synchronizer to make sure that two threads don't
 * intersperse writes that are part of two single messages.
 *
 * PUBLIC: int __repmgr_send_one __P((ENV *, REPMGR_CONNECTION *,
 * PUBLIC:    u_int, const DBT *, const DBT *, int));
 */
int
__repmgr_send_one(env, conn, msg_type, control, rec, blockable)
	ENV *env;
	REPMGR_CONNECTION *conn;
	u_int msg_type;
	const DBT *control, *rec;
	int blockable;
{
	struct sending_msg msg;

	setup_sending_msg(&msg, msg_type, control, rec);
	return (__repmgr_send_internal(env, conn, &msg, blockable));
}

/*
 * Attempts a "best effort" to send a message on the given site.  If there is an
 * excessive backlog of message already queued on the connection, what shall we
 * do?  If the caller doesn't mind blocking, we'll wait (a limited amount of
 * time) for the queue to drain.  Otherwise we'll simply drop the message.  This
 * is always allowed by the replication protocol.  But in the case of a
 * multi-message response to a request like PAGE_REQ, LOG_REQ or ALL_REQ we
 * almost always get a flood of messages that instantly fills our queue, so
 * blocking improves performance (by avoiding the need for the client to
 * re-request).
 *
 * How long shall we wait?  We could of course create a new timeout
 * configuration type, so that the application could set it directly.  But that
 * would start to overwhelm the user with too many choices to think about.  We
 * already have an ACK timeout, which is the user's estimate of how long it
 * should take to send a message to the client, have it be processed, and return
 * a message back to us.  We multiply that by the queue size, because that's how
 * many messages have to be swallowed up by the client before we're able to
 * start sending again (at least to a rough approximation).
 */
static int
__repmgr_send_internal(env, conn, msg, blockable)
	ENV *env;
	REPMGR_CONNECTION *conn;
	struct sending_msg *msg;
	int blockable;
{
	DB_REP *db_rep;
	REPMGR_IOVECS iovecs;
	SITE_STRING_BUFFER buffer;
	db_timeout_t drain_to;
	int ret;
	size_t nw;
	size_t total_written;

	db_rep = env->rep_handle;

	DB_ASSERT(env,
	    conn->state != CONN_CONNECTING && conn->state != CONN_DEFUNCT);
	if (!STAILQ_EMPTY(&conn->outbound_queue)) {
		/*
		 * Output to this site is currently owned by the select()
		 * thread, so we can't try sending in-line here.  We can only
		 * queue the msg for later.
		 */
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "msg to %s to be queued",
		    __repmgr_format_eid_loc(env->rep_handle,
		    conn->eid, buffer)));
		if (conn->out_queue_length >= OUT_QUEUE_LIMIT &&
		    blockable && conn->state != CONN_CONGESTED) {
			RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			    "block msg thread, await queue space"));

			if ((drain_to = db_rep->ack_timeout) == 0)
				drain_to = DB_REPMGR_DEFAULT_ACK_TIMEOUT;
			RPRINT(env, DB_VERB_REPMGR_MISC,
			    (env, "will await drain"));
			conn->blockers++;
			ret = __repmgr_await_drain(env,
			    conn, drain_to * OUT_QUEUE_LIMIT);
			conn->blockers--;
			RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			    "drain returned %d (%d,%d)", ret,
			    db_rep->finished, conn->out_queue_length));
			if (db_rep->finished)
				return (DB_TIMEOUT);
			if (ret != 0)
				return (ret);
			if (STAILQ_EMPTY(&conn->outbound_queue))
				goto empty;
		}
		if (conn->out_queue_length < OUT_QUEUE_LIMIT)
			return (enqueue_msg(env, conn, msg, 0));
		else {
			RPRINT(env, DB_VERB_REPMGR_MISC,
			    (env, "queue limit exceeded"));
			STAT(env->rep_handle->
			    region->mstat.st_msgs_dropped++);
			return (blockable ? DB_TIMEOUT : 0);
		}
	}
empty:

	/*
	 * Send as much data to the site as we can, without blocking.  Keep
	 * writing as long as we're making some progress.  Make a scratch copy
	 * of iovecs for our use, since we destroy it in the process of
	 * adjusting pointers after each partial I/O.
	 */
	memcpy(&iovecs, &msg->iovecs, sizeof(iovecs));
	total_written = 0;
	while ((ret = __repmgr_writev(conn->fd, &iovecs.vectors[iovecs.offset],
	    iovecs.count-iovecs.offset, &nw)) == 0) {
		total_written += nw;
		if (__repmgr_update_consumed(&iovecs, nw)) /* all written */
			return (0);
	}

	if (ret != WOULDBLOCK) {
#ifdef EBADF
		DB_ASSERT(env, ret != EBADF);
#endif
		__db_err(env, ret, "socket writing failure");
		STAT(env->rep_handle->region->mstat.st_connection_drop++);
		return (DB_REP_UNAVAIL);
	}

	RPRINT(env, DB_VERB_REPMGR_MISC, (env, "wrote only %lu bytes to %s",
	    (u_long)total_written,
	    __repmgr_format_eid_loc(env->rep_handle, conn->eid, buffer)));
	/*
	 * We can't send any more without blocking: queue (a pointer to) a
	 * "flattened" copy of the message, so that the select() thread will
	 * finish sending it later.
	 */
	if ((ret = enqueue_msg(env, conn, msg, total_written)) != 0)
		return (ret);

	STAT(env->rep_handle->region->mstat.st_msgs_queued++);

	/*
	 * Wake the main select thread so that it can discover that it has
	 * received ownership of this connection.  Note that we didn't have to
	 * do this in the previous case (above), because the non-empty queue
	 * implies that the select() thread is already managing ownership of
	 * this connection.
	 */
#ifdef DB_WIN32
	if (WSAEventSelect(conn->fd, conn->event_object,
	    FD_READ|FD_WRITE|FD_CLOSE) == SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "can't add FD_WRITE event bit");
		return (ret);
	}
#endif
	return (__repmgr_wake_main_thread(env));
}

/*
 * PUBLIC: int __repmgr_is_permanent __P((ENV *, const DB_LSN *));
 *
 * Count up how many sites have ack'ed the given LSN.  Returns TRUE if enough
 * sites have ack'ed; FALSE otherwise.
 *
 * !!!
 * Caller must hold the mutex.
 */
int
__repmgr_is_permanent(env, lsnp)
	ENV *env;
	const DB_LSN *lsnp;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	u_int eid, nsites, npeers;
	int is_perm, has_missing_peer;

	db_rep = env->rep_handle;

	if (db_rep->perm_policy == DB_REPMGR_ACKS_NONE)
		return (TRUE);

	nsites = npeers = 0;
	has_missing_peer = FALSE;
	for (eid = 0; eid < db_rep->site_cnt; eid++) {
		site = SITE_FROM_EID(eid);
		if (!F_ISSET(site, SITE_HAS_PRIO)) {
			/*
			 * Never connected to this site: since we can't know
			 * whether it's a peer, assume the worst.
			 */
			has_missing_peer = TRUE;
			continue;
		}

		if (LOG_COMPARE(&site->max_ack, lsnp) >= 0) {
			nsites++;
			if (site->priority > 0)
				npeers++;
		} else {
			/* This site hasn't ack'ed the message. */
			if (site->priority > 0)
				has_missing_peer = TRUE;
		}
	}

	switch (db_rep->perm_policy) {
	case DB_REPMGR_ACKS_ONE:
		is_perm = (nsites >= 1);
		break;
	case DB_REPMGR_ACKS_ONE_PEER:
		is_perm = (npeers >= 1);
		break;
	case DB_REPMGR_ACKS_QUORUM:
		/*
		 * The minimum number of acks necessary to ensure that the
		 * transaction is durable if an election is held (given that we
		 * always conduct elections according to the standard,
		 * recommended practice of requiring votes from a majority of
		 * sites).
		 */
		if (__repmgr_get_nsites(db_rep) == 2 &&
		    !FLD_ISSET(db_rep->region->config, REP_C_2SITE_STRICT)) {
			/*
			 * Unless instructed otherwise, our special handling for
			 * 2-site groups means that a client that loses contact
			 * with the master elects itself master (even though
			 * that doesn't constitute a majority).  In order to
			 * provide the expected guarantee implied by the
			 * definition of "quorum" we have to fudge the ack
			 * calculation in this case: specifically, we need to
			 * make sure that the client has received it in order
			 * for us to consider it "perm".
			 *
			 * Note that turning the usual strict behavior back on
			 * in a 2-site group results in "0" as the number of
			 * clients needed to ack a txn in order for it to have
			 * arrived at a quorum.  This is the correct result,
			 * strange as it may seem!  This may well mean that in a
			 * 2-site group the QUORUM policy is rarely the right
			 * choice.
			 */
			is_perm = (npeers >= 1);
		} else
			is_perm = (npeers >= (__repmgr_get_nsites(db_rep)-1)/2);
		break;
	case DB_REPMGR_ACKS_ALL:
		/* Adjust by 1, since get_nsites includes local site. */
		is_perm = (nsites >= __repmgr_get_nsites(db_rep) - 1);
		break;
	case DB_REPMGR_ACKS_ALL_PEERS:
		if (db_rep->site_cnt < __repmgr_get_nsites(db_rep) - 1) {
			/* Assume missing site might be a peer. */
			has_missing_peer = TRUE;
		}
		is_perm = !has_missing_peer;
		break;
	default:
		is_perm = FALSE;
		(void)__db_unknown_path(env, "__repmgr_is_permanent");
	}
	return (is_perm);
}

/*
 * Abandons a connection, to recover from an error.  Takes necessary recovery
 * action.  Note that we don't actually close and clean up the connection here;
 * that happens later, in the select() thread main loop.  See further
 * explanation at function __repmgr_disable_connection().
 *
 * PUBLIC: int __repmgr_bust_connection __P((ENV *, REPMGR_CONNECTION *));
 *
 * !!!
 * Caller holds mutex.
 */
int
__repmgr_bust_connection(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	int connecting, ret, subordinate_conn, eid;

	db_rep = env->rep_handle;
	ret = 0;

	eid = conn->eid;
	connecting = (conn->state == CONN_CONNECTING);

	__repmgr_disable_connection(env, conn);

	/*
	 * Any sort of connection, in any active state, could produce an error.
	 * But when we're done DEFUNCT-ifying it here it should end up on the
	 * orphans list.  So, move it if it's not already there.
	 */
	if (IS_VALID_EID(eid)) {
		site = SITE_FROM_EID(eid);
		subordinate_conn = (conn != site->ref.conn);

		/* Note: schedule_connection_attempt wakes the main thread. */
		if (!subordinate_conn &&
		    (ret = __repmgr_schedule_connection_attempt(env,
		    (u_int)eid, FALSE)) != 0)
			return (ret);

		/*
		 * If the failed connection was the one between us and the
		 * master, assume that the master may have failed, and call for
		 * an election.  But only do this for the connection to the main
		 * master process, not a subordinate one.  And only do it if
		 * we're our site's main process, not a subordinate one.  And
		 * only do it if the connection had managed to progress beyond
		 * the "connecting" state, because otherwise it was just a
		 * reconnection attempt that may have found the site unreachable
		 * or the master process not running.
		 */
		if (!IS_SUBORDINATE(db_rep) && !subordinate_conn &&
		    !connecting && eid == db_rep->master_eid &&
		    (ret = __repmgr_init_election(
		    env, ELECT_FAILURE_ELECTION)) != 0)
			return (ret);
	} else {
		/*
		 * The connection was not marked with a valid EID, so we know it
		 * must have been an incoming connection in the very early
		 * stages.  Obviously it's correct for us to avoid the
		 * site-specific recovery steps above.  But even if we have just
		 * learned which site the connection came from, and are killing
		 * it because it's redundant, it means we already have a
		 * perfectly fine connection, and so -- again -- it makes sense
		 * for us to be skipping scheduling a reconnection, and checking
		 * for a master crash.
		 *
		 * One way or another, make sure the main thread is poked, so
		 * that we do the deferred clean-up.
		 */
		ret = __repmgr_wake_main_thread(env);
	}
	return (ret);
}

/*
 * Remove a connection from the possibility of any further activity, making sure
 * it ends up on the main connections list, so that it will be cleaned up at the
 * next opportunity in the select() thread.
 *
 * Various threads write onto TCP/IP sockets, and an I/O error could occur at
 * any time.  However, only the dedicated "select()" thread may close the socket
 * file descriptor, because under POSIX we have to drop our mutex and then call
 * select() as two distinct (non-atomic) operations.
 *
 * To simplify matters, there is a single place in the select thread where we
 * close and clean up after any defunct connection.  Even if the I/O error
 * happens in the select thread we follow this convention.
 *
 * When an error occurs, we disable the connection (mark it defunct so that no
 * one else will try to use it, and so that the select thread will find it and
 * clean it up), and then usually take some additional recovery action: schedule
 * a connection retry for later, and possibly call for an election if it was a
 * connection to the master.  (This happens in the function
 * __repmgr_bust_connection.)  But sometimes we don't want to do the recovery
 * part; just the disabling part.
 *
 * PUBLIC: void __repmgr_disable_connection __P((ENV *, REPMGR_CONNECTION *));
 */
void
__repmgr_disable_connection(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	int eid;

	db_rep = env->rep_handle;
	eid = conn->eid;

	if (IS_VALID_EID(eid)) {
		site = SITE_FROM_EID(eid);
		if (conn != site->ref.conn)
			/* It's a subordinate connection. */
			TAILQ_REMOVE(&site->sub_conns, conn, entries);
		TAILQ_INSERT_TAIL(&db_rep->connections, conn, entries);
	}

	conn->state = CONN_DEFUNCT;
	conn->eid = -1;
}

/*
 * PUBLIC: int __repmgr_cleanup_connection __P((ENV *, REPMGR_CONNECTION *));
 *
 * !!!
 * Idempotent.  This can be called repeatedly as blocking message threads (of
 * which there could be multiples) wake up in case of error on the connection.
 */
int
__repmgr_cleanup_connection(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	int ret;

	db_rep = env->rep_handle;

	if ((ret = __repmgr_close_connection(env, conn)) != 0)
		goto out;

	/*
	 * If there's a blocked message thread waiting, we mustn't yank the
	 * connection struct out from under it.  Instead, just wake it up.
	 * We'll get another chance to come back through here soon.
	 */
	if (conn->blockers > 0) {
		ret = __repmgr_signal(&conn->drained);
		goto out;
	}

	DB_ASSERT(env, !IS_VALID_EID(conn->eid) && conn->state == CONN_DEFUNCT);
	TAILQ_REMOVE(&db_rep->connections, conn, entries);
	ret = __repmgr_destroy_connection(env, conn);

out:
	return (ret);
}

static void
remove_connection(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;

	db_rep = env->rep_handle;
	if (IS_VALID_EID(conn->eid)) {
		site = SITE_FROM_EID(conn->eid);

		if (site->state == SITE_CONNECTED && conn == site->ref.conn) {
			/* Not on any list, so no need to do anything. */
		} else
			TAILQ_REMOVE(&site->sub_conns, conn, entries);
	} else
		TAILQ_REMOVE(&db_rep->connections, conn, entries);
}

static int
__repmgr_close_connection(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	int ret;

	DB_ASSERT(env,
	    conn->state == CONN_DEFUNCT || env->rep_handle->finished);

	ret = 0;
	if (conn->fd != INVALID_SOCKET) {
		ret = closesocket(conn->fd);
		conn->fd = INVALID_SOCKET;
		if (ret == SOCKET_ERROR) {
			ret = net_errno;
			__db_err(env, ret, "closing socket");
		}
#ifdef DB_WIN32
		if (!WSACloseEvent(conn->event_object) && ret == 0)
			ret = net_errno;
#endif
	}
	return (ret);
}

static int
__repmgr_destroy_connection(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	QUEUED_OUTPUT *out;
	REPMGR_FLAT *msg;
	DBT *dbt;
	int ret;

	/*
	 * Deallocate any input and output buffers we may have.
	 */
	if (conn->reading_phase == DATA_PHASE) {
		if (conn->msg_type == REPMGR_REP_MESSAGE)
			__os_free(env, conn->input.rep_message);
		else {
			dbt = &conn->input.repmgr_msg.cntrl;
			if (dbt->size > 0)
				__os_free(env, dbt->data);
			dbt = &conn->input.repmgr_msg.rec;
			if (dbt->size > 0)
				__os_free(env, dbt->data);
		}
	}
	while (!STAILQ_EMPTY(&conn->outbound_queue)) {
		out = STAILQ_FIRST(&conn->outbound_queue);
		STAILQ_REMOVE_HEAD(&conn->outbound_queue, entries);
		msg = out->msg;
		if (--msg->ref_count <= 0)
			__os_free(env, msg);
		__os_free(env, out);
	}

	ret = __repmgr_free_cond(&conn->drained);
	__os_free(env, conn);
	return (ret);
}

static int
enqueue_msg(env, conn, msg, offset)
	ENV *env;
	REPMGR_CONNECTION *conn;
	struct sending_msg *msg;
	size_t offset;
{
	QUEUED_OUTPUT *q_element;
	int ret;

	if (msg->fmsg == NULL && ((ret = flatten(env, msg)) != 0))
		return (ret);
	if ((ret = __os_malloc(env, sizeof(QUEUED_OUTPUT), &q_element)) != 0)
		return (ret);
	q_element->msg = msg->fmsg;
	msg->fmsg->ref_count++;	/* encapsulation would be sweeter */
	q_element->offset = offset;

	/* Put it on the connection's outbound queue. */
	STAILQ_INSERT_TAIL(&conn->outbound_queue, q_element, entries);
	conn->out_queue_length++;
	return (0);
}

/*
 * Either "control" or "rec" (or both) may be NULL, in which case we treat it
 * like a zero-length DBT.
 */
static void
setup_sending_msg(msg, type, control, rec)
	struct sending_msg *msg;
	u_int type;
	const DBT *control, *rec;
{
	u_int32_t control_size, rec_size;

	/*
	 * The wire protocol is documented in a comment at the top of this
	 * module.
	 */
	__repmgr_iovec_init(&msg->iovecs);
	msg->type = type;
	__repmgr_add_buffer(&msg->iovecs, &msg->type, sizeof(msg->type));

	control_size = control == NULL ? 0 : control->size;
	msg->control_size_buf = htonl(control_size);
	__repmgr_add_buffer(&msg->iovecs,
	    &msg->control_size_buf, sizeof(msg->control_size_buf));

	rec_size = rec == NULL ? 0 : rec->size;
	msg->rec_size_buf = htonl(rec_size);
	__repmgr_add_buffer(
	    &msg->iovecs, &msg->rec_size_buf, sizeof(msg->rec_size_buf));

	if (control->size > 0)
		__repmgr_add_dbt(&msg->iovecs, control);

	if (rec_size > 0)
		__repmgr_add_dbt(&msg->iovecs, rec);

	msg->fmsg = NULL;
}

/*
 * Convert a message stored as iovec pointers to various pieces, into flattened
 * form, by copying all the pieces, and then make the iovec just point to the
 * new simplified form.
 */
static int
flatten(env, msg)
	ENV *env;
	struct sending_msg *msg;
{
	u_int8_t *p;
	size_t msg_size;
	int i, ret;

	DB_ASSERT(env, msg->fmsg == NULL);

	msg_size = msg->iovecs.total_bytes;
	if ((ret = __os_malloc(env, sizeof(*msg->fmsg) + msg_size,
	    &msg->fmsg)) != 0)
		return (ret);
	msg->fmsg->length = msg_size;
	msg->fmsg->ref_count = 0;
	p = &msg->fmsg->data[0];

	for (i = 0; i < msg->iovecs.count; i++) {
		memcpy(p, msg->iovecs.vectors[i].iov_base,
		    msg->iovecs.vectors[i].iov_len);
		p = &p[msg->iovecs.vectors[i].iov_len];
	}
	__repmgr_iovec_init(&msg->iovecs);
	__repmgr_add_buffer(&msg->iovecs, &msg->fmsg->data[0], msg_size);
	return (0);
}

/*
 * PUBLIC: REPMGR_SITE *__repmgr_find_site __P((ENV *, const char *, u_int));
 */
REPMGR_SITE *
__repmgr_find_site(env, host, port)
	ENV *env;
	const char *host;
	u_int port;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	u_int i;

	db_rep = env->rep_handle;
	for (i = 0; i < db_rep->site_cnt; i++) {
		site = &db_rep->sites[i];

		if (strcmp(site->net_addr.host, host) == 0 &&
		    site->net_addr.port == port)
			return (site);
	}

	return (NULL);
}

/*
 * Initialize the fields of a (given) netaddr structure, with the given values.
 * We copy the host name, but take ownership of the ADDRINFO buffer.
 *
 * All inputs are assumed to have been already validated.
 *
 * PUBLIC: int __repmgr_pack_netaddr __P((ENV *, const char *,
 * PUBLIC:     u_int, ADDRINFO *, repmgr_netaddr_t *));
 */
int
__repmgr_pack_netaddr(env, host, port, list, addr)
	ENV *env;
	const char *host;
	u_int port;
	ADDRINFO *list;
	repmgr_netaddr_t *addr;
{
	int ret;

	DB_ASSERT(env, host != NULL);

	if ((ret = __os_strdup(env, host, &addr->host)) != 0)
		return (ret);
	addr->port = (u_int16_t)port;
	addr->address_list = list;
	addr->current = NULL;
	return (0);
}

/*
 * PUBLIC: int __repmgr_getaddr __P((ENV *,
 * PUBLIC:     const char *, u_int, int, ADDRINFO **));
 */
int
__repmgr_getaddr(env, host, port, flags, result)
	ENV *env;
	const char *host;
	u_int port;
	int flags;    /* Matches struct addrinfo declaration. */
	ADDRINFO **result;
{
	ADDRINFO *answer, hints;
	char buffer[10];		/* 2**16 fits in 5 digits. */

	/*
	 * Ports are really 16-bit unsigned values, but it's too painful to
	 * push that type through the API.
	 */
	if (port > UINT16_MAX) {
		__db_errx(env, "port %u larger than max port %u",
		    port, UINT16_MAX);
		return (EINVAL);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = flags;
	(void)snprintf(buffer, sizeof(buffer), "%u", port);

	/*
	 * Although it's generally bad to discard error information, the return
	 * code from __os_getaddrinfo is undependable.  Our callers at least
	 * would like to be able to distinguish errors in getaddrinfo (which we
	 * want to consider to be re-tryable), from other failure (e.g., EINVAL,
	 * above).
	 */
	if (__os_getaddrinfo(env, host, port, buffer, &hints, &answer) != 0)
		return (DB_REP_UNAVAIL);
	*result = answer;

	return (0);
}

/*
 * Adds a new site to our array of known sites (unless it already exists),
 * and schedules it for immediate connection attempt.  Whether it exists or not,
 * we set newsitep, either to the already existing site, or to the newly created
 * site.  Unless newsitep is passed in as NULL, which is allowed.
 *
 * PUBLIC: int __repmgr_add_site
 * PUBLIC:     __P((ENV *, const char *, u_int, REPMGR_SITE **, u_int32_t));
 *
 * !!!
 * Caller is expected to hold db_rep->mutex on entry.
 */
int
__repmgr_add_site(env, host, port, sitep, flags)
	ENV *env;
	const char *host;
	u_int port;
	REPMGR_SITE **sitep;
	u_int32_t flags;
{
	int peer, state;

	state = SITE_IDLE;
	peer = LF_ISSET(DB_REPMGR_PEER);
	return (__repmgr_add_site_int(env, host, port, sitep, peer, state));
}

/*
 * PUBLIC: int __repmgr_add_site_int
 * PUBLIC:     __P((ENV *, const char *, u_int, REPMGR_SITE **, int, int));
 */
int
__repmgr_add_site_int(env, host, port, sitep, peer, state)
	ENV *env;
	const char *host;
	u_int port;
	REPMGR_SITE **sitep;
	int peer, state;
{
	DB_REP *db_rep;
	REP *rep;
	DB_THREAD_INFO *ip;
	REPMGR_SITE *site;
	u_int base;
	int eid, locked, pre_exist, ret, t_ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	COMPQUIET(site, NULL);
	COMPQUIET(pre_exist, 0);
	eid = DB_EID_INVALID;

	/* Make sure we're up to date before adding to our local list. */
	ENV_ENTER(env, ip);
	MUTEX_LOCK(env, rep->mtx_repmgr);
	locked = TRUE;
	base = db_rep->site_cnt;
	if ((ret = __repmgr_copy_in_added_sites(env)) != 0)
		goto out;

	/* Once we're this far, we're committed to doing init_new_sites. */

	/* If it's still not found, now it's safe to add it. */
	if ((site = __repmgr_find_site(env, host, port)) == NULL) {
		pre_exist = FALSE;

		/*
		 * Store both locally and in shared region.
		 */
		if ((ret = __repmgr_new_site(env,
		    &site, host, port, state)) != 0)
			goto init;
		eid = EID_FROM_SITE(site);
		DB_ASSERT(env, (u_int)eid == db_rep->site_cnt - 1);

		if ((ret = __repmgr_share_netaddrs(env,
		    rep, (u_int)eid, db_rep->site_cnt)) != 0) {
			/*
			 * Rescind the added local slot.
			 */
			db_rep->site_cnt--;
			__repmgr_cleanup_netaddr(env, &site->net_addr);
		}
	} else {
		pre_exist = TRUE;
		eid = EID_FROM_SITE(site);
	}

	/*
	 * Bump sequence count explicitly, to cover the pre-exist case.  In the
	 * other case it's wasted, but that's not worth worrying about.
	 */
	if (peer) {
		db_rep->peer = rep->peer = EID_FROM_SITE(site);
		db_rep->siteaddr_seq = ++rep->siteaddr_seq;
	}

init:
	MUTEX_UNLOCK(env, rep->mtx_repmgr);
	locked = FALSE;

	/*
	 * Initialize all new sites (including the ones we snarfed via
	 * copy_in_added_sites), even if it doesn't include a pre_existing one.
	 * But if the new one is already connected, it doesn't need this
	 * initialization, so skip over that one (which we accomplish by making
	 * two calls with sub-ranges).
	 */
	if (state != SITE_CONNECTED || eid == DB_EID_INVALID)
		t_ret = __repmgr_init_new_sites(env, base, db_rep->site_cnt);
	else
		if ((t_ret = __repmgr_init_new_sites(env,
		    base, (u_int)eid)) == 0)
			t_ret = __repmgr_init_new_sites(env,
			    (u_int)(eid+1), db_rep->site_cnt);
	if (t_ret != 0 && ret == 0)
		ret = t_ret;

out:
	if (locked)
		MUTEX_UNLOCK(env, rep->mtx_repmgr);
	ENV_LEAVE(env, ip);
	if (ret == 0) {
		if (sitep != NULL)
			*sitep = site;
		if (pre_exist)
			ret = EEXIST;
	}
	return (ret);
}

/*
 * Initialize a socket for listening.  Sets a file descriptor for the socket,
 * ready for an accept() call in a thread that we're happy to let block.
 *
 * PUBLIC:  int __repmgr_listen __P((ENV *));
 */
int
__repmgr_listen(env)
	ENV *env;
{
	ADDRINFO *ai;
	DB_REP *db_rep;
	char *why;
	int sockopt, ret;
	socket_t s;

	db_rep = env->rep_handle;

	/* Use OOB value as sentinel to show no socket open. */
	s = INVALID_SOCKET;

	if ((ai = ADDR_LIST_FIRST(&db_rep->my_addr)) == NULL) {
		if ((ret = __repmgr_getaddr(env, db_rep->my_addr.host,
		    db_rep->my_addr.port, AI_PASSIVE, &ai)) == 0)
			ADDR_LIST_INIT(&db_rep->my_addr, ai);
		else
			return (ret);
	}

	/*
	 * Given the assert is correct, we execute the loop at least once, which
	 * means 'why' will have been set by the time it's needed.  But of
	 * course lint doesn't know about DB_ASSERT.
	 */
	COMPQUIET(why, "");
	DB_ASSERT(env, ai != NULL);
	for (; ai != NULL; ai = ADDR_LIST_NEXT(&db_rep->my_addr)) {

		if ((s = socket(ai->ai_family,
		    ai->ai_socktype, ai->ai_protocol)) == INVALID_SOCKET) {
			why = "can't create listen socket";
			continue;
		}

		/*
		 * When testing, it's common to kill and restart regularly.  On
		 * some systems, this causes bind to fail with "address in use"
		 * errors unless this option is set.
		 */
		sockopt = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (sockopt_t)&sockopt,
		    sizeof(sockopt)) != 0) {
			why = "can't set REUSEADDR socket option";
			break;
		}

		if (bind(s, ai->ai_addr, (socklen_t)ai->ai_addrlen) != 0) {
			why = "can't bind socket to listening address";
			(void)closesocket(s);
			s = INVALID_SOCKET;
			continue;
		}

		if (listen(s, 5) != 0) {
			why = "listen()";
			break;
		}

		if ((ret = __repmgr_set_nonblocking(s)) != 0) {
			__db_err(env, ret, "can't unblock listen socket");
			goto clean;
		}

		db_rep->listen_fd = s;
		return (0);
	}

	ret = net_errno;
	__db_err(env, ret, why);
clean:	if (s != INVALID_SOCKET)
		(void)closesocket(s);
	return (ret);
}

/*
 * PUBLIC: int __repmgr_net_close __P((ENV *));
 */
int
__repmgr_net_close(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	ret = __repmgr_each_connection(env, final_cleanup, NULL, FALSE);

	if (db_rep->listen_fd != INVALID_SOCKET) {
		if (closesocket(db_rep->listen_fd) == SOCKET_ERROR && ret == 0)
			ret = net_errno;
		db_rep->listen_fd = INVALID_SOCKET;
		rep->listener = 0;
	}
	return (ret);
}

static int
final_cleanup(env, conn, unused)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *unused;
{
	int ret, t_ret;

	COMPQUIET(unused, NULL);

	remove_connection(env, conn);
	ret = __repmgr_close_connection(env, conn);
	if ((t_ret = __repmgr_destroy_connection(env, conn)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * PUBLIC: void __repmgr_net_destroy __P((ENV *, DB_REP *));
 */
void
__repmgr_net_destroy(env, db_rep)
	ENV *env;
	DB_REP *db_rep;
{
	REPMGR_RETRY *retry;
	REPMGR_SITE *site;
	u_int i;

	__repmgr_cleanup_netaddr(env, &db_rep->my_addr);

	if (db_rep->sites == NULL)
		return;

	while (!TAILQ_EMPTY(&db_rep->retries)) {
		retry = TAILQ_FIRST(&db_rep->retries);
		TAILQ_REMOVE(&db_rep->retries, retry, entries);
		__os_free(env, retry);
	}

	DB_ASSERT(env, TAILQ_EMPTY(&db_rep->connections));

	for (i = 0; i < db_rep->site_cnt; i++) {
		site = &db_rep->sites[i];
		DB_ASSERT(env, TAILQ_EMPTY(&site->sub_conns));
		__repmgr_cleanup_netaddr(env, &site->net_addr);
	}
	__os_free(env, db_rep->sites);
	db_rep->sites = NULL;
}
