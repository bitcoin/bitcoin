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

#define	INITIAL_SITES_ALLOCATION	10	     /* Arbitrary guess. */
#define	RETRY_TIME_ADJUST		200000	     /* Arbitrary estimate. */

static int __repmgr_addrcmp __P((repmgr_netaddr_t *, repmgr_netaddr_t *));

/*
 * Schedules a future attempt to re-establish a connection with the given site.
 * Usually, we wait the configured retry_wait period.  But if the "immediate"
 * parameter is given as TRUE, we'll make the wait time 0, and put the request
 * at the _beginning_ of the retry queue.
 *
 * PUBLIC: int __repmgr_schedule_connection_attempt __P((ENV *, u_int, int));
 *
 * !!!
 * Caller should hold mutex.
 *
 * Unless an error occurs, we always attempt to wake the main thread;
 * __repmgr_bust_connection relies on this behavior.
 */
int
__repmgr_schedule_connection_attempt(env, eid, immediate)
	ENV *env;
	u_int eid;
	int immediate;
{
	DB_REP *db_rep;
	REPMGR_RETRY *retry, *target;
	REPMGR_SITE *site;
	db_timespec t;
	int cmp, ret;

	db_rep = env->rep_handle;
	if ((ret = __os_malloc(env, sizeof(*retry), &retry)) != 0)
		return (ret);

	site = SITE_FROM_EID(eid);
	__os_gettime(env, &t, 1);
	if (immediate)
		TAILQ_INSERT_HEAD(&db_rep->retries, retry, entries);
	else {
		TIMESPEC_ADD_DB_TIMEOUT(&t, db_rep->connection_retry_wait);

		/*
		 * Although it's extremely rare, two sites could be trying to
		 * connect to each other simultaneously, and each could kill its
		 * own connection when it received the other's.  And this could
		 * continue, in sync, since configured retry times are usually
		 * the same.  So, perturb one site's retry time by a small
		 * amount to break the cycle.  Since each site has its own
		 * address, it's always possible to decide which is "greater
		 * than".
		 *     (The mnemonic is that a server conventionally has a
		 * small well-known port number.  And clients have the right to
		 * connect to servers, not the other way around.)
		 */
		cmp = __repmgr_addrcmp(&site->net_addr, &db_rep->my_addr);
		DB_ASSERT(env, cmp != 0);
		if (cmp == 1)
			TIMESPEC_ADD_DB_TIMEOUT(&t, RETRY_TIME_ADJUST);

		/*
		 * Insert the new "retry" on the (time-ordered) list in its
		 * proper position.  To do so, find the list entry ("target")
		 * with a later time; insert the new entry just before that.
		 */
		TAILQ_FOREACH(target, &db_rep->retries, entries) {
			if (timespeccmp(&target->time, &t, >))
				break;
		}
		if (target == NULL)
			TAILQ_INSERT_TAIL(&db_rep->retries, retry, entries);
		else
			TAILQ_INSERT_BEFORE(target, retry, entries);

	}
	retry->eid = eid;
	retry->time = t;

	site->state = SITE_IDLE;
	site->ref.retry = retry;

	return (__repmgr_wake_main_thread(env));
}

/*
 * Compare two network addresses (lexicographically), and return -1, 0, or 1, as
 * the first is less than, equal to, or greater than the second.
 */
static int
__repmgr_addrcmp(addr1, addr2)
	repmgr_netaddr_t *addr1, *addr2;
{
	int cmp;

	cmp = strcmp(addr1->host, addr2->host);
	if (cmp != 0)
		return (cmp);

	if (addr1->port < addr2->port)
		return (-1);
	else if (addr1->port > addr2->port)
		return (1);
	return (0);
}

/*
 * Initialize the necessary control structures to begin reading a new input
 * message.
 *
 * PUBLIC: void __repmgr_reset_for_reading __P((REPMGR_CONNECTION *));
 */
void
__repmgr_reset_for_reading(con)
	REPMGR_CONNECTION *con;
{
	con->reading_phase = SIZES_PHASE;
	__repmgr_iovec_init(&con->iovecs);
	__repmgr_add_buffer(&con->iovecs, &con->msg_type,
	    sizeof(con->msg_type));
	__repmgr_add_buffer(&con->iovecs, &con->control_size_buf,
	    sizeof(con->control_size_buf));
	__repmgr_add_buffer(&con->iovecs, &con->rec_size_buf,
	    sizeof(con->rec_size_buf));
}

