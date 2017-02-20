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

static int kick_blockers __P((ENV *, REPMGR_CONNECTION *, void *));
static int mismatch_err __P((const ENV *));
static int __repmgr_await_threads __P((ENV *));

/*
 * PUBLIC: int __repmgr_start __P((DB_ENV *, int, u_int32_t));
 */
int
__repmgr_start(dbenv, nthreads, flags)
	DB_ENV *dbenv;
	int nthreads;
	u_int32_t flags;
{
	DBT my_addr;
	DB_REP *db_rep;
	REP *rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REPMGR_RUNNABLE *messenger;
	int i, is_listener, locked, need_masterseek, ret;

	env = dbenv->env;
	db_rep = env->rep_handle;

	switch (flags) {
	case DB_REP_CLIENT:
	case DB_REP_ELECTION:
	case DB_REP_MASTER:
		break;
	default:
		__db_errx(env,
		    "repmgr_start: unrecognized flags parameter value");
		return (EINVAL);
	}

	ENV_REQUIRES_CONFIG_XX(
	    env, rep_handle, "DB_ENV->repmgr_start", DB_INIT_REP);
	if (!F_ISSET(env, ENV_THREAD)) {
		__db_errx(env,
		    "Replication Manager needs an environment with DB_THREAD");
		return (EINVAL);
	}

	if (APP_IS_BASEAPI(env)) {
		__db_errx(env,
"DB_ENV->repmgr_start: cannot call from base replication application");
		return (EINVAL);
	}

	/* Check that the required initialization has been done. */
	if (db_rep->my_addr.host == NULL) {
		__db_errx(env,
		    "repmgr_set_local_site must be called before repmgr_start");
		return (EINVAL);
	}

	if (db_rep->selector != NULL || db_rep->finished) {
		__db_errx(env,
		    "DB_ENV->repmgr_start may not be called more than once");
		return (EINVAL);
	}

	/*
	 * See if anyone else is already fulfilling the listener role.  If not,
	 * we'll do so.
	 */
	rep = db_rep->region;
	ENV_ENTER(env, ip);
	MUTEX_LOCK(env, rep->mtx_repmgr);
	if (rep->listener == 0) {
		is_listener = TRUE;
		__os_id(dbenv, &rep->listener, NULL);
	} else {
		is_listener = FALSE;
		nthreads = 0;
	}
	MUTEX_UNLOCK(env, rep->mtx_repmgr);
	ENV_LEAVE(env, ip);

	/*
	 * The minimum legal number of threads is either 1 or 0, depending upon
	 * whether we're the main process or a subordinate.
	 */
	locked = FALSE;
	if (nthreads < (is_listener ? 1 : 0)) {
		__db_errx(env,
		    "repmgr_start: nthreads parameter must be >= 1");
		ret = EINVAL;
		goto err;
	}

	if ((ret = __repmgr_init(env)) != 0)
		goto err;
	if (is_listener && (ret = __repmgr_listen(env)) != 0)
		goto err;

	/*
	 * Make some sort of call to rep_start before starting other threads, to
	 * ensure that incoming messages being processed always have a rep
	 * context properly configured.  Note that in a way this is wasted, in
	 * the sense that any messages that rep_start sends won't really go
	 * anywhere, because we haven't started the select() thread yet, so we
	 * don't yet really have any connections to any remote sites.  But
	 * trying to do it the other way ends up requiring complicated code;
	 * this way we know easily that by the time we receive a message, we've
	 * already called rep_start, so it'll be legal to call
	 * rep_process_message.
	 *     Note that even if we're starting without recovery, we need a
	 * rep_start call in case we're using leases.  Leases keep track of
	 * rep_start calls even within an env region lifetime.
	 */
	if ((ret = __rep_set_transport_int(env, SELF_EID, __repmgr_send)) != 0)
		goto err;
	need_masterseek = FALSE;
	if (!is_listener) {
		/* Another process currently already listening in this env. */
		db_rep->master_eid = rep->master_id;
	} else if ((db_rep->init_policy = flags) == DB_REP_MASTER)
		ret = __repmgr_become_master(env);
	else {
		if ((ret = __repmgr_prepare_my_addr(env, &my_addr)) != 0)
			goto err;
		ret = __rep_start_int(env, &my_addr, DB_REP_CLIENT);
		__os_free(env, my_addr.data);

		if (rep->master_id == DB_EID_INVALID ||
		    rep->master_id == SELF_EID) {
			need_masterseek = TRUE;
		} else {
			/*
			 * Restarted without recovery.  Use existing known
			 * master.
			 */
			db_rep->master_eid = rep->master_id;
		}
	}
	if (ret != 0)
		goto err;
	if ((ret = __repmgr_start_selector(env)) != 0)
		goto err;

	if (is_listener) {
		/*
		 * Since these allocated memory blocks are used by other
		 * threads, we have to be a bit careful about freeing them in
		 * case of any errors.  __repmgr_await_threads (which we call in
		 * the err: coda below) takes care of that.
		 */
		if ((ret = __os_calloc(env, (u_int)nthreads,
		    sizeof(REPMGR_RUNNABLE *), &db_rep->messengers)) != 0)
			goto err;
		db_rep->nthreads = nthreads;

		for (i = 0; i < nthreads; i++) {
			if ((ret = __os_calloc(env, 1, sizeof(REPMGR_RUNNABLE),
			    &messenger)) != 0)
				goto err;

			messenger->env = env;
			messenger->run = __repmgr_msg_thread;
			if ((ret = __repmgr_thread_start(env,
			    messenger)) != 0) {
				__os_free(env, messenger);
				goto err;
			}
			db_rep->messengers[i] = messenger;
		}
	}

	if (need_masterseek) {
		LOCK_MUTEX(db_rep->mutex);
		locked = TRUE;
		if ((ret = __repmgr_init_election(env, ELECT_REPSTART)) != 0)
			goto err;
		UNLOCK_MUTEX(db_rep->mutex);
		locked = FALSE;
	}

	return (is_listener ? 0 : DB_REP_IGNORE);

err:
	/* If we couldn't succeed at everything, undo the parts we did do. */
	if (locked)
		UNLOCK_MUTEX(db_rep->mutex);
	if (db_rep->selector != NULL) {
		(void)__repmgr_stop_threads(env);
		(void)__repmgr_await_threads(env);
	}
	LOCK_MUTEX(db_rep->mutex);
	(void)__repmgr_net_close(env);
	if (REPMGR_INITED(db_rep))
		(void)__repmgr_deinit(env);
	UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}

