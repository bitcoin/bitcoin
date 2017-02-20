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
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/qam.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/partition.h"

#ifdef HAVE_STATISTICS
static int __db_print_all __P((DB *, u_int32_t));
static int __db_print_citem __P((DBC *));
static int __db_print_cursor __P((DB *));
static int __db_print_stats __P((DB *, DB_THREAD_INFO *, u_int32_t));
static int __db_stat __P((DB *, DB_THREAD_INFO *, DB_TXN *, void *, u_int32_t));
static int __db_stat_arg __P((DB *, u_int32_t));

/*
 * __db_stat_pp --
 *	DB->stat pre/post processing.
 *
 * PUBLIC: int __db_stat_pp __P((DB *, DB_TXN *, void *, u_int32_t));
 */
int
__db_stat_pp(dbp, txn, spp, flags)
	DB *dbp;
	DB_TXN *txn;
	void *spp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->stat");

	if ((ret = __db_stat_arg(dbp, flags)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0,
	    txn != NULL)) != 0) {
		handle_check = 0;
		goto err;
	}

	ret = __db_stat(dbp, ip, txn, spp, flags);

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_stat --
 *	DB->stat.
 *
 */
static int
__db_stat(dbp, ip, txn, spp, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	void *spp;
	u_int32_t flags;
{
	DBC *dbc;
	ENV *env;
	int ret, t_ret;

	env = dbp->env;

	/* Acquire a cursor. */
	if ((ret = __db_cursor(dbp, ip, txn,
	     &dbc, LF_ISSET(DB_READ_COMMITTED | DB_READ_UNCOMMITTED))) != 0)
		return (ret);

	DEBUG_LWRITE(dbc, NULL, "DB->stat", NULL, NULL, flags);
	LF_CLR(DB_READ_COMMITTED | DB_READ_UNCOMMITTED);
#ifdef HAVE_PARTITION
	if (DB_IS_PARTITIONED(dbp))
		ret = __partition_stat(dbc, spp, flags);
	else
#endif
	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		ret = __bam_stat(dbc, spp, flags);
		break;
	case DB_HASH:
		ret = __ham_stat(dbc, spp, flags);
		break;
	case DB_QUEUE:
		ret = __qam_stat(dbc, spp, flags);
		break;
	case DB_UNKNOWN:
	default:
		ret = (__db_unknown_type(env, "DB->stat", dbp->type));
		break;
	}

	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_stat_arg --
 *	Check DB->stat arguments.
 */
static int
__db_stat_arg(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	ENV *env;

	env = dbp->env;

	/* Check for invalid function flags. */
	LF_CLR(DB_READ_COMMITTED | DB_READ_UNCOMMITTED);
	switch (flags) {
	case 0:
	case DB_FAST_STAT:
		break;
	default:
		return (__db_ferr(env, "DB->stat", 0));
	}

	return (0);
}

/*
 * __db_stat_print_pp --
 *	DB->stat_print pre/post processing.
 *
 * PUBLIC: int __db_stat_print_pp __P((DB *, u_int32_t));
 */
int
__db_stat_print_pp(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int handle_check, ret, t_ret;

	env = dbp->env;

	DB_ILLEGAL_BEFORE_OPEN(dbp, "DB->stat_print");

	/*
	 * !!!
	 * The actual argument checking is simple, do it inline.
	 */
	if ((ret = __db_fchk(env,
	    "DB->stat_print", flags, DB_FAST_STAT | DB_STAT_ALL)) != 0)
		return (ret);

	ENV_ENTER(env, ip);

	/* Check for replication block. */
	handle_check = IS_ENV_REPLICATED(env);
	if (handle_check && (ret = __db_rep_enter(dbp, 1, 0, 0)) != 0) {
		handle_check = 0;
		goto err;
	}

	ret = __db_stat_print(dbp, ip, flags);

	/* Release replication block. */
	if (handle_check && (t_ret = __env_db_rep_exit(env)) != 0 && ret == 0)
		ret = t_ret;

err:	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __db_stat_print --
 *	DB->stat_print.
 *
 * PUBLIC: int __db_stat_print __P((DB *, DB_THREAD_INFO *, u_int32_t));
 */
int
__db_stat_print(dbp, ip, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	u_int32_t flags;
{
	time_t now;
	int ret;
	char time_buf[CTIME_BUFLEN];

	(void)time(&now);
	__db_msg(dbp->env, "%.24s\tLocal time", __os_ctime(&now, time_buf));

	if (LF_ISSET(DB_STAT_ALL) && (ret = __db_print_all(dbp, flags)) != 0)
		return (ret);

	if ((ret = __db_print_stats(dbp, ip, flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __db_print_stats --
 *	Display default DB handle statistics.
 */
static int
__db_print_stats(dbp, ip, flags)
	DB *dbp;
	DB_THREAD_INFO *ip;
	u_int32_t flags;
{
	DBC *dbc;
	ENV *env;
	int ret, t_ret;

	env = dbp->env;

	/* Acquire a cursor. */
	if ((ret = __db_cursor(dbp, ip, NULL, &dbc, 0)) != 0)
		return (ret);

	DEBUG_LWRITE(dbc, NULL, "DB->stat_print", NULL, NULL, 0);

	switch (dbp->type) {
	case DB_BTREE:
	case DB_RECNO:
		ret = __bam_stat_print(dbc, flags);
		break;
	case DB_HASH:
		ret = __ham_stat_print(dbc, flags);
		break;
	case DB_QUEUE:
		ret = __qam_stat_print(dbc, flags);
		break;
	case DB_UNKNOWN:
	default:
		ret = (__db_unknown_type(env, "DB->stat_print", dbp->type));
		break;
	}

	if ((t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __db_print_all --
 *	Display debugging DB handle statistics.
 */
static int
__db_print_all(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ DB_AM_CHKSUM,			"DB_AM_CHKSUM" },
		{ DB_AM_COMPENSATE,		"DB_AM_COMPENSATE" },
		{ DB_AM_CREATED,		"DB_AM_CREATED" },
		{ DB_AM_CREATED_MSTR,		"DB_AM_CREATED_MSTR" },
		{ DB_AM_DBM_ERROR,		"DB_AM_DBM_ERROR" },
		{ DB_AM_DELIMITER,		"DB_AM_DELIMITER" },
		{ DB_AM_DISCARD,		"DB_AM_DISCARD" },
		{ DB_AM_DUP,			"DB_AM_DUP" },
		{ DB_AM_DUPSORT,		"DB_AM_DUPSORT" },
		{ DB_AM_ENCRYPT,		"DB_AM_ENCRYPT" },
		{ DB_AM_FIXEDLEN,		"DB_AM_FIXEDLEN" },
		{ DB_AM_INMEM,			"DB_AM_INMEM" },
		{ DB_AM_IN_RENAME,		"DB_AM_IN_RENAME" },
		{ DB_AM_NOT_DURABLE,		"DB_AM_NOT_DURABLE" },
		{ DB_AM_OPEN_CALLED,		"DB_AM_OPEN_CALLED" },
		{ DB_AM_PAD,			"DB_AM_PAD" },
		{ DB_AM_PGDEF,			"DB_AM_PGDEF" },
		{ DB_AM_RDONLY,			"DB_AM_RDONLY" },
		{ DB_AM_READ_UNCOMMITTED,	"DB_AM_READ_UNCOMMITTED" },
		{ DB_AM_RECNUM,			"DB_AM_RECNUM" },
		{ DB_AM_RECOVER,		"DB_AM_RECOVER" },
		{ DB_AM_RENUMBER,		"DB_AM_RENUMBER" },
		{ DB_AM_REVSPLITOFF,		"DB_AM_REVSPLITOFF" },
		{ DB_AM_SECONDARY,		"DB_AM_SECONDARY" },
		{ DB_AM_SNAPSHOT,		"DB_AM_SNAPSHOT" },
		{ DB_AM_SUBDB,			"DB_AM_SUBDB" },
		{ DB_AM_SWAP,			"DB_AM_SWAP" },
		{ DB_AM_TXN,			"DB_AM_TXN" },
		{ DB_AM_VERIFYING,		"DB_AM_VERIFYING" },
		{ 0,				NULL }
	};
	ENV *env;
	char time_buf[CTIME_BUFLEN];

	env = dbp->env;

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB handle information:");
	STAT_ULONG("Page size", dbp->pgsize);
	STAT_ISSET("Append recno", dbp->db_append_recno);
	STAT_ISSET("Feedback", dbp->db_feedback);
	STAT_ISSET("Dup compare", dbp->dup_compare);
	STAT_ISSET("App private", dbp->app_private);
	STAT_ISSET("DbEnv", dbp->env);
	STAT_STRING("Type", __db_dbtype_to_string(dbp->type));

	__mutex_print_debug_single(env, "Thread mutex", dbp->mutex, flags);

	STAT_STRING("File", dbp->fname);
	STAT_STRING("Database", dbp->dname);
	STAT_HEX("Open flags", dbp->open_flags);

	__db_print_fileid(env, dbp->fileid, "\tFile ID");

	STAT_ULONG("Cursor adjust ID", dbp->adj_fileid);
	STAT_ULONG("Meta pgno", dbp->meta_pgno);
	if (dbp->locker != NULL)
		STAT_ULONG("Locker ID", dbp->locker->id);
	if (dbp->cur_locker != NULL)
		STAT_ULONG("Handle lock", dbp->cur_locker->id);
	if (dbp->associate_locker != NULL)
		STAT_ULONG("Associate lock", dbp->associate_locker->id);
	STAT_ULONG("RPC remote ID", dbp->cl_id);

	__db_msg(env,
	    "%.24s\tReplication handle timestamp",
	    dbp->timestamp == 0 ? "0" : __os_ctime(&dbp->timestamp, time_buf));

	STAT_ISSET("Secondary callback", dbp->s_callback);
	STAT_ISSET("Primary handle", dbp->s_primary);

	STAT_ISSET("api internal", dbp->api_internal);
	STAT_ISSET("Btree/Recno internal", dbp->bt_internal);
	STAT_ISSET("Hash internal", dbp->h_internal);
	STAT_ISSET("Queue internal", dbp->q_internal);

	__db_prflags(env, NULL, dbp->flags, fn, NULL, "\tFlags");

	if (dbp->log_filename == NULL)
		STAT_ISSET("File naming information", dbp->log_filename);
	else
		__dbreg_print_fname(env, dbp->log_filename);

	(void)__db_print_cursor(dbp);

	return (0);
}

/*
 * __db_print_cursor --
 *	Display the cursor active and free queues.
 */
static int
__db_print_cursor(dbp)
	DB *dbp;
{
	DBC *dbc;
	ENV *env;
	int ret, t_ret;

	env = dbp->env;

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB handle cursors:");

	ret = 0;
	MUTEX_LOCK(dbp->env, dbp->mutex);
	__db_msg(env, "Active queue:");
	TAILQ_FOREACH(dbc, &dbp->active_queue, links)
		if ((t_ret = __db_print_citem(dbc)) != 0 && ret == 0)
			ret = t_ret;
	__db_msg(env, "Join queue:");
	TAILQ_FOREACH(dbc, &dbp->join_queue, links)
		if ((t_ret = __db_print_citem(dbc)) != 0 && ret == 0)
			ret = t_ret;
	__db_msg(env, "Free queue:");
	TAILQ_FOREACH(dbc, &dbp->free_queue, links)
		if ((t_ret = __db_print_citem(dbc)) != 0 && ret == 0)
			ret = t_ret;
	MUTEX_UNLOCK(dbp->env, dbp->mutex);

	return (ret);
}

static int
__db_print_citem(dbc)
	DBC *dbc;
{
	static const FN fn[] = {
		{ DBC_ACTIVE,		"DBC_ACTIVE" },
		{ DBC_DONTLOCK,		"DBC_DONTLOCK" },
		{ DBC_MULTIPLE,		"DBC_MULTIPLE" },
		{ DBC_MULTIPLE_KEY,	"DBC_MULTIPLE_KEY" },
		{ DBC_OPD,		"DBC_OPD" },
		{ DBC_OWN_LID,		"DBC_OWN_LID" },
		{ DBC_READ_COMMITTED,	"DBC_READ_COMMITTED" },
		{ DBC_READ_UNCOMMITTED,	"DBC_READ_UNCOMMITTED" },
		{ DBC_RECOVER,		"DBC_RECOVER" },
		{ DBC_RMW,		"DBC_RMW" },
		{ DBC_TRANSIENT,	"DBC_TRANSIENT" },
		{ DBC_WAS_READ_COMMITTED,"DBC_WAS_READ_COMMITTED" },
		{ DBC_WRITECURSOR,	"DBC_WRITECURSOR" },
		{ DBC_WRITER,		"DBC_WRITER" },
		{ 0,			NULL }
	};
	DB *dbp;
	DBC_INTERNAL *cp;
	ENV *env;

	dbp = dbc->dbp;
	env = dbp->env;
	cp = dbc->internal;

	STAT_POINTER("DBC", dbc);
	STAT_POINTER("Associated dbp", dbc->dbp);
	STAT_POINTER("Associated txn", dbc->txn);
	STAT_POINTER("Internal", cp);
	STAT_HEX("Default locker ID", dbc->lref == NULL ? 0 : dbc->lref->id);
	STAT_HEX("Locker", P_TO_ULONG(dbc->locker));
	STAT_STRING("Type", __db_dbtype_to_string(dbc->dbtype));

	STAT_POINTER("Off-page duplicate cursor", cp->opd);
	STAT_POINTER("Referenced page", cp->page);
	STAT_ULONG("Root", cp->root);
	STAT_ULONG("Page number", cp->pgno);
	STAT_ULONG("Page index", cp->indx);
	STAT_STRING("Lock mode", __db_lockmode_to_string(cp->lock_mode));
	__db_prflags(env, NULL, dbc->flags, fn, NULL, "\tFlags");

	switch (dbc->dbtype) {
	case DB_BTREE:
	case DB_RECNO:
		__bam_print_cursor(dbc);
		break;
	case DB_HASH:
		__ham_print_cursor(dbc);
		break;
	case DB_UNKNOWN:
		DB_ASSERT(env, dbp->type != DB_UNKNOWN);
		/* FALLTHROUGH */
	case DB_QUEUE:
	default:
		break;
	}
	return (0);
}

#else /* !HAVE_STATISTICS */

int
__db_stat_pp(dbp, txn, spp, flags)
	DB *dbp;
	DB_TXN *txn;
	void *spp;
	u_int32_t flags;
{
	COMPQUIET(spp, NULL);
	COMPQUIET(txn, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbp->env));
}

int
__db_stat_print_pp(dbp, flags)
	DB *dbp;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbp->env));
}
#endif