/*
 * Constructs a DB_REPMGR_CONNECTION structure, and puts it on the main list of
 * connections.  It does not initialize eid, since that isn't needed and/or
 * immediately known in all cases.
 *
 * PUBLIC:  int __repmgr_new_connection __P((ENV *, REPMGR_CONNECTION **,
 * PUBLIC:				   socket_t, int));
 */
int
__repmgr_new_connection(env, connp, s, state)
	ENV *env;
	REPMGR_CONNECTION **connp;
	socket_t s;
	int state;
{
	REPMGR_CONNECTION *c;
	int ret;

	if ((ret = __os_calloc(env, 1, sizeof(REPMGR_CONNECTION), &c)) != 0)
		return (ret);
	if ((ret = __repmgr_alloc_cond(&c->drained)) != 0) {
		__os_free(env, c);
		return (ret);
	}
	c->blockers = 0;

	c->fd = s;
	c->state = state;

	STAILQ_INIT(&c->outbound_queue);
	c->out_queue_length = 0;

	__repmgr_reset_for_reading(c);
	*connp = c;

	return (0);
}

/*
 * PUBLIC: int __repmgr_new_site __P((ENV *, REPMGR_SITE**,
 * PUBLIC:     const char *, u_int, int));
 *
 * Manipulates the process-local copy of the sites list.  So, callers should
 * hold the db_rep->mutex (except for single-threaded, pre-open configuration).
 */
int
__repmgr_new_site(env, sitep, host, port, state)
	ENV *env;
	REPMGR_SITE **sitep;
	const char *host;
	u_int port;
	int state;
{
	DB_REP *db_rep;
	REPMGR_SITE *site;
	char *p;
	u_int new_site_max;
	int ret;

	db_rep = env->rep_handle;
	if (db_rep->site_cnt >= db_rep->site_max) {
		new_site_max = db_rep->site_max == 0 ?
		    INITIAL_SITES_ALLOCATION : db_rep->site_max * 2;
		if ((ret = __os_realloc(env,
		     sizeof(REPMGR_SITE) * new_site_max, &db_rep->sites)) != 0)
			 return (ret);
		db_rep->site_max = new_site_max;
	}
	if ((ret = __os_strdup(env, host, &p)) != 0) {
		/* No harm in leaving the increased site_max intact. */
		return (ret);
	}
	site = &db_rep->sites[db_rep->site_cnt++];

	site->net_addr.host = p;
	site->net_addr.port = (u_int16_t)port;
	site->net_addr.address_list = NULL;
	site->net_addr.current = NULL;

	ZERO_LSN(site->max_ack);
	site->flags = 0;
	timespecclear(&site->last_rcvd_timestamp);
	TAILQ_INIT(&site->sub_conns);
	site->state = state;

	*sitep = site;
	return (0);
}

/*
 * Kind of like a destructor for a repmgr_netaddr_t: cleans up any subordinate
 * allocated memory pointed to by the addr, though it does not free the struct
 * itself.
 *
 * PUBLIC: void __repmgr_cleanup_netaddr __P((ENV *, repmgr_netaddr_t *));
 */
void
__repmgr_cleanup_netaddr(env, addr)
	ENV *env;
	repmgr_netaddr_t *addr;
{
	if (addr->address_list != NULL) {
		__os_freeaddrinfo(env, addr->address_list);
		addr->address_list = addr->current = NULL;
	}
	if (addr->host != NULL) {
		__os_free(env, addr->host);
		addr->host = NULL;
	}
}

/*
 * PUBLIC: void __repmgr_iovec_init __P((REPMGR_IOVECS *));
 */
void
__repmgr_iovec_init(v)
	REPMGR_IOVECS *v;
{
	v->offset = v->count = 0;
	v->total_bytes = 0;
}

/*
 * PUBLIC: void __repmgr_add_buffer __P((REPMGR_IOVECS *, void *, size_t));
 *
 * !!!
 * There is no checking for overflow of the vectors[5] array.
 */
void
__repmgr_add_buffer(v, address, length)
	REPMGR_IOVECS *v;
	void *address;
	size_t length;
{
	v->vectors[v->count].iov_base = address;
	v->vectors[v->count++].iov_len = length;
	v->total_bytes += length;
}

/*
 * PUBLIC: void __repmgr_add_dbt __P((REPMGR_IOVECS *, const DBT *));
 */
