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

static int __rep_egen_init  __P((ENV *, REP *));
static int __rep_gen_init  __P((ENV *, REP *));

/*
 * __rep_open --
 *	Initialize the shared memory state for the replication system.
 *
 * PUBLIC: int __rep_open __P((ENV *));
 */
int
__rep_open(env)
	ENV *env;
{
	DB_REP *db_rep;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	int ret;

	db_rep = env->rep_handle;
	infop = env->reginfo;
	renv = infop->primary;
	ret = 0;

	if (renv->rep_off == INVALID_ROFF) {
		/* Must create the region. */
		if ((ret = __env_alloc(infop, sizeof(REP), &rep)) != 0)
			return (ret);
		memset(rep, 0, sizeof(*rep));

		/*
		 * We have the region; fill in the values.  Some values may
		 * have been configured before we open the region, and those
		 * are taken from the DB_REP structure.
		 */
		if ((ret = __mutex_alloc(
		    env, MTX_REP_REGION, 0, &rep->mtx_region)) != 0)
			return (ret);
		/*
		 * Because we have no way to prevent deadlocks and cannot log
		 * changes made to it, we single-thread access to the client
		 * bookkeeping database.  This is suboptimal, but it only gets
		 * accessed when messages arrive out-of-order, so it should
		 * stay small and not be used in a high-performance app.
		 */
		if ((ret = __mutex_alloc(
		    env, MTX_REP_DATABASE, 0, &rep->mtx_clientdb)) != 0)
			return (ret);

		if ((ret = __mutex_alloc(
		    env, MTX_REP_CHKPT, 0, &rep->mtx_ckp)) != 0)
			return (ret);

		if ((ret = __mutex_alloc(
		    env, MTX_REP_EVENT, 0, &rep->mtx_event)) != 0)
			return (ret);

		rep->newmaster_event_gen = 0;
		rep->notified_egen = 0;
		rep->lease_off = INVALID_ROFF;
		rep->tally_off = INVALID_ROFF;
		rep->v2tally_off = INVALID_ROFF;
		rep->eid = db_rep->eid;
		rep->master_id = DB_EID_INVALID;
		rep->gen = 0;
		rep->version = DB_REPVERSION;
		rep->config = db_rep->config;
		if ((ret = __rep_gen_init(env, rep)) != 0)
			return (ret);
		if ((ret = __rep_egen_init(env, rep)) != 0)
			return (ret);
		rep->gbytes = db_rep->gbytes;
		rep->bytes = db_rep->bytes;
		rep->request_gap = db_rep->request_gap;
		rep->max_gap = db_rep->max_gap;
		rep->config_nsites = db_rep->config_nsites;
		rep->elect_timeout = db_rep->elect_timeout;
		rep->full_elect_timeout = db_rep->full_elect_timeout;
		rep->lease_timeout = db_rep->lease_timeout;
		rep->clock_skew = db_rep->clock_skew;
		rep->clock_base = db_rep->clock_base;
		timespecclear(&rep->lease_duration);
		timespecclear(&rep->grant_expire);
		rep->chkpt_delay = db_rep->chkpt_delay;
		rep->priority = db_rep->my_priority;

		F_SET(rep, REP_F_NOARCHIVE);

		/* Copy application type flags if set before env open. */
		if (F_ISSET(db_rep, DBREP_APP_REPMGR))
			F_SET(rep, REP_F_APP_REPMGR);
		if (F_ISSET(db_rep, DBREP_APP_BASEAPI))
			F_SET(rep, REP_F_APP_BASEAPI);

		/* Initialize encapsulating region. */
		renv->rep_off = R_OFFSET(infop, rep);
		(void)time(&renv->rep_timestamp);
		renv->op_timestamp = 0;
		F_CLR(renv, DB_REGENV_REPLOCKED);

#ifdef HAVE_REPLICATION_THREADS
		if ((ret = __repmgr_open(env, rep)) != 0)
			return (ret);
#endif
	} else {
		rep = R_ADDR(infop, renv->rep_off);
		/*
		 * Prevent an application type mismatch between a process
		 * and the environment it is trying to join.
		 */
		if ((F_ISSET(db_rep, DBREP_APP_REPMGR) &&
		    F_ISSET(rep, REP_F_APP_BASEAPI)) ||
		    (F_ISSET(db_rep, DBREP_APP_BASEAPI) &&
		    F_ISSET(rep, REP_F_APP_REPMGR))) {
			__db_errx(env,
"Application type mismatch for a replication process joining the environment");
			return (EINVAL);
		}
#ifdef HAVE_REPLICATION_THREADS
		if ((ret = __repmgr_join(env, rep)) != 0)
			return (ret);
#endif
	}

	db_rep->region = rep;

	return (0);
}

/*
 * __rep_env_refresh --
 *	Replication-specific refresh of the ENV structure.
 *
 * PUBLIC: int __rep_env_refresh __P((ENV *));
 */
