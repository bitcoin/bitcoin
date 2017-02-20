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
#include "dbinc/fop.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __dbreg_check_master __P((ENV *, u_int8_t *, char *));

/*
 * __dbreg_add_dbentry --
 *	Adds a DB entry to the dbreg DB entry table.
 *
 * PUBLIC: int __dbreg_add_dbentry __P((ENV *, DB_LOG *, DB *, int32_t));
 */
int
__dbreg_add_dbentry(env, dblp, dbp, ndx)
	ENV *env;
	DB_LOG *dblp;
	DB *dbp;
	int32_t ndx;
{
	int32_t i;
	int ret;

	ret = 0;

	MUTEX_LOCK(env, dblp->mtx_dbreg);

	/*
	 * Check if we need to grow the table.  Note, ndx is 0-based (the
	 * index into the DB entry table) an dbentry_cnt is 1-based, the
	 * number of available slots.
	 */
	if (dblp->dbentry_cnt <= ndx) {
		if ((ret = __os_realloc(env,
		    (size_t)(ndx + DB_GROW_SIZE) * sizeof(DB_ENTRY),
		    &dblp->dbentry)) != 0)
			goto err;

		/* Initialize the new entries. */
		for (i = dblp->dbentry_cnt; i < ndx + DB_GROW_SIZE; i++) {
			dblp->dbentry[i].dbp = NULL;
			dblp->dbentry[i].deleted = 0;
		}
		dblp->dbentry_cnt = i;
	}

	DB_ASSERT(env, dblp->dbentry[ndx].dbp == NULL);
	dblp->dbentry[ndx].deleted = dbp == NULL;
	dblp->dbentry[ndx].dbp = dbp;

err:	MUTEX_UNLOCK(env, dblp->mtx_dbreg);
	return (ret);
}

/*
 * __dbreg_rem_dbentry
 *	Remove an entry from the DB entry table.
 *
 * PUBLIC: int __dbreg_rem_dbentry __P((DB_LOG *, int32_t));
 */
int
__dbreg_rem_dbentry(dblp, ndx)
	DB_LOG *dblp;
	int32_t ndx;
{
	MUTEX_LOCK(dblp->env, dblp->mtx_dbreg);
	if (dblp->dbentry_cnt > ndx) {
		dblp->dbentry[ndx].dbp = NULL;
		dblp->dbentry[ndx].deleted = 0;
	}
	MUTEX_UNLOCK(dblp->env, dblp->mtx_dbreg);

	return (0);
}

/*
 * __dbreg_log_files --
 *	Put a DBREG_CHKPNT/CLOSE log record for each open database.
 *
 * PUBLIC: int __dbreg_log_files __P((ENV *, u_int32_t));
 */
int
__dbreg_log_files(env, opcode)
	ENV *env;
	u_int32_t opcode;
{
	DBT *dbtp, fid_dbt, t;
	DB_LOG *dblp;
	DB_LSN r_unused;
	FNAME *fnp;
	LOG *lp;
	int ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	ret = 0;

	MUTEX_LOCK(env, lp->mtx_filelist);

	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname) {
		/* This id was revoked by a switch in replication master. */
		if (fnp->id == DB_LOGFILEID_INVALID)
			continue;
		if (fnp->fname_off == INVALID_ROFF)
			dbtp = NULL;
		else {
			memset(&t, 0, sizeof(t));
			t.data = R_ADDR(&dblp->reginfo, fnp->fname_off);
			t.size = (u_int32_t)strlen(t.data) + 1;
			dbtp = &t;
		}
		memset(&fid_dbt, 0, sizeof(fid_dbt));
		fid_dbt.data = fnp->ufid;
		fid_dbt.size = DB_FILE_ID_LEN;
		/*
		 * Output DBREG_CHKPNT records which will be processed during
		 * the OPENFILES pass of recovery.  At the end of recovery we
		 * want to output the files that were open so a future recovery
		 * run will have the correct files open during a backward pass.
		 * For this we output DBREG_RCLOSE records so the files will be
		 * closed on the forward pass.
		 */
		if ((ret = __dbreg_register_log(env, NULL, &r_unused,
		    F_ISSET(fnp, DB_FNAME_DURABLE) ? 0 : DB_LOG_NOT_DURABLE,
		    opcode,
		    dbtp, &fid_dbt, fnp->id, fnp->s_type, fnp->meta_pgno,
		    TXN_INVALID)) != 0)
			break;
	}

	MUTEX_UNLOCK(env, lp->mtx_filelist);

	return (ret);
}