void
__repmgr_add_dbt(v, dbt)
	REPMGR_IOVECS *v;
	const DBT *dbt;
{
	v->vectors[v->count].iov_base = dbt->data;
	v->vectors[v->count++].iov_len = dbt->size;
	v->total_bytes += dbt->size;
}

/*
 * Update a set of iovecs to reflect the number of bytes transferred in an I/O
 * operation, so that the iovecs can be used to continue transferring where we
 * left off.
 *     Returns TRUE if the set of buffers is now fully consumed, FALSE if more
 * remains.
 *
 * PUBLIC: int __repmgr_update_consumed __P((REPMGR_IOVECS *, size_t));
 */
int
__repmgr_update_consumed(v, byte_count)
	REPMGR_IOVECS *v;
	size_t byte_count;
{
	db_iovec_t *iov;
	int i;

	for (i = v->offset; ; i++) {
		DB_ASSERT(NULL, i < v->count && byte_count > 0);
		iov = &v->vectors[i];
		if (byte_count > iov->iov_len) {
			/*
			 * We've consumed (more than) this vector's worth.
			 * Adjust count and continue.
			 */
			byte_count -= iov->iov_len;
		} else {
			/*
			 * Adjust length of remaining portion of vector.
			 * byte_count can never be greater than iov_len, or we
			 * would not be in this section of the if clause.
			 */
			iov->iov_len -= (u_int32_t)byte_count;
			if (iov->iov_len > 0) {
				/*
				 * Still some left in this vector.  Adjust base
				 * address too, and leave offset pointing here.
				 */
				iov->iov_base = (void *)
				    ((u_int8_t *)iov->iov_base + byte_count);
				v->offset = i;
			} else {
				/*
				 * Consumed exactly to a vector boundary.
				 * Advance to next vector for next time.
				 */
				v->offset = i+1;
			}
			/*
			 * If offset has reached count, the entire thing is
			 * consumed.
			 */
			return (v->offset >= v->count);
		}
	}
}

/*
 * Builds a buffer containing our network address information, suitable for
 * publishing as cdata via a call to rep_start, and sets up the given DBT to
 * point to it.  The buffer is dynamically allocated memory, and the caller must
 * assume responsibility for it.
 *
 * PUBLIC: int __repmgr_prepare_my_addr __P((ENV *, DBT *));
 */
int
__repmgr_prepare_my_addr(env, dbt)
	ENV *env;
	DBT *dbt;
{
	DB_REP *db_rep;
	size_t size, hlen;
	u_int16_t port_buffer;
	u_int8_t *ptr;
	int ret;

	db_rep = env->rep_handle;

	/*
	 * The cdata message consists of the 2-byte port number, in network byte
	 * order, followed by the null-terminated host name string.
	 */
	port_buffer = htons(db_rep->my_addr.port);
	size = sizeof(port_buffer) +
	    (hlen = strlen(db_rep->my_addr.host) + 1);
	if ((ret = __os_malloc(env, size, &ptr)) != 0)
		return (ret);

	DB_INIT_DBT(*dbt, ptr, size);

	memcpy(ptr, &port_buffer, sizeof(port_buffer));
	ptr = &ptr[sizeof(port_buffer)];
	memcpy(ptr, db_rep->my_addr.host, hlen);

	return (0);
}

/*
 * Provide the appropriate value for nsites, the number of sites in the
 * replication group.  If the application has specified a value, use that.
 * Otherwise, just use the number of sites we know of.
 *
 * !!!
 * This may only be called after the environment has been opened, because we
 * assume we have a rep region.  That should be OK, because we only need this
 * for starting an election, or counting acks after sending a PERM message.
 *
 * PUBLIC: u_int __repmgr_get_nsites __P((DB_REP *));
 */
u_int
__repmgr_get_nsites(db_rep)
	DB_REP *db_rep;
{
	REP *rep;
	u_int32_t nsites;

	rep = db_rep->region;
	nsites = rep->config_nsites;
	if (nsites > 0)
		return ((u_int)nsites);

	/*
	 * The number of other sites in our table, plus 1 to count ourself.
	 */
	return (db_rep->site_cnt + 1);
}

/*
 * PUBLIC: void __repmgr_thread_failure __P((ENV *, int));
 */
void
__repmgr_thread_failure(env, why)
	ENV *env;
	int why;
{
	(void)__repmgr_stop_threads(env);
	(void)__env_panic(env, why);
}

