/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"

/*
 * __log_env_create --
 *	Log specific initialization of the DB_ENV structure.
 *
 * PUBLIC: int __log_env_create __P((DB_ENV *));
 */
int
__log_env_create(dbenv)
	DB_ENV *dbenv;
{
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 */
	dbenv->lg_bsize = 0;
	dbenv->lg_regionmax = LG_BASE_REGION_SIZE;

	return (0);
}

/*
 * __log_env_destroy --
 *	Log specific destruction of the DB_ENV structure.
 *
 * PUBLIC: void __log_env_destroy __P((DB_ENV *));
 */
void
__log_env_destroy(dbenv)
	DB_ENV *dbenv;
{
	COMPQUIET(dbenv, NULL);
}

/*
 * PUBLIC: int __log_get_lg_bsize __P((DB_ENV *, u_int32_t *));
 */
int
__log_get_lg_bsize(dbenv, lg_bsizep)
	DB_ENV *dbenv;
	u_int32_t *lg_bsizep;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lg_handle, "DB_ENV->get_lg_bsize", DB_INIT_LOG);

	if (LOGGING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*lg_bsizep =
		    ((LOG *)env->lg_handle->reginfo.primary)->buffer_size;
	} else
		*lg_bsizep = dbenv->lg_bsize;
	return (0);
}

/*
 * __log_set_lg_bsize --
 *	DB_ENV->set_lg_bsize.
 *
 * PUBLIC: int __log_set_lg_bsize __P((DB_ENV *, u_int32_t));
 */
int
__log_set_lg_bsize(dbenv, lg_bsize)
	DB_ENV *dbenv;
	u_int32_t lg_bsize;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lg_bsize");

	dbenv->lg_bsize = lg_bsize;
	return (0);
}

/*
 * PUBLIC: int __log_get_lg_filemode __P((DB_ENV *, int *));
 */
int
__log_get_lg_filemode(dbenv, lg_modep)
	DB_ENV *dbenv;
	int *lg_modep;
{
	DB_LOG *dblp;
	DB_THREAD_INFO *ip;
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lg_handle, "DB_ENV->get_lg_filemode", DB_INIT_LOG);

	if (LOGGING_ON(env)) {
		dblp = env->lg_handle;
		ENV_ENTER(env, ip);
		LOG_SYSTEM_LOCK(env);
		*lg_modep = ((LOG *)dblp->reginfo.primary)->filemode;
		LOG_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		*lg_modep = dbenv->lg_filemode;

	return (0);
}

/*
 * __log_set_lg_filemode --
 *	DB_ENV->set_lg_filemode.
 *
 * PUBLIC: int __log_set_lg_filemode __P((DB_ENV *, int));
 */
int
__log_set_lg_filemode(dbenv, lg_mode)
	DB_ENV *dbenv;
	int lg_mode;
{
	DB_LOG *dblp;
	DB_THREAD_INFO *ip;
	ENV *env;
	LOG *lp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lg_handle, "DB_ENV->set_lg_filemode", DB_INIT_LOG);

	if (LOGGING_ON(env)) {
		dblp = env->lg_handle;
		lp = dblp->reginfo.primary;
		ENV_ENTER(env, ip);
		LOG_SYSTEM_LOCK(env);
		lp->filemode = lg_mode;
		LOG_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		dbenv->lg_filemode = lg_mode;

	return (0);
}

/*
 * PUBLIC: int __log_get_lg_max __P((DB_ENV *, u_int32_t *));
 */
int
__log_get_lg_max(dbenv, lg_maxp)
	DB_ENV *dbenv;
	u_int32_t *lg_maxp;
{
	DB_LOG *dblp;
	DB_THREAD_INFO *ip;
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lg_handle, "DB_ENV->get_lg_max", DB_INIT_LOG);

	if (LOGGING_ON(env)) {
		dblp = env->lg_handle;
		ENV_ENTER(env, ip);
		LOG_SYSTEM_LOCK(env);
		*lg_maxp = ((LOG *)dblp->reginfo.primary)->log_nsize;
		LOG_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		*lg_maxp = dbenv->lg_size;

	return (0);
}

/*
 * __log_set_lg_max --
 *	DB_ENV->set_lg_max.
 *
 * PUBLIC: int __log_set_lg_max __P((DB_ENV *, u_int32_t));
 */
int
__log_set_lg_max(dbenv, lg_max)
	DB_ENV *dbenv;
	u_int32_t lg_max;
{
	DB_LOG *dblp;
	DB_THREAD_INFO *ip;
	ENV *env;
	LOG *lp;
	int ret;

	env = dbenv->env;
	ret = 0;

	ENV_NOT_CONFIGURED(env,
	    env->lg_handle, "DB_ENV->set_lg_max", DB_INIT_LOG);

	if (LOGGING_ON(env)) {
		dblp = env->lg_handle;
		lp = dblp->reginfo.primary;
		ENV_ENTER(env, ip);
		if ((ret = __log_check_sizes(env, lg_max, 0)) == 0) {
			LOG_SYSTEM_LOCK(env);
			lp->log_nsize = lg_max;
			LOG_SYSTEM_UNLOCK(env);
		}
		ENV_LEAVE(env, ip);
	} else
		dbenv->lg_size = lg_max;

	return (ret);
}

