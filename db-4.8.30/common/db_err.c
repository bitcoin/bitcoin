/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static void __db_msgcall __P((const DB_ENV *, const char *, va_list));
static void __db_msgfile __P((const DB_ENV *, const char *, va_list));

/*
 * __db_fchk --
 *	General flags checking routine.
 *
 * PUBLIC: int __db_fchk __P((ENV *, const char *, u_int32_t, u_int32_t));
 */
int
__db_fchk(env, name, flags, ok_flags)
	ENV *env;
	const char *name;
	u_int32_t flags, ok_flags;
{
	return (LF_ISSET(~ok_flags) ? __db_ferr(env, name, 0) : 0);
}

/*
 * __db_fcchk --
 *	General combination flags checking routine.
 *
 * PUBLIC: int __db_fcchk
 * PUBLIC:    __P((ENV *, const char *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__db_fcchk(env, name, flags, flag1, flag2)
	ENV *env;
	const char *name;
	u_int32_t flags, flag1, flag2;
{
	return (LF_ISSET(flag1) &&
	    LF_ISSET(flag2) ? __db_ferr(env, name, 1) : 0);
}

/*
 * __db_ferr --
 *	Common flag errors.
 *
 * PUBLIC: int __db_ferr __P((const ENV *, const char *, int));
 */
int
__db_ferr(env, name, iscombo)
	const ENV *env;
	const char *name;
	int iscombo;
{
	__db_errx(env, "illegal flag %sspecified to %s",
	    iscombo ? "combination " : "", name);
	return (EINVAL);
}

/*
 * __db_fnl --
 *	Common flag-needs-locking message.
 *
 * PUBLIC: int __db_fnl __P((const ENV *, const char *));
 */
int
__db_fnl(env, name)
	const ENV *env;
	const char *name;
{
	__db_errx(env,
    "%s: DB_READ_COMMITTED, DB_READ_UNCOMMITTED and DB_RMW require locking",
	    name);
	return (EINVAL);
}

/*
 * __db_pgerr --
 *	Error when unable to retrieve a specified page.
 *
 * PUBLIC: int __db_pgerr __P((DB *, db_pgno_t, int));
 */
int
__db_pgerr(dbp, pgno, errval)
	DB *dbp;
	db_pgno_t pgno;
	int errval;
{
	/*
	 * Three things are certain:
	 * Death, taxes, and lost data.
	 * Guess which has occurred.
	 */
	__db_errx(dbp->env,
	    "unable to create/retrieve page %lu", (u_long)pgno);
	return (__env_panic(dbp->env, errval));
}

/*
 * __db_pgfmt --
 *	Error when a page has the wrong format.
 *
 * PUBLIC: int __db_pgfmt __P((ENV *, db_pgno_t));
 */
int
__db_pgfmt(env, pgno)
	ENV *env;
	db_pgno_t pgno;
{
	__db_errx(env, "page %lu: illegal page type or format", (u_long)pgno);
	return (__env_panic(env, EINVAL));
}

#ifdef DIAGNOSTIC
/*
 * __db_assert --
 *	Error when an assertion fails.  Only checked if #DIAGNOSTIC defined.
 *
 * PUBLIC: #ifdef DIAGNOSTIC
 * PUBLIC: void __db_assert __P((ENV *, const char *, const char *, int));
 * PUBLIC: #endif
 */
void
__db_assert(env, e, file, line)
	ENV *env;
	const char *e, *file;
	int line;
{
	__db_errx(env, "assert failure: %s/%d: \"%s\"", file, line, e);

	__os_abort(env);
	/* NOTREACHED */
}
#endif

/*
 * __env_panic_msg --
 *	Just report that someone else paniced.
 *
 * PUBLIC: int __env_panic_msg __P((ENV *));
 */
int
__env_panic_msg(env)
	ENV *env;
{
	DB_ENV *dbenv;
	int ret;

	dbenv = env->dbenv;

	ret = DB_RUNRECOVERY;

	__db_errx(env, "PANIC: fatal region error detected; run recovery");

	if (dbenv->db_paniccall != NULL)		/* Deprecated */
		dbenv->db_paniccall(dbenv, ret);

	/* Must check for DB_EVENT_REG_PANIC panic first because it is never
	 * set by itself.  If set, it means panic came from DB_REGISTER code
	 * only, otherwise it could be from many possible places in the code.
	 */
	if ((env->reginfo != NULL) &&
	    (((REGENV *)env->reginfo->primary)->reg_panic))
		DB_EVENT(env, DB_EVENT_REG_PANIC, &ret);
	else
		DB_EVENT(env, DB_EVENT_PANIC, &ret);

	return (ret);
}