/*
 * Format a printable representation of a site location, suitable for inclusion
 * in an error message.  The buffer must be at least as big as
 * MAX_SITE_LOC_STRING.
 *
 * PUBLIC: char *__repmgr_format_eid_loc __P((DB_REP *, int, char *));
 */
char *
__repmgr_format_eid_loc(db_rep, eid, buffer)
	DB_REP *db_rep;
	int eid;
	char *buffer;
{
	if (IS_VALID_EID(eid))
		return (__repmgr_format_site_loc(SITE_FROM_EID(eid), buffer));

	snprintf(buffer, MAX_SITE_LOC_STRING, "(unidentified site)");
	return (buffer);
}

/*
 * PUBLIC: char *__repmgr_format_site_loc __P((REPMGR_SITE *, char *));
 */
char *
__repmgr_format_site_loc(site, buffer)
	REPMGR_SITE *site;
	char *buffer;
{
	snprintf(buffer, MAX_SITE_LOC_STRING, "site %s:%lu",
	    site->net_addr.host, (u_long)site->net_addr.port);
	return (buffer);
}

/*
 * PUBLIC: int __repmgr_repstart __P((ENV *, u_int32_t));
 */
int
__repmgr_repstart(env, flags)
	ENV *env;
	u_int32_t flags;
{
	DBT my_addr;
	int ret;

	if ((ret = __repmgr_prepare_my_addr(env, &my_addr)) != 0)
		return (ret);
	ret = __rep_start_int(env, &my_addr, flags);
	__os_free(env, my_addr.data);
	if (ret != 0)
		__db_err(env, ret, "rep_start");
	return (ret);
}

/*
 * Visits all the connections we know about, performing the desired action.
 * "err_quit" determines whether we give up, or soldier on, in case of an
 * error.
 *
 * PUBLIC: int __repmgr_each_connection __P((ENV *,
 * PUBLIC:     CONNECTION_ACTION, void *, int));
 *
 * !!!
 * Caller must hold mutex.
 */
int
__repmgr_each_connection(env, callback, info, err_quit)
	ENV *env;
	CONNECTION_ACTION callback;
	void *info;
	int err_quit;
{
	DB_REP *db_rep;
	REPMGR_CONNECTION *conn, *next;
	REPMGR_SITE *site;
	u_int eid;
	int ret, t_ret;

#define	HANDLE_ERROR		        \
	do {			        \
		if (err_quit)	        \
			return (t_ret); \
		if (ret == 0)	        \
			ret = t_ret;    \
	} while (0)

	db_rep = env->rep_handle;
	ret = 0;

	/*
	 * We might have used TAILQ_FOREACH here, except that in some cases we
	 * need to unlink an element along the way.
	 */
	for (conn = TAILQ_FIRST(&db_rep->connections);
	     conn != NULL;
	     conn = next) {
		next = TAILQ_NEXT(conn, entries);

		if ((t_ret = (*callback)(env, conn, info)) != 0)
			HANDLE_ERROR;
	}

	for (eid = 0; eid < db_rep->site_cnt; eid++) {
		site = SITE_FROM_EID(eid);

		if (site->state == SITE_CONNECTED) {
			conn = site->ref.conn;
			if ((t_ret = (*callback)(env, conn, info)) != 0)
				HANDLE_ERROR;
		}

		for (conn = TAILQ_FIRST(&site->sub_conns);
		     conn != NULL;
		     conn = next) {
			next = TAILQ_NEXT(conn, entries);
			if ((t_ret = (*callback)(env, conn, info)) != 0)
				HANDLE_ERROR;
		}
	}

	return (0);
}

/*
 * Initialize repmgr's portion of the shared region area.  Note that we can't
 * simply get the REP* address from the env as we usually do, because at the
 * time of this call it hasn't been linked into there yet.
 *
 * This function is only called during creation of the region.  If anything
 * fails, our caller will panic and remove the region.  So, if we have any
 * failure, we don't have to clean up any partial allocation.
 *
 * PUBLIC: int __repmgr_open __P((ENV *, void *));
 */
