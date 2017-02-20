/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2006-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#define	__INCLUDE_NETWORKING	1
#include "db_int.h"

typedef int (*HEARTBEAT_ACTION) __P((ENV *));

static int accept_handshake __P((ENV *, REPMGR_CONNECTION *, char *));
static int accept_v1_handshake __P((ENV *, REPMGR_CONNECTION *, char *));
static int __repmgr_call_election __P((ENV *));
static int __repmgr_connect __P((ENV*, socket_t *, REPMGR_SITE *));
static int dispatch_msgin __P((ENV *, REPMGR_CONNECTION *));
static int find_version_info __P((ENV *, REPMGR_CONNECTION *, DBT *));
static int introduce_site __P((ENV *, char *, u_int, REPMGR_SITE**, u_int32_t));
static int __repmgr_next_timeout __P((ENV *,
    db_timespec *, HEARTBEAT_ACTION *));
static int dispatch_phase_completion __P((ENV *, REPMGR_CONNECTION *));
static REPMGR_CONNECTION *__repmgr_master_connection __P((ENV *));
static int process_parameters __P((ENV *,
    REPMGR_CONNECTION *, char *, u_int, u_int32_t, u_int32_t));
static int read_version_response __P((ENV *, REPMGR_CONNECTION *));
static int record_ack __P((ENV *, REPMGR_CONNECTION *));
static int __repmgr_retry_connections __P((ENV *));
static int send_handshake __P((ENV *, REPMGR_CONNECTION *, void *, size_t));
static int __repmgr_send_heartbeat __P((ENV *));
static int send_v1_handshake __P((ENV *,
    REPMGR_CONNECTION *, void *, size_t));
static int send_version_response __P((ENV *, REPMGR_CONNECTION *));
static int __repmgr_try_one __P((ENV *, u_int));

#define	ONLY_HANDSHAKE(env, conn) do {				     \
	if (conn->msg_type != REPMGR_HANDSHAKE) {		     \
		__db_errx(env, "unexpected msg type %d in state %d", \
		    (int)conn->msg_type, conn->state);		     \
		return (DB_REP_UNAVAIL);			     \
	}							     \
} while (0)

/*
 * PUBLIC: void *__repmgr_select_thread __P((void *));
 */
void *
__repmgr_select_thread(args)
	void *args;
{
	ENV *env = args;
	int ret;

	if ((ret = __repmgr_select_loop(env)) != 0) {
		__db_err(env, ret, "select loop failed");
		__repmgr_thread_failure(env, ret);
	}
	return (NULL);
}

/*
 * PUBLIC: int __repmgr_accept __P((ENV *));
 */
int
__repmgr_accept(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_CONNECTION *conn;
	struct sockaddr_in siaddr;
	socklen_t addrlen;
	socket_t s;
	int ret;
#ifdef DB_WIN32
	WSAEVENT event_obj;
#endif

	db_rep = env->rep_handle;
	addrlen = sizeof(siaddr);
	if ((s = accept(db_rep->listen_fd, (struct sockaddr *)&siaddr,
	    &addrlen)) == -1) {
		/*
		 * Some errors are innocuous and so should be ignored.  MSDN
		 * Library documents the Windows ones; the Unix ones are
		 * advocated in Stevens' UNPv1, section 16.6; and Linux
		 * Application Development, p. 416.
		 */
		switch (ret = net_errno) {
#ifdef DB_WIN32
		case WSAECONNRESET:
		case WSAEWOULDBLOCK:
#else
		case EINTR:
		case EWOULDBLOCK:
		case ECONNABORTED:
		case ENETDOWN:
#ifdef EPROTO
		case EPROTO:
#endif
		case ENOPROTOOPT:
		case EHOSTDOWN:
#ifdef ENONET
		case ENONET:
#endif
		case EHOSTUNREACH:
		case EOPNOTSUPP:
		case ENETUNREACH:
#endif
			RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			    "accept error %d considered innocuous", ret));
			return (0);
		default:
			__db_err(env, ret, "accept error");
			return (ret);
		}
	}
	RPRINT(env, DB_VERB_REPMGR_MISC, (env, "accepted a new connection"));

	if ((ret = __repmgr_set_nonblocking(s)) != 0) {
		__db_err(env, ret, "can't set nonblock after accept");
		(void)closesocket(s);
		return (ret);
	}

#ifdef DB_WIN32
	if ((event_obj = WSACreateEvent()) == WSA_INVALID_EVENT) {
		ret = net_errno;
		__db_err(env, ret, "can't create WSA event");
		(void)closesocket(s);
		return (ret);
	}
	if (WSAEventSelect(s, event_obj, FD_READ|FD_CLOSE) == SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "can't set desired event bits");
		(void)WSACloseEvent(event_obj);
		(void)closesocket(s);
		return (ret);
	}
#endif
	if ((ret =
	    __repmgr_new_connection(env, &conn, s, CONN_NEGOTIATE)) != 0) {
#ifdef DB_WIN32
		(void)WSACloseEvent(event_obj);
#endif
		(void)closesocket(s);
		return (ret);
	}
	F_SET(conn, CONN_INCOMING);

	/*
	 * We don't yet know which site this connection is coming from.  So for
	 * now, put it on the "orphans" list; we'll move it to the appropriate
	 * site struct later when we discover who we're talking with, and what
	 * type of connection it is.
	 */
	conn->eid = -1;
	TAILQ_INSERT_TAIL(&db_rep->connections, conn, entries);

#ifdef DB_WIN32
	conn->event_object = event_obj;
#endif
	return (0);
}

/*
 * Computes how long we should wait for input, in other words how long until we
 * have to wake up and do something.  Returns TRUE if timeout is set; FALSE if
 * there is nothing to wait for.
 *
 * Note that the resulting timeout could be zero; but it can't be negative.
 *
 * PUBLIC: int __repmgr_compute_timeout __P((ENV *, db_timespec *));
 */