/*
 * PUBLIC: int __log_get_lg_regionmax __P((DB_ENV *, u_int32_t *));
 */
int
__log_get_lg_regionmax(dbenv, lg_regionmaxp)
	DB_ENV *dbenv;
	u_int32_t *lg_regionmaxp;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->lg_handle, "DB_ENV->get_lg_regionmax", DB_INIT_LOG);

	if (LOGGING_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		*lg_regionmaxp =
		    ((LOG *)env->lg_handle->reginfo.primary)->regionmax;
	} else
		*lg_regionmaxp = dbenv->lg_regionmax;
	return (0);
}

/*
 * __log_set_lg_regionmax --
 *	DB_ENV->set_lg_regionmax.
 *
 * PUBLIC: int __log_set_lg_regionmax __P((DB_ENV *, u_int32_t));
 */
int
__log_set_lg_regionmax(dbenv, lg_regionmax)
	DB_ENV *dbenv;
	u_int32_t lg_regionmax;
{
	ENV *env;

	env = dbenv->env;

	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_lg_regionmax");

					/* Let's not be silly. */
	if (lg_regionmax != 0 && lg_regionmax < LG_BASE_REGION_SIZE) {
		__db_errx(env,
		    "log region size must be >= %d", LG_BASE_REGION_SIZE);
		return (EINVAL);
	}

	dbenv->lg_regionmax = lg_regionmax;
	return (0);
}

/*
 * PUBLIC: int __log_get_lg_dir __P((DB_ENV *, const char **));
 */
int
__log_get_lg_dir(dbenv, dirp)
	DB_ENV *dbenv;
	const char **dirp;
{
	*dirp = dbenv->db_log_dir;
	return (0);
}

/*
 * __log_set_lg_dir --
 *	DB_ENV->set_lg_dir.
 *
 * PUBLIC: int __log_set_lg_dir __P((DB_ENV *, const char *));
 */
int
__log_set_lg_dir(dbenv, dir)
	DB_ENV *dbenv;
	const char *dir;
{
	ENV *env;

	env = dbenv->env;

	if (dbenv->db_log_dir != NULL)
		__os_free(env, dbenv->db_log_dir);
	return (__os_strdup(env, dir, &dbenv->db_log_dir));
}

/*
 * __log_get_flags --
 *	DB_ENV->get_flags.
 *
 * PUBLIC: void __log_get_flags __P((DB_ENV *, u_int32_t *));
 */
void
__log_get_flags(dbenv, flagsp)
	DB_ENV *dbenv;
	u_int32_t *flagsp;
{
	DB_LOG *dblp;
	ENV *env;
	LOG *lp;
	u_int32_t flags;

	env = dbenv->env;

	if ((dblp = env->lg_handle) == NULL)
		return;

	lp = dblp->reginfo.primary;

	flags = *flagsp;
	if (lp->db_log_autoremove)
		LF_SET(DB_LOG_AUTO_REMOVE);
	else
		LF_CLR(DB_LOG_AUTO_REMOVE);
	if (lp->db_log_inmemory)
		LF_SET(DB_LOG_IN_MEMORY);
	else
		LF_CLR(DB_LOG_IN_MEMORY);
	*flagsp = flags;
}

/*
 * __log_set_flags --
 *	DB_ENV->set_flags.
 *
 * PUBLIC: void __log_set_flags __P((ENV *, u_int32_t, int));
 */
void
__log_set_flags(env, flags, on)
	ENV *env;
	u_int32_t flags;
	int on;
{
	DB_LOG *dblp;
	LOG *lp;

	if ((dblp = env->lg_handle) == NULL)
		return;

	lp = dblp->reginfo.primary;

	if (LF_ISSET(DB_LOG_AUTO_REMOVE))
		lp->db_log_autoremove = on ? 1 : 0;
	if (LF_ISSET(DB_LOG_IN_MEMORY))
		lp->db_log_inmemory = on ? 1 : 0;
}

/*
 * List of flags we can handle here.  DB_LOG_INMEMORY must be
 * processed before creating the region, leave it out for now.
 */
#undef	OK_FLAGS
#define	OK_FLAGS							\
    (DB_LOG_AUTO_REMOVE | DB_LOG_DIRECT |				\
    DB_LOG_DSYNC | DB_LOG_IN_MEMORY | DB_LOG_ZERO)