int
__repmgr_open(env, rep_)
	ENV *env;
	void *rep_;
{
	DB_REP *db_rep;
	REGINFO *infop;
	REP *rep;
	size_t sz;
	char *host, *hostbuf;
	int ret;

	db_rep = env->rep_handle;
	infop = env->reginfo;
	rep = rep_;

	if ((ret = __mutex_alloc(env, MTX_REPMGR, 0, &rep->mtx_repmgr)) != 0)
		return (ret);

	DB_ASSERT(env, rep->siteaddr_seq == 0 && db_rep->siteaddr_seq == 0);
	rep->netaddr_off = INVALID_ROFF;
	rep->siteaddr_seq = 0;
	if ((ret = __repmgr_share_netaddrs(env, rep, 0, db_rep->site_cnt)) != 0)
		return (ret);
	rep->peer = db_rep->peer;

	if ((host = db_rep->my_addr.host) != NULL) {
		sz = strlen(host) + 1;
		if ((ret = __env_alloc(infop, sz, &hostbuf)) != 0)
			return (ret);
		(void)strcpy(hostbuf, host);
		rep->my_addr.host = R_OFFSET(infop, hostbuf);
		rep->my_addr.port = db_rep->my_addr.port;
		rep->siteaddr_seq++;
	} else
		rep->my_addr.host = INVALID_ROFF;

	if ((ret = __os_malloc(env,
	    sizeof(mgr_mutex_t), &db_rep->mutex)) == 0 &&
	    (ret = __repmgr_create_mutex_pf(db_rep->mutex)) != 0) {
		__os_free(env, db_rep->mutex);
		db_rep->mutex = NULL;
	}
	return (0);
}

/*
 * Join an existing environment, by setting up our local site info structures
 * from shared network address configuration in the region.
 *
 * As __repmgr_open(), note that we can't simply get the REP* address from the
 * env as we usually do, because at the time of this call it hasn't been linked
 * into there yet.
 *
 * PUBLIC: int __repmgr_join __P((ENV *, void *));
 */
int
__repmgr_join(env, rep_)
	ENV *env;
	void *rep_;
{
	DB_REP *db_rep;
	REGINFO *infop;
	REP *rep;
	SITEADDR *p;
	REPMGR_SITE temp, *unused;
	repmgr_netaddr_t *addrp;
	char *host;
	u_int i, j;
	int ret;

	db_rep = env->rep_handle;
	infop = env->reginfo;
	rep = rep_;
	ret = 0;

	MUTEX_LOCK(env, rep->mtx_repmgr);

	if (rep->my_addr.host != INVALID_ROFF) {
		/*
		 * For now just record the config info.  After all, this process
		 * may not be intending ever to start a listener.  If/when it
		 * does, we'll get the necessary address info at that time.
		 */
		host = R_ADDR(infop, rep->my_addr.host);
		if (db_rep->my_addr.host == NULL) {
			if ((ret = __repmgr_pack_netaddr(env, host,
			    rep->my_addr.port, NULL, &db_rep->my_addr)) != 0)
				goto unlock;
		} else if (strcmp(host, db_rep->my_addr.host) != 0 ||
		    rep->my_addr.port != db_rep->my_addr.port) {
			__db_errx(env,
	    "A mismatching local site address has been set in the environment");
			ret = EINVAL;
			goto unlock;
		}
	}

	/*
	 * Merge local and shared lists of remote sites.  Note that the
	 * placement of entries in the shared array must not change.  To
	 * accomplish the merge, pull in entries from the shared list, into the
	 * proper position, shuffling not-yet-resolved local entries if
	 * necessary.  Then add any remaining locally known entries to the
	 * shared list.
	 */
	i = 0;
	if (rep->netaddr_off != INVALID_ROFF) {
		p = R_ADDR(infop, rep->netaddr_off);

		/* For each address in the shared list ... */
		for (; i < rep->site_cnt; i++) {
			host = R_ADDR(infop, p[i].host);

			RPRINT(env, DB_VERB_REPMGR_MISC,
			    (env, "Site %s:%lu found at EID %u",
				host, (u_long)p[i].port, i));
			/*
			 * Find it in the local list.  Everything before 'i'
			 * already matches the shared list, and is therefore in
			 * the right place.  So we only need to search starting
			 * from 'i'.
			 */
			for (j = i; j < db_rep->site_cnt; j++) {
				addrp = &db_rep->sites[j].net_addr;
				if (strcmp(host, addrp->host) == 0 &&
				    p[i].port == addrp->port)
					break;
			}

			if (j == db_rep->site_cnt &&
			    (ret = __repmgr_new_site(env, &unused,
			    host, p[i].port, SITE_IDLE)) != 0)
				goto unlock;
			DB_ASSERT(env, j < db_rep->site_cnt);

			/* Found or added at 'j', but belongs at 'i': swap. */
			if (i != j) {
				temp = db_rep->sites[j];
				db_rep->sites[j] = db_rep->sites[i];
				db_rep->sites[i] = temp;

				/*
				 * Keep peer pointer in sync with swapped
				 * location.
				 */
				if (db_rep->peer == (int)j)
					db_rep->peer = (int)i;
				else if (db_rep->peer == (int)i)
					db_rep->peer = (int)j;
			}
		}
	}
	if ((ret = __repmgr_share_netaddrs(env, rep, i, db_rep->site_cnt)) != 0)
		goto unlock;

	/*
	 * Assume that any config settings I've made locally are "fresher" than
	 * anything lying around in the shared region, so the local setting
	 * overrides here.
	 */
	if (IS_VALID_EID(db_rep->peer))
		rep->peer = db_rep->peer;
	db_rep->siteaddr_seq = rep->siteaddr_seq;

unlock:
	MUTEX_UNLOCK(env, rep->mtx_repmgr);

	if (ret == 0 && (ret =
	    __os_malloc(env, sizeof(mgr_mutex_t), &db_rep->mutex)) == 0 &&
	    (ret = __repmgr_create_mutex_pf(db_rep->mutex)) != 0) {
		__os_free(env, db_rep->mutex);
		db_rep->mutex = NULL;
	}

	return (ret);
}

