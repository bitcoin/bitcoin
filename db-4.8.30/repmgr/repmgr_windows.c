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

/* Convert time-out from microseconds to milliseconds, rounding up. */
#define	DB_TIMEOUT_TO_WINDOWS_TIMEOUT(t) (((t) + (US_PER_MS - 1)) / US_PER_MS)

typedef struct __ack_waiter {
	HANDLE event;
	const DB_LSN *lsnp;
	int next_free;
} ACK_WAITER;

#define	WAITER_SLOT_IN_USE(w) ((w)->lsnp != NULL)

/*
 * Array slots [0:next_avail-1] are initialized, and either in use or on the
 * free list.  Slots beyond that are virgin territory, whose memory contents
 * could be garbage.  In particular, note that slots [0:next_avail-1] have a
 * Win32 Event Object created for them, which have to be freed when cleaning up
 * this data structure.
 *
 * "first_free" points to a list of not-in-use slots threaded through the first
 * section of the array.
 */
struct __ack_waiters_table {
	struct __ack_waiter *array;
	int size;
	int next_avail;
	int first_free;
};

/*
 * Aggregated control info needed for preparing for WSAWaitForMultipleEvents()
 * call.
 */
struct io_info {
	REPMGR_CONNECTION **connections;
	WSAEVENT *events;
	DWORD nevents;
};

static int allocate_wait_slot __P((ENV *, int *));
static void free_wait_slot __P((ENV *, int));
static int handle_completion __P((ENV *, REPMGR_CONNECTION *));
static int finish_connecting __P((ENV *, REPMGR_CONNECTION *,
				     LPWSANETWORKEVENTS));
static int prepare_io __P((ENV *, REPMGR_CONNECTION *, void *));

int
__repmgr_thread_start(env, runnable)
	ENV *env;
	REPMGR_RUNNABLE *runnable;
{
	HANDLE thread_id;

	runnable->finished = FALSE;

	thread_id = CreateThread(NULL, 0,
	    (LPTHREAD_START_ROUTINE)runnable->run, env, 0, NULL);
	if (thread_id == NULL)
		return (GetLastError());
	runnable->thread_id = thread_id;
	return (0);
}

int
__repmgr_thread_join(thread)
	REPMGR_RUNNABLE *thread;
{
	if (WaitForSingleObject(thread->thread_id, INFINITE) == WAIT_OBJECT_0)
		return (0);
	return (GetLastError());
}

int
__repmgr_set_nonblocking(s)
	SOCKET s;
{
	int ret;
	u_long arg;

	arg = 1;		/* any non-zero value */
	if ((ret = ioctlsocket(s, FIONBIO, &arg)) == SOCKET_ERROR)
		return (WSAGetLastError());
	return (0);
}

/*
 * Wake any send()-ing threads waiting for an acknowledgement.
 *
 * !!!
 * Caller must hold the repmgr->mutex, if this thread synchronization is to work
 * properly.
 */
int
__repmgr_wake_waiting_senders(env)
	ENV *env;
{
	ACK_WAITER *slot;
	DB_REP *db_rep;
	int i, ret;

	ret = 0;
	db_rep = env->rep_handle;
	for (i = 0; i < db_rep->waiters->next_avail; i++) {
		 slot = &db_rep->waiters->array[i];
		 if (!WAITER_SLOT_IN_USE(slot))
			 continue;
		 if (__repmgr_is_permanent(env, slot->lsnp))
			 if (!SetEvent(slot->event) && ret == 0)
				 ret = GetLastError();
	}
	return (ret);
}

/*
 * !!!
 * Caller must hold mutex.
 */
int
__repmgr_await_ack(env, lsnp)
	ENV *env;
	const DB_LSN *lsnp;
{
	ACK_WAITER *waiter;
	DB_REP *db_rep;
	DWORD ret, timeout;
	int i;

	db_rep = env->rep_handle;

	if ((ret = allocate_wait_slot(env, &i)) != 0)
		goto err;
	waiter = &db_rep->waiters->array[i];

	timeout = db_rep->ack_timeout > 0 ?
	    DB_TIMEOUT_TO_WINDOWS_TIMEOUT(db_rep->ack_timeout) : INFINITE;
	waiter->lsnp = lsnp;
	if ((ret = SignalObjectAndWait(*db_rep->mutex, waiter->event, timeout,
	    FALSE)) == WAIT_FAILED) {
		ret = GetLastError();
	} else if (ret == WAIT_TIMEOUT)
		ret = DB_REP_UNAVAIL;
	else
		DB_ASSERT(env, ret == WAIT_OBJECT_0);

	LOCK_MUTEX(db_rep->mutex);
	free_wait_slot(env, i);

err:
	return (ret);
}