/*
 * __env_panic --
 *	Lock out the database environment due to unrecoverable error.
 *
 * PUBLIC: int __env_panic __P((ENV *, int));
 */
int
__env_panic(env, errval)
	ENV *env;
	int errval;
{
	DB_ENV *dbenv;

	dbenv = env->dbenv;

	if (env != NULL) {
		__env_panic_set(env, 1);

		__db_err(env, errval, "PANIC");

		if (dbenv->db_paniccall != NULL)	/* Deprecated */
			dbenv->db_paniccall(dbenv, errval);

		/* Must check for DB_EVENT_REG_PANIC first because it is never
		 * set by itself.  If set, it means panic came from DB_REGISTER
		 * code only, otherwise it could be from many possible places
		 * in the code.
		 */
		if ((env->reginfo != NULL) &&
		    (((REGENV *)env->reginfo->primary)->reg_panic))
			DB_EVENT(env, DB_EVENT_REG_PANIC, &errval);
		else
			DB_EVENT(env, DB_EVENT_PANIC, &errval);
	}

#if defined(DIAGNOSTIC) && !defined(CONFIG_TEST)
	/*
	 * We want a stack trace of how this could possibly happen.
	 *
	 * Don't drop core if it's the test suite -- it's reasonable for the
	 * test suite to check to make sure that DB_RUNRECOVERY is returned
	 * under certain conditions.
	 */
	__os_abort(env);
	/* NOTREACHED */
#endif

	/*
	 * Chaos reigns within.
	 * Reflect, repent, and reboot.
	 * Order shall return.
	 */
	return (DB_RUNRECOVERY);
}

/*
 * db_strerror --
 *	ANSI C strerror(3) for DB.
 *
 * EXTERN: char *db_strerror __P((int));
 */
char *
db_strerror(error)
	int error;
{
	char *p;

	if (error == 0)
		return ("Successful return: 0");
	if (error > 0) {
		if ((p = strerror(error)) != NULL)
			return (p);
		return (__db_unknown_error(error));
	}

	/*
	 * !!!
	 * The Tcl API requires that some of these return strings be compared
	 * against strings stored in application scripts.  So, any of these
	 * errors that do not invariably result in a Tcl exception may not be
	 * altered.
	 */
	switch (error) {
	case DB_BUFFER_SMALL:
		return
		    ("DB_BUFFER_SMALL: User memory too small for return value");
	case DB_DONOTINDEX:
		return ("DB_DONOTINDEX: Secondary index callback returns null");
	case DB_FOREIGN_CONFLICT:
		return
       ("DB_FOREIGN_CONFLICT: A foreign database constraint has been violated");
	case DB_KEYEMPTY:
		return ("DB_KEYEMPTY: Non-existent key/data pair");
	case DB_KEYEXIST:
		return ("DB_KEYEXIST: Key/data pair already exists");
	case DB_LOCK_DEADLOCK:
		return
		    ("DB_LOCK_DEADLOCK: Locker killed to resolve a deadlock");
	case DB_LOCK_NOTGRANTED:
		return ("DB_LOCK_NOTGRANTED: Lock not granted");
	case DB_LOG_BUFFER_FULL:
		return ("DB_LOG_BUFFER_FULL: In-memory log buffer is full");
	case DB_NOSERVER:
		return ("DB_NOSERVER: Fatal error, no RPC server");
	case DB_NOSERVER_HOME:
		return ("DB_NOSERVER_HOME: Home unrecognized at server");
	case DB_NOSERVER_ID:
		return ("DB_NOSERVER_ID: Identifier unrecognized at server");
	case DB_NOTFOUND:
		return ("DB_NOTFOUND: No matching key/data pair found");
	case DB_OLD_VERSION:
		return ("DB_OLDVERSION: Database requires a version upgrade");
	case DB_PAGE_NOTFOUND:
		return ("DB_PAGE_NOTFOUND: Requested page not found");
	case DB_REP_DUPMASTER:
		return ("DB_REP_DUPMASTER: A second master site appeared");
	case DB_REP_HANDLE_DEAD:
		return ("DB_REP_HANDLE_DEAD: Handle is no longer valid");
	case DB_REP_HOLDELECTION:
		return ("DB_REP_HOLDELECTION: Need to hold an election");
	case DB_REP_IGNORE:
		return ("DB_REP_IGNORE: Replication record/operation ignored");
	case DB_REP_ISPERM:
		return ("DB_REP_ISPERM: Permanent record written");
	case DB_REP_JOIN_FAILURE:
		return
	    ("DB_REP_JOIN_FAILURE: Unable to join replication group");
	case DB_REP_LEASE_EXPIRED:
		return
	    ("DB_REP_LEASE_EXPIRED: Replication leases have expired");
	case DB_REP_LOCKOUT:
		return
	    ("DB_REP_LOCKOUT: Waiting for replication recovery to complete");
	case DB_REP_NEWSITE:
		return ("DB_REP_NEWSITE: A new site has entered the system");
	case DB_REP_NOTPERM:
		return ("DB_REP_NOTPERM: Permanent log record not written");
	case DB_REP_UNAVAIL:
		return ("DB_REP_UNAVAIL: Unable to elect a master");
	case DB_RUNRECOVERY:
		return ("DB_RUNRECOVERY: Fatal error, run database recovery");
	case DB_SECONDARY_BAD:
		return
	    ("DB_SECONDARY_BAD: Secondary index inconsistent with primary");
	case DB_VERIFY_BAD:
		return ("DB_VERIFY_BAD: Database verification failed");
	case DB_VERSION_MISMATCH:
		return
	    ("DB_VERSION_MISMATCH: Database environment version mismatch");
	default:
		break;
	}

	return (__db_unknown_error(error));
}