/*
 * PUBLIC: int __repmgr_env_refresh __P((ENV *env));
 */
int
__repmgr_env_refresh(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret, t_ret;

	db_rep = env->rep_handle;

	ret = 0;
	if (db_rep->mutex != NULL) {
		ret = __repmgr_destroy_mutex_pf(db_rep->mutex);
		__os_free(env, db_rep->mutex);
		db_rep->mutex = NULL;
	}

	if (F_ISSET(env, ENV_PRIVATE) &&
	    (t_ret = __mutex_free(env, &db_rep->region->mtx_repmgr)) != 0 &&
	    ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * Copy network address information from the indicated local array slots into
 * the shared region.
 *
 * PUBLIC: int __repmgr_share_netaddrs __P((ENV *, void *, u_int, u_int));
 *
 * !!! The rep pointer is passed, because it may not yet have been installed
 * into the env handle.
 *
 * !!! Assumes caller holds mtx_repmgr lock.
 */
int
__repmgr_share_netaddrs(env, rep_, start, limit)
	ENV *env;
	void *rep_;
	u_int start, limit;
{
	DB_REP *db_rep;
	REP *rep;
	REGINFO *infop;
	REGENV *renv;
	SITEADDR *orig, *shared_array;
	char *host, *hostbuf;
	size_t sz;
	u_int i, n;
	int eid, ret, touched;

	db_rep = env->rep_handle;
	infop = env->reginfo;
	renv = infop->primary;
	rep = rep_;
	ret = 0;
	touched = FALSE;

	MUTEX_LOCK(env, renv->mtx_regenv);

	for (i = start; i < limit; i++) {
		if (rep->site_cnt >= rep->site_max) {
			/* Table is full, we need more space. */
			if (rep->netaddr_off == INVALID_ROFF) {
				n = INITIAL_SITES_ALLOCATION;
				sz = n * sizeof(SITEADDR);
				if ((ret = __env_alloc(infop,
				    sz, &shared_array)) != 0)
					goto out;
			} else {
				n = 2 * rep->site_max;
				sz = n * sizeof(SITEADDR);
				if ((ret = __env_alloc(infop,
				    sz, &shared_array)) != 0)
					goto out;
				orig = R_ADDR(infop, rep->netaddr_off);
				memcpy(shared_array, orig,
				    sizeof(SITEADDR) * rep->site_cnt);
				__env_alloc_free(infop, orig);
			}
			rep->netaddr_off = R_OFFSET(infop, shared_array);
			rep->site_max = n;
		} else
			shared_array = R_ADDR(infop, rep->netaddr_off);

		DB_ASSERT(env, rep->site_cnt < rep->site_max &&
		    rep->netaddr_off != INVALID_ROFF);

		host = db_rep->sites[i].net_addr.host;
		sz = strlen(host) + 1;
		if ((ret = __env_alloc(infop, sz, &hostbuf)) != 0)
			goto out;
		eid = (int)rep->site_cnt++;
		(void)strcpy(hostbuf, host);
		shared_array[eid].host = R_OFFSET(infop, hostbuf);
		shared_array[eid].port = db_rep->sites[i].net_addr.port;
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "EID %d is assigned for site %s:%lu",
			eid, host, (u_long)shared_array[eid].port));
		touched = TRUE;
	}

out:
	if (touched)
		rep->siteaddr_seq++;
	MUTEX_UNLOCK(env, renv->mtx_regenv);
	return (ret);
}