/*
 * __dbreg_close_files --
 *	Remove the id's of open files and actually close those
 *	files that were opened by the recovery daemon.  We sync the
 *	file, unless its mpf pointer has been NULLed by a db_remove or
 *	db_rename.  We may not have flushed the log_register record that
 *	closes the file.
 *
 * PUBLIC: int __dbreg_close_files __P((ENV *, int));
 */
int
__dbreg_close_files(env, do_restored)
	ENV *env;
	int do_restored;
{
	DB *dbp;
	DB_LOG *dblp;
	int ret, t_ret;
	int32_t i;

	/* If we haven't initialized logging, we have nothing to do. */
	if (!LOGGING_ON(env))
		return (0);

	dblp = env->lg_handle;
	ret = 0;

	MUTEX_LOCK(env, dblp->mtx_dbreg);
	for (i = 0; i < dblp->dbentry_cnt; i++) {
		/*
		 * We only want to close dbps that recovery opened.  Any
		 * dbps that weren't opened by recovery but show up here
		 * are about to be unconditionally removed from the table.
		 * Before doing so, we need to revoke their log fileids
		 * so that we don't end up leaving around FNAME entries
		 * for dbps that shouldn't have them.
		 */
		if ((dbp = dblp->dbentry[i].dbp) != NULL) {
			/*
			 * It's unsafe to call DB->close or revoke_id
			 * while holding the thread lock, because
			 * we'll call __dbreg_rem_dbentry and grab it again.
			 *
			 * Just drop it.  Since dbreg ids go monotonically
			 * upward, concurrent opens should be safe, and the
			 * user should have no business closing files while
			 * we're in this loop anyway--we're in the process of
			 * making all outstanding dbps invalid.
			 */
			/*
			 * If we only want to close those FNAMES marked
			 * as restored, check now.
			 */
			if (do_restored &&
			    !F_ISSET(dbp->log_filename, DB_FNAME_RESTORED))
				continue;
			MUTEX_UNLOCK(env, dblp->mtx_dbreg);
			if (F_ISSET(dbp, DB_AM_RECOVER))
				t_ret = __db_close(dbp,
				    NULL, dbp->mpf == NULL ? DB_NOSYNC : 0);
			else
				t_ret = __dbreg_revoke_id(
				     dbp, 0, DB_LOGFILEID_INVALID);
			if (ret == 0)
				ret = t_ret;
			MUTEX_LOCK(env, dblp->mtx_dbreg);
		}

		dblp->dbentry[i].deleted = 0;
		dblp->dbentry[i].dbp = NULL;
	}
	MUTEX_UNLOCK(env, dblp->mtx_dbreg);
	return (ret);
}

/*
 * __dbreg_close_file --
 *	Close a database file opened by recovery.
 * PUBLIC: int __dbreg_close_file __P((ENV *, FNAME *));
 */
int
__dbreg_close_file(env, fnp)
	ENV *env;
	FNAME *fnp;
{
	DB *dbp;
	DB_LOG *dblp;

	dblp = env->lg_handle;

	dbp = dblp->dbentry[fnp->id].dbp;
	if (dbp == NULL)
		return (0);
	DB_ASSERT(env, dbp->log_filename == fnp);
	DB_ASSERT(env, F_ISSET(dbp, DB_AM_RECOVER));
	return (__db_close(dbp, NULL, DB_NOSYNC));
}

/*
 * __dbreg_mark_restored --
 *	Mark files when we change replication roles and there are outstanding
 * prepared txns that may use these files.  These will be invalidated later
 * when all outstanding prepared txns are resolved.
 *
 * PUBLIC: int __dbreg_mark_restored __P((ENV *));
 */
int
__dbreg_mark_restored(env)
	ENV *env;
{
	DB_LOG *dblp;
	FNAME *fnp;
	LOG *lp;

	/* If we haven't initialized logging, we have nothing to do. */
	if (!LOGGING_ON(env))
		return (0);

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	MUTEX_LOCK(env, lp->mtx_filelist);
	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname)
		if (fnp->id != DB_LOGFILEID_INVALID)
			F_SET(fnp, DB_FNAME_RESTORED);

	MUTEX_UNLOCK(env, lp->mtx_filelist);
	return (0);
}