/*
 * __db_unknown_error --
 *	Format an unknown error value into a static buffer.
 *
 * PUBLIC: char *__db_unknown_error __P((int));
 */
char *
__db_unknown_error(error)
	int error;
{
	/*
	 * !!!
	 * Room for a 64-bit number + slop.  This buffer is only used
	 * if we're given an unknown error number, which should never
	 * happen.
	 *
	 * We're no longer thread-safe if it does happen, but the worst
	 * result is a corrupted error string because there will always
	 * be a trailing nul byte since the error buffer is nul filled
	 * and longer than any error message.
	 */
	(void)snprintf(DB_GLOBAL(error_buf),
	    sizeof(DB_GLOBAL(error_buf)), "Unknown error: %d", error);
	return (DB_GLOBAL(error_buf));
}

/*
 * __db_syserr --
 *	Standard error routine.
 *
 * PUBLIC: void __db_syserr __P((const ENV *, int, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
void
#ifdef STDC_HEADERS
__db_syserr(const ENV *env, int error, const char *fmt, ...)
#else
__db_syserr(env, error, fmt, va_alist)
	const ENV *env;
	int error;
	const char *fmt;
	va_dcl
#endif
{
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	/*
	 * The same as DB->err, except we don't default to writing to stderr
	 * after any output channel has been configured, and we use a system-
	 * specific function to translate errors to strings.
	 */
	DB_REAL_ERR(dbenv, error, DB_ERROR_SYSTEM, 0, fmt);
}

/*
 * __db_err --
 *	Standard error routine.
 *
 * PUBLIC: void __db_err __P((const ENV *, int, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
void
#ifdef STDC_HEADERS
__db_err(const ENV *env, int error, const char *fmt, ...)
#else
__db_err(env, error, fmt, va_alist)
	const ENV *env;
	int error;
	const char *fmt;
	va_dcl
#endif
{
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	/*
	 * The same as DB->err, except we don't default to writing to stderr
	 * once an output channel has been configured.
	 */
	DB_REAL_ERR(dbenv, error, DB_ERROR_SET, 0, fmt);
}

/*
 * __db_errx --
 *	Standard error routine.
 *
 * PUBLIC: void __db_errx __P((const ENV *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 2, 3)));
 */
void
#ifdef STDC_HEADERS
__db_errx(const ENV *env, const char *fmt, ...)
#else
__db_errx(env, fmt, va_alist)
	const ENV *env;
	const char *fmt;
	va_dcl
#endif
{
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	/*
	 * The same as DB->errx, except we don't default to writing to stderr
	 * once an output channel has been configured.
	 */
	DB_REAL_ERR(dbenv, 0, DB_ERROR_NOT_SET, 0, fmt);
}

