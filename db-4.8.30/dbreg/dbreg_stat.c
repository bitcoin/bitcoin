/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

#ifdef HAVE_STATISTICS
static int __dbreg_print_all __P((ENV *, u_int32_t));

/*
 * __dbreg_stat_print --
 *	Print the dbreg statistics.
 *
 * PUBLIC: int __dbreg_stat_print __P((ENV *, u_int32_t));
 */
int
__dbreg_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	int ret;

	if (LF_ISSET(DB_STAT_ALL) &&
	    (ret = __dbreg_print_all(env, flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __dbreg_print_fname --
 *	Display the contents of an FNAME structure.
 *
 * PUBLIC: void __dbreg_print_fname __P((ENV *, FNAME *));
 */
void
__dbreg_print_fname(env, fnp)
	ENV *env;
	FNAME *fnp;
{
	static const FN fn[] = {
		{ DB_FNAME_DURABLE,	"DB_FNAME_DURABLE" },
		{ DB_FNAME_NOTLOGGED,	"DB_FNAME_NOTLOGGED" },
		{ 0,			NULL }
	};

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB handle FNAME contents:");
	STAT_LONG("log ID", fnp->id);
	STAT_ULONG("Meta pgno", fnp->meta_pgno);
	__db_print_fileid(env, fnp->ufid, "\tFile ID");
	STAT_ULONG("create txn", fnp->create_txnid);
	__db_prflags(env, NULL, fnp->flags, fn, NULL, "\tFlags");
}

/*
 * __dbreg_print_all --
 *	Display the ENV's list of files.
 */
static int
__dbreg_print_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	DB *dbp;
	DB_LOG *dblp;
	FNAME *fnp;
	LOG *lp;
	int32_t *stack;
	int del, first;
	u_int32_t i;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	__db_msg(env, "LOG FNAME list:");
	__mutex_print_debug_single(
	    env, "File name mutex", lp->mtx_filelist, flags);

	STAT_LONG("Fid max", lp->fid_max);
	STAT_LONG("Log buffer size", lp->buffer_size);

	MUTEX_LOCK(env, lp->mtx_filelist);
	first = 1;
	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname) {
		if (first) {
			first = 0;
			__db_msg(env,
		    "ID\tName\t\tType\tPgno\tPid\tTxnid\tFlags\tDBP-info");
		}
		dbp = fnp->id >= dblp->dbentry_cnt ? NULL :
		    dblp->dbentry[fnp->id].dbp;
		del = fnp->id >= dblp->dbentry_cnt ? 0 :
		    dblp->dbentry[fnp->id].deleted;
		__db_msg(env,
		    "%ld\t%-8s%s%-8s%s\t%lu\t%lu\t%lx\t%lx\t%s (%d %lx %lx)",
		    (long)fnp->id,
		    fnp->fname_off == INVALID_ROFF ?
			"" : (char *)R_ADDR(&dblp->reginfo, fnp->fname_off),
		    fnp->dname_off == INVALID_ROFF ? "" : ":",
		    fnp->dname_off == INVALID_ROFF ?
			"" : (char *)R_ADDR(&dblp->reginfo, fnp->dname_off),
		    __db_dbtype_to_string(fnp->s_type),
		    (u_long)fnp->meta_pgno, (u_long)fnp->pid,
		    (u_long)fnp->create_txnid, (u_long)fnp->flags,
		    dbp == NULL ? "No DBP" : "DBP", del, P_TO_ULONG(dbp),
		    (u_long)(dbp == NULL ? 0 : dbp->flags));
	}
	MUTEX_UNLOCK(env, lp->mtx_filelist);

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "LOG region list of free IDs.");
	if (lp->free_fid_stack == INVALID_ROFF)
		__db_msg(env, "Free id stack is empty.");
	else {
		STAT_ULONG("Free id array size", lp->free_fids_alloced);
		STAT_ULONG("Number of ids on the free stack", lp->free_fids);
		stack = R_ADDR(&dblp->reginfo, lp->free_fid_stack);
		for (i = 0; i < lp->free_fids; i++)
			STAT_LONG("fid", stack[i]);
	}

	return (0);
}
#endif