/*
 * PUBLIC: int __repmgr_autostart __P((ENV *));
 *
 * Preconditions: rep_start() has been called; we're within an ENV_ENTER.
 *     Because of this, we mustn't call __rep_set_transport(), but rather we
 *     poke in send() function address manually.
 */
int
__repmgr_autostart(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret;

	db_rep = env->rep_handle;

	DB_ASSERT(env, REP_ON(env));
	LOCK_MUTEX(db_rep->mutex);

	if (REPMGR_INITED(db_rep))
		ret = 0;
	else
		ret = __repmgr_init(env);
	if (ret != 0)
		goto out;

	RPRINT(env, DB_VERB_REPMGR_MISC,
	    (env, "Automatically joining existing repmgr env"));

	db_rep->send = __repmgr_send;

	if (db_rep->selector == NULL && !db_rep->finished)
		ret = __repmgr_start_selector(env);

out:
	UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}

/*
 * PUBLIC: int __repmgr_start_selector __P((ENV *));
 */
int
__repmgr_start_selector(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_RUNNABLE *selector;
	int ret;

	db_rep = env->rep_handle;
	if ((ret = __os_calloc(env, 1, sizeof(REPMGR_RUNNABLE), &selector))
	    != 0)
		return (ret);
	selector->env = env;
	selector->run = __repmgr_select_thread;

	/*
	 * In case the select thread ever examines db_rep->selector, set it
	 * before starting the thread (since once we create it we could be
	 * racing with it).
	 */
	db_rep->selector = selector;
	if ((ret = __repmgr_thread_start(env, selector)) != 0) {
		__db_err(env, ret, "can't start selector thread");
		__os_free(env, selector);
		db_rep->selector = NULL;
		return (ret);
	}

	return (0);
}

/*
 * PUBLIC: int __repmgr_close __P((ENV *));
 */
