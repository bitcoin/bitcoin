/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2005-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#define	__INCLUDE_NETWORKING	1
#define	__INCLUDE_SELECT_H	1
#include "db_int.h"

/*
 * A very rough guess at the maximum stack space one of our threads could ever
 * need, which we hope is plenty conservative.  This can be patched in the field
 * if necessary.
 */
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
size_t __repmgr_guesstimated_max = (128 * 1024);
#endif

/*
 * Invalid open file descriptor value, that can be used as an out-of-band
 * sentinel to mark our signalling pipe as unopened.
 */
#define	NO_SUCH_FILE_DESC	(-1)

/* Aggregated control info needed for preparing for select() call. */
struct io_info {
	fd_set *reads, *writes;
	int maxfd;
};

static int __repmgr_conn_work __P((ENV *, REPMGR_CONNECTION *, void *));
static int finish_connecting __P((ENV *, REPMGR_CONNECTION *));
static int prepare_io __P((ENV *, REPMGR_CONNECTION *, void *));

/*
 * Starts the thread described in the argument, and stores the resulting thread
 * ID therein.
 *
 * PUBLIC: int __repmgr_thread_start __P((ENV *, REPMGR_RUNNABLE *));
 */
int
__repmgr_thread_start(env, runnable)
	ENV *env;
	REPMGR_RUNNABLE *runnable;
{
	pthread_attr_t *attrp;
#ifdef _POSIX_THREAD_ATTR_STACKSIZE
	pthread_attr_t attributes;
	size_t size;
	int ret;
#endif

	runnable->finished = FALSE;

#ifdef _POSIX_THREAD_ATTR_STACKSIZE
	attrp = &attributes;
	if ((ret = pthread_attr_init(&attributes)) != 0) {
		__db_err(env,
		    ret, "pthread_attr_init in repmgr_thread_start");
		return (ret);
	}

	/*
	 * On a 64-bit machine it seems reasonable that we could need twice as
	 * much stack space as we did on a 32-bit machine.
	 */
	size = __repmgr_guesstimated_max;
	if (sizeof(size_t) > 4)
		size *= 2;
#ifdef PTHREAD_STACK_MIN
	if (size < PTHREAD_STACK_MIN)
		size = PTHREAD_STACK_MIN;
#endif
	if ((ret = pthread_attr_setstacksize(&attributes, size)) != 0) {
		__db_err(env,
		    ret, "pthread_attr_setstacksize in repmgr_thread_start");
		return (ret);
	}
#else
	attrp = NULL;
#endif

	return (pthread_create(&runnable->thread_id, attrp,
		    runnable->run, env));
}

/*
 * PUBLIC: int __repmgr_thread_join __P((REPMGR_RUNNABLE *));
 */
int
__repmgr_thread_join(thread)
	REPMGR_RUNNABLE *thread;
{
	return (pthread_join(thread->thread_id, NULL));
}

/*
 * PUBLIC: int __repmgr_set_nonblocking __P((socket_t));
 */
int
__repmgr_set_nonblocking(fd)
	socket_t fd;
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL, 0)) < 0)
		return (errno);
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return (errno);
	return (0);
}

/*
 * PUBLIC: int __repmgr_wake_waiting_senders __P((ENV *));
 *
 * Wake any send()-ing threads waiting for an acknowledgement.
 *
 * !!!
 * Caller must hold the db_rep->mutex, if this thread synchronization is to work
 * properly.
 */
int
__repmgr_wake_waiting_senders(env)
	ENV *env;
{
	return (pthread_cond_broadcast(&env->rep_handle->ack_condition));
}

/*
 * PUBLIC: int __repmgr_await_ack __P((ENV *, const DB_LSN *));
 *
 * Waits (a limited time) for configured number of remote sites to ack the given
 * LSN.
 *
 * !!!
 * Caller must hold repmgr->mutex.
 */
int
__repmgr_await_ack(env, lsnp)
	ENV *env;
	const DB_LSN *lsnp;
{
	DB_REP *db_rep;
	struct timespec deadline;
	int ret, timed;

	db_rep = env->rep_handle;

	if ((timed = (db_rep->ack_timeout > 0)))
		__repmgr_compute_wait_deadline(env, &deadline,
		    db_rep->ack_timeout);
	else
		COMPQUIET(deadline.tv_sec, 0);

	while (!__repmgr_is_permanent(env, lsnp)) {
		if (timed)
			ret = pthread_cond_timedwait(&db_rep->ack_condition,
			    db_rep->mutex, &deadline);
		else
			ret = pthread_cond_wait(&db_rep->ack_condition,
			    db_rep->mutex);
		if (db_rep->finished)
			return (DB_REP_UNAVAIL);
		if (ret != 0)
			return (ret);
	}
	return (0);
}