int
__rep_env_refresh(env)
	ENV *env;
{
	DB_REP *db_rep;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	int ret, t_ret;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	infop = env->reginfo;
	renv = infop->primary;
	ret = 0;

	/*
	 * If we are the last reference closing the env, clear our knowledge of
	 * belonging to a group and that there is a valid handle where
	 * rep_start had already been called.
	 */
	if (renv->refcnt == 1) {
		F_CLR(rep, REP_F_GROUP_ESTD);
		F_CLR(rep, REP_F_START_CALLED);
	}

#ifdef HAVE_REPLICATION_THREADS
	ret = __repmgr_env_refresh(env);
#endif

	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if (F_ISSET(env, ENV_PRIVATE)) {
		db_rep = env->rep_handle;
		if (db_rep->region != NULL) {
			ret = __mutex_free(env, &db_rep->region->mtx_region);
			if ((t_ret = __mutex_free(env,
			    &db_rep->region->mtx_clientdb)) != 0 && ret == 0)
				ret = t_ret;
			if ((t_ret = __mutex_free(env,
			    &db_rep->region->mtx_ckp)) != 0 && ret == 0)
				ret = t_ret;
			if ((t_ret = __mutex_free(env,
			    &db_rep->region->mtx_event)) != 0 && ret == 0)
				ret = t_ret;
		}

		if (renv->rep_off != INVALID_ROFF)
			__env_alloc_free(infop, R_ADDR(infop, renv->rep_off));
	}

	env->rep_handle->region = NULL;
	return (ret);
}

/*
 * __rep_close --
 *      Shut down all of replication.
 *
 * PUBLIC: int __rep_env_close __P((ENV *));
 */