int
__repmgr_close(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret, t_ret;

	ret = 0;
	db_rep = env->rep_handle;
	if (db_rep->selector != NULL) {
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "Stopping repmgr threads"));
		ret = __repmgr_stop_threads(env);
		if ((t_ret = __repmgr_await_threads(env)) != 0 && ret == 0)
			ret = t_ret;
		RPRINT(env, DB_VERB_REPMGR_MISC,
		    (env, "Repmgr threads are finished"));
	}

	if ((t_ret = __repmgr_net_close(env)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __repmgr_deinit(env)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * PUBLIC: int __repmgr_set_ack_policy __P((DB_ENV *, int));
 */
int
__repmgr_set_ack_policy(dbenv, policy)
	DB_ENV *dbenv;
	int policy;
{
	DB_REP *db_rep;
	ENV *env;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->repmgr_set_ack_policy", DB_INIT_REP);

	if (APP_IS_BASEAPI(env)) {
		__db_errx(env, "%s %s", "DB_ENV->repmgr_set_ack_policy:",
		    "cannot call from base replication application");
		return (EINVAL);
	}

	switch (policy) {
	case DB_REPMGR_ACKS_ALL: /* FALLTHROUGH */
	case DB_REPMGR_ACKS_ALL_PEERS: /* FALLTHROUGH */
	case DB_REPMGR_ACKS_NONE: /* FALLTHROUGH */
	case DB_REPMGR_ACKS_ONE: /* FALLTHROUGH */
	case DB_REPMGR_ACKS_ONE_PEER: /* FALLTHROUGH */
	case DB_REPMGR_ACKS_QUORUM:
		env->rep_handle->perm_policy = policy;
		/*
		 * Setting an ack policy makes this a replication manager
		 * application.
		 */
		APP_SET_REPMGR(env);
		return (0);
	default:
		__db_errx(env,
		    "unknown ack_policy in DB_ENV->repmgr_set_ack_policy");
		return (EINVAL);
	}
}

/*
 * PUBLIC: int __repmgr_get_ack_policy __P((DB_ENV *, int *));
 */
int
__repmgr_get_ack_policy(dbenv, policy)
	DB_ENV *dbenv;
	int *policy;
{
	DB_REP *db_rep;
	ENV *env;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->repmgr_get_ack_policy", DB_INIT_REP);

	*policy = env->rep_handle->perm_policy;
	return (0);
}

/*
 * PUBLIC: int __repmgr_env_create __P((ENV *, DB_REP *));
 */
int
__repmgr_env_create(env, db_rep)
	ENV *env;
	DB_REP *db_rep;
{
	COMPQUIET(env, NULL);

	/* Set some default values. */
	db_rep->ack_timeout = DB_REPMGR_DEFAULT_ACK_TIMEOUT;
	db_rep->connection_retry_wait = DB_REPMGR_DEFAULT_CONNECTION_RETRY;
	db_rep->election_retry_wait = DB_REPMGR_DEFAULT_ELECTION_RETRY;
	db_rep->config_nsites = 0;
	db_rep->peer = DB_EID_INVALID;
	db_rep->perm_policy = DB_REPMGR_ACKS_QUORUM;

	db_rep->listen_fd = INVALID_SOCKET;
	db_rep->master_eid = DB_EID_INVALID;
	TAILQ_INIT(&db_rep->connections);
	TAILQ_INIT(&db_rep->retries);

	db_rep->input_queue.size = 0;
	STAILQ_INIT(&db_rep->input_queue.header);

	__repmgr_env_create_pf(db_rep);

	return (0);
}

/*
 * PUBLIC: void __repmgr_env_destroy __P((ENV *, DB_REP *));
 */
void
__repmgr_env_destroy(env, db_rep)
	ENV *env;
	DB_REP *db_rep;
{
	__repmgr_queue_destroy(env);
	__repmgr_net_destroy(env, db_rep);
	if (db_rep->messengers != NULL) {
		__os_free(env, db_rep->messengers);
		db_rep->messengers = NULL;
	}
}

/*
 * PUBLIC: int __repmgr_stop_threads __P((ENV *));
 */
int
__repmgr_stop_threads(env)
	ENV *env;
{
	DB_REP *db_rep;
	int ret;

	db_rep = env->rep_handle;

	/*
	 * Hold mutex for the purpose of waking up threads, but then get out of
	 * the way to let them clean up and exit.
	 */
	LOCK_MUTEX(db_rep->mutex);
	db_rep->finished = TRUE;
	if (db_rep->elect_thread != NULL &&
	    (ret = __repmgr_signal(&db_rep->check_election)) != 0)
		goto unlock;

	if ((ret = __repmgr_signal(&db_rep->queue_nonempty)) != 0)
		goto unlock;

	if ((ret = __repmgr_each_connection(env,
	    kick_blockers, NULL, TRUE)) != 0)
		goto unlock;
	UNLOCK_MUTEX(db_rep->mutex);

	return (__repmgr_wake_main_thread(env));

unlock:
	UNLOCK_MUTEX(db_rep->mutex);
	return (ret);
}

static int
kick_blockers(env, conn, unused)
	ENV *env;
	REPMGR_CONNECTION *conn;
	void *unused;
{
	COMPQUIET(env, NULL);
	COMPQUIET(unused, NULL);

	return (conn->blockers > 0 ? __repmgr_signal(&conn->drained) : 0);
}

static int
__repmgr_await_threads(env)
	ENV *env;
{
	DB_REP *db_rep;
	REPMGR_RUNNABLE *messenger;
	int ret, t_ret, i;

	db_rep = env->rep_handle;
	ret = 0;
	if (db_rep->elect_thread != NULL) {
		ret = __repmgr_thread_join(db_rep->elect_thread);
		__os_free(env, db_rep->elect_thread);
		db_rep->elect_thread = NULL;
	}

	for (i = 0;
	    i < db_rep->nthreads && db_rep->messengers[i] != NULL; i++) {
		messenger = db_rep->messengers[i];
		if ((t_ret = __repmgr_thread_join(messenger)) != 0 && ret == 0)
			ret = t_ret;
		__os_free(env, messenger);
	}
	__os_free(env, db_rep->messengers);
	db_rep->messengers = NULL;

	if (db_rep->selector != NULL) {
		if ((t_ret = __repmgr_thread_join(db_rep->selector)) != 0 &&
		    ret == 0)
			ret = t_ret;
		__os_free(env, db_rep->selector);
		db_rep->selector = NULL;
	}

	return (ret);
}

/*
 * PUBLIC: int __repmgr_set_local_site __P((DB_ENV *, const char *, u_int,
 * PUBLIC:     u_int32_t));
 */
int
__repmgr_set_local_site(dbenv, host, port, flags)
	DB_ENV *dbenv;
	const char *host;
	u_int port;
	u_int32_t flags;
{
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	ENV *env;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	repmgr_netaddr_t addr;
	char *myhost;
	int locked, ret;

	env = dbenv->env;
	db_rep = env->rep_handle;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->repmgr_set_local_site", DB_INIT_REP);

	if (APP_IS_BASEAPI(env)) {
		__db_errx(env, "%s %s", "DB_ENV->repmgr_set_local_site:",
		    "cannot call from base replication application");
		return (EINVAL);
	}

	if (db_rep->selector != NULL) {
		__db_errx(env,
"DB_ENV->repmgr_set_local_site: must be called before DB_ENV->repmgr_start");
		return (EINVAL);
	}

	if (flags != 0)
		return (__db_ferr(env, "DB_ENV->repmgr_set_local_site", 0));

	if (host == NULL || port == 0) {
		__db_errx(env,
		    "repmgr_set_local_site: host name and port (>0) required");
		return (EINVAL);
	}

	/*
	 * If the local site address hasn't already been set, just set it from
	 * the given inputs.  If it has, all we do is verify that it matches
	 * what had already been set previously.
	 *
	 * Do this in the shared region if we have one, or else just in the
	 * local handle.
	 *
	 * In either case, don't perturb global structures until we're sure
	 * everything will succeed.
	 */
	COMPQUIET(rep, NULL);
	COMPQUIET(ip, NULL);
	COMPQUIET(renv, NULL);
	locked = FALSE;
	ret = 0;
	if (REP_ON(env)) {
		rep = db_rep->region;
		ENV_ENTER(env, ip);
		MUTEX_LOCK(env, rep->mtx_repmgr);

		infop = env->reginfo;
		renv = infop->primary;
		MUTEX_LOCK(env, renv->mtx_regenv);
		locked = TRUE;
		if (rep->my_addr.host == INVALID_ROFF) {
			if ((ret = __repmgr_pack_netaddr(env,
			    host, port, NULL, &addr)) != 0)
				goto unlock;

			if ((ret = __env_alloc(infop,
			    strlen(host)+1, &myhost)) == 0) {
				(void)strcpy(myhost, host);
				rep->my_addr.host = R_OFFSET(infop, myhost);
				rep->my_addr.port = port;
			} else {
				__repmgr_cleanup_netaddr(env, &addr);
				goto unlock;
			}
			memcpy(&db_rep->my_addr, &addr, sizeof(addr));
			rep->siteaddr_seq++;
		} else {
			myhost = R_ADDR(infop, rep->my_addr.host);
			if (strcmp(myhost, host) != 0 ||
			    port != rep->my_addr.port) {
				ret = mismatch_err(env);
				goto unlock;
			}
		}
	} else {
		if (db_rep->my_addr.host == NULL) {
			if ((ret = __repmgr_pack_netaddr(env,
			    host, port, NULL, &db_rep->my_addr)) != 0)
				goto unlock;
		} else if (strcmp(host, db_rep->my_addr.host) != 0 ||
		    port != db_rep->my_addr.port) {
			ret = mismatch_err(env);
			goto unlock;
		}
	}

unlock:
	if (locked) {
		MUTEX_UNLOCK(env, renv->mtx_regenv);
		MUTEX_UNLOCK(env, rep->mtx_repmgr);
		ENV_LEAVE(env, ip);
	}
	/*
	 * Setting a local site makes this a replication manager application.
	 */
	if (ret == 0)
		APP_SET_REPMGR(env);
	return (ret);
}

static int
mismatch_err(env)
	const ENV *env;
{
	__db_errx(env, "A (different) local site address has already been set");
	return (EINVAL);
}

/*
 * If the application only calls this method from a single thread (e.g., during
 * its initialization), it will avoid the problems with the non-thread-safe host
 * name lookup.  In any case, if we relegate the blocking lookup to here it
 * won't affect our select() loop.
 *
 * PUBLIC: int __repmgr_add_remote_site __P((DB_ENV *, const char *, u_int,
 * PUBLIC: int *, u_int32_t));
 */
int
__repmgr_add_remote_site(dbenv, host, port, eidp, flags)
	DB_ENV *dbenv;
	const char *host;
	u_int port;
	int *eidp;
	u_int32_t flags;
{
	DB_REP *db_rep;
	ENV *env;
	REPMGR_SITE *site;
	int eid, locked, ret;

	env = dbenv->env;
	db_rep = env->rep_handle;
	locked = FALSE;
	ret = 0;

	ENV_NOT_CONFIGURED(
	    env, db_rep->region, "DB_ENV->repmgr_add_remote_site", DB_INIT_REP);

	if (APP_IS_BASEAPI(env)) {
		__db_errx(env, "%s %s", "DB_ENV->repmgr_add_remote_site:",
		    "cannot call from base replication application");
		return (EINVAL);
	}

	if ((ret = __db_fchk(env,
	    "DB_ENV->repmgr_add_remote_site", flags, DB_REPMGR_PEER)) != 0)
		return (ret);

	if (host == NULL) {
		__db_errx(env,
		    "repmgr_add_remote_site: host name is required");
		return (EINVAL);
	}

	if (REP_ON(env)) {
		LOCK_MUTEX(db_rep->mutex);
		locked = TRUE;

		ret = __repmgr_add_site(env, host, port, &site, flags);
		if (ret == EEXIST) {
			/*
			 * With NEWSITE messages arriving at any time, it would
			 * be impractical for applications to avoid this.  Also
			 * this provides a way they can still set peer.
			 */
			ret = 0;
		}
		if (ret != 0)
			goto out;
		eid = EID_FROM_SITE(site);
		if (eidp != NULL)
			*eidp = eid;
	} else {
		if ((site = __repmgr_find_site(env, host, port)) == NULL &&
		    (ret = __repmgr_new_site(env,
		    &site, host, port, SITE_IDLE)) != 0)
			goto out;
		eid = EID_FROM_SITE(site);

		/*
		 * Set provisional EID of peer; may be adjusted at env open/join
		 * time.
		 */
		if (LF_ISSET(DB_REPMGR_PEER))
			db_rep->peer = eid;
	}

out:
	if (locked)
		UNLOCK_MUTEX(db_rep->mutex);
	/*
	 * Adding a remote site makes this a replication manager application.
	 */
	if (ret == 0)
		APP_SET_REPMGR(env);
	return (ret);
}