/*
 * __repmgr_compute_wait_deadline --
 *	Computes a deadline time a certain distance into the future.
 *
 * PUBLIC: void __repmgr_compute_wait_deadline __P((ENV*,
 * PUBLIC:    struct timespec *, db_timeout_t));
 */
void
__repmgr_compute_wait_deadline(env, result, wait)
	ENV *env;
	struct timespec *result;
	db_timeout_t wait;
{
	/*
	 * The result is suitable for the pthread_cond_timewait call.  (That
	 * call uses nano-second resolution; elsewhere we use microseconds.)
	 *
	 * Start with "now"; then add the "wait" offset.
	 *
	 * A db_timespec is the same as a "struct timespec" so we can pass
	 * result directly to the underlying Berkeley DB OS routine.
	 *
	 * !!!
	 * We use the system clock for the pthread_cond_timedwait call, but
	 * that's not optimal on systems with monotonic timers.   Instead,
	 * we should call pthread_condattr_setclock on systems where it and
	 * monotonic timers are available, and then configure both this call
	 * and the subsequent pthread_cond_timewait call to use a monotonic
	 * timer.
	 */
	__os_gettime(env, (db_timespec *)result, 0);
	TIMESPEC_ADD_DB_TIMEOUT(result, wait);
}

/*
 * PUBLIC: int __repmgr_await_drain __P((ENV *,
 * PUBLIC:    REPMGR_CONNECTION *, db_timeout_t));
 *
 * Waits for space to become available on the connection's output queue.
 * Various ways we can exit:
 *
 * 1. queue becomes non-full
 * 2. exceed time limit
 * 3. connection becomes defunct (due to error in another thread)
 * 4. repmgr is shutting down
 * 5. any unexpected system resource failure
 *
 * In cases #3 and #5 we return an error code.  Caller is responsible for
 * distinguishing the remaining cases if desired.
 *
 * !!!
 * Caller must hold repmgr->mutex.
 */
int
__repmgr_await_drain(env, conn, timeout)
	ENV *env;
	REPMGR_CONNECTION *conn;
	db_timeout_t timeout;
{
	DB_REP *db_rep;
	struct timespec deadline;
	int ret;

	db_rep = env->rep_handle;

	__repmgr_compute_wait_deadline(env, &deadline, timeout);

	ret = 0;
	while (conn->out_queue_length >= OUT_QUEUE_LIMIT) {
		ret = pthread_cond_timedwait(&conn->drained,
		    db_rep->mutex, &deadline);
		switch (ret) {
		case 0:
			if (db_rep->finished)
				goto out; /* #4. */
			/*
			 * Another thread could have stumbled into an error on
			 * the socket while we were waiting.
			 */
			if (conn->state == CONN_DEFUNCT) {
				ret = DB_REP_UNAVAIL; /* #3. */
				goto out;
			}
			break;
		case ETIMEDOUT:
			conn->state = CONN_CONGESTED;
			ret = 0;
			goto out; /* #2. */
		default:
			goto out; /* #5. */
		}
	}
	/* #1. */

out:
	return (ret);
}

/*
 * PUBLIC: int __repmgr_alloc_cond __P((cond_var_t *));
 *
 * Initialize a condition variable (in allocated space).
 */
int
__repmgr_alloc_cond(c)
	cond_var_t *c;
{
	return (pthread_cond_init(c, NULL));
}

/*
 * PUBLIC: int __repmgr_free_cond __P((cond_var_t *));
 *
 * Clean up a previously initialized condition variable.
 */
int
__repmgr_free_cond(c)
	cond_var_t *c;
{
	return (pthread_cond_destroy(c));
}

/*
 * PUBLIC: void __repmgr_env_create_pf __P((DB_REP *));
 */
void
__repmgr_env_create_pf(db_rep)
	DB_REP *db_rep;
{
	db_rep->read_pipe = db_rep->write_pipe = NO_SUCH_FILE_DESC;
}