/*
 * __dbreg_invalidate_files --
 *	Invalidate files when we change replication roles.  Save the
 * id so that another process will be able to clean up the information
 * when it notices.
 *
 * PUBLIC: int __dbreg_invalidate_files __P((ENV *, int));
 */
int
__dbreg_invalidate_files(env, do_restored)
	ENV *env;
	int do_restored;
{
	DB_LOG *dblp;
	FNAME *fnp;
	LOG *lp;
	int ret;

	/* If we haven't initialized logging, we have nothing to do. */
	if (!LOGGING_ON(env))
		return (0);

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	ret = 0;
	MUTEX_LOCK(env, lp->mtx_filelist);
	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname) {
		/*
		 * Normally, skip any file with DB_FNAME_RESTORED
		 * set.  If do_restored is set, only invalidate
		 * those files with the flag set and skip all others.
		 */
		if (F_ISSET(fnp, DB_FNAME_RESTORED) && !do_restored)
			continue;
		if (!F_ISSET(fnp, DB_FNAME_RESTORED) && do_restored)
			continue;
		if (fnp->id != DB_LOGFILEID_INVALID) {
			if ((ret = __dbreg_log_close(env,
			    fnp, NULL, DBREG_RCLOSE)) != 0)
				goto err;
			fnp->old_id = fnp->id;
			fnp->id = DB_LOGFILEID_INVALID;
		}
	}
err:	MUTEX_UNLOCK(env, lp->mtx_filelist);
	return (ret);
}

/*
 * __dbreg_id_to_db --
 *	Return the DB corresponding to the specified dbreg id.
 *
 * PUBLIC: int __dbreg_id_to_db __P((ENV *, DB_TXN *, DB **, int32_t, int));
 */
int
__dbreg_id_to_db(env, txn, dbpp, ndx, tryopen)
	ENV *env;
	DB_TXN *txn;
	DB **dbpp;
	int32_t ndx;
	int tryopen;
{
	DB_LOG *dblp;
	FNAME *fname;
	int ret;
	char *name;

	dblp = env->lg_handle;
	ret = 0;

	MUTEX_LOCK(env, dblp->mtx_dbreg);

	/*
	 * We take a final parameter that indicates whether we should attempt
	 * to open the file if no mapping is found.  During recovery, the
	 * recovery routines all want to try to open the file (and this is
	 * called from __dbreg_id_to_db), however, if we have a multi-process
	 * environment where some processes may not have the files open,
	 * then we also get called from __dbreg_assign_id and it's OK if
	 * there is no mapping.
	 *
	 * Under failchk, a process different than the one issuing DB
	 * operations may abort a transaction.  In this case, the "recovery"
	 * routines are run by a process that does not necessarily have the
	 * file open, so we we must open the file explicitly.
	 */
	if (ndx >= dblp->dbentry_cnt ||
	    (!dblp->dbentry[ndx].deleted && dblp->dbentry[ndx].dbp == NULL)) {
		if (!tryopen || F_ISSET(dblp, DBLOG_RECOVER)) {
			ret = ENOENT;
			goto err;
		}

		/*
		 * __dbreg_id_to_fname acquires the mtx_filelist mutex, which
		 * we can't safely acquire while we hold the thread lock.  We
		 * no longer need it anyway--the dbentry table didn't have what
		 * we needed.
		 */
		MUTEX_UNLOCK(env, dblp->mtx_dbreg);

		if (__dbreg_id_to_fname(dblp, ndx, 0, &fname) != 0)
			/*
			 * With transactional opens, we may actually have
			 * closed this file in the transaction in which
			 * case this will fail too.  Then it's up to the
			 * caller to reopen the file.
			 */
			return (ENOENT);

		/*
		 * Note that we're relying on fname not to change, even though
		 * we released the mutex that protects it (mtx_filelist) inside
		 * __dbreg_id_to_fname.  This should be a safe assumption, the
		 * other process that has the file open shouldn't be closing it
		 * while we're trying to abort.
		 */
		name = fname->fname_off == INVALID_ROFF ?
		    NULL : R_ADDR(&dblp->reginfo, fname->fname_off);

		/*
		 * At this point, we are not holding the thread lock, so exit
		 * directly instead of going through the exit code at the
		 * bottom.  If the __dbreg_do_open succeeded, then we don't need
		 * to do any of the remaining error checking at the end of this
		 * routine.
		 * If TXN_INVALID is passed then no txnlist is needed.
		 */
		if ((ret = __dbreg_do_open(env, txn, dblp,
		    fname->ufid, name, fname->s_type, ndx, fname->meta_pgno,
		    NULL, TXN_INVALID, F_ISSET(fname, DB_FNAME_INMEM) ?
		    DBREG_REOPEN : DBREG_OPEN)) != 0)
			return (ret);

		*dbpp = dblp->dbentry[ndx].dbp;
		return (*dbpp == NULL ? DB_DELETED : 0);
	}

	/*
	 * Return DB_DELETED if the file has been deleted (it's not an error).
	 */
	if (dblp->dbentry[ndx].deleted) {
		ret = DB_DELETED;
		goto err;
	}

	/* It's an error if we don't have a corresponding writeable DB. */
	if ((*dbpp = dblp->dbentry[ndx].dbp) == NULL)
		ret = ENOENT;
	else
		/*
		 * If we are in recovery, then set that the file has
		 * been written.  It is possible to run recovery,
		 * find all the pages in their post update state
		 * in the OS buffer pool, put a checkpoint in the log
		 * and then crash the system without forcing the pages
		 * to disk. If this is an in-memory file, we may not have
		 * an mpf yet.
		 */
		if ((*dbpp)->mpf != NULL && (*dbpp)->mpf->mfp != NULL)
			(*dbpp)->mpf->mfp->file_written = 1;

err:	MUTEX_UNLOCK(env, dblp->mtx_dbreg);
	return (ret);
}