int
__rep_env_close(env)
	ENV *env;
{
	int ret, t_ret;

	ret = __rep_preclose(env);
	if ((t_ret = __rep_closefiles(env)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_preclose --
 *	If we are a client, shut down our client database and send
 * any outstanding bulk buffers.
 *
 * PUBLIC: int __rep_preclose __P((ENV *));
 */
int
__rep_preclose(env)
	ENV *env;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	LOG *lp;
	REP_BULK bulk;
	int ret;

	ret = 0;

	db_rep = env->rep_handle;
	dblp = env->lg_handle;

	/*
	 * If we have a rep region, we can preclose.  Otherwise, return.
	 * If we're on an error path from env open, we may not have
	 * a region, even though we have a handle.
	 */
	if (db_rep == NULL || db_rep->region == NULL)
		return (ret);
	MUTEX_LOCK(env, db_rep->region->mtx_clientdb);
	if (db_rep->rep_db != NULL) {
		ret = __db_close(db_rep->rep_db, NULL, DB_NOSYNC);
		db_rep->rep_db = NULL;
	}
	/*
	 * We could be called early in an env_open error path, so
	 * only do this if we have a log region set up.
	 */
	if (dblp == NULL)
		goto out;
	lp = dblp->reginfo.primary;
	/*
	 * If we have something in the bulk buffer, send anything in it
	 * if we are able to.
	 */
	if (lp->bulk_off != 0 && db_rep->send != NULL) {
		memset(&bulk, 0, sizeof(bulk));
		bulk.addr = R_ADDR(&dblp->reginfo, lp->bulk_buf);
		bulk.offp = &lp->bulk_off;
		bulk.len = lp->bulk_len;
		bulk.type = REP_BULK_LOG;
		bulk.eid = DB_EID_BROADCAST;
		bulk.flagsp = &lp->bulk_flags;
		/*
		 * Ignore send errors here.  This can be called on the
		 * env->close path - make a best attempt to send.
		 */
		(void)__rep_send_bulk(env, &bulk, 0);
	}
out:	MUTEX_UNLOCK(env, db_rep->region->mtx_clientdb);
	return (ret);
}

/*
 * __rep_closefiles --
 *	If we were a client and are now a master, close all databases
 *	we've opened while applying messages as a client.  This can
 *	be called from __env_close and we need to check if the env,
 *	handles and regions are set up, or not.
 *
 * PUBLIC: int __rep_closefiles __P((ENV *));
 */
int
__rep_closefiles(env)
	ENV *env;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	int ret;

	ret = 0;

	db_rep = env->rep_handle;
	dblp = env->lg_handle;

	if (db_rep == NULL || db_rep->region == NULL)
		return (ret);
	if (dblp == NULL)
		return (ret);
	if ((ret = __dbreg_close_files(env, 0)) == 0)
		F_CLR(db_rep, DBREP_OPENFILES);

	return (ret);
}

/*
 * __rep_egen_init --
 *	Initialize the value of egen in the region.  Called only from
 *	__rep_region_init, which is guaranteed to be single-threaded
 *	as we create the rep region.  We set the rep->egen field which
 *	is normally protected by db_rep->region->mutex.
 */
static int
__rep_egen_init(env, rep)
	ENV *env;
	REP *rep;
{
	DB_FH *fhp;
	int ret;
	size_t cnt;
	char *p;

	if ((ret = __db_appname(env,
	    DB_APP_NONE, REP_EGENNAME, NULL, &p)) != 0)
		return (ret);
	/*
	 * If the file doesn't exist, create it now and initialize with 1.
	 */
	if (__os_exists(env, p, NULL) != 0) {
		rep->egen = rep->gen + 1;
		if ((ret = __rep_write_egen(env, rep, rep->egen)) != 0)
			goto err;
	} else {
		/*
		 * File exists, open it and read in our egen.
		 */
		if ((ret = __os_open(env, p, 0,
		    DB_OSO_RDONLY, DB_MODE_600, &fhp)) != 0)
			goto err;
		if ((ret = __os_read(env, fhp, &rep->egen, sizeof(u_int32_t),
		    &cnt)) != 0 || cnt != sizeof(u_int32_t))
			goto err1;
		RPRINT(env, DB_VERB_REP_MISC,
		    (env, "Read in egen %lu", (u_long)rep->egen));
err1:		 (void)__os_closehandle(env, fhp);
	}
err:	__os_free(env, p);
	return (ret);
}

/*
 * __rep_write_egen --
 *	Write out the egen into the env file.
 *
 * PUBLIC: int __rep_write_egen __P((ENV *, REP *, u_int32_t));
 */
int
__rep_write_egen(env, rep, egen)
	ENV *env;
	REP *rep;
	u_int32_t egen;
{
	DB_FH *fhp;
	int ret;
	size_t cnt;
	char *p;

	/*
	 * If running in-memory replication, return without any file
	 * operations.
	 */
	if (FLD_ISSET(rep->config, REP_C_INMEM)) {
		return (0);
	}

	if ((ret = __db_appname(env,
	    DB_APP_NONE, REP_EGENNAME, NULL, &p)) != 0)
		return (ret);
	if ((ret = __os_open(
	    env, p, 0, DB_OSO_CREATE | DB_OSO_TRUNC, DB_MODE_600, &fhp)) == 0) {
		if ((ret = __os_write(env, fhp, &egen, sizeof(u_int32_t),
		    &cnt)) != 0 || ((ret = __os_fsync(env, fhp)) != 0))
			__db_err(env, ret, "%s", p);
		(void)__os_closehandle(env, fhp);
	}
	__os_free(env, p);
	return (ret);
}

/*
 * __rep_gen_init --
 *	Initialize the value of gen in the region.  Called only from
 *	__rep_region_init, which is guaranteed to be single-threaded
 *	as we create the rep region.  We set the rep->gen field which
 *	is normally protected by db_rep->region->mutex.
 */
static int
__rep_gen_init(env, rep)
	ENV *env;
	REP *rep;
{
	DB_FH *fhp;
	int ret;
	size_t cnt;
	char *p;

	if ((ret = __db_appname(env,
	    DB_APP_NONE, REP_GENNAME, NULL, &p)) != 0)
		return (ret);
	/*
	 * If the file doesn't exist, create it now and initialize with 0.
	 */
	if (__os_exists(env, p, NULL) != 0) {
		rep->gen = 0;
		if ((ret = __rep_write_gen(env, rep, rep->gen)) != 0)
			goto err;
	} else {
		/*
		 * File exists, open it and read in our gen.
		 */
		if ((ret = __os_open(env, p, 0,
		    DB_OSO_RDONLY, DB_MODE_600, &fhp)) != 0)
			goto err;
		if ((ret = __os_read(env, fhp, &rep->gen, sizeof(u_int32_t),
		    &cnt)) < 0 || cnt == 0)
			goto err1;
		RPRINT(env, DB_VERB_REP_MISC, (env, "Read in gen %lu",
		    (u_long)rep->gen));
err1:		 (void)__os_closehandle(env, fhp);
	}
err:	__os_free(env, p);
	return (ret);
}

/*
 * __rep_write_gen --
 *	Write out the gen into the env file.
 *
 * PUBLIC: int __rep_write_gen __P((ENV *, REP *, u_int32_t));
 */
int
__rep_write_gen(env, rep, gen)
	ENV *env;
	REP *rep;
	u_int32_t gen;
{
	DB_FH *fhp;
	int ret;
	size_t cnt;
	char *p;

	/*
	 * If running in-memory replication, return without any file
	 * operations.
	 */
	if (FLD_ISSET(rep->config, REP_C_INMEM)) {
		return (0);
	}

	if ((ret = __db_appname(env,
	    DB_APP_NONE, REP_GENNAME, NULL, &p)) != 0)
		return (ret);
	if ((ret = __os_open(
	    env, p, 0, DB_OSO_CREATE | DB_OSO_TRUNC, DB_MODE_600, &fhp)) == 0) {
		if ((ret = __os_write(env, fhp, &gen, sizeof(u_int32_t),
		    &cnt)) != 0 || ((ret = __os_fsync(env, fhp)) != 0))
			__db_err(env, ret, "%s", p);
		(void)__os_closehandle(env, fhp);
	}
	__os_free(env, p);
	return (ret);
}