/*
 * PUBLIC: int __repmgr_create_mutex_pf __P((mgr_mutex_t *));
 */
int
__repmgr_create_mutex_pf(mutex)
	mgr_mutex_t *mutex;
{
	return (pthread_mutex_init(mutex, NULL));
}

/*
 * PUBLIC: int __repmgr_destroy_mutex_pf __P((mgr_mutex_t *));
 */
int
__repmgr_destroy_mutex_pf(mutex)
	mgr_mutex_t *mutex;
{
	return (pthread_mutex_destroy(mutex));
}

/*
 * PUBLIC: int __repmgr_init __P((ENV *));
 */
int
__repmgr_init(env)
	ENV *env;
{
	DB_REP *db_rep;
	struct sigaction sigact;
	int ack_inited, elect_inited, file_desc[2], queue_inited, ret;

	db_rep = env->rep_handle;

	/*
	 * Make sure we're not ignoring SIGPIPE, 'cuz otherwise we'd be killed
	 * just for trying to write onto a socket that had been reset.  Note
	 * that we don't undo this in case of a later error, since we document
	 * that we leave the signal handling state like this, even after env
	 * close.
	 */
	if (sigaction(SIGPIPE, NULL, &sigact) == -1) {
		ret = errno;
		__db_err(env, ret, "can't access signal handler");
		return (ret);
	}
	if (sigact.sa_handler == SIG_DFL) {
		sigact.sa_handler = SIG_IGN;
		sigact.sa_flags = 0;
		if (sigaction(SIGPIPE, &sigact, NULL) == -1) {
			ret = errno;
			__db_err(env, ret, "can't access signal handler");
			return (ret);
		}
	}

	ack_inited = elect_inited = queue_inited = FALSE;
	if ((ret = pthread_cond_init(&db_rep->ack_condition, NULL)) != 0)
		goto err;
	ack_inited = TRUE;

	if ((ret = pthread_cond_init(&db_rep->check_election, NULL)) != 0)
		goto err;
	elect_inited = TRUE;

	if ((ret = pthread_cond_init(&db_rep->queue_nonempty, NULL)) != 0)
		goto err;
	queue_inited = TRUE;

	if ((ret = pipe(file_desc)) == -1) {
		ret = errno;
		goto err;
	}

	db_rep->read_pipe = file_desc[0];
	db_rep->write_pipe = file_desc[1];
	return (0);
err:
	if (queue_inited)
		(void)pthread_cond_destroy(&db_rep->queue_nonempty);
	if (elect_inited)
		(void)pthread_cond_destroy(&db_rep->check_election);
	if (ack_inited)
		(void)pthread_cond_destroy(&db_rep->ack_condition);
	db_rep->read_pipe = db_rep->write_pipe = NO_SUCH_FILE_DESC;

	return (ret);
}

/*
 * PUBLIC: int __repmgr_deinit __P((ENV *));
 */
int
__repmgr_deinit(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret, t_ret;

	db_rep = env->rep_handle;

	if (!(REPMGR_INITED(db_rep)))
		return (0);

	ret = pthread_cond_destroy(&db_rep->queue_nonempty);

	if ((t_ret = pthread_cond_destroy(&db_rep->check_election)) != 0 &&
	    ret == 0)
		ret = t_ret;

	if ((t_ret = pthread_cond_destroy(&db_rep->ack_condition)) != 0 &&
	    ret == 0)
		ret = t_ret;

	if (close(db_rep->read_pipe) == -1 && ret == 0)
		ret = errno;
	if (close(db_rep->write_pipe) == -1 && ret == 0)
		ret = errno;

	db_rep->read_pipe = db_rep->write_pipe = NO_SUCH_FILE_DESC;
	return (ret);
}

/*
 * PUBLIC: int __repmgr_lock_mutex __P((mgr_mutex_t *));
 */
int
__repmgr_lock_mutex(mutex)
	mgr_mutex_t  *mutex;
{
	return (pthread_mutex_lock(mutex));
}

/*
 * PUBLIC: int __repmgr_unlock_mutex __P((mgr_mutex_t *));
 */
int
__repmgr_unlock_mutex(mutex)
	mgr_mutex_t  *mutex;
{
	return (pthread_mutex_unlock(mutex));
}