/*
 * __dbreg_id_to_fname --
 *	Traverse the shared-memory region looking for the entry that
 *	matches the passed dbreg id.  Returns 0 on success; -1 on error.
 *
 * PUBLIC: int __dbreg_id_to_fname __P((DB_LOG *, int32_t, int, FNAME **));
 */
int
__dbreg_id_to_fname(dblp, id, have_lock, fnamep)
	DB_LOG *dblp;
	int32_t id;
	int have_lock;
	FNAME **fnamep;
{
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	int ret;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	ret = -1;

	if (!have_lock)
		MUTEX_LOCK(env, lp->mtx_filelist);
	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname)
		if (fnp->id == id) {
			*fnamep = fnp;
			ret = 0;
			break;
		}
	if (!have_lock)
		MUTEX_UNLOCK(env, lp->mtx_filelist);

	return (ret);
}
/*
 * __dbreg_fid_to_fname --
 *	Traverse the shared-memory region looking for the entry that
 *	matches the passed file unique id.  Returns 0 on success; -1 on error.
 *
 * PUBLIC: int __dbreg_fid_to_fname __P((DB_LOG *, u_int8_t *, int, FNAME **));
 */
int
__dbreg_fid_to_fname(dblp, fid, have_lock, fnamep)
	DB_LOG *dblp;
	u_int8_t *fid;
	int have_lock;
	FNAME **fnamep;
{
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	int ret;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	ret = -1;

	if (!have_lock)
		MUTEX_LOCK(env, lp->mtx_filelist);
	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname)
		if (memcmp(fnp->ufid, fid, DB_FILE_ID_LEN) == 0) {
			*fnamep = fnp;
			ret = 0;
			break;
		}
	if (!have_lock)
		MUTEX_UNLOCK(env, lp->mtx_filelist);

	return (ret);
}

/*
 * __dbreg_get_name
 *
 * Interface to get name of registered files.  This is mainly diagnostic
 * and the name passed could be transient unless there is something
 * ensuring that the file cannot be closed.
 *
 * PUBLIC: int __dbreg_get_name __P((ENV *, u_int8_t *, char **, char **));
 */
int
__dbreg_get_name(env, fid, fnamep, dnamep)
	ENV *env;
	u_int8_t *fid;
	char **fnamep, **dnamep;
{
	DB_LOG *dblp;
	FNAME *fnp;

	dblp = env->lg_handle;

	if (dblp != NULL && __dbreg_fid_to_fname(dblp, fid, 0, &fnp) == 0) {
		*fnamep = fnp->fname_off == INVALID_ROFF ?
		    NULL : R_ADDR(&dblp->reginfo, fnp->fname_off);
		*dnamep = fnp->dname_off == INVALID_ROFF ?
		    NULL : R_ADDR(&dblp->reginfo, fnp->dname_off);
		return (0);
	}

	*fnamep = *dnamep = NULL;
	return (-1);
}