int
__repmgr_compute_timeout(env, timeout)
	ENV *env;
	db_timespec *timeout;
{
	DB_REP *db_rep;
	REPMGR_RETRY *retry;
	db_timespec now, t;
	int have_timeout;

	db_rep = env->rep_handle;

	/*
	 * There are two factors to consider: are heartbeats in use?  and, do we
	 * have any sites with broken connections that we ought to retry?
	 */
	have_timeout = __repmgr_next_timeout(env, &t, NULL);

	/* List items are in order, so we only have to examine the first one. */
	if (!TAILQ_EMPTY(&db_rep->retries)) {
		retry = TAILQ_FIRST(&db_rep->retries);
		if (have_timeout) {
			/* Choose earliest timeout deadline. */
			t = timespeccmp(&retry->time, &t, <) ? retry->time : t;
		} else {
			t = retry->time;
			have_timeout = TRUE;
		}
	}

	if (have_timeout) {
		__os_gettime(env, &now, 1);
		if (timespeccmp(&now, &t, >=))
			timespecclear(timeout);
		else {
			*timeout = t;
			timespecsub(timeout, &now);
		}
	}

	return (have_timeout);
}

/*
 * Figures out the next heartbeat-related thing to be done, and when it should
 * be done.  The code is factored this way because this computation needs to be
 * done both before each select() call, and after (when we're checking for timer
 * expiration).
 */
static int
__repmgr_next_timeout(env, deadline, action)
	ENV *env;
	db_timespec *deadline;
	HEARTBEAT_ACTION *action;
{
	DB_REP *db_rep;
	HEARTBEAT_ACTION my_action;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *site;
	db_timespec t;

	db_rep = env->rep_handle;

	if (db_rep->master_eid == SELF_EID && db_rep->heartbeat_frequency > 0) {
		t = db_rep->last_bcast;
		TIMESPEC_ADD_DB_TIMEOUT(&t, db_rep->heartbeat_frequency);
		my_action = __repmgr_send_heartbeat;
	} else if ((conn = __repmgr_master_connection(env)) != NULL &&
	    !IS_SUBORDINATE(db_rep) &&
	    db_rep->heartbeat_monitor_timeout > 0 &&
	    conn->version >= HEARTBEAT_MIN_VERSION) {
		/*
		 * If we have a working connection to a heartbeat-aware master,
		 * let's monitor it.  Otherwise there's really nothing we can
		 * do.
		 */
		site = SITE_FROM_EID(db_rep->master_eid);
		t = site->last_rcvd_timestamp;
		TIMESPEC_ADD_DB_TIMEOUT(&t, db_rep->heartbeat_monitor_timeout);
		my_action = __repmgr_call_election;
	} else
		return (FALSE);

	*deadline = t;
	if (action != NULL)
		*action = my_action;
	return (TRUE);
}

static int
__repmgr_send_heartbeat(env)
	ENV *env;
{
	DBT control, rec;
	u_int unused1, unused2;

	DB_INIT_DBT(control, NULL, 0);
	DB_INIT_DBT(rec, NULL, 0);
	return (__repmgr_send_broadcast(env,
	    REPMGR_HEARTBEAT, &control, &rec, &unused1, &unused2));
}

static REPMGR_CONNECTION *
__repmgr_master_connection(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_CONNECTION *conn;
	REPMGR_SITE *master;

	db_rep = env->rep_handle;

	if (db_rep->master_eid == SELF_EID ||
	    !IS_VALID_EID(db_rep->master_eid))
		return (NULL);
	master = SITE_FROM_EID(db_rep->master_eid);
	if (master->state != SITE_CONNECTED)
		return (NULL);
	conn = master->ref.conn;
	if (IS_READY_STATE(conn->state))
		return (conn);
	return (NULL);
}

static int
__repmgr_call_election(env)
	ENV *env;
{
	REPMGR_CONNECTION *conn;

	conn = __repmgr_master_connection(env);
	DB_ASSERT(env, conn != NULL);
	RPRINT(env, DB_VERB_REPMGR_MISC,
	    (env, "heartbeat monitor timeout expired"));
	STAT(env->rep_handle->region->mstat.st_connection_drop++);
	return (__repmgr_bust_connection(env, conn));
}

/*
 * PUBLIC: int __repmgr_check_timeouts __P((ENV *));
 *
 * !!!
 * Assumes caller holds the mutex.
 */
int
__repmgr_check_timeouts(env)
	ENV *env;
{
	db_timespec when, now;
	HEARTBEAT_ACTION action;
	int ret;

	/*
	 * Figure out the next heartbeat-related thing to be done.  Then, if
	 * it's time to do it, do so.
	 */
	if (__repmgr_next_timeout(env, &when, &action)) {
		__os_gettime(env, &now, 1);
		if (timespeccmp(&when, &now, <=) &&
		    (ret = (*action)(env)) != 0)
			return (ret);
	}

	return (__repmgr_retry_connections(env));
}

/*
 * Initiates connection attempts for any sites on the idle list whose retry
 * times have expired.
 */
