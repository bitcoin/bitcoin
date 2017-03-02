/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"

static int __log_printf_int __P((ENV *, DB_TXN *, const char *, va_list));

/*
 * __log_printf_capi --
 *	Write a printf-style format string into the DB log.
 *
 * PUBLIC: int __log_printf_capi __P((DB_ENV *, DB_TXN *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
int
#ifdef STDC_HEADERS
__log_printf_capi(DB_ENV *dbenv, DB_TXN *txnid, const char *fmt, ...)
#else
__log_printf_capi(dbenv, txnid, fmt, va_alist)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	int ret;

#ifdef STDC_HEADERS
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = __log_printf_pp(dbenv, txnid, fmt, ap);
	va_end(ap);

	return (ret);
}

/*
 * __log_printf_pp --
 *	Handle the arguments and call an internal routine to do the work.
 *
 *	The reason this routine isn't just folded into __log_printf_capi
 *	is because the C++ API has to call a C API routine, and you can
 *	only pass variadic arguments to a single routine.
 *
 * PUBLIC: int __log_printf_pp
 * PUBLIC:     __P((DB_ENV *, DB_TXN *, const char *, va_list));
 */
int
__log_printf_pp(dbenv, txnid, fmt, ap)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	const char *fmt;
	va_list ap;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_ENV->log_printf", DB_INIT_LOG);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_printf_int(env, txnid, fmt, ap)), 0, ret);
	va_end(ap);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __log_printf --
 *	Write a printf-style format string into the DB log.
 *
 * PUBLIC: int __log_printf __P((ENV *, DB_TXN *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
int
#ifdef STDC_HEADERS
__log_printf(ENV *env, DB_TXN *txnid, const char *fmt, ...)
#else
__log_printf(env, txnid, fmt, va_alist)
	ENV *env;
	DB_TXN *txnid;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	int ret;

#ifdef STDC_HEADERS
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = __log_printf_int(env, txnid, fmt, ap);
	va_end(ap);

	return (ret);
}

/*
 * __log_printf_int --
 *	Write a printf-style format string into the DB log (internal).
 */
static int
__log_printf_int(env, txnid, fmt, ap)
	ENV *env;
	DB_TXN *txnid;
	const char *fmt;
	va_list ap;
{
	DBT opdbt, msgdbt;
	DB_LSN lsn;
	char __logbuf[2048];	/* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

	if (!DBENV_LOGGING(env)) {
		__db_errx(env, "Logging not currently permitted");
		return (EAGAIN);
	}

	memset(&opdbt, 0, sizeof(opdbt));
	opdbt.data = "DIAGNOSTIC";
	opdbt.size = sizeof("DIAGNOSTIC") - 1;

	memset(&msgdbt, 0, sizeof(msgdbt));
	msgdbt.data = __logbuf;
	msgdbt.size = (u_int32_t)vsnprintf(__logbuf, sizeof(__logbuf), fmt, ap);

	return (__db_debug_log(
	    env, txnid, &lsn, 0, &opdbt, -1, &msgdbt, NULL, 0));
}