/*
 * Signals a condition variable.
 *
 * !!!
 * Caller must hold mutex.
 *
 * PUBLIC: int __repmgr_signal __P((cond_var_t *));
 */
int
__repmgr_signal(v)
	cond_var_t *v;
{
	return (pthread_cond_broadcast(v));
}

/*
 * PUBLIC: int __repmgr_wake_main_thread __P((ENV*));
 */
int
__repmgr_wake_main_thread(env)
	ENV *env;
{
	DB_REP *db_rep;
	u_int8_t any_value;

	COMPQUIET(any_value, 0);
	db_rep = env->rep_handle;

	/*
	 * It doesn't matter what byte value we write.  Just the appearance of a
	 * byte in the stream is enough to wake up the select() thread reading
	 * the pipe.
	 */
	if (write(db_rep->write_pipe, &any_value, 1) == -1)
		return (errno);
	return (0);
}

/*
 * PUBLIC: int __repmgr_writev __P((socket_t, db_iovec_t *, int, size_t *));
 */
int
__repmgr_writev(fd, iovec, buf_count, byte_count_p)
	socket_t fd;
	db_iovec_t *iovec;
	int buf_count;
	size_t *byte_count_p;
{
	int nw;

	if ((nw = writev(fd, iovec, buf_count)) == -1)
		return (errno);
	*byte_count_p = (size_t)nw;
	return (0);
}

/*
 * PUBLIC: int __repmgr_readv __P((socket_t, db_iovec_t *, int, size_t *));
 */
int
__repmgr_readv(fd, iovec, buf_count, byte_count_p)
	socket_t fd;
	db_iovec_t *iovec;
	int buf_count;
	size_t *byte_count_p;
{
	ssize_t nw;

	if ((nw = readv(fd, iovec, buf_count)) == -1)
		return (errno);
	*byte_count_p = (size_t)nw;
	return (0);
}

/*
 * PUBLIC: int __repmgr_select_loop __P((ENV *));
 */
int
__repmgr_select_loop(env)
	ENV *env;
{
	struct timeval select_timeout, *select_timeout_p;
	DB_REP *db_rep;
	db_timespec timeout;
	fd_set reads, writes;
	struct io_info io_info;
	int ret;
	u_int8_t buf[10];	/* arbitrary size */

	db_rep = env->rep_handle;
	/*
	 * Almost this entire thread operates while holding the mutex.  But note
	 * that it never blocks, except in the call to select() (which is the
	 * one place we relinquish the mutex).
	 */
	LOCK_MUTEX(db_rep->mutex);
	if ((ret = __repmgr_first_try_connections(env)) != 0)
		goto out;
	for (;;) {
		FD_ZERO(&reads);
		FD_ZERO(&writes);

		/*
		 * Figure out which sockets to ask for input and output.  It's
		 * simple for the signalling pipe and listen socket; but depends
		 * on backlog states for the connections to other sites.
		 */
		FD_SET((u_int)db_rep->read_pipe, &reads);
		io_info.maxfd = db_rep->read_pipe;

		if (!IS_SUBORDINATE(db_rep)) {
			FD_SET((u_int)db_rep->listen_fd, &reads);
			if (db_rep->listen_fd > io_info.maxfd)
				io_info.maxfd = db_rep->listen_fd;
		}

		io_info.reads = &reads;
		io_info.writes = &writes;
		if ((ret = __repmgr_each_connection(env,
		    prepare_io, &io_info, TRUE)) != 0)
			goto out;

		if (__repmgr_compute_timeout(env, &timeout)) {
			/* Convert the timespec to a timeval. */
			select_timeout.tv_sec = timeout.tv_sec;
			select_timeout.tv_usec = timeout.tv_nsec / NS_PER_US;
			select_timeout_p = &select_timeout;
		} else {
			/* No time-based events, so wait only for I/O. */
			select_timeout_p = NULL;
		}

		UNLOCK_MUTEX(db_rep->mutex);

		if ((ret = select(io_info.maxfd + 1,
		    &reads, &writes, NULL, select_timeout_p)) == -1) {
			switch (ret = errno) {
			case EINTR:
			case EWOULDBLOCK:
				LOCK_MUTEX(db_rep->mutex);
				continue; /* simply retry */
			default:
				__db_err(env, ret, "select");
				return (ret);
			}
		}
		LOCK_MUTEX(db_rep->mutex);

		/*
		 * Timer expiration events include retrying of lost connections.
		 * Obviously elements can be added to the connection list there.
		 */
		if ((ret = __repmgr_check_timeouts(env)) != 0)
			goto out;

		if ((ret = __repmgr_each_connection(env,
		    __repmgr_conn_work, &io_info, TRUE)) != 0)
			goto out;

		/*
		 * Read any bytes in the signalling pipe.  Note that we don't
		 * actually need to do anything with them; they're just there to
		 * wake us up when necessary.
		 */
		if (FD_ISSET((u_int)db_rep->read_pipe, &reads)) {
			if (read(db_rep->read_pipe, buf, sizeof(buf)) <= 0) {
				ret = errno;
				goto out;
			} else if (db_rep->finished) {
				ret = 0;
				goto out;
			}
		}
		/*
		 * Obviously elements can be added to the connection list here.
		 */
		if (!IS_SUBORDINATE(db_rep) &&
		    FD_ISSET((u_int)db_rep->listen_fd, &reads) &&
		    (ret = __repmgr_accept(env)) != 0)
			goto out;
	}
out:
	UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}