/*
 * Copy into our local list any newly added remote site addresses that we
 * haven't seen yet.
 *
 * !!! Caller must hold db_rep->mutex and mtx_repmgr locks.
 *
 * PUBLIC: int __repmgr_copy_in_added_sites __P((ENV *));
 */
int
__repmgr_copy_in_added_sites(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	REGINFO *infop;
	SITEADDR *base, *p;
	REPMGR_SITE *site;
	char *host;
	int ret;
	u_int i;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	if (rep->netaddr_off == INVALID_ROFF)
		return (0);

	infop = env->reginfo;
	base = R_ADDR(infop, rep->netaddr_off);
	for (i = db_rep->site_cnt; i < rep->site_cnt; i++) {
		p = &base[i];
		host = R_ADDR(infop, p->host);
		if ((ret = __repmgr_new_site(env,
		    &site, host, p->port, SITE_IDLE)) != 0)
			return (ret);
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "Site %s:%lu found at EID %u",
			host, (u_long)p->port, i));
	}

	DB_ASSERT(env, db_rep->site_cnt == rep->site_cnt);
	db_rep->peer = rep->peer;
	db_rep->siteaddr_seq = rep->siteaddr_seq;
	return (0);
}

/*
 * Initialize a range of sites newly added to our site list array.  Process each
 * array entry in the range from <= x < limit.  Passing from >= limit is
 * allowed, and is effectively a no-op.
 *
 * PUBLIC: int __repmgr_init_new_sites __P((ENV *, u_int, u_int));
 *
 * !!! Assumes caller holds db_rep->mutex.
 */
int
__repmgr_init_new_sites(env, from, limit)
	ENV *env;
	u_int from, limit;
{
	DB_REP *db_rep;
	u_int i;
	int ret;

	db_rep = env->rep_handle;

	for (i = from; i < limit; i++) {
		if ((ret = __repmgr_check_host_name(env, (int)i)) != 0)
			return (ret);
		if (db_rep->selector != NULL &&
		    (ret = __repmgr_schedule_connection_attempt(env,
		    i, TRUE)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * PUBLIC: int __repmgr_check_host_name __P((ENV *, int));
 *
 * !!! Assumes caller holds db_rep->mutex.
 */
int
__repmgr_check_host_name(env, eid)
	ENV *env;
	int eid;
{
	DB_REP *db_rep;
	ADDRINFO *list;
	repmgr_netaddr_t *addr;
	int ret;

	db_rep = env->rep_handle;
	ret = 0;
	addr = &SITE_FROM_EID(eid)->net_addr;

	if (addr->address_list == NULL && REPMGR_INITED(db_rep)) {
		if ((ret = __repmgr_getaddr(env,
		    addr->host, addr->port, 0, &list)) == 0)
			ADDR_LIST_INIT(addr, list);
		else if (ret == DB_REP_UNAVAIL)
			ret = 0;
	}

	return (ret);
}

/*
 * PUBLIC: int __repmgr_failchk __P((ENV *));
 */
int
__repmgr_failchk(env)
	ENV *env;
{
	DB_ENV *dbenv;
	DB_REP *db_rep;
	REP *rep;
	db_threadid_t unused;

	dbenv = env->dbenv;
	db_rep = env->rep_handle;
	rep = db_rep->region;

	COMPQUIET(unused, 0);
	MUTEX_LOCK(env, rep->mtx_repmgr);

	/*
	 * Check to see if the main (listener) replication process may have died
	 * without cleaning up the flag.  If so, we only have to clear it, and
	 * another process should then be able to come along and become the
	 * listener.  So in either case we can return success.
	 */
	if (rep->listener != 0 && !dbenv->is_alive(dbenv,
	    rep->listener, unused, DB_MUTEX_PROCESS_ONLY))
		rep->listener = 0;
	MUTEX_UNLOCK(env, rep->mtx_repmgr);

	return (0);
}
