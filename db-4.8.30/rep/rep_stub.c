/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef HAVE_REPLICATION
#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"

/*
 * If the library wasn't compiled with replication support, various routines
 * aren't available.  Stub them here, returning an appropriate error.
 */
static int __db_norep __P((ENV *));

/*
 * __db_norep --
 *	Error when a Berkeley DB build doesn't include replication support.
 */
static int
__db_norep(env)
	ENV *env;
{
	__db_errx(env,
	    "library build did not include support for replication");
	return (DB_OPNOTSUP);
}

int
__db_rep_enter(dbp, checkgen, checklock, return_now)
	DB *dbp;
	int checkgen, checklock, return_now;
{
	COMPQUIET(checkgen, 0);
	COMPQUIET(checklock, 0);
	COMPQUIET(return_now, 0);
	return (__db_norep(dbp->env));
}

int
__env_rep_enter(env, checklock)
	ENV *env;
	int checklock;
{
	COMPQUIET(checklock, 0);
	return (__db_norep(env));
}

int
__env_db_rep_exit(env)
	ENV *env;
{
	return (__db_norep(env));
}

int
__op_rep_enter(env)
	ENV *env;
{
	return (__db_norep(env));
}

int
__op_rep_exit(env)
	ENV *env;
{
	return (__db_norep(env));
}

int
__rep_bulk_message(env, bulkp, repth, lsnp, dbt, flags)
	ENV *env;
	REP_BULK *bulkp;
	REP_THROTTLE *repth;
	DB_LSN *lsnp;
	const DBT *dbt;
	u_int32_t flags;
{
	COMPQUIET(bulkp, NULL);
	COMPQUIET(repth, NULL);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(dbt, NULL);
	COMPQUIET(flags, 0);
	return (__db_norep(env));
}

int
__rep_env_refresh(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	return (0);
}

int
__rep_elect_pp(dbenv, nsites, nvotes, flags)
	DB_ENV *dbenv;
	u_int32_t nsites, nvotes;
	u_int32_t flags;
{
	COMPQUIET(nsites, 0);
	COMPQUIET(nvotes, 0);
	COMPQUIET(flags, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_flush(dbenv)
	DB_ENV *dbenv;
{
	return (__db_norep(dbenv->env));
}

int
__rep_lease_check(env, refresh)
	ENV *env;
	int refresh;
{
	COMPQUIET(refresh, 0);
	return (__db_norep(env));
}

int
__rep_lease_expire(env)
	ENV *env;
{
	return (__db_norep(env));
}

int
__rep_get_clockskew(dbenv, fast_clockp, slow_clockp)
	DB_ENV *dbenv;
	u_int32_t *fast_clockp, *slow_clockp;
{
	COMPQUIET(fast_clockp, NULL);
	COMPQUIET(slow_clockp, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_set_clockskew(dbenv, fast_clock, slow_clock)
	DB_ENV *dbenv;
	u_int32_t fast_clock, slow_clock;
{
	COMPQUIET(fast_clock, 0);
	COMPQUIET(slow_clock, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_set_nsites(dbenv, n)
	DB_ENV *dbenv;
	u_int32_t n;
{
	COMPQUIET(n, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_get_nsites(dbenv, n)
	DB_ENV *dbenv;
	u_int32_t *n;
{
	COMPQUIET(n, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_set_priority(dbenv, priority)
	DB_ENV *dbenv;
	u_int32_t priority;
{
	COMPQUIET(priority, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_get_priority(dbenv, priority)
	DB_ENV *dbenv;
	u_int32_t *priority;
{
	COMPQUIET(priority, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_set_timeout(dbenv, which, timeout)
	DB_ENV *dbenv;
	int which;
	db_timeout_t timeout;
{
	COMPQUIET(which, 0);
	COMPQUIET(timeout, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_get_timeout(dbenv, which, timeout)
	DB_ENV *dbenv;
	int which;
	db_timeout_t *timeout;
{
	COMPQUIET(which, 0);
	COMPQUIET(timeout, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_get_config(dbenv, which, onp)
	DB_ENV *dbenv;
	u_int32_t which;
	int *onp;
{
	COMPQUIET(which, 0);
	COMPQUIET(onp, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_set_config(dbenv, which, on)
	DB_ENV *dbenv;
	u_int32_t which;
	int on;
{
	COMPQUIET(which, 0);
	COMPQUIET(on, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_get_limit(dbenv, gbytesp, bytesp)
	DB_ENV *dbenv;
	u_int32_t *gbytesp, *bytesp;
{
	COMPQUIET(gbytesp, NULL);
	COMPQUIET(bytesp, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_noarchive(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	return (0);
}

int
__rep_open(env)
	ENV *env;
{
	COMPQUIET(env, NULL);
	return (0);
}

int
__rep_preclose(env)
	ENV *env;
{
	return (__db_norep(env));
}

int
__rep_process_message_pp(dbenv, control, rec, eid, ret_lsnp)
	DB_ENV *dbenv;
	DBT *control, *rec;
	int eid;
	DB_LSN *ret_lsnp;
{
	COMPQUIET(control, NULL);
	COMPQUIET(rec, NULL);
	COMPQUIET(eid, 0);
	COMPQUIET(ret_lsnp, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_send_message(env, eid, rtype, lsnp, dbtp, logflags, repflags)
	ENV *env;
	int eid;
	u_int32_t rtype;
	DB_LSN *lsnp;
	const DBT *dbtp;
	u_int32_t logflags, repflags;
{
	COMPQUIET(eid, 0);
	COMPQUIET(rtype, 0);
	COMPQUIET(lsnp, NULL);
	COMPQUIET(dbtp, NULL);
	COMPQUIET(logflags, 0);
	COMPQUIET(repflags, 0);
	return (__db_norep(env));
}

int
__rep_set_limit(dbenv, gbytes, bytes)
	DB_ENV *dbenv;
	u_int32_t gbytes, bytes;
{
	COMPQUIET(gbytes, 0);
	COMPQUIET(bytes, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_set_transport_pp(dbenv, eid, f_send)
	DB_ENV *dbenv;
	int eid;
	int (*f_send) __P((DB_ENV *, const DBT *, const DBT *, const DB_LSN *,
	    int, u_int32_t));
{
	COMPQUIET(eid, 0);
	COMPQUIET(f_send, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_set_request(dbenv, min, max)
	DB_ENV *dbenv;
	u_int32_t min, max;
{
	COMPQUIET(min, 0);
	COMPQUIET(max, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_get_request(dbenv, minp, maxp)
	DB_ENV *dbenv;
	u_int32_t *minp, *maxp;
{
	COMPQUIET(minp, NULL);
	COMPQUIET(maxp, NULL);
	return (__db_norep(dbenv->env));
}

int
__rep_start_pp(dbenv, dbt, flags)
	DB_ENV *dbenv;
	DBT *dbt;
	u_int32_t flags;
{
	COMPQUIET(dbt, NULL);
	COMPQUIET(flags, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_REP_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_norep(dbenv->env));
}

int
__rep_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_norep(env));
}

int
__rep_sync(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_norep(dbenv->env));
}
#endif /* !HAVE_REPLICATION */
