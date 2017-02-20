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

static int message_loop __P((ENV *));
static int process_message __P((ENV*, DBT*, DBT*, int));
static int handle_newsite __P((ENV *, const DBT *));
static int ack_message __P((ENV *, u_int32_t, DB_LSN *));
static int ack_msg_conn __P((ENV *, REPMGR_CONNECTION *, u_int32_t, DB_LSN *));

/*
 * PUBLIC: void *__repmgr_msg_thread __P((void *));
 */
void *
__repmgr_msg_thread(args)
	void *args;
{
	ENV *env = args;
	int ret;

	if ((ret = message_loop(env)) != 0) {
		__db_err(env, ret, "message thread failed");
		__repmgr_thread_failure(env, ret);
	}
	return (NULL);
}

static int
message_loop(env)
	ENV *env;
{
	REPMGR_MESSAGE *msg;
	int ret;

	while ((ret = __repmgr_queue_get(env, &msg)) == 0) {
		while ((ret = process_message(env, &msg->control, &msg->rec,
		    msg->originating_eid)) == DB_LOCK_DEADLOCK)
			RPRINT(env, DB_VERB_REPMGR_MISC,
			    (env, "repmgr deadlock retry"));

		__os_free(env, msg);
		if (ret != 0)
			return (ret);
	}

	return (ret == DB_REP_UNAVAIL ? 0 : ret);
}

static int
process_message(env, control, rec, eid)
	ENV *env;
	DBT *control, *rec;
	int eid;
{
	DB_LSN permlsn;
	DB_REP *db_rep;
	REP *rep;
	int ret;
	u_int32_t generation;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	/*
	 * Save initial generation number, in case it changes in a close race
	 * with a NEWMASTER.
	 */
	generation = rep->gen;

	switch (ret =
	    __rep_process_message_int(env, control, rec, eid, &permlsn)) {
	case 0:
		if (db_rep->takeover_pending) {
			db_rep->takeover_pending = FALSE;
			return (__repmgr_become_master(env));
		}
		break;

	case DB_REP_NEWSITE:
		return (handle_newsite(env, rec));

	case DB_REP_HOLDELECTION:
		LOCK_MUTEX(db_rep->mutex);
		ret = __repmgr_init_election(env, ELECT_ELECTION);
		UNLOCK_MUTEX(db_rep->mutex);
		if (ret != 0)
			return (ret);
		break;

	case DB_REP_DUPMASTER:
		if ((ret = __repmgr_repstart(env, DB_REP_CLIENT)) != 0)
			return (ret);
		LOCK_MUTEX(db_rep->mutex);
		ret = __repmgr_init_election(env, ELECT_ELECTION);
		UNLOCK_MUTEX(db_rep->mutex);
		if (ret != 0)
			return (ret);
		break;

	case DB_REP_ISPERM:
		/*
		 * Don't bother sending ack if master doesn't care about it.
		 */
		if (db_rep->perm_policy == DB_REPMGR_ACKS_NONE ||
		    (IS_PEER_POLICY(db_rep->perm_policy) &&
		    rep->priority == 0))
			break;

		if ((ret = ack_message(env, generation, &permlsn)) != 0)
			return (ret);

		break;

	case DB_REP_NOTPERM: /* FALLTHROUGH */
	case DB_REP_IGNORE: /* FALLTHROUGH */
	case DB_LOCK_DEADLOCK:
		break;

	default:
		__db_err(env, ret, "DB_ENV->rep_process_message");
		return (ret);
	}
	return (0);
}

/*
 * Handle replication-related events.  Returns only 0 or DB_EVENT_NOT_HANDLED;
 * no other error returns are tolerated.
 *
 * PUBLIC: int __repmgr_handle_event __P((ENV *, u_int32_t, void *));
 */
int
__repmgr_handle_event(env, event, info)
	ENV *env;
	u_int32_t event;
	void *info;
{
	DB_REP *db_rep;

	db_rep = env->rep_handle;

	if (db_rep->selector == NULL) {
		/* Repmgr is not in use, so all events go to application. */
		return (DB_EVENT_NOT_HANDLED);
	}

	switch (event) {
	case DB_EVENT_REP_ELECTED:
		DB_ASSERT(env, info == NULL);

		db_rep->found_master = TRUE;
		db_rep->takeover_pending = TRUE;

		/*
		 * The application doesn't really need to see this, because the
		 * purpose of this event is to tell the winning site that it
		 * should call rep_start(MASTER), and in repmgr we do that
		 * automatically.  Still, they could conceivably be curious, and
		 * it doesn't hurt anything to let them know.
		 */
		break;
	case DB_EVENT_REP_NEWMASTER:
		DB_ASSERT(env, info != NULL);

		db_rep->found_master = TRUE;
		db_rep->master_eid = *(int *)info;

		/* Application still needs to see this. */
		break;
	default:
		break;
	}
	return (DB_EVENT_NOT_HANDLED);
}