/*
 * __dbreg_do_open --
 *	Open files referenced in the log.  This is the part of the open that
 * is not protected by the thread mutex.
 * PUBLIC: int __dbreg_do_open __P((ENV *,
 * PUBLIC:     DB_TXN *, DB_LOG *, u_int8_t *, char *, DBTYPE,
 * PUBLIC:     int32_t, db_pgno_t, void *, u_int32_t, u_int32_t));
 */
int
__dbreg_do_open(env,
    txn, lp, uid, name, ftype, ndx, meta_pgno, info, id, opcode)
	ENV *env;
	DB_TXN *txn;
	DB_LOG *lp;
	u_int8_t *uid;
	char *name;
	DBTYPE ftype;
	int32_t ndx;
	db_pgno_t meta_pgno;
	void *info;
	u_int32_t id, opcode;
{
	DB *dbp;
	u_int32_t cstat, ret_stat;
	int ret, t_ret, try_inmem;
	char *dname, *fname;

	cstat = TXN_EXPECTED;
	fname = name;
	dname = NULL;
	try_inmem = 0;

retry_inmem:
	if ((ret = __db_create_internal(&dbp, lp->env, 0)) != 0)
		return (ret);

	/*
	 * We can open files under a number of different scenarios.
	 * First, we can open a file during a normal txn_abort, if that file
	 * was opened and closed during the transaction (as is the master
	 * database of a sub-database).
	 * Second, we might be aborting a transaction in a process other than
	 * the one that did it (failchk).
	 * Third, we might be in recovery.
	 * In case 3, there is no locking, so there is no issue.
	 * In cases 1 and 2, we are guaranteed to already hold any locks
	 * that we need, since we're still in the same transaction, so by
	 * setting DB_AM_RECOVER, we guarantee that we don't log and that
	 * we don't try to acquire locks on behalf of a different locker id.
	 */
	F_SET(dbp, DB_AM_RECOVER);
	if (meta_pgno != PGNO_BASE_MD) {
		memcpy(dbp->fileid, uid, DB_FILE_ID_LEN);
		dbp->meta_pgno = meta_pgno;
	}
	if (opcode == DBREG_PREOPEN) {
		dbp->type = ftype;
		if ((ret = __dbreg_setup(dbp, name, NULL, id)) != 0)
			goto err;
		MAKE_INMEM(dbp);
		goto skip_open;
	}

	if (opcode == DBREG_REOPEN || try_inmem) {
		MAKE_INMEM(dbp);
		fname = NULL;
		dname = name;
	}

	if ((ret = __db_open(dbp, NULL, txn, fname, dname, ftype,
	    DB_DURABLE_UNKNOWN | DB_ODDFILESIZE,
	    DB_MODE_600, meta_pgno)) == 0) {
skip_open:
		/*
		 * Verify that we are opening the same file that we were
		 * referring to when we wrote this log record.
		 */
		if ((meta_pgno != PGNO_BASE_MD &&
		    __dbreg_check_master(env, uid, name) != 0) ||
		    memcmp(uid, dbp->fileid, DB_FILE_ID_LEN) != 0)
			cstat = TXN_UNEXPECTED;
		else
			cstat = TXN_EXPECTED;

		/* Assign the specific dbreg id to this dbp. */
		if ((ret = __dbreg_assign_id(dbp, ndx, 0)) != 0)
			goto err;

		/*
		 * If we successfully opened this file, then we need to
		 * convey that information to the txnlist so that we
		 * know how to handle the subtransaction that created
		 * the file system object.
		 */
		if (id != TXN_INVALID)
			ret = __db_txnlist_update(env,
			    info, id, cstat, NULL, &ret_stat, 1);

err:		if (cstat == TXN_UNEXPECTED)
			goto not_right;
		return (ret);
	} else if (ret == ENOENT) {
		/*
		 * If the open failed with ENOENT, retry it as a named in-mem
		 * database.  Some record types do not distinguish between a
		 * named in-memory database and one on-disk.  Therefore, an
		 * internal init via replication that is trying to open and
		 * access this as a named in-mem database will not find it
		 * on-disk, and we need to try to open it in-memory too.
		 *     But don't do this for [P]REOPEN, since we're already
		 * handling those cases specially, above.
		 */
		if (try_inmem == 0 &&
		    opcode != DBREG_PREOPEN && opcode != DBREG_REOPEN) {
			if ((ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0)
				return (ret);
			try_inmem = 1;
			goto retry_inmem;
		} else if (try_inmem != 0)
			CLR_INMEM(dbp);

		/*
		 * If it exists neither on disk nor in memory
		 * record that the open failed in the txnlist.
		 */
		if (id != TXN_INVALID && (ret = __db_txnlist_update(env,
		    info, id, TXN_UNEXPECTED, NULL, &ret_stat, 1)) != 0)
			goto not_right;
		
		/*
		 * If this is file is missing then we may have crashed
		 * without writing the corresponding close, record
		 * the open so recovery will write a close record
		 * with its checkpoint.
		 */
		if ((opcode == DBREG_CHKPNT || opcode == DBREG_OPEN) &&
		    dbp->log_filename == NULL &&
		    (ret = __dbreg_setup(dbp, name, NULL, id)) != 0)
			return (ret);
		ret = __dbreg_assign_id(dbp, ndx, 1);
		return (ret);
	}
not_right:
	if ((t_ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0)
		return (ret == 0 ? t_ret : ret);

	/* Add this file as deleted. */
	if ((t_ret = __dbreg_add_dbentry(env, lp, NULL, ndx)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

static int
__dbreg_check_master(env, uid, name)
	ENV *env;
	u_int8_t *uid;
	char *name;
{
	DB *dbp;
	int ret;

	ret = 0;
	if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
		return (ret);
	F_SET(dbp, DB_AM_RECOVER);
	ret = __db_open(dbp, NULL, NULL,
	    name, NULL, DB_BTREE, 0, DB_MODE_600, PGNO_BASE_MD);

	if (ret == 0 && memcmp(uid, dbp->fileid, DB_FILE_ID_LEN) != 0)
		ret = EINVAL;

	(void)__db_close(dbp, NULL, 0);
	return (ret);
}

/*
 * __dbreg_lazy_id --
 *	When a replication client gets upgraded to being a replication master,
 * it may have database handles open that have not been assigned an ID, but
 * which have become legal to use for logging.
 *
 *	This function lazily allocates a new ID for such a function, in a
 * new transaction created for the purpose.  We need to do this in a new
 * transaction because we definitely wish to commit the dbreg_register, but
 * at this point we have no way of knowing whether the log record that incited
 * us to call this will be part of a committed transaction.
 *
 *	We first revoke any old id this handle may have had.  That can happen
 * if a master becomes a client and then becomes a master again and
 * there are other processes with valid open handles to this env.
 *
 * PUBLIC: int __dbreg_lazy_id __P((DB *));
 */
int
__dbreg_lazy_id(dbp)
	DB *dbp;
{
	DB_LOG *dblp;
	DB_TXN *txn;
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	int32_t id;
	int ret;

	env = dbp->env;

	DB_ASSERT(env, IS_REP_MASTER(env));

	env = dbp->env;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	fnp = dbp->log_filename;

	/* The mtx_filelist protects the FNAME list and id management. */
	MUTEX_LOCK(env, lp->mtx_filelist);
	if (fnp->id != DB_LOGFILEID_INVALID) {
		MUTEX_UNLOCK(env, lp->mtx_filelist);
		return (0);
	}
	id = DB_LOGFILEID_INVALID;
	/*
	 * When we became master we moved the fnp->id to old_id in
	 * every FNAME structure that was open.  If our id was changed,
	 * we need to revoke and give back that id.
	 */
	if (fnp->old_id != DB_LOGFILEID_INVALID &&
	    (ret = __dbreg_revoke_id(dbp, 1, DB_LOGFILEID_INVALID)) != 0)
		goto err;
	if ((ret = __txn_begin(env, NULL, NULL, &txn, 0)) != 0)
		goto err;

	if ((ret = __dbreg_get_id(dbp, txn, &id)) != 0) {
		(void)__txn_abort(txn);
		goto err;
	}

	if ((ret = __txn_commit(txn, DB_TXN_NOSYNC)) != 0)
		goto err;

	/*
	 * All DB related logging routines check the id value *without*
	 * holding the mtx_filelist to know whether we need to call
	 * dbreg_lazy_id to begin with.  We must set the ID after a
	 * *successful* commit so that there is no possibility of a second
	 * modification call finding a valid ID in the dbp before the
	 * dbreg_register and commit records are in the log.
	 * If there was an error, then we call __dbreg_revoke_id to
	 * remove the entry from the lists.
	 */
	fnp->id = id;
err:
	if (ret != 0 && id != DB_LOGFILEID_INVALID)
		(void)__dbreg_revoke_id(dbp, 1, id);
	MUTEX_UNLOCK(env, lp->mtx_filelist);
	return (ret);
}