static int
__repmgr_retry_connections(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_RETRY *retry;
	db_timespec now;
	u_int eid;
	int ret;

	db_rep = env->rep_handle;
	__os_gettime(env, &now, 1);

	while (!TAILQ_EMPTY(&db_rep->retries)) {
		retry = TAILQ_FIRST(&db_rep->retries);
		if (timespeccmp(&retry->time, &now, >=))
			break;	/* since items are in time order */

		TAILQ_REMOVE(&db_rep->retries, retry, entries);

		eid = retry->eid;
		__os_free(env, retry);

		if ((ret = __repmgr_try_one(env, eid)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * PUBLIC: int __repmgr_first_try_connections __P((ENV *));
 *
 * !!!
 * Assumes caller holds the mutex.
 */
int
__repmgr_first_try_connections(env)
	ENV *env;
{
	DB_REP *db_rep;
	u_int eid;
	int ret;

	db_rep = env->rep_handle;
	for (eid = 0; eid < db_rep->site_cnt; eid++)
		if ((ret = __repmgr_try_one(env, eid)) != 0)
			return (ret);
	return (0);
}

/*
 * Makes a best-effort attempt to connect to the indicated site.  Returns a
 * non-zero error indication only for disastrous failures.  For re-tryable
 * errors, we will have scheduled another attempt, and that can be considered
 * success enough.
 */
static int
__repmgr_try_one(env, eid)
	ENV *env;
	u_int eid;
{
	ADDRINFO *list;
	DB_REP *db_rep;
	repmgr_netaddr_t *addr;
	int ret;

	db_rep = env->rep_handle;

	addr = &SITE_FROM_EID(eid)->net_addr;
	if (ADDR_LIST_FIRST(addr) == NULL) {
		if ((ret = __repmgr_getaddr(env,
		    addr->host, addr->port, 0, &list)) == 0) {
			addr->address_list = list;
			(void)ADDR_LIST_FIRST(addr);
		} else if (ret == DB_REP_UNAVAIL)
			return (__repmgr_schedule_connection_attempt(
			    env, eid, FALSE));
		else
			return (ret);
	}

	/* Here, when we have a valid address. */
	return (__repmgr_connect_site(env, eid));
}

/*
 * Tries to establish a connection with the site indicated by the given eid,
 * starting with the "current" element of its address list and trying as many
 * addresses as necessary until the list is exhausted.
 *
 * PUBLIC: int __repmgr_connect_site __P((ENV *, u_int eid));
 */
int
__repmgr_connect_site(env, eid)
	ENV *env;
	u_int eid;
{
	DB_REP *db_rep;
	REPMGR_CONNECTION *con;
	REPMGR_SITE *site;
	socket_t s;
	int state;
	int ret;
#ifdef DB_WIN32
	long desired_event;
	WSAEVENT event_obj;
#endif

	db_rep = env->rep_handle;
	site = SITE_FROM_EID(eid);

	switch (ret = __repmgr_connect(env, &s, site)) {
	case 0:
		state = CONN_CONNECTED;
#ifdef DB_WIN32
		desired_event = FD_READ|FD_CLOSE;
#endif
		break;
	case INPROGRESS:
		state = CONN_CONNECTING;
#ifdef DB_WIN32
		desired_event = FD_CONNECT;
#endif
		break;
	default:
		STAT(db_rep->region->mstat.st_connect_fail++);
		return (
		    __repmgr_schedule_connection_attempt(env, eid, FALSE));
	}

#ifdef DB_WIN32
	if ((event_obj = WSACreateEvent()) == WSA_INVALID_EVENT) {
		ret = net_errno;
		__db_err(env, ret, "can't create WSA event");
		(void)closesocket(s);
		return (ret);
	}
	if (WSAEventSelect(s, event_obj, desired_event) == SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "can't set desired event bits");
		(void)WSACloseEvent(event_obj);
		(void)closesocket(s);
		return (ret);
	}
#endif

	if ((ret = __repmgr_new_connection(env, &con, s, state)) != 0) {
#ifdef DB_WIN32
		(void)WSACloseEvent(event_obj);
#endif
		(void)closesocket(s);
		return (ret);
	}
#ifdef DB_WIN32
	con->event_object = event_obj;
#endif

	con->eid = (int)eid;
	site->ref.conn = con;
	site->state = SITE_CONNECTED;

	if (state == CONN_CONNECTED) {
		__os_gettime(env, &site->last_rcvd_timestamp, 1);
		switch (ret = __repmgr_propose_version(env, con)) {
		case 0:
			break;
		case DB_REP_UNAVAIL:
			return (__repmgr_bust_connection(env, con));
		default:
			return (ret);
		}
	}

	return (0);
}

static int
__repmgr_connect(env, socket_result, site)
	ENV *env;
	socket_t *socket_result;
	REPMGR_SITE *site;
{
	repmgr_netaddr_t *addr;
	ADDRINFO *ai;
	socket_t s;
	char *why;
	int ret;
	SITE_STRING_BUFFER buffer;

	/*
	 * Lint doesn't know about DB_ASSERT, so it can't tell that this
	 * loop will always get executed at least once, giving 'why' a value.
	 */
	COMPQUIET(why, "");
	addr = &site->net_addr;
	ai = ADDR_LIST_CURRENT(addr);
	DB_ASSERT(env, ai != NULL);
	for (; ai != NULL; ai = ADDR_LIST_NEXT(addr)) {

		if ((s = socket(ai->ai_family,
		    ai->ai_socktype, ai->ai_protocol)) == SOCKET_ERROR) {
			why = "can't create socket to connect";
			continue;
		}

		if ((ret = __repmgr_set_nonblocking(s)) != 0) {
			__db_err(env,
			    ret, "can't make nonblock socket to connect");
			(void)closesocket(s);
			return (ret);
		}

		if (connect(s, ai->ai_addr, (socklen_t)ai->ai_addrlen) != 0)
			ret = net_errno;

		if (ret == 0 || ret == INPROGRESS) {
			*socket_result = s;
			RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			    "init connection to %s with result %d",
			    __repmgr_format_site_loc(site, buffer), ret));
			return (ret);
		}

		why = "connection failed";
		(void)closesocket(s);
	}

	/* We've exhausted all possible addresses. */
	ret = net_errno;
	__db_err(env, ret, "%s to %s", why,
	    __repmgr_format_site_loc(site, buffer));
	return (ret);
}

/*
 * Sends a proposal for version negotiation.
 *
 * PUBLIC: int __repmgr_propose_version __P((ENV *, REPMGR_CONNECTION *));
 */
int
__repmgr_propose_version(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	__repmgr_version_proposal_args versions;
	repmgr_netaddr_t *my_addr;
	size_t hostname_len, rec_length;
	u_int8_t *buf, *p;
	int ret;

	db_rep = env->rep_handle;
	my_addr = &db_rep->my_addr;

	/*
	 * In repmgr wire protocol version 1, a handshake message had a rec part
	 * that looked like this:
	 *
	 *  +-----------------+----+
	 *  |  host name ...  | \0 |
	 *  +-----------------+----+
	 *
	 * To ensure its own sanity, the old repmgr would write a NUL into the
	 * last byte of a received message, and then use normal C library string
	 * operations (e.g., * strlen, strcpy).
	 *
	 * Now, a version proposal has a rec part that looks like this:
	 *
	 *  +-----------------+----+------------------+------+
	 *  |  host name ...  | \0 |  extra info ...  |  \0  |
	 *  +-----------------+----+------------------+------+
	 *
	 * The "extra info" contains the version parameters, in marshaled form.
	 */

	hostname_len = strlen(my_addr->host);
	rec_length = hostname_len + 1 +
	    __REPMGR_VERSION_PROPOSAL_SIZE + 1;
	if ((ret = __os_malloc(env, rec_length, &buf)) != 0)
		goto out;
	p = buf;
	(void)strcpy((char*)p, my_addr->host);

	p += hostname_len + 1;
	versions.min = DB_REPMGR_MIN_VERSION;
	versions.max = DB_REPMGR_VERSION;
	__repmgr_version_proposal_marshal(env, &versions, p);

	ret = send_v1_handshake(env, conn, buf, rec_length);
	__os_free(env, buf);
out:
	return (ret);
}

static int
send_v1_handshake(env, conn, buf, len)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *buf;
	size_t len;
{
	DB_REP *db_rep;
	REP *rep;
	repmgr_netaddr_t *my_addr;
	DB_REPMGR_V1_HANDSHAKE buffer;
	DBT cntrl, rec;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	my_addr = &db_rep->my_addr;

	buffer.version = 1;
	buffer.priority = htonl(rep->priority);
	buffer.port = my_addr->port;
	cntrl.data = &buffer;
	cntrl.size = sizeof(buffer);

	rec.data = buf;
	rec.size = (u_int32_t)len;

	/*
	 * It would of course be disastrous to block the select() thread, so
	 * pass the "blockable" argument as FALSE.  Fortunately blocking should
	 * never be necessary here, because the hand-shake is always the first
	 * thing we send.  Which is a good thing, because it would be almost as
	 * disastrous if we allowed ourselves to drop a handshake.
	 */
	return (__repmgr_send_one(env,
	    conn, REPMGR_HANDSHAKE, &cntrl, &rec, FALSE));
}

/*
 * PUBLIC: int __repmgr_read_from_site __P((ENV *, REPMGR_CONNECTION *));
 *
 * !!!
 * Caller is assumed to hold repmgr->mutex, 'cuz we call queue_put() from here.
 */
int
__repmgr_read_from_site(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	SITE_STRING_BUFFER buffer;
	size_t nr;
	int ret;

	db_rep = env->rep_handle;
	/*
	 * Keep reading pieces as long as we're making some progress, or until
	 * we complete the current read phase.
	 */
	for (;;) {
		if ((ret = __repmgr_readv(conn->fd,
		    &conn->iovecs.vectors[conn->iovecs.offset],
		    conn->iovecs.count - conn->iovecs.offset, &nr)) != 0) {
			switch (ret) {
#ifndef DB_WIN32
			case EINTR:
				continue;
#endif
			case WOULDBLOCK:
				return (0);
			default:
#ifdef EBADF
				DB_ASSERT(env, ret != EBADF);
#endif
				(void)__repmgr_format_eid_loc(env->rep_handle,
				    conn->eid, buffer);
				__db_err(env, ret,
				    "can't read from %s", buffer);
				STAT(env->rep_handle->
				    region->mstat.st_connection_drop++);
				return (DB_REP_UNAVAIL);
			}
		}

		if (nr > 0) {
			if (IS_VALID_EID(conn->eid)) {
				site = SITE_FROM_EID(conn->eid);
				__os_gettime(
				    env, &site->last_rcvd_timestamp, 1);
			}
			if (__repmgr_update_consumed(&conn->iovecs, nr))
				return (dispatch_phase_completion(env,
					    conn));
		} else {
			(void)__repmgr_format_eid_loc(env->rep_handle,
			    conn->eid, buffer);
			__db_errx(env, "EOF on connection from %s", buffer);
			STAT(env->rep_handle->
			    region->mstat.st_connection_drop++);
			return (DB_REP_UNAVAIL);
		}
	}
}

/*
 * Handles whatever needs to be done upon the completion of a reading phase on a
 * given connection.
 */
static int
dispatch_phase_completion(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
#define	MEM_ALIGN sizeof(double)
	DBT *dbt;
	u_int32_t control_size, rec_size;
	size_t memsize, control_offset, rec_offset;
	void *membase;
	int ret;

	switch (conn->reading_phase) {
	case SIZES_PHASE:
		/*
		 * We've received the header: a message type and the lengths of
		 * the two pieces of the message.  Set up buffers to read the
		 * two pieces.  This set-up is a bit different for a
		 * REPMGR_REP_MESSAGE, because we plan to pass it off to the msg
		 * threads.
		 */
		__repmgr_iovec_init(&conn->iovecs);
		control_size = ntohl(conn->control_size_buf);
		rec_size = ntohl(conn->rec_size_buf);

		if (conn->msg_type == REPMGR_REP_MESSAGE) {
			if (control_size == 0) {
				__db_errx(
				    env, "illegal size for rep msg");
				return (DB_REP_UNAVAIL);
			}
			/*
			 * Allocate a block of memory large enough to hold a
			 * DB_REPMGR_MESSAGE wrapper, plus the (one or) two DBT
			 * data areas that it points to.  Start by calculating
			 * the total memory needed, rounding up for the start of
			 * each DBT, to ensure possible alignment requirements.
			 */
			memsize = (size_t)
			    DB_ALIGN(sizeof(REPMGR_MESSAGE), MEM_ALIGN);
			control_offset = memsize;
			memsize += control_size;
			if (rec_size > 0) {
				memsize = (size_t)DB_ALIGN(memsize, MEM_ALIGN);
				rec_offset = memsize;
				memsize += rec_size;
			} else
				COMPQUIET(rec_offset, 0);
			if ((ret = __os_malloc(env, memsize, &membase)) != 0)
				return (ret);
			conn->input.rep_message = membase;

			conn->input.rep_message->originating_eid = conn->eid;
			DB_INIT_DBT(conn->input.rep_message->control,
			    (u_int8_t*)membase + control_offset, control_size);
			__repmgr_add_dbt(&conn->iovecs,
			    &conn->input.rep_message->control);

			if (rec_size > 0) {
				DB_INIT_DBT(conn->input.rep_message->rec,
				    (rec_size > 0 ?
					(u_int8_t*)membase + rec_offset : NULL),
				    rec_size);
				__repmgr_add_dbt(&conn->iovecs,
				    &conn->input.rep_message->rec);
			} else
				DB_INIT_DBT(conn->input.rep_message->rec,
				    NULL, 0);
		} else {
			conn->input.repmgr_msg.cntrl.size = control_size;
			conn->input.repmgr_msg.rec.size = rec_size;

			if (control_size > 0) {
				dbt = &conn->input.repmgr_msg.cntrl;
				if ((ret = __os_malloc(env, control_size,
				    &dbt->data)) != 0)
					return (ret);
				__repmgr_add_dbt(&conn->iovecs, dbt);
			}

			if (rec_size > 0) {
				dbt = &conn->input.repmgr_msg.rec;
				if ((ret = __os_malloc(env, rec_size,
				     &dbt->data)) != 0) {
					if (control_size > 0)
						__os_free(env,
						    conn->input.repmgr_msg.
						    cntrl.data);
					return (ret);
				}
				__repmgr_add_dbt(&conn->iovecs, dbt);
			}
		}

		conn->reading_phase = DATA_PHASE;

		if (control_size > 0 || rec_size > 0)
			break;

		/*
		 * However, if they're both 0, we're ready to complete
		 * DATA_PHASE.
		 */
		/* FALLTHROUGH */

	case DATA_PHASE:
		return (dispatch_msgin(env, conn));

	default:
		DB_ASSERT(env, FALSE);
	}

	return (0);
}

/*
 * Processes an incoming message, depending on our current state.
 */
static int
dispatch_msgin(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DBT *dbt;
	char *hostname;
	int given, ret;

	given = FALSE;

	switch (conn->state) {
	case CONN_CONNECTED:
		/*
		 * In this state, we know we're working with an outgoing
		 * connection.  We've sent a version proposal, and now expect
		 * the response (which could be a dumb old V1 handshake).
		 */
		ONLY_HANDSHAKE(env, conn);
		if ((ret = read_version_response(env, conn)) != 0)
			return (ret);
		break;

	case CONN_NEGOTIATE:
		/*
		 * Since we're in this state, we know we're working with an
		 * incoming connection, and this is the first message we've
		 * received.  So it must be a version negotiation proposal (or a
		 * legacy V1 handshake).  (We'll verify this of course.)
		 */
		ONLY_HANDSHAKE(env, conn);
		if ((ret = send_version_response(env, conn)) != 0)
			return (ret);
		break;

	case CONN_PARAMETERS:
		/*
		 * We've previously agreed on a (>1) version, and are now simply
		 * awaiting the other side's parameters handshake.
		 */
		ONLY_HANDSHAKE(env, conn);
		dbt = &conn->input.repmgr_msg.rec;
		hostname = dbt->data;
		hostname[dbt->size-1] = '\0';
		if ((ret = accept_handshake(env, conn, hostname)) != 0)
			return (ret);
		conn->state = CONN_READY;
		break;

	case CONN_READY:	/* FALLTHROUGH */
	case CONN_CONGESTED:
		/*
		 * We have a complete message, so process it.  Acks and
		 * handshakes get processed here, in line.  Regular rep messages
		 * get posted to a queue, to be handled by a thread from the
		 * message thread pool.
		 */
		switch (conn->msg_type) {
		case REPMGR_ACK:
			if ((ret = record_ack(env, conn)) != 0)
				return (ret);
			break;

		case REPMGR_HEARTBEAT:
			/*
			 * The underlying byte-receiving mechanism will already
			 * have noted the fact that we got some traffic on this
			 * connection.  And that's all we really have to do, so
			 * there's nothing more needed at this point.
			 */
			break;

		case REPMGR_REP_MESSAGE:
			if ((ret = __repmgr_queue_put(env,
			    conn->input.rep_message)) != 0)
				return (ret);
			/*
			 * The queue has taken over responsibility for the
			 * rep_message buffer, and will free it later.
			 */
			given = TRUE;
			break;

		default:
			__db_errx(env,
			    "unexpected msg type rcvd in ready state: %d",
			    (int)conn->msg_type);
			return (DB_REP_UNAVAIL);
		}
		break;

	case CONN_DEFUNCT:
		break;

	default:
		DB_ASSERT(env, FALSE);
	}

	if (!given) {
		dbt = &conn->input.repmgr_msg.cntrl;
		if (dbt->size > 0)
			__os_free(env, dbt->data);
		dbt = &conn->input.repmgr_msg.rec;
		if (dbt->size > 0)
			__os_free(env, dbt->data);
	}
	__repmgr_reset_for_reading(conn);
	return (0);
}

/*
 * Examine and verify the incoming version proposal message, and send an
 * appropriate response.
 */
static int
send_version_response(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	__repmgr_version_proposal_args versions;
	__repmgr_version_confirmation_args conf;
	repmgr_netaddr_t *my_addr;
	char *hostname;
	u_int8_t buf[__REPMGR_VERSION_CONFIRMATION_SIZE+1];
	DBT vi;
	int ret;

	db_rep = env->rep_handle;
	my_addr = &db_rep->my_addr;

	if ((ret = find_version_info(env, conn, &vi)) != 0)
		return (ret);
	if (vi.size == 0) {
		/* No version info, so we must be talking to a v1 site. */
		hostname = conn->input.repmgr_msg.rec.data;
		if ((ret = accept_v1_handshake(env, conn, hostname)) != 0)
			return (ret);
		if ((ret = send_v1_handshake(env, conn, my_addr->host,
		     strlen(my_addr->host) + 1)) != 0)
			return (ret);
		conn->state = CONN_READY;
	} else {
		if ((ret = __repmgr_version_proposal_unmarshal(env,
		    &versions, vi.data, vi.size, NULL)) != 0)
			return (DB_REP_UNAVAIL);

		if (DB_REPMGR_VERSION >= versions.min &&
		    DB_REPMGR_VERSION <= versions.max)
			conf.version = DB_REPMGR_VERSION;
		else if (versions.max >= DB_REPMGR_MIN_VERSION &&
		    versions.max <= DB_REPMGR_VERSION)
			conf.version = versions.max;
		else {
			/*
			 * User must have wired up a combination of versions
			 * exceeding what we said we'd support.
			 */
			__db_errx(env,
			    "No available version between %lu and %lu",
			    (u_long)versions.min, (u_long)versions.max);
			return (DB_REP_UNAVAIL);
		}
		conn->version = conf.version;

		__repmgr_version_confirmation_marshal(env, &conf, buf);
		if ((ret = send_handshake(env, conn, buf, sizeof(buf))) != 0)
			return (ret);

		conn->state = CONN_PARAMETERS;
	}
	return (ret);
}

/*
 * Sends a version-aware handshake to the remote site, only after we've verified
 * that it is indeed version-aware.  We can send either v2 or v3 handshake,
 * depending on the connection's version.
 */
static int
send_handshake(env, conn, opt, optlen)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *opt;
	size_t optlen;
{
	DB_REP *db_rep;
	REP *rep;
	DBT cntrl, rec;
	__repmgr_handshake_args hs;
	__repmgr_v2handshake_args v2hs;
	repmgr_netaddr_t *my_addr;
	size_t hostname_len, rec_len;
	void *buf;
	u_int8_t *p;
	u_int32_t cntrl_len;
	int ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	my_addr = &db_rep->my_addr;

	/*
	 * The cntrl part has port and priority.  The rec part has the host
	 * name, followed by whatever optional extra data was passed to us.
	 *
	 * Version awareness was introduced with protocol version 2.
	 */
	DB_ASSERT(env, conn->version >= 2);
	cntrl_len = conn->version == 2 ?
	    __REPMGR_V2HANDSHAKE_SIZE : __REPMGR_HANDSHAKE_SIZE;
	hostname_len = strlen(my_addr->host);
	rec_len = hostname_len + 1 +
	    (opt == NULL ? 0 : optlen);

	if ((ret = __os_malloc(env, cntrl_len + rec_len, &buf)) != 0)
		return (ret);

	cntrl.data = p = buf;
	if (conn->version == 2) {
		/* Not allowed to use multi-process feature in v2 group. */
		DB_ASSERT(env, !IS_SUBORDINATE(db_rep));
		v2hs.port = my_addr->port;
		v2hs.priority = rep->priority;
		__repmgr_v2handshake_marshal(env, &v2hs, p);
	} else {
		hs.port = my_addr->port;
		hs.priority = rep->priority;
		hs.flags = IS_SUBORDINATE(db_rep) ? REPMGR_SUBORDINATE : 0;
		__repmgr_handshake_marshal(env, &hs, p);
	}
	cntrl.size = cntrl_len;

	p = rec.data = &p[cntrl_len];
	(void)strcpy((char*)p, my_addr->host);
	p += hostname_len + 1;
	if (opt != NULL) {
		memcpy(p, opt, optlen);
		p += optlen;
	}
	rec.size = (u_int32_t)(p - (u_int8_t*)rec.data);

	/* Never block on select thread: pass blockable as FALSE. */
	ret = __repmgr_send_one(env,
	    conn, REPMGR_HANDSHAKE, &cntrl, &rec, FALSE);
	__os_free(env, buf);
	return (ret);
}

static int
read_version_response(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	__repmgr_version_confirmation_args conf;
	DBT vi;
	char *hostname;
	int ret;

	if ((ret = find_version_info(env, conn, &vi)) != 0)
		return (ret);
	hostname = conn->input.repmgr_msg.rec.data;
	if (vi.size == 0) {
		if ((ret = accept_v1_handshake(env, conn, hostname)) != 0)
			return (ret);
	} else {
		if ((ret = __repmgr_version_confirmation_unmarshal(env,
		    &conf, vi.data, vi.size, NULL)) != 0)
			return (DB_REP_UNAVAIL);
		if (conf.version >= DB_REPMGR_MIN_VERSION &&
		    conf.version <= DB_REPMGR_VERSION)
			conn->version = conf.version;
		else {
			/*
			 * Remote site "confirmed" a version outside of the
			 * range we proposed.  It should never do that.
			 */
			__db_errx(env,
			    "Can't support confirmed version %lu",
			    (u_long)conf.version);
			return (DB_REP_UNAVAIL);
		}

		if ((ret = accept_handshake(env, conn, hostname)) != 0)
			return (ret);
		if ((ret = send_handshake(env, conn, NULL, 0)) != 0)
			return (ret);
	}
	conn->state = CONN_READY;
	return (ret);
}

/*
 * Examine the rec part of a handshake message to see if it has any version
 * information in it.  This is the magic that lets us allows version-aware sites
 * to exchange information, and yet avoids tripping up v1 sites, which don't
 * know how to look for it.
 */
static int
find_version_info(env, conn, vi)
	ENV *env;
	REPMGR_CONNECTION *conn;
	DBT *vi;
{
	DBT *dbt;
	char *hostname;
	u_int32_t hostname_len;

	dbt = &conn->input.repmgr_msg.rec;
	if (dbt->size == 0) {
		__db_errx(env, "handshake is missing rec part");
		return (DB_REP_UNAVAIL);
	}
	hostname = dbt->data;
	hostname[dbt->size-1] = '\0';
	hostname_len = (u_int32_t)strlen(hostname);
	if (hostname_len + 1 == dbt->size) {
		/*
		 * The rec DBT held only the host name.  This is a simple legacy
		 * V1 handshake; it contains no version information.
		 */
		vi->size = 0;
	} else {
		/*
		 * There's more data than just the host name.  The remainder is
		 * available to be treated as a normal byte buffer (and read in
		 * by one of the unmarshal functions).  Note that the remaining
		 * length should not include the padding byte that we have
		 * already clobbered.
		 */
		vi->data = &((u_int8_t *)dbt->data)[hostname_len + 1];
		vi->size = (dbt->size - (hostname_len+1)) - 1;
	}
	return (0);
}

static int
accept_handshake(env, conn, hostname)
	ENV *env;
	REPMGR_CONNECTION *conn;
	char *hostname;
{
	__repmgr_handshake_args hs;
	__repmgr_v2handshake_args hs2;
	u_int port;
	u_int32_t pri, flags;

	/*
	 * Current version is 3, and only other version that supports version
	 * negotiation is 2.
	 */
	DB_ASSERT(env, conn->version == 2 || conn->version == 3);

	/* Extract port and priority from cntrl. */
	if (conn->version == 2) {
		if (__repmgr_v2handshake_unmarshal(env, &hs2,
		    conn->input.repmgr_msg.cntrl.data,
		    conn->input.repmgr_msg.cntrl.size, NULL) != 0)
			return (DB_REP_UNAVAIL);
		port = hs2.port;
		pri = hs2.priority;
		flags = 0;
	} else {
		if (__repmgr_handshake_unmarshal(env, &hs,
		   conn->input.repmgr_msg.cntrl.data,
		   conn->input.repmgr_msg.cntrl.size, NULL) != 0)
			return (DB_REP_UNAVAIL);
		port = hs.port;
		pri = hs.priority;
		flags = hs.flags;
	}

	return (process_parameters(env,
		    conn, hostname, port, pri, flags));
}

static int
accept_v1_handshake(env, conn, hostname)
	ENV *env;
	REPMGR_CONNECTION *conn;
	char *hostname;
{
	DB_REPMGR_V1_HANDSHAKE *handshake;
	u_int32_t prio;

	handshake = conn->input.repmgr_msg.cntrl.data;
	if (conn->input.repmgr_msg.cntrl.size != sizeof(*handshake) ||
	    handshake->version != 1) {
		__db_errx(env, "malformed V1 handshake");
		return (DB_REP_UNAVAIL);
	}

	conn->version = 1;
	prio = ntohl(handshake->priority);
	return (process_parameters(env,
		    conn, hostname, handshake->port, prio, 0));
}

static int
process_parameters(env, conn, host, port, priority, flags)
	ENV *env;
	REPMGR_CONNECTION *conn;
	char *host;
	u_int port;
	u_int32_t priority, flags;
{
	DB_REP *db_rep;
	REPMGR_RETRY *retry;
	REPMGR_SITE *site;
	int eid, ret, sockopt;

	db_rep = env->rep_handle;

	if (F_ISSET(conn, CONN_INCOMING)) {
		/*
		 * Incoming connection: we don't yet know what site it belongs
		 * to, so it must be on the "orphans" list.
		 */
		DB_ASSERT(env, !IS_VALID_EID(conn->eid));
		TAILQ_REMOVE(&db_rep->connections, conn, entries);

		/*
		 * Now that we've been given the host and port, use them to find
		 * the site (or create a new one if necessary, etc.).
		 */
		if ((site = __repmgr_find_site(env, host, port)) != NULL) {
			eid = EID_FROM_SITE(site);
			if (LF_ISSET(REPMGR_SUBORDINATE)) {
				/*
				 * Accept it, as a supplementary source of
				 * input, but nothing else.
				 */
				TAILQ_INSERT_TAIL(&site->sub_conns,
				    conn, entries);
				conn->eid = eid;

#ifdef SO_KEEPALIVE
				sockopt = 1;
				if (setsockopt(conn->fd, SOL_SOCKET,
				    SO_KEEPALIVE, (sockopt_t)&sockopt,
				     sizeof(sockopt)) != 0) {
					ret = net_errno;
					__db_err(env, ret,
					   "can't set KEEPALIVE socket option");
					return (ret);
				}
#endif
			} else {
				if (site->state == SITE_IDLE) {
					RPRINT(env, DB_VERB_REPMGR_MISC, (env,
					"handshake from idle site %s:%u EID %u",
					    host, port, eid));
					retry = site->ref.retry;
					TAILQ_REMOVE(&db_rep->retries,
					    retry, entries);
					__os_free(env, retry);
				} else {
					/*
					 * We got an incoming connection for a
					 * site we were already connected to; at
					 * least we thought we were.
					 */
					RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			     "connection from %s:%u EID %u supersedes existing",
					    host, port, eid));

					/*
					 * No need to schedule a retry for
					 * later, since we now have a
					 * replacement connection.
					 */
					__repmgr_disable_connection(env,
					     site->ref.conn);
				}
				conn->eid = eid;
				site->state = SITE_CONNECTED;
				site->ref.conn = conn;
				__os_gettime(env,
				    &site->last_rcvd_timestamp, 1);
			}
		} else {
			if ((ret = introduce_site(env,
			    host, port, &site, flags)) == 0)
				RPRINT(env, DB_VERB_REPMGR_MISC, (env,
			"handshake introduces unknown site %s:%u", host, port));
			else if (ret != EEXIST)
				return (ret);
			eid = EID_FROM_SITE(site);

			if (LF_ISSET(REPMGR_SUBORDINATE)) {
				TAILQ_INSERT_TAIL(&site->sub_conns,
				    conn, entries);
#ifdef SO_KEEPALIVE
				sockopt = 1;
				if ((ret = setsockopt(conn->fd, SOL_SOCKET,
				    SO_KEEPALIVE, (sockopt_t)&sockopt,
				     sizeof(sockopt))) != 0) {
					__db_err(env, ret,
					   "can't set KEEPALIVE socket option");
					return (ret);
				}
#endif
			} else {
				site->state = SITE_CONNECTED;
				site->ref.conn = conn;
				__os_gettime(env,
				    &site->last_rcvd_timestamp, 1);
			}
			conn->eid = eid;
		}
	} else {
		/*
		 * Since we initiated this as an outgoing connection, we
		 * obviously already know the host, port and site.  We just need
		 * the other site's priority.
		 */
		DB_ASSERT(env, IS_VALID_EID(conn->eid));
		site = SITE_FROM_EID(conn->eid);
		RPRINT(env, DB_VERB_REPMGR_MISC, (env,
		    "handshake from connection to %s:%lu EID %u",
		    site->net_addr.host,
		    (u_long)site->net_addr.port, conn->eid));
	}

	site->priority = priority;
	F_SET(site, SITE_HAS_PRIO);

	/*
	 * If we're moping around wishing we knew who the master was, then
	 * getting in touch with another site might finally provide sufficient
	 * connectivity to find out.  But just do this once, because otherwise
	 * we get messages while the subsequent rep_start operations are going
	 * on, and rep tosses them in that case.
	 */
	if (!IS_SUBORDINATE(db_rep) && /* us */
	    db_rep->master_eid == DB_EID_INVALID &&
	    db_rep->init_policy != DB_REP_MASTER &&
	    !db_rep->done_one &&
	    !LF_ISSET(REPMGR_SUBORDINATE)) { /* the remote site */
		db_rep->done_one = TRUE;
		RPRINT(env, DB_VERB_REPMGR_MISC, (env,
		    "handshake with no known master to wake election thread"));
		if ((ret = __repmgr_init_election(env, ELECT_REPSTART)) != 0)
			return (ret);
	}

	return (0);
}