/*
 * !!!
 * Caller must hold the mutex.
 */
static int
allocate_wait_slot(env, resultp)
	ENV *env;
	int *resultp;
{
	ACK_WAITER *w;
	ACK_WAITERS_TABLE *table;
	DB_REP *db_rep;
	int i, ret;

	db_rep = env->rep_handle;
	table = db_rep->waiters;
	if (table->first_free == -1) {
		if (table->next_avail >= table->size) {
			/*
			 * Grow the array.
			 */
			table->size *= 2;
			w = table->array;
			if ((ret = __os_realloc(env, table->size * sizeof(*w),
			     &w)) != 0)
				return (ret);
			table->array = w;
		}
		/*
		 * Here if, one way or another, we're good to go for using the
		 * next slot (for the first time).
		 */
		i = table->next_avail++;
		w = &table->array[i];
		if ((w->event = CreateEvent(NULL, FALSE, FALSE, NULL)) ==
		    NULL) {
			/*
			 * Maintain the sanctity of our rule that
			 * [0:next_avail-1] contain valid Event Objects.
			 */
			--table->next_avail;
			return (GetLastError());
		}
	} else {
		i = table->first_free;
		w = &table->array[i];
		table->first_free = w->next_free;
	}
	*resultp = i;
	return (0);
}

static void
free_wait_slot(env, slot_index)
	ENV *env;
	int slot_index;
{
	DB_REP *db_rep;
	ACK_WAITER *slot;

	db_rep = env->rep_handle;
	slot = &db_rep->waiters->array[slot_index];

	slot->lsnp = NULL;	/* show it's not in use */
	slot->next_free = db_rep->waiters->first_free;
	db_rep->waiters->first_free = slot_index;
}

/* (See requirements described in repmgr_posix.c.) */
int
__repmgr_await_drain(env, conn, timeout)
	ENV *env;
	REPMGR_CONNECTION *conn;
	db_timeout_t timeout;
{
	DB_REP *db_rep;
	db_timespec deadline, delta, now;
	db_timeout_t t;
	DWORD duration, ret;
	int round_up;

	db_rep = env->rep_handle;

	__os_gettime(env, &deadline, 1);
	TIMESPEC_ADD_DB_TIMEOUT(&deadline, timeout);

	while (conn->out_queue_length >= OUT_QUEUE_LIMIT) {
		if (!ResetEvent(conn->drained))
			return (GetLastError());

		/* How long until the deadline? */
		__os_gettime(env, &now, 1);
		if (timespeccmp(&now, &deadline, >=)) {
			conn->state = CONN_CONGESTED;
			return (0);
		}
		delta = deadline;
		timespecsub(&delta, &now);
		round_up = TRUE;
		DB_TIMESPEC_TO_TIMEOUT(t, &delta, round_up);
		duration = DB_TIMEOUT_TO_WINDOWS_TIMEOUT(t);

		ret = SignalObjectAndWait(*db_rep->mutex,
		    conn->drained, duration, FALSE);
		LOCK_MUTEX(db_rep->mutex);
		if (ret == WAIT_FAILED)
			return (GetLastError());
		else if (ret == WAIT_TIMEOUT) {
			conn->state = CONN_CONGESTED;
			return (0);
		} else
			DB_ASSERT(env, ret == WAIT_OBJECT_0);

		if (db_rep->finished)
			return (0);
		if (conn->state == CONN_DEFUNCT)
			return (DB_REP_UNAVAIL);
	}
	return (0);
}

/*
 * Creates a manual reset event, which is usually our best choice when we may
 * have multiple threads waiting on a single event.
 */