#ifndef BREW
static
#endif
const FLAG_MAP LogMap[] = {
	{ DB_LOG_AUTO_REMOVE,	DBLOG_AUTOREMOVE},
	{ DB_LOG_DIRECT,	DBLOG_DIRECT},
	{ DB_LOG_DSYNC,		DBLOG_DSYNC},
	{ DB_LOG_IN_MEMORY,	DBLOG_INMEMORY},
	{ DB_LOG_ZERO,		DBLOG_ZERO}
};
/*
 * __log_get_config --
 *	Configure the logging subsystem.
 *
 * PUBLIC: int __log_get_config __P((DB_ENV *, u_int32_t, int *));
 */
int
__log_get_config(dbenv, which, onp)
	DB_ENV *dbenv;
	u_int32_t which;
	int *onp;
{
	ENV *env;
	DB_LOG *dblp;
	u_int32_t flags;

	env = dbenv->env;
	if (FLD_ISSET(which, ~OK_FLAGS))
		return (__db_ferr(env, "DB_ENV->log_get_config", 0));
	dblp = env->lg_handle;
	ENV_REQUIRES_CONFIG(env, dblp, "DB_ENV->log_get_config", DB_INIT_LOG);

	__env_fetch_flags(LogMap, sizeof(LogMap), &dblp->flags, &flags);
	__log_get_flags(dbenv, &flags);
	if (LF_ISSET(which))
		*onp = 1;
	else
		*onp = 0;

	return (0);
}

/*
 * __log_set_config --
 *	Configure the logging subsystem.
 *
 * PUBLIC: int __log_set_config __P((DB_ENV *, u_int32_t, int));
 */
int
__log_set_config(dbenv, flags, on)
	DB_ENV *dbenv;
	u_int32_t flags;
	int on;
{
	return (__log_set_config_int(dbenv, flags, on, 0));
}
/*
 * __log_set_config_int --
 *	Configure the logging subsystem.
 *
 * PUBLIC: int __log_set_config_int __P((DB_ENV *, u_int32_t, int, int));
 */
int
__log_set_config_int(dbenv, flags, on, in_open)
	DB_ENV *dbenv;
	u_int32_t flags;
	int on;
	int in_open;
{
	ENV *env;
	DB_LOG *dblp;
	u_int32_t mapped_flags;

	env = dbenv->env;
	dblp = env->lg_handle;
	if (FLD_ISSET(flags, ~OK_FLAGS))
		return (__db_ferr(env, "DB_ENV->log_set_config", 0));
	ENV_NOT_CONFIGURED(env, dblp, "DB_ENV->log_set_config", DB_INIT_LOG);
	if (LF_ISSET(DB_LOG_DIRECT) && __os_support_direct_io() == 0) {
		__db_errx(env,
"DB_ENV->log_set_config: direct I/O either not configured or not supported");
		return (EINVAL);
	}

	if (LOGGING_ON(env)) {
		if (!in_open && LF_ISSET(DB_LOG_IN_MEMORY))
			ENV_ILLEGAL_AFTER_OPEN(env,
			     "DB_ENV->log_set_config: DB_LOG_IN_MEMORY");
		__log_set_flags(env, flags, on);
		mapped_flags = 0;
		__env_map_flags(LogMap, sizeof(LogMap), &flags, &mapped_flags);
		if (on)
			F_SET(dblp, mapped_flags);
		else
			F_CLR(dblp, mapped_flags);
	} else {
		/*
		 * DB_LOG_IN_MEMORY, DB_TXN_NOSYNC and DB_TXN_WRITE_NOSYNC
		 * are mutually incompatible.  If we're setting one of them,
		 * clear all current settings.
		 */
		if (on && LF_ISSET(DB_LOG_IN_MEMORY))
			F_CLR(dbenv,
			     DB_ENV_TXN_NOSYNC | DB_ENV_TXN_WRITE_NOSYNC);

		if (on)
			FLD_SET(dbenv->lg_flags, flags);
		else
			FLD_CLR(dbenv->lg_flags, flags);
	}

	return (0);
}

/*
 * __log_check_sizes --
 *	Makes sure that the log file size and log buffer size are compatible.
 *
 * PUBLIC: int __log_check_sizes __P((ENV *, u_int32_t, u_int32_t));
 */
int
__log_check_sizes(env, lg_max, lg_bsize)
	ENV *env;
	u_int32_t lg_max;
	u_int32_t lg_bsize;
{
	DB_ENV *dbenv;
	LOG *lp;
	int inmem;

	dbenv = env->dbenv;

	if (LOGGING_ON(env)) {
		lp = env->lg_handle->reginfo.primary;
		inmem = lp->db_log_inmemory;
		lg_bsize = lp->buffer_size;
	} else
		inmem = (FLD_ISSET(dbenv->lg_flags, DB_LOG_IN_MEMORY) != 0);

	if (inmem) {
		if (lg_bsize == 0)
			lg_bsize = LG_BSIZE_INMEM;
		if (lg_max == 0)
			lg_max = LG_MAX_INMEM;

		if (lg_bsize <= lg_max) {
			__db_errx(env,
		  "in-memory log buffer must be larger than the log file size");
			return (EINVAL);
		}
	}

	return (0);
}