/*
 * Examines a connection to see what sort of I/O to ask for.  Clean up defunct
 * connections.
 */
static int
prepare_io(env, conn, info_)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *info_;
{
	struct io_info *info;

	info = info_;

	if (conn->state == CONN_DEFUNCT)
		return (__repmgr_cleanup_connection(env, conn));

	if (conn->state == CONN_CONNECTING) {
		FD_SET((u_int)conn->fd, info->reads);
		FD_SET((u_int)conn->fd, info->writes);
		if (conn->fd > info->maxfd)
			info->maxfd = conn->fd;
		return (0);
	}

	if (!STAILQ_EMPTY(&conn->outbound_queue)) {
		FD_SET((u_int)conn->fd, info->writes);
		if (conn->fd > info->maxfd)
			info->maxfd = conn->fd;
	}
	/*
	 * For now we always accept incoming data.  If we ever implement some
	 * kind of flow control, we should override it for fledgling connections
	 * (!IS_VALID_EID(conn->eid)) -- in other words, allow reading such a
	 * connection even during flow control duress.
	 */
	FD_SET((u_int)conn->fd, info->reads);
	if (conn->fd > info->maxfd)
		info->maxfd = conn->fd;

	return (0);
}

/*
 * Examine a connection, to see what work needs to be done.
 */
static int
__repmgr_conn_work(env, conn, info_)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *info_;
{
	struct io_info *info;
	int ret;
	u_int fd;

	ret = 0;
	fd = (u_int)conn->fd;
	info = info_;

	if (conn->state == CONN_DEFUNCT)
		return (0);

	if (conn->state == CONN_CONNECTING) {
		if (FD_ISSET(fd, info->reads) || FD_ISSET(fd, info->writes))
			ret = finish_connecting(env, conn);
	} else {
		/*
		 * Here, the site is connected, and the FD_SET's are valid.
		 */
		if (FD_ISSET(fd, info->writes))
			ret = __repmgr_write_some(env, conn);

		if (ret == 0 && FD_ISSET(fd, info->reads))
			ret = __repmgr_read_from_site(env, conn);
	}

	if (ret == DB_REP_UNAVAIL)
		ret = __repmgr_bust_connection(env, conn);
	return (ret);
}

static int
finish_connecting(env, conn)
	ENV *env;
	REPMGR_CONNECTION *conn;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	socklen_t len;
	SITE_STRING_BUFFER buffer;
	u_int eid;
	int error, ret;

	db_rep = env->rep_handle;

	DB_ASSERT(env, IS_VALID_EID(conn->eid));
	eid = (u_int)conn->eid;
	site = SITE_FROM_EID(eid);

	len = sizeof(error);
	if (getsockopt(
	    conn->fd, SOL_SOCKET, SO_ERROR, (sockopt_t)&error, &len) < 0)
		goto err_rpt;
	if (error) {
		errno = error;
		goto err_rpt;
	}

	conn->state = CONN_CONNECTED;
	__os_gettime(env, &site->last_rcvd_timestamp, 1);
	return (__repmgr_propose_version(env, conn));

err_rpt:
	__db_err(env, errno,
	    "connecting to %s", __repmgr_format_site_loc(site, buffer));

	/* If we've exhausted the list of possible addresses, give up. */
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