int
__repmgr_alloc_cond(c)
	cond_var_t *c;
{
	HANDLE event;

	if ((event = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
		return (GetLastError());
	*c = event;
	return (0);
}

int
__repmgr_free_cond(c)
	cond_var_t *c;
{
	if (CloseHandle(*c))
		return (0);
	return (GetLastError());
}

void
__repmgr_env_create_pf(db_rep)
	DB_REP *db_rep;
{
	db_rep->waiters = NULL;
}

int
__repmgr_create_mutex_pf(mutex)
	mgr_mutex_t *mutex;
{
	if ((*mutex = CreateMutex(NULL, FALSE, NULL)) == NULL)
		return (GetLastError());
	return (0);
}

int
__repmgr_destroy_mutex_pf(mutex)
	mgr_mutex_t  *mutex;
{
	return (CloseHandle(*mutex) ? 0 : GetLastError());
}

int
__repmgr_init(env)
     ENV *env;
{
#define	INITIAL_ALLOCATION 5		/* arbitrary size */
	DB_REP *db_rep;
	ACK_WAITERS_TABLE *table;
	WSADATA wsaData;
	int ret;

	db_rep = env->rep_handle;
	table = NULL;

	if ((ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
		__db_err(env, ret, "unable to initialize Windows networking");
		return (ret);
	}

	if ((db_rep->signaler = CreateEvent(NULL, /* security attr */
	    FALSE,	/* (not) of the manual reset variety  */
	    FALSE,		/* (not) initially signaled */
	    NULL)) == NULL)		/* name */
		goto geterr;

	if ((db_rep->queue_nonempty = CreateEvent(NULL, TRUE, FALSE, NULL))
	    == NULL)
		goto geterr;

	if ((db_rep->check_election = CreateEvent(NULL, FALSE, FALSE, NULL))
	    == NULL)
		goto geterr;

	if ((ret = __os_calloc(env, 1, sizeof(ACK_WAITERS_TABLE), &table))
	    != 0)
		goto err;

	if ((ret = __os_calloc(env, INITIAL_ALLOCATION, sizeof(ACK_WAITER),
	    &table->array)) != 0)
		goto err;

	table->size = INITIAL_ALLOCATION;
	table->first_free = -1;
	table->next_avail = 0;

	/* There's a restaurant joke in there somewhere. */
	db_rep->waiters = table;
	return (0);

geterr:
	ret = GetLastError();
err:
	if (db_rep->check_election != NULL)
		CloseHandle(db_rep->check_election);
	if (db_rep->queue_nonempty != NULL)
		CloseHandle(db_rep->queue_nonempty);
	if (db_rep->signaler != NULL)
		CloseHandle(db_rep->signaler);
	if (table != NULL)
		__os_free(env, table);
	db_rep->signaler =
	    db_rep->queue_nonempty = db_rep->check_election = NULL;
	db_rep->waiters = NULL;
	(void)WSACleanup();
	return (ret);
}

int
__repmgr_deinit(env)
     ENV *env;
{
	DB_REP *db_rep;
	int i, ret;

	db_rep = env->rep_handle;
	if (!(REPMGR_INITED(db_rep)))
		return (0);

	ret = 0;
	if (WSACleanup() == SOCKET_ERROR)
		ret = WSAGetLastError();

	for (i = 0; i < db_rep->waiters->next_avail; i++) {
		if (!CloseHandle(db_rep->waiters->array[i].event) && ret == 0)
			ret = GetLastError();
	}
	__os_free(env, db_rep->waiters->array);
	__os_free(env, db_rep->waiters);

	if (!CloseHandle(db_rep->check_election) && ret == 0)
		ret = GetLastError();

	if (!CloseHandle(db_rep->queue_nonempty) && ret == 0)
		ret = GetLastError();

	if (!CloseHandle(db_rep->signaler) && ret == 0)
		ret = GetLastError();

	db_rep->waiters = NULL;
	return (ret);
}

int
__repmgr_lock_mutex(mutex)
	mgr_mutex_t  *mutex;
{
	if (WaitForSingleObject(*mutex, INFINITE) == WAIT_OBJECT_0)
		return (0);
	return (GetLastError());
}

int
__repmgr_unlock_mutex(mutex)
	mgr_mutex_t  *mutex;
{
	if (ReleaseMutex(*mutex))
		return (0);
	return (GetLastError());
}

int
__repmgr_signal(v)
	cond_var_t *v;
{
	return (SetEvent(*v) ? 0 : GetLastError());
}

int
__repmgr_wake_main_thread(env)
	ENV *env;
{
	if (!SetEvent(env->rep_handle->signaler))
		return (GetLastError());
	return (0);
}

int
__repmgr_writev(fd, iovec, buf_count, byte_count_p)
	socket_t fd;
	db_iovec_t *iovec;
	int buf_count;
	size_t *byte_count_p;
{
	DWORD bytes;

	if (WSASend(fd, iovec,
	    (DWORD)buf_count, &bytes, 0, NULL, NULL) == SOCKET_ERROR)
		return (net_errno);

	*byte_count_p = (size_t)bytes;
	return (0);
}

int
__repmgr_readv(fd, iovec, buf_count, xfr_count_p)
	socket_t fd;
	db_iovec_t *iovec;
	int buf_count;
	size_t *xfr_count_p;
{
	DWORD bytes, flags;

	flags = 0;
	if (WSARecv(fd, iovec,
	    (DWORD)buf_count, &bytes, &flags, NULL, NULL) == SOCKET_ERROR)
		return (net_errno);

	*xfr_count_p = (size_t)bytes;
	return (0);
}

int
__repmgr_select_loop(env)
	ENV *env;
{
	DB_REP *db_rep;
	DWORD ret;
	DWORD select_timeout;
	REPMGR_CONNECTION *connections[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
	db_timespec timeout;
	WSAEVENT listen_event;
	WSANETWORKEVENTS net_events;
	struct io_info io_info;
	int i;

	db_rep = env->rep_handle;
	io_info.connections = connections;
	io_info.events = events;

	if ((listen_event = WSACreateEvent()) == WSA_INVALID_EVENT) {
		__db_err(
		    env, net_errno, "can't create event for listen socket");
		return (net_errno);
	}
	if (!IS_SUBORDINATE(db_rep) &&
	    WSAEventSelect(db_rep->listen_fd, listen_event, FD_ACCEPT) ==
	    SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "can't enable event for listener");
		goto out;
	}

	LOCK_MUTEX(db_rep->mutex);
	if ((ret = __repmgr_first_try_connections(env)) != 0)
		goto unlock;
	for (;;) {
		/* Start with the two events that we always wait for. */
#define	SIGNALER_INDEX	0
#define	LISTENER_INDEX	1
		events[SIGNALER_INDEX] = db_rep->signaler;
		if (IS_SUBORDINATE(db_rep))
			io_info.nevents = 1;
		else {
			events[LISTENER_INDEX] = listen_event;
			io_info.nevents = 2;
		}

		if ((ret = __repmgr_each_connection(env,
		    prepare_io, &io_info, TRUE)) != 0)
			goto unlock;

		if (__repmgr_compute_timeout(env, &timeout))
			select_timeout =
			    (DWORD)(timeout.tv_sec * MS_PER_SEC +
			    timeout.tv_nsec / NS_PER_MS);
		else {
			/* No time-based events to wake us up. */
			select_timeout = WSA_INFINITE;
		}

		UNLOCK_MUTEX(db_rep->mutex);
		ret = WSAWaitForMultipleEvents(
		    io_info.nevents, events, FALSE, select_timeout, FALSE);
		if (db_rep->finished) {
			ret = 0;
			goto out;
		}
		LOCK_MUTEX(db_rep->mutex);

		/*
		 * !!!
		 * Note that `ret' remains set as the return code from
		 * WSAWaitForMultipleEvents, above.
		 */
		if (ret >= WSA_WAIT_EVENT_0 &&
		    ret < WSA_WAIT_EVENT_0 + io_info.nevents) {
			if ((i = ret - WSA_WAIT_EVENT_0) == SIGNALER_INDEX) {
				/* Another thread woke us. */
			} else if (!IS_SUBORDINATE(db_rep) &&
			    i == LISTENER_INDEX) {
				if ((ret = WSAEnumNetworkEvents(
				    db_rep->listen_fd, listen_event,
				    &net_events)) == SOCKET_ERROR) {
					ret = net_errno;
					goto unlock;
				}
				DB_ASSERT(env,
				    net_events.lNetworkEvents & FD_ACCEPT);
				if ((ret = net_events.iErrorCode[FD_ACCEPT_BIT])
				    != 0)
					goto unlock;
				if ((ret = __repmgr_accept(env)) != 0)
					goto unlock;
			} else {
				if (connections[i]->state != CONN_DEFUNCT &&
				    (ret = handle_completion(env,
				    connections[i])) != 0)
					goto unlock;
			}
		} else if (ret == WSA_WAIT_TIMEOUT) {
			if ((ret = __repmgr_check_timeouts(env)) != 0)
				goto unlock;
		} else if (ret == WSA_WAIT_FAILED) {
			ret = net_errno;
			goto unlock;
		}
	}

unlock:
	UNLOCK_MUTEX(db_rep->mutex);
out:
	if (!CloseHandle(listen_event) && ret == 0)
		ret = GetLastError();
	return (ret);
}

static int
prepare_io(env, conn, info_)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *info_;
{
	struct io_info *info;

	if (conn->state == CONN_DEFUNCT)
		return (__repmgr_cleanup_connection(env, conn));

	/*
	 * Note that even if we're suffering flow control, we
	 * nevertheless still read if we haven't even yet gotten
	 * a handshake.  Why?  (1) Handshakes are important; and
	 * (2) they don't hurt anything flow-control-wise.
	 */
	info = info_;

	/*
	 * If we ever implemented flow control, we would have some conditions to
	 * examine here.  But as it is, we always are willing to accept I/O on
	 * every connection.
	 *
	 * We can only handle as many connections as the number of events the
	 * WSAWaitForMultipleEvents function allows (minus 2, for our overhead:
	 * the listener and the signaler).
	 */
	DB_ASSERT(env, info->nevents < WSA_MAXIMUM_WAIT_EVENTS);
	info->events[info->nevents] = conn->event_object;
	info->connections[info->nevents++] = conn;

	return (0);
}

static int
handle_completion(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	int ret;
	WSANETWORKEVENTS events;

	if ((ret = WSAEnumNetworkEvents(conn->fd, conn->event_object, &events))
	    == SOCKET_ERROR) {
		__db_err(env, net_errno, "EnumNetworkEvents");
		STAT(env->rep_handle->region->mstat.st_connection_drop++);
		ret = DB_REP_UNAVAIL;
		goto err;
	}

	if (conn->state == CONN_CONNECTING) {
		if ((ret = finish_connecting(env, conn, &events)) != 0)
			goto err;
	} else {		/* Check both writing and reading. */
		if (events.lNetworkEvents & FD_CLOSE) {
			__db_err(env,
			    events.iErrorCode[FD_CLOSE_BIT],
			    "connection closed");
			STAT(env->rep_handle->
			    region->mstat.st_connection_drop++);
			ret = DB_REP_UNAVAIL;
			goto err;
		}

		if (events.lNetworkEvents & FD_WRITE) {
			if (events.iErrorCode[FD_WRITE_BIT] != 0) {
				__db_err(env,
				    events.iErrorCode[FD_WRITE_BIT],
				    "error writing");
				STAT(env->rep_handle->
				    region->mstat.st_connection_drop++);
				ret = DB_REP_UNAVAIL;
				goto err;
			} else if ((ret =
			    __repmgr_write_some(env, conn)) != 0)
				goto err;
		}

		if (events.lNetworkEvents & FD_READ) {
			if (events.iErrorCode[FD_READ_BIT] != 0) {
				__db_err(env,
				    events.iErrorCode[FD_READ_BIT],
				    "error reading");
				STAT(env->rep_handle->
				    region->mstat.st_connection_drop++);
				ret = DB_REP_UNAVAIL;
				goto err;
			} else if ((ret =
			    __repmgr_read_from_site(env, conn)) != 0)
				goto err;
		}
	}

err:
	if (ret == DB_REP_UNAVAIL)
		ret = __repmgr_bust_connection(env, conn);
	return (ret);
}

static int
finish_connecting(env, conn, events)
	ENV *env;
	REPMGR_CONNECTION *conn;
	LPWSANETWORKEVENTS events;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	u_int eid;
/*	char reason[100]; */
	int ret/*, t_ret*/;
/*	DWORD_PTR values[1]; */

	if (!(events->lNetworkEvents & FD_CONNECT))
		return (0);

	db_rep = env->rep_handle;

	DB_ASSERT(env, IS_VALID_EID(conn->eid));
	eid = (u_int)conn->eid;
	site = SITE_FROM_EID(eid);

	if ((ret = events->iErrorCode[FD_CONNECT_BIT]) != 0) {
/*		t_ret = FormatMessage( */
/*		    FORMAT_MESSAGE_IGNORE_INSERTS | */
/*		    FORMAT_MESSAGE_FROM_SYSTEM | */
/*		    FORMAT_MESSAGE_ARGUMENT_ARRAY, */
/*		    NULL, ret, 0, (LPTSTR)reason, sizeof(reason), values); */
/*		__db_err(env/\*, ret*\/, "connecting: %s", */
/*		    reason); */
/*		LocalFree(reason); */
		__db_err(env, ret, "connecting");
		goto err;
	}

	conn->state = CONN_CONNECTED;
	__os_gettime(env, &site->last_rcvd_timestamp, 1);

	if (WSAEventSelect(conn->fd, conn->event_object, FD_READ | FD_CLOSE) ==
	    SOCKET_ERROR) {
		ret = net_errno;
		__db_err(env, ret, "setting event bits for reading");
		return (ret);
	}

	return (__repmgr_propose_version(env, conn));

err:

	if (ADDR_LIST_NEXT(&site->net_addr) == NULL) {
		STAT(db_rep->region->mstat.st_connect_fail++);
		return (DB_REP_UNAVAIL);
	}

	/*
	 * Since we're immediately trying the next address in the list, simply
	 * disable the failed connection, without the usual recovery.
	 */
	__repmgr_disable_connection(env, conn);

	ret = __repmgr_connect_site(env, eid);
	DB_ASSERT(env, ret != DB_REP_UNAVAIL);
	return (ret);
}