/*
 * __db_errcall --
 *	Do the error message work for callback functions.
 *
 * PUBLIC: void __db_errcall
 * PUBLIC:    __P((const DB_ENV *, int, db_error_set_t, const char *, va_list));
 */
void
__db_errcall(dbenv, error, error_set, fmt, ap)
	const DB_ENV *dbenv;
	int error;
	db_error_set_t error_set;
	const char *fmt;
	va_list ap;
{
	char *p;
	char buf[2048];		/* !!!: END OF THE STACK DON'T TRUST SPRINTF. */
	char sysbuf[1024];	/* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

	p = buf;
	if (fmt != NULL)
		p += vsnprintf(buf, sizeof(buf), fmt, ap);
	if (error_set != DB_ERROR_NOT_SET)
		p += snprintf(p,
		    sizeof(buf) - (size_t)(p - buf), ": %s",
		    error_set == DB_ERROR_SET ? db_strerror(error) :
		    __os_strerror(error, sysbuf, sizeof(sysbuf)));

	dbenv->db_errcall(dbenv, dbenv->db_errpfx, buf);
}

/*
 * __db_errfile --
 *	Do the error message work for FILE *s.
 *
 * PUBLIC: void __db_errfile
 * PUBLIC:    __P((const DB_ENV *, int, db_error_set_t, const char *, va_list));
 */
void
__db_errfile(dbenv, error, error_set, fmt, ap)
	const DB_ENV *dbenv;
	int error;
	db_error_set_t error_set;
	const char *fmt;
	va_list ap;
{
	FILE *fp;
	int need_sep;
	char sysbuf[1024];	/* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

	fp = dbenv == NULL ||
	    dbenv->db_errfile == NULL ? stderr : dbenv->db_errfile;
	need_sep = 0;

	if (dbenv != NULL && dbenv->db_errpfx != NULL) {
		(void)fprintf(fp, "%s", dbenv->db_errpfx);
		need_sep = 1;
	}
	if (fmt != NULL && fmt[0] != '\0') {
		if (need_sep)
			(void)fprintf(fp, ": ");
		need_sep = 1;
		(void)vfprintf(fp, fmt, ap);
	}
	if (error_set != DB_ERROR_NOT_SET)
		(void)fprintf(fp, "%s%s",
		    need_sep ? ": " : "",
		    error_set == DB_ERROR_SET ? db_strerror(error) :
		    __os_strerror(error, sysbuf, sizeof(sysbuf)));
	(void)fprintf(fp, "\n");
	(void)fflush(fp);
}

/*
 * __db_msgadd --
 *	Aggregate a set of strings into a buffer for the callback API.
 *
 * PUBLIC: void __db_msgadd __P((ENV *, DB_MSGBUF *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 3, 4)));
 */
void
#ifdef STDC_HEADERS
__db_msgadd(ENV *env, DB_MSGBUF *mbp, const char *fmt, ...)
#else
__db_msgadd(env, mbp, fmt, va_alist)
	ENV *env;
	DB_MSGBUF *mbp;
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;

#ifdef STDC_HEADERS
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	__db_msgadd_ap(env, mbp, fmt, ap);
	va_end(ap);
}

/*
 * __db_msgadd_ap --
 *	Aggregate a set of strings into a buffer for the callback API.
 *
 * PUBLIC: void __db_msgadd_ap
 * PUBLIC:     __P((ENV *, DB_MSGBUF *, const char *, va_list));
 */
void
__db_msgadd_ap(env, mbp, fmt, ap)
	ENV *env;
	DB_MSGBUF *mbp;
	const char *fmt;
	va_list ap;
{
	size_t len, olen;
	char buf[2048];		/* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

	len = (size_t)vsnprintf(buf, sizeof(buf), fmt, ap);

	/*
	 * There's a heap buffer in the ENV handle we use to aggregate the
	 * message chunks.  We maintain a pointer to the buffer, the next slot
	 * to be filled in in the buffer, and a total buffer length.
	 */
	olen = (size_t)(mbp->cur - mbp->buf);
	if (olen + len >= mbp->len) {
		if (__os_realloc(env, mbp->len + len + 256, &mbp->buf))
			return;
		mbp->len += (len + 256);
		mbp->cur = mbp->buf + olen;
	}

	memcpy(mbp->cur, buf, len + 1);
	mbp->cur += len;
}

/*
 * __db_msg --
 *	Standard DB stat message routine.
 *
 * PUBLIC: void __db_msg __P((const ENV *, const char *, ...))
 * PUBLIC:    __attribute__ ((__format__ (__printf__, 2, 3)));
 */
void
#ifdef STDC_HEADERS
__db_msg(const ENV *env, const char *fmt, ...)
#else
__db_msg(env, fmt, va_alist)
	const ENV *env;
	const char *fmt;
	va_dcl
#endif
{
	DB_ENV *dbenv;

	dbenv = env == NULL ? NULL : env->dbenv;

	DB_REAL_MSG(dbenv, fmt);
}

/*
 * __db_msgcall --
 *	Do the message work for callback functions.
 */
static void
__db_msgcall(dbenv, fmt, ap)
	const DB_ENV *dbenv;
	const char *fmt;
	va_list ap;
{
	char buf[2048];		/* !!!: END OF THE STACK DON'T TRUST SPRINTF. */

	(void)vsnprintf(buf, sizeof(buf), fmt, ap);

	dbenv->db_msgcall(dbenv, buf);
}

/*
 * __db_msgfile --
 *	Do the message work for FILE *s.
 */
static void
__db_msgfile(dbenv, fmt, ap)
	const DB_ENV *dbenv;
	const char *fmt;
	va_list ap;
{
	FILE *fp;

	fp = dbenv == NULL ||
	    dbenv->db_msgfile == NULL ? stdout : dbenv->db_msgfile;
	(void)vfprintf(fp, fmt, ap);

	(void)fprintf(fp, "\n");
	(void)fflush(fp);
}

/*
 * __db_unknown_flag -- report internal error
 *
 * PUBLIC: int __db_unknown_flag __P((ENV *, char *, u_int32_t));
 */
int
__db_unknown_flag(env, routine, flag)
	ENV *env;
	char *routine;
	u_int32_t flag;
{
	__db_errx(env, "%s: Unknown flag: %#x", routine, (u_int)flag);

#ifdef DIAGNOSTIC
	__os_abort(env);
	/* NOTREACHED */
#endif
	return (EINVAL);
}

/*
 * __db_unknown_type -- report internal database type error
 *
 * PUBLIC: int __db_unknown_type __P((ENV *, char *, DBTYPE));
 */
int
__db_unknown_type(env, routine, type)
	ENV *env;
	char *routine;
	DBTYPE type;
{
	__db_errx(env,
	    "%s: Unexpected database type: %s",
	    routine, __db_dbtype_to_string(type));

#ifdef DIAGNOSTIC
	__os_abort(env);
	/* NOTREACHED */
#endif
	return (EINVAL);
}

/*
 * __db_unknown_path -- report unexpected database code path error.
 *
 * PUBLIC: int __db_unknown_path __P((ENV *, char *));
 */
int
__db_unknown_path(env, routine)
	ENV *env;
	char *routine;
{
	__db_errx(env, "%s: Unexpected code path error", routine);

#ifdef DIAGNOSTIC
	__os_abort(env);
	/* NOTREACHED */
#endif
	return (EINVAL);
}

/*
 * __db_check_txn --
 *	Check for common transaction errors.
 *
 * PUBLIC: int __db_check_txn __P((DB *, DB_TXN *, DB_LOCKER *, int));
 */
int
__db_check_txn(dbp, txn, assoc_locker, read_op)
	DB *dbp;
	DB_TXN *txn;
	DB_LOCKER *assoc_locker;
	int read_op;
{
	ENV *env;
	int isp, ret;

	env = dbp->env;

	/*
	 * If we are in recovery or aborting a transaction, then we
	 * don't need to enforce the rules about dbp's not allowing
	 * transactional operations in non-transactional dbps and
	 * vica-versa.  This happens all the time as the dbp during
	 * an abort may be transactional, but we undo operations
	 * outside a transaction since we're aborting.
	 */
	if (IS_RECOVERING(env) || F_ISSET(dbp, DB_AM_RECOVER))
		return (0);

	/*
	 * Check for common transaction errors:
	 *	an operation on a handle whose open commit hasn't completed.
	 *	a transaction handle in a non-transactional environment
	 *	a transaction handle for a non-transactional database
	 */
	if (txn == NULL || F_ISSET(txn, TXN_PRIVATE)) {
		if (dbp->cur_locker != NULL &&
		    dbp->cur_locker->id >= TXN_MINIMUM)
			goto open_err;

		if (!read_op && F_ISSET(dbp, DB_AM_TXN)) {
			__db_errx(env,
		    "Transaction not specified for a transactional database");
			return (EINVAL);
		}
	} else if (F_ISSET(txn, TXN_CDSGROUP)) {
		if (!CDB_LOCKING(env)) {
			__db_errx(env,
			    "CDS groups can only be used in a CDS environment");
			return (EINVAL);
		}
		/*
		 * CDS group handles can be passed to any method, since they
		 * only determine locker IDs.
		 */
		return (0);
	} else {
		if (!TXN_ON(env))
			 return (__db_not_txn_env(env));

		if (!F_ISSET(dbp, DB_AM_TXN)) {
			__db_errx(env,
		    "Transaction specified for a non-transactional database");
			return (EINVAL);
		}

		if (F_ISSET(txn, TXN_DEADLOCK))
			return (__db_txn_deadlock_err(env, txn));
		if (dbp->cur_locker != NULL &&
		    dbp->cur_locker->id >= TXN_MINIMUM &&
		     dbp->cur_locker->id != txn->txnid) {
			if ((ret = __lock_locker_is_parent(env,
			     dbp->cur_locker, txn->locker, &isp)) != 0)
				return (ret);
			if (!isp)
				goto open_err;
		}
	}

	/*
	 * If dbp->associate_locker is not NULL, that means we're in
	 * the middle of a DB->associate with DB_CREATE (i.e., a secondary index
	 * creation).
	 *
	 * In addition to the usual transaction rules, we need to lock out
	 * non-transactional updates that aren't part of the associate (and
	 * thus are using some other locker ID).
	 *
	 * Transactional updates should simply block;  from the time we
	 * decide to build the secondary until commit, we'll hold a write
	 * lock on all of its pages, so it should be safe to attempt to update
	 * the secondary in another transaction (presumably by updating the
	 * primary).
	 */
	if (!read_op && dbp->associate_locker != NULL &&
	    txn != NULL && dbp->associate_locker != assoc_locker) {
		__db_errx(env,
	    "Operation forbidden while secondary index is being created");
		return (EINVAL);
	}

	/*
	 * Check the txn and dbp are from the same env.
	 */
	if (txn != NULL && env != txn->mgrp->env) {
		__db_errx(env,
	    "Transaction and database from different environments");
		return (EINVAL);
	}

	return (0);
open_err:
	__db_errx(env,
	    "Transaction that opened the DB handle is still active");
	return (EINVAL);
}

/*
 * __db_txn_deadlock_err --
 *	Transaction has allready been deadlocked.
 *
 * PUBLIC: int __db_txn_deadlock_err __P((ENV *, DB_TXN *));
 */
int
__db_txn_deadlock_err(env, txn)
	ENV *env;
	DB_TXN *txn;
{
	const char *name;

	name = NULL;
	(void)__txn_get_name(txn, &name);

	__db_errx(env,
	    "%s%sprevious transaction deadlock return not resolved",
	    name == NULL ? "" : name, name == NULL ? "" : ": ");

	return (EINVAL);
}

/*
 * __db_not_txn_env --
 *	DB handle must be in an environment that supports transactions.
 *
 * PUBLIC: int __db_not_txn_env __P((ENV *));
 */
int
__db_not_txn_env(env)
	ENV *env;
{
	__db_errx(env, "DB environment not configured for transactions");
	return (EINVAL);
}

/*
 * __db_rec_toobig --
 *	Fixed record length exceeded error message.
 *
 * PUBLIC: int __db_rec_toobig __P((ENV *, u_int32_t, u_int32_t));
 */
int
__db_rec_toobig(env, data_len, fixed_rec_len)
	ENV *env;
	u_int32_t data_len, fixed_rec_len;
{
	__db_errx(env,
	    "%lu larger than database's maximum record length %lu",
	    (u_long)data_len, (u_long)fixed_rec_len);
	return (EINVAL);
}

/*
 * __db_rec_repl --
 *	Fixed record replacement length error message.
 *
 * PUBLIC: int __db_rec_repl __P((ENV *, u_int32_t, u_int32_t));
 */
int
__db_rec_repl(env, data_size, data_dlen)
	ENV *env;
	u_int32_t data_size, data_dlen;
{
	__db_errx(env,
	    "%s: replacement length %lu differs from replaced length %lu",
	    "Record length error", (u_long)data_size, (u_long)data_dlen);
	return (EINVAL);
}

#if defined(DIAGNOSTIC) || defined(DEBUG_ROP)  || defined(DEBUG_WOP)
/*
 * __dbc_logging --
 *	In DIAGNOSTIC mode, check for bad replication combinations.
 *
 * PUBLIC: int __dbc_logging __P((DBC *));
 */
int
__dbc_logging(dbc)
	DBC *dbc;
{
	DB_REP *db_rep;
	ENV *env;
	int ret;

	env = dbc->env;
	db_rep = env->rep_handle;

	ret = LOGGING_ON(env) &&
	    !F_ISSET(dbc, DBC_RECOVER) && !IS_REP_CLIENT(env);

	/*
	 * If we're not using replication or running recovery, return.
	 */
	if (db_rep == NULL || F_ISSET(dbc, DBC_RECOVER))
		return (ret);

#ifndef	DEBUG_ROP
	/*
	 *  Only check when DEBUG_ROP is not configured.  People often do
	 * non-transactional reads, and debug_rop is going to write
	 * a log record.
	 */
	{
	REP *rep;

	rep = db_rep->region;

	/*
	 * If we're a client and not running recovery or non durably, error.
	 */
	if (IS_REP_CLIENT(env) && !F_ISSET(dbc->dbp, DB_AM_NOT_DURABLE)) {
		__db_errx(env, "dbc_logging: Client update");
		goto err;
	}

#ifndef DEBUG_WOP
	/*
	 * If DEBUG_WOP is enabled, then we'll generate debugging log records
	 * that are non-transactional.  This is OK.
	 */
	if (IS_REP_MASTER(env) &&
	    dbc->txn == NULL && !F_ISSET(dbc->dbp, DB_AM_NOT_DURABLE)) {
		__db_errx(env, "Dbc_logging: Master non-txn update");
		goto err;
	}
#endif

	if (0) {
err:		__db_errx(env, "Rep: flags 0x%lx msg_th %lu",
		    (u_long)rep->flags, (u_long)rep->msg_th);
		__db_errx(env, "Rep: handle %lu, opcnt %lu",
		    (u_long)rep->handle_cnt, (u_long)rep->op_cnt);
		__os_abort(env);
		/* NOTREACHED */
	}
	}
#endif
	return (ret);
}
#endif

/*
 * __db_check_lsn --
 *	Display the log sequence error message.
 *
 * PUBLIC: int __db_check_lsn __P((ENV *, DB_LSN *, DB_LSN *));
 */
int
__db_check_lsn(env, lsn, prev)
	ENV *env;
	DB_LSN *lsn, *prev;
{
	__db_errx(env,
	    "Log sequence error: page LSN %lu %lu; previous LSN %lu %lu",
	    (u_long)(lsn)->file, (u_long)(lsn)->offset,
	    (u_long)(prev)->file, (u_long)(prev)->offset);
	return (EINVAL);
}

/*
 * __db_rdonly --
 *	Common readonly message.
 * PUBLIC: int __db_rdonly __P((const ENV *, const char *));
 */
int
__db_rdonly(env, name)
	const ENV *env;
	const char *name;
{
	__db_errx(env, "%s: attempt to modify a read-only database", name);
	return (EACCES);
}

/*
 * __db_space_err --
 *	Common out of space message.
 * PUBLIC: int __db_space_err __P((const DB *));
 */
int
__db_space_err(dbp)
	const DB *dbp;
{
	__db_errx(dbp->env,
	    "%s: file limited to %lu pages",
	    dbp->fname, (u_long)dbp->mpf->mfp->maxpgno);
	return (ENOSPC);
}

/*
 * __db_failed --
 *	Common failed thread  message.
 *
 * PUBLIC: int __db_failed __P((const ENV *,
 * PUBLIC:      const char *, pid_t, db_threadid_t));
 */
int
__db_failed(env, msg, pid, tid)
	const ENV *env;
	const char *msg;
	pid_t pid;
	db_threadid_t tid;
{
	DB_ENV *dbenv;
	char buf[DB_THREADID_STRLEN];

	dbenv = env->dbenv;

	__db_errx(env, "Thread/process %s failed: %s",
	    dbenv->thread_id_string(dbenv, pid, tid, buf),  msg);
	return (DB_RUNRECOVERY);
}
