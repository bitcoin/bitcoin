/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef HAVE_MUTEX_SUPPORT
#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"

/*
 * If the library wasn't compiled with mutex support, various routines
 * aren't available.  Stub them here, returning an appropriate error.
 */
static int __db_nomutex __P((ENV *));

/*
 * __db_nomutex --
 *	Error when a Berkeley DB build doesn't include mutexes.
 */
static int
__db_nomutex(env)
	ENV *env;
{
	__db_errx(env, "library build did not include support for mutexes");
	return (DB_OPNOTSUP);
}

int
__mutex_alloc_pp(dbenv, flags, indxp)
	DB_ENV *dbenv;
	u_int32_t flags;
	db_mutex_t *indxp;
{
	COMPQUIET(flags, 0);
	COMPQUIET(indxp, NULL);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_alloc(env, alloc_id, flags, indxp)
	ENV *env;
	int alloc_id;
	u_int32_t flags;
	db_mutex_t *indxp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(alloc_id, 0);
	COMPQUIET(flags, 0);
	*indxp = MUTEX_INVALID;
	return (0);
}

void
__mutex_clear(env, mutex)
	ENV *env;
	db_mutex_t mutex;
{
	COMPQUIET(env, NULL);
	COMPQUIET(mutex, MUTEX_INVALID);
}

int
__mutex_free_pp(dbenv, indx)
	DB_ENV *dbenv;
	db_mutex_t indx;
{
	COMPQUIET(indx, 0);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_free(env, indxp)
	ENV *env;
	db_mutex_t *indxp;
{
	COMPQUIET(env, NULL);
	*indxp = MUTEX_INVALID;
	return (0);
}

int
__mutex_get_align(dbenv, alignp)
	DB_ENV *dbenv;
	u_int32_t *alignp;
{
	COMPQUIET(alignp, NULL);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_get_increment(dbenv, incrementp)
	DB_ENV *dbenv;
	u_int32_t *incrementp;
{
	COMPQUIET(incrementp, NULL);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_get_max(dbenv, maxp)
	DB_ENV *dbenv;
	u_int32_t *maxp;
{
	COMPQUIET(maxp, NULL);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_get_tas_spins(dbenv, tas_spinsp)
	DB_ENV *dbenv;
	u_int32_t *tas_spinsp;
{
	COMPQUIET(tas_spinsp, NULL);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_lock_pp(dbenv, indx)
	DB_ENV *dbenv;
	db_mutex_t indx;
{
	COMPQUIET(indx, 0);
	return (__db_nomutex(dbenv->env));
}

void
__mutex_print_debug_single(env, tag, mutex, flags)
	ENV *env;
	const char *tag;
	db_mutex_t mutex;
	u_int32_t flags;
{
	COMPQUIET(env, NULL);
	COMPQUIET(tag, NULL);
	COMPQUIET(mutex, MUTEX_INVALID);
	COMPQUIET(flags, 0);
}

void
__mutex_print_debug_stats(env, mbp, mutex, flags)
	ENV *env;
	DB_MSGBUF *mbp;
	db_mutex_t mutex;
	u_int32_t flags;
{
	COMPQUIET(env, NULL);
	COMPQUIET(mbp, NULL);
	COMPQUIET(mutex, MUTEX_INVALID);
	COMPQUIET(flags, 0);
}

int
__mutex_set_align(dbenv, align)
	DB_ENV *dbenv;
	u_int32_t align;
{
	COMPQUIET(align, 0);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_set_increment(dbenv, increment)
	DB_ENV *dbenv;
	u_int32_t increment;
{
	COMPQUIET(increment, 0);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_set_max(dbenv, max)
	DB_ENV *dbenv;
	u_int32_t max;
{
	COMPQUIET(max, 0);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_set_tas_spins(dbenv, tas_spins)
	DB_ENV *dbenv;
	u_int32_t tas_spins;
{
	COMPQUIET(tas_spins, 0);
	return (__db_nomutex(dbenv->env));
}

void
__mutex_set_wait_info(env, mutex, waitp, nowaitp)
	ENV *env;
	db_mutex_t mutex;
	uintmax_t *waitp, *nowaitp;
{
	COMPQUIET(env, NULL);
	COMPQUIET(mutex, MUTEX_INVALID);
	*waitp = *nowaitp = 0;
}

int
__mutex_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_MUTEX_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);
	return (__db_nomutex(dbenv->env));
}

int
__mutex_unlock_pp(dbenv, indx)
	DB_ENV *dbenv;
	db_mutex_t indx;
{
	COMPQUIET(indx, 0);
	return (__db_nomutex(dbenv->env));
}
#endif /* !HAVE_MUTEX_SUPPORT */