static int
introduce_site(env, host, port, sitep, flags)
	ENV *env;
	char *host;
	u_int port;
	REPMGR_SITE **sitep;
	u_int32_t flags;
{
	int peer, state;

	/*
	 * SITE_CONNECTED means we have the main connection to the site.  But
	 * we're here when we first learn of a site by getting a subordinate
	 * connection, so this doesn't suffice to put us in "connected" state.
	 */
	state = LF_ISSET(REPMGR_SUBORDINATE) ? SITE_IDLE : SITE_CONNECTED;
	peer = FALSE;

	return (__repmgr_add_site_int(env, host, port, sitep, peer, state));
}

static int
record_ack(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	__repmgr_ack_args *ackp, ack;
	SITE_STRING_BUFFER location;
	u_int32_t gen;
	int ret;

	db_rep = env->rep_handle;

	DB_ASSERT(env, conn->version > 0 &&
	    IS_READY_STATE(conn->state) && IS_VALID_EID(conn->eid));
	site = SITE_FROM_EID(conn->eid);

	/*
	 * Extract the LSN.  Save it only if it is an improvement over what the
	 * site has already ack'ed.
	 */
	if (conn->version == 1) {
		ackp = conn->input.repmgr_msg.cntrl.data;
		if (conn->input.repmgr_msg.cntrl.size != sizeof(ack) ||
		    conn->input.repmgr_msg.rec.size != 0) {
			__db_errx(env, "bad ack msg size");
			return (DB_REP_UNAVAIL);
		}
	} else {
		ackp = &ack;
		if ((ret = __repmgr_ack_unmarshal(env, ackp,
			 conn->input.repmgr_msg.cntrl.data,
			 conn->input.repmgr_msg.cntrl.size, NULL)) != 0)
			return (DB_REP_UNAVAIL);
	}

	/* Ignore stale acks. */
	gen = db_rep->region->gen;
	if (ackp->generation < gen) {
		RPRINT(env, DB_VERB_REPMGR_MISC, (env,
		    "ignoring stale ack (%lu<%lu), from %s",
		     (u_long)ackp->generation, (u_long)gen,
		     __repmgr_format_site_loc(site, location)));
		return (0);
	}
	RPRINT(env, DB_VERB_REPMGR_MISC, (env,
	    "got ack [%lu][%lu](%lu) from %s", (u_long)ackp->lsn.file,
	    (u_long)ackp->lsn.offset, (u_long)ackp->generation,
	    __repmgr_format_site_loc(site, location)));

	if (ackp->generation == gen &&
	    LOG_COMPARE(&ackp->lsn, &site->max_ack) == 1) {
		memcpy(&site->max_ack, &ackp->lsn, sizeof(DB_LSN));
		if ((ret = __repmgr_wake_waiting_senders(env)) != 0)
			return (ret);
	}
	return (0);
}