static int
ack_message(env, generation, lsn)
	ENV *env;
	u_int32_t generation;
	DB_LSN *lsn;
{
	DB_REP *db_rep;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *site;
	int ret;

	db_rep = env->rep_handle;
	/*
	 * Regardless of where a message came from, all ack's go to the master
	 * site.  If we're not in touch with the master, we drop it, since
	 * there's not much else we can do.
	 */
	ret = 0;
	LOCK_MUTEX(db_rep->mutex);
	if (!IS_VALID_EID(db_rep->master_eid) ||
	    db_rep->master_eid == SELF_EID) {
		RPRINT(env, DB_VERB_REPMGR_MISC, (env,
		    "dropping ack with master %d", db_rep->master_eid));
		goto unlock;
	}

	site = SITE_FROM_EID(db_rep->master_eid);

	/*
	 * Send the ack out on any/all connections that need it, rather than
	 * going to the trouble of trying to keep track of what LSN's each
	 * connection may be waiting for.
	 */
	if (site->state == SITE_CONNECTED &&
	    (ret = ack_msg_conn(env, site->ref.conn, generation, lsn)) != 0)
		goto unlock;
	TAILQ_FOREACH(conn, &site->sub_conns, entries) {
		if ((ret = ack_msg_conn(env, conn, generation, lsn)) != 0)
			goto unlock;
	}

unlock:
	UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}

/*
 * Sends an acknowledgment on one connection, if it needs it.
 *
 * !!! Called with mutex held.
 */
static int
ack_msg_conn(env, conn, generation, lsn)
	ENV *env;
	REPMGR_CONNECTION *conn;
	u_int32_t generation;
	DB_LSN *lsn;
{
	DBT control2, rec2;
	__repmgr_ack_args ack;
	u_int8_t buf[__REPMGR_ACK_SIZE];
	int ret;

	ret = 0;

	if (conn->state == CONN_READY) {
		DB_ASSERT(env, conn->version > 0);
		ack.generation = generation;
		memcpy(&ack.lsn, lsn, sizeof(DB_LSN));
		if (conn->version == 1) {
			control2.data = &ack;
			control2.size = sizeof(ack);
		} else {
			__repmgr_ack_marshal(env, &ack, buf);
			control2.data = buf;
			control2.size = __REPMGR_ACK_SIZE;
		}
		rec2.size = 0;
		/*
		 * It's hard to imagine anyone would care about a lost ack if
		 * the path to the master is so congested as to need blocking;
		 * so pass "blockable" argument as FALSE.
		 */
		if ((ret = __repmgr_send_one(env, conn, REPMGR_ACK,
		    &control2, &rec2, FALSE)) == DB_REP_UNAVAIL)
			ret = __repmgr_bust_connection(env, conn);
	}
	return (ret);
}

/*
 * Does everything necessary to handle the processing of a NEWSITE return.
 */
static int
handle_newsite(env, rec)
	ENV *env;
	const DBT *rec;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	SITE_STRING_BUFFER buffer;
	size_t hlen;
	u_int16_t port;
	int ret;
	char *host;

	db_rep = env->rep_handle;
	/*
	 * Check if we got sent connect information and if we did, if
	 * this is me or if we already have a connection to this new
	 * site.  If we don't, establish a new one.
	 *
	 * Unmarshall the cdata: a 2-byte port number, in network byte order,
	 * followed by the host name string, which should already be
	 * null-terminated, but let's make sure.
	 */
	if (rec->size < sizeof(port) + 1) {
		__db_errx(env, "unexpected cdata size, msg ignored");
		return (0);
	}
	memcpy(&port, rec->data, sizeof(port));
	port = ntohs(port);

	host = (char*)((u_int8_t*)rec->data + sizeof(port));
	hlen = (rec->size - sizeof(port)) - 1;
	host[hlen] = '\0';

	/* It's me, do nothing. */
	if (strcmp(host, db_rep->my_addr.host) == 0 &&
	    port == db_rep->my_addr.port) {
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "repmgr ignores own NEWSITE info"));
		return (0);
	}

	LOCK_MUTEX(db_rep->mutex);
	if ((ret = __repmgr_add_site(env, host, port, &site, 0)) == EEXIST) {
		RPRINT(env, DB_VERB_REPMGR_MISC, (env,
		    "NEWSITE info from %s was already known",
		    __repmgr_format_site_loc(site, buffer)));
		/*
		 * This is a good opportunity to look up the host name, if
		 * needed, because we're on a message thread (not the critical
		 * select() thread).
		 */
		if ((ret = __repmgr_check_host_name(env,
		    EID_FROM_SITE(site))) != 0)
			return (ret);

		if (site->state == SITE_CONNECTED)
			goto unlock; /* Nothing to do. */
	} else {
		if (ret != 0)
			goto unlock;
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "NEWSITE info added %s",
		    __repmgr_format_site_loc(site, buffer)));
	}

	/*
	 * Wake up the main thread to connect to the new or reawakened
	 * site.
	 */
	ret = __repmgr_wake_main_thread(env);

unlock: UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}