/*
 * PUBLIC: int __repmgr_write_some __P((ENV *, REPMGR_CONNECTION *));
 */
int
__repmgr_write_some(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	QUEUED_OUTPUT *output;
	REPMGR_FLAT *msg;
	int bytes, ret;

	while (!STAILQ_EMPTY(&conn->outbound_queue)) {
		output = STAILQ_FIRST(&conn->outbound_queue);
		msg = output->msg;
		if ((bytes = send(conn->fd, &msg->data[output->offset],
		    (size_t)msg->length - output->offset, 0)) == SOCKET_ERROR) {
			if ((ret = net_errno) == WOULDBLOCK)
				return (0);
			else {
				__db_err(env, ret, "writing data");
				STAT(env->rep_handle->
				    region->mstat.st_connection_drop++);
				return (DB_REP_UNAVAIL);
			}
		}

		if ((output->offset += (size_t)bytes) >= msg->length) {
			STAILQ_REMOVE_HEAD(&conn->outbound_queue, entries);
			__os_free(env, output);
			conn->out_queue_length--;
			if (--msg->ref_count <= 0)
				__os_free(env, msg);

			/*
			 * We've achieved enough movement to free up at least
			 * one space in the outgoing queue.  Wake any message
			 * threads that may be waiting for space.  Leave
			 * CONGESTED state so that when the queue reaches the
			 * high-water mark again, the filling thread will be
			 * allowed to try waiting again.
			 */
			conn->state = CONN_READY;
			if (conn->blockers > 0 &&
			    (ret = __repmgr_signal(&conn->drained)) != 0)
				return (ret);
		}
	}

#ifdef DB_WIN32
	/*
	 * With the queue now empty, it's time to relinquish ownership of this
	 * connection again, so that the next call to send() can write the
	 * message in line, instead of posting it to the queue for us.
	 */
	if (WSAEventSelect(conn->fd, conn->event_object, FD_READ|FD_CLOSE)
	    == SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "can't remove FD_WRITE event bit");
		return (ret);
	}
#endif

	return (0);
}
