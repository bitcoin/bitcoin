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
#include "dbinc/log.h"
#include "dbinc/txn.h"
#include "dbinc/db_am.h"

static int __dbreg_push_id __P((ENV *, int32_t));
static int __dbreg_pop_id __P((ENV *, int32_t *));
static int __dbreg_pluck_id __P((ENV *, int32_t));

/*
 * The dbreg subsystem, as its name implies, registers database handles so
 * that we can associate log messages with them without logging a filename
 * or a full, unique DB ID.  Instead, we assign each dbp an int32_t which is
 * easy and cheap to log, and use this subsystem to map back and forth.
 *
 * Overview of how dbreg ids are managed:
 *
 * OPEN
 *	dbreg_setup (Creates FNAME struct.)
 *	dbreg_new_id (Assigns new ID to dbp and logs it.  May be postponed
 *	until we attempt to log something else using that dbp, if the dbp
 *	was opened on a replication client.)
 *
 * CLOSE
 *	dbreg_close_id  (Logs closure of dbp/revocation of ID.)
 *	dbreg_revoke_id (As name implies, revokes ID.)
 *	dbreg_teardown (Destroys FNAME.)
 *
 * RECOVERY
 *	dbreg_setup
 *	dbreg_assign_id (Assigns a particular ID we have in the log to a dbp.)
 *
 *	sometimes: dbreg_revoke_id; dbreg_teardown
 *	other times: normal close path
 *
 * A note about locking:
 *
 *	FNAME structures are referenced only by their corresponding dbp's
 *	until they have a valid id.
 *
 *	Once they have a valid id, they must get linked into the log
 *	region list so they can get logged on checkpoints.
 *
 *	An FNAME that may/does have a valid id must be accessed under
 *	protection of the mtx_filelist, with the following exception:
 *
 *	We don't want to have to grab the mtx_filelist on every log
 *	record, and it should be safe not to do so when we're just
 *	looking at the id, because once allocated, the id should
 *	not change under a handle until the handle is closed.
 *
 *	If a handle is closed during an attempt by another thread to
 *	log with it, well, the application doing the close deserves to
 *	go down in flames and a lot else is about to fail anyway.
 *
 *	When in the course of logging we encounter an invalid id
 *	and go to allocate it lazily, we *do* need to check again
 *	after grabbing the mutex, because it's possible to race with
 *	another thread that has also decided that it needs to allocate
 *	a id lazily.
 *
 * See SR #5623 for further discussion of the new dbreg design.
 */

/*
 * __dbreg_setup --
 *	Allocate and initialize an FNAME structure.  The FNAME structures
 * live in the log shared region and map one-to-one with open database handles.
 * When the handle needs to be logged, the FNAME should have a valid fid
 * allocated.  If the handle currently isn't logged, it still has an FNAME
 * entry.  If we later discover that the handle needs to be logged, we can
 * allocate a id for it later.  (This happens when the handle is on a
 * replication client that later becomes a master.)
 *
 * PUBLIC: int __dbreg_setup __P((DB *, const char *, const char *, u_int32_t));
 */
int
__dbreg_setup(dbp, fname, dname, create_txnid)
	DB *dbp;
	const char *fname, *dname;
	u_int32_t create_txnid;
{
	DB_LOG *dblp;
	ENV *env;
	FNAME *fnp;
	REGINFO *infop;
	int ret;
	size_t len;
	void *p;

	env = dbp->env;
	dblp = env->lg_handle;
	infop = &dblp->reginfo;

	fnp = NULL;
	p = NULL;

	/* Allocate an FNAME and, if necessary, a buffer for the name itself. */
	LOG_SYSTEM_LOCK(env);
	if ((ret = __env_alloc(infop, sizeof(FNAME), &fnp)) != 0)
		goto err;
	memset(fnp, 0, sizeof(FNAME));
	if (fname == NULL)
		fnp->fname_off = INVALID_ROFF;
	else {
		len = strlen(fname) + 1;
		if ((ret = __env_alloc(infop, len, &p)) != 0)
			goto err;
		fnp->fname_off = R_OFFSET(infop, p);
		memcpy(p, fname, len);
	}
	if (dname == NULL)
		fnp->dname_off = INVALID_ROFF;
	else {
		len = strlen(dname) + 1;
		if ((ret = __env_alloc(infop, len, &p)) != 0)
			goto err;
		fnp->dname_off = R_OFFSET(infop, p);
		memcpy(p, dname, len);
	}
	LOG_SYSTEM_UNLOCK(env);

	/*
	 * Fill in all the remaining info that we'll need later to register
	 * the file, if we use it for logging.
	 */
	fnp->id = fnp->old_id = DB_LOGFILEID_INVALID;
	fnp->s_type = dbp->type;
	memcpy(fnp->ufid, dbp->fileid, DB_FILE_ID_LEN);
	fnp->meta_pgno = dbp->meta_pgno;
	fnp->create_txnid = create_txnid;
	dbp->dbenv->thread_id(dbp->dbenv, &fnp->pid, NULL);

	if (F_ISSET(dbp, DB_AM_INMEM))
		F_SET(fnp, DB_FNAME_INMEM);
	if (F_ISSET(dbp, DB_AM_RECOVER))
		F_SET(fnp, DB_FNAME_RECOVER);
	fnp->txn_ref = 1;
	fnp->mutex = dbp->mutex;

	dbp->log_filename = fnp;

	return (0);

err:	LOG_SYSTEM_UNLOCK(env);
	if (ret == ENOMEM)
		__db_errx(env,
    "Logging region out of memory; you may need to increase its size");

	return (ret);
}

/*
 * __dbreg_teardown --
 *	Destroy a DB handle's FNAME struct.  This is only called when closing
 * the DB.
 *
 * PUBLIC: int __dbreg_teardown __P((DB *));
 */
int
__dbreg_teardown(dbp)
	DB *dbp;
{
	int ret;

	/*
	 * We may not have an FNAME if we were never opened.  This is not an
	 * error.
	 */
	if (dbp->log_filename == NULL)
		return (0);

	ret = __dbreg_teardown_int(dbp->env, dbp->log_filename);

	/* We freed the copy of the mutex from the FNAME. */
	dbp->log_filename = NULL;
	dbp->mutex = MUTEX_INVALID;

	return (ret);
}

/*
 * __dbreg_teardown_int --
 *	Destroy an FNAME struct.
 *
 * PUBLIC: int __dbreg_teardown_int __P((ENV *, FNAME *));
 */
int
__dbreg_teardown_int(env, fnp)
	ENV *env;
	FNAME *fnp;
{
	DB_LOG *dblp;
	REGINFO *infop;
	int ret;

	if (F_ISSET(fnp, DB_FNAME_NOTLOGGED))
		return (0);
	dblp = env->lg_handle;
	infop = &dblp->reginfo;

	DB_ASSERT(env, fnp->id == DB_LOGFILEID_INVALID);
	ret = __mutex_free(env, &fnp->mutex);

	LOG_SYSTEM_LOCK(env);
	if (fnp->fname_off != INVALID_ROFF)
		__env_alloc_free(infop, R_ADDR(infop, fnp->fname_off));
	if (fnp->dname_off != INVALID_ROFF)
		__env_alloc_free(infop, R_ADDR(infop, fnp->dname_off));
	__env_alloc_free(infop, fnp);
	LOG_SYSTEM_UNLOCK(env);

	return (ret);
}

/*
 * __dbreg_new_id --
 *	Get an unused dbreg id to this database handle.
 *	Used as a wrapper to acquire the mutex and
 *	only set the id on success.
 *
 * PUBLIC: int __dbreg_new_id __P((DB *, DB_TXN *));
 */
int
__dbreg_new_id(dbp, txn)
	DB *dbp;
	DB_TXN *txn;
{
	DB_LOG *dblp;
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	int32_t id;
	int ret;

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
	if ((ret = __dbreg_get_id(dbp, txn, &id)) == 0)
		fnp->id = id;
	MUTEX_UNLOCK(env, lp->mtx_filelist);
	return (ret);
}

/*
 * __dbreg_get_id --
 *	Assign an unused dbreg id to this database handle.
 *	Assume the caller holds the mtx_filelist locked.  Assume the
 *	caller will set the fnp->id field with the id we return.
 *
 * PUBLIC: int __dbreg_get_id __P((DB *, DB_TXN *, int32_t *));
 */
int
__dbreg_get_id(dbp, txn, idp)
	DB *dbp;
	DB_TXN *txn;
	int32_t *idp;
{
	DB_LOG *dblp;
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	int32_t id;
	int ret;

	env = dbp->env;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	fnp = dbp->log_filename;

	/*
	 * It's possible that after deciding we needed to call this function,
	 * someone else allocated an ID before we grabbed the lock.  Check
	 * to make sure there was no race and we have something useful to do.
	 */
	/* Get an unused ID from the free list. */
	if ((ret = __dbreg_pop_id(env, &id)) != 0)
		goto err;

	/* If no ID was found, allocate a new one. */
	if (id == DB_LOGFILEID_INVALID)
		id = lp->fid_max++;

	/* If the file is durable (i.e., not, not-durable), mark it as such. */
	if (!F_ISSET(dbp, DB_AM_NOT_DURABLE))
		F_SET(fnp, DB_FNAME_DURABLE);

	/* Hook the FNAME into the list of open files. */
	SH_TAILQ_INSERT_HEAD(&lp->fq, fnp, q, __fname);

	/*
	 * Log the registry.  We should only request a new ID in situations
	 * where logging is reasonable.
	 */
	DB_ASSERT(env, !F_ISSET(dbp, DB_AM_RECOVER));

	if ((ret = __dbreg_log_id(dbp, txn, id, 0)) != 0)
		goto err;

	/*
	 * Once we log the create_txnid, we need to make sure we never
	 * log it again (as might happen if this is a replication client
	 * that later upgrades to a master).
	 */
	fnp->create_txnid = TXN_INVALID;

	DB_ASSERT(env, dbp->type == fnp->s_type);
	DB_ASSERT(env, dbp->meta_pgno == fnp->meta_pgno);

	if ((ret = __dbreg_add_dbentry(env, dblp, dbp, id)) != 0)
		goto err;
	/*
	 * If we have a successful call, set the ID.  Otherwise
	 * we have to revoke it and remove it from all the lists
	 * it has been added to, and return an invalid id.
	 */
err:
	if (ret != 0 && id != DB_LOGFILEID_INVALID) {
		(void)__dbreg_revoke_id(dbp, 1, id);
		id = DB_LOGFILEID_INVALID;
	}
	*idp = id;
	return (ret);
}

/*
 * __dbreg_assign_id --
 *	Assign a particular dbreg id to this database handle.
 *
 * PUBLIC: int __dbreg_assign_id __P((DB *, int32_t, int));
 */
int
__dbreg_assign_id(dbp, id, deleted)
	DB *dbp;
	int32_t id;
	int deleted;
{
	DB *close_dbp;
	DB_LOG *dblp;
	ENV *env;
	FNAME *close_fnp, *fnp;
	LOG *lp;
	int ret;

	env = dbp->env;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	fnp = dbp->log_filename;

	close_dbp = NULL;
	close_fnp = NULL;

	/* The mtx_filelist protects the FNAME list and id management. */
	MUTEX_LOCK(env, lp->mtx_filelist);

	/* We should only call this on DB handles that have no ID. */
	DB_ASSERT(env, fnp->id == DB_LOGFILEID_INVALID);

	/*
	 * Make sure there isn't already a file open with this ID. There can
	 * be in recovery, if we're recovering across a point where an ID got
	 * reused.
	 */
	if (__dbreg_id_to_fname(dblp, id, 1, &close_fnp) == 0) {
		/*
		 * We want to save off any dbp we have open with this id.  We
		 * can't safely close it now, because we hold the mtx_filelist,
		 * but we should be able to rely on it being open in this
		 * process, and we're running recovery, so no other thread
		 * should muck with it if we just put off closing it until
		 * we're ready to return.
		 *
		 * Once we have the dbp, revoke its id;  we're about to
		 * reuse it.
		 */
		ret = __dbreg_id_to_db(env, NULL, &close_dbp, id, 0);
		if (ret == ENOENT) {
			ret = 0;
			goto cont;
		} else if (ret != 0)
			goto err;

		if ((ret = __dbreg_revoke_id(close_dbp, 1,
		    DB_LOGFILEID_INVALID)) != 0)
			goto err;
	}

	/*
	 * Remove this ID from the free list, if it's there, and make sure
	 * we don't allocate it anew.
	 */
cont:	if ((ret = __dbreg_pluck_id(env, id)) != 0)
		goto err;
	if (id >= lp->fid_max)
		lp->fid_max = id + 1;

	/* Now go ahead and assign the id to our dbp. */
	fnp->id = id;
	/* If the file is durable (i.e., not, not-durable), mark it as such. */
	if (!F_ISSET(dbp, DB_AM_NOT_DURABLE))
		F_SET(fnp, DB_FNAME_DURABLE);
	SH_TAILQ_INSERT_HEAD(&lp->fq, fnp, q, __fname);

	/*
	 * If we get an error adding the dbentry, revoke the id.
	 * We void the return value since we want to retain and
	 * return the original error in ret anyway.
	 */
	if ((ret = __dbreg_add_dbentry(env, dblp, dbp, id)) != 0)
		(void)__dbreg_revoke_id(dbp, 1, id);
	else
		dblp->dbentry[id].deleted = deleted;

err:	MUTEX_UNLOCK(env, lp->mtx_filelist);

	/* There's nothing useful that our caller can do if this close fails. */
	if (close_dbp != NULL)
		(void)__db_close(close_dbp, NULL, DB_NOSYNC);

	return (ret);
}

/*
 * __dbreg_revoke_id --
 *	Take a log id away from a dbp, in preparation for closing it,
 *	but without logging the close.
 *
 * PUBLIC: int __dbreg_revoke_id __P((DB *, int, int32_t));
 */
int
__dbreg_revoke_id(dbp, have_lock, force_id)
	DB *dbp;
	int have_lock;
	int32_t force_id;
{
	DB_REP *db_rep;
	ENV *env;
	int push;

	env = dbp->env;

	/*
	 * If we are not in recovery but the file was opened for a recovery
	 * operation, then this process aborted a transaction for another
	 * process and the id may still be in use, so don't reuse this id.
	 * If our fid generation in replication has changed, this fid
	 * should not be reused
	 */
	db_rep = env->rep_handle;
	push = (!F_ISSET(dbp, DB_AM_RECOVER) || IS_RECOVERING(env)) &&
	    (!REP_ON(env) || ((REP *)db_rep->region)->gen == dbp->fid_gen);

	return (__dbreg_revoke_id_int(dbp->env,
	      dbp->log_filename, have_lock, push, force_id));
}
/*
 * __dbreg_revoke_id_int --
 *	Revoke a log, in preparation for closing it, but without logging
 *	the close.
 *
 * PUBLIC: int __dbreg_revoke_id_int
 * PUBLIC:     __P((ENV *, FNAME *, int, int, int32_t));
 */
int
__dbreg_revoke_id_int(env, fnp, have_lock, push, force_id)
	ENV *env;
	FNAME *fnp;
	int have_lock, push;
	int32_t force_id;
{
	DB_LOG *dblp;
	LOG *lp;
	int32_t id;
	int ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	ret = 0;

	/* If we lack an ID, this is a null-op. */
	if (fnp == NULL)
		return (0);

	/*
	 * If we have a force_id, we had an error after allocating
	 * the id, and putting it on the fq list, but before we
	 * finished setting up fnp.  So, if we have a force_id use it.
	 */
	if (force_id != DB_LOGFILEID_INVALID)
		id = force_id;
	else if (fnp->id == DB_LOGFILEID_INVALID) {
		if (fnp->old_id == DB_LOGFILEID_INVALID)
			return (0);
		id = fnp->old_id;
	} else
		id = fnp->id;
	if (!have_lock)
		MUTEX_LOCK(env, lp->mtx_filelist);

	fnp->id = DB_LOGFILEID_INVALID;
	fnp->old_id = DB_LOGFILEID_INVALID;

	/* Remove the FNAME from the list of open files. */
	SH_TAILQ_REMOVE(&lp->fq, fnp, q, __fname);

	/*
	 * This FNAME may be for a DBP which is already closed.  Its ID may
	 * still be in use by an aborting transaction.  If not,
	 * remove this id from the dbentry table and push it onto the
	 * free list.
	 */
	if (!F_ISSET(fnp, DB_FNAME_CLOSED) &&
	    (ret = __dbreg_rem_dbentry(dblp, id)) == 0 && push)
		ret = __dbreg_push_id(env, id);

	if (!have_lock)
		MUTEX_UNLOCK(env, lp->mtx_filelist);
	return (ret);
}

/*
 * __dbreg_close_id --
 *	Take a dbreg id away from a dbp that we're closing, and log
 * the unregistry if the refcount goes to 0.
 *
 * PUBLIC: int __dbreg_close_id __P((DB *, DB_TXN *, u_int32_t));
 */
int
__dbreg_close_id(dbp, txn, op)
	DB *dbp;
	DB_TXN *txn;
	u_int32_t op;
{
	DB_LOG *dblp;
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	int ret, t_ret;

	env = dbp->env;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	fnp = dbp->log_filename;

	/* If we lack an ID, this is a null-op. */
	if (fnp == NULL)
		return (0);

	if (fnp->id == DB_LOGFILEID_INVALID) {
		ret = __dbreg_revoke_id(dbp, 0, DB_LOGFILEID_INVALID);
		goto done;
	}

	/*
	 * If we are the last reference to this db then we need to log it
	 * as closed.  Otherwise the last transaction will do the logging.
	 * Remove the DBP from the db entry table since it can nolonger
	 * be used.  If we abort it will have to be reopened.
	 */
	ret = 0;
	DB_ASSERT(env, fnp->txn_ref > 0);
	if (fnp->txn_ref > 1) {
		MUTEX_LOCK(env, dbp->mutex);
		if (fnp->txn_ref > 1) {
			if (!F_ISSET(fnp, DB_FNAME_CLOSED) &&
			    (t_ret = __dbreg_rem_dbentry(
			    env->lg_handle, fnp->id)) != 0 && ret == 0)
				ret = t_ret;

			/*
			 * The DB handle has been closed in the logging system.
			 * Transactions may still have a ref to this name.
			 * Mark it so that if recovery reopens the file id
			 * the transaction will not close the wrong handle.
			 */
			F_SET(fnp, DB_FNAME_CLOSED);
			fnp->txn_ref--;
			MUTEX_UNLOCK(env, dbp->mutex);
			/* The mutex now lives only in the FNAME. */
			dbp->mutex = MUTEX_INVALID;
			dbp->log_filename = NULL;
			goto no_log;
		}
	}
	MUTEX_LOCK(env, lp->mtx_filelist);

	if ((ret = __dbreg_log_close(env, fnp, txn, op)) != 0)
		goto err;
	ret = __dbreg_revoke_id(dbp, 1, DB_LOGFILEID_INVALID);

err:	MUTEX_UNLOCK(env, lp->mtx_filelist);

done:	if ((t_ret = __dbreg_teardown(dbp)) != 0 && ret == 0)
		ret = t_ret;
no_log:
	return (ret);
}
/*
 * __dbreg_close_id_int --
 *	Close down a dbreg id and log the unregistry.  This is called only
 * when a transaction has the last ref to the fname.
 *
 * PUBLIC: int __dbreg_close_id_int __P((ENV *, FNAME *, u_int32_t, int));
 */
int
__dbreg_close_id_int(env, fnp, op, locked)
	ENV *env;
	FNAME *fnp;
	u_int32_t op;
	int locked;
{
	DB_LOG *dblp;
	LOG *lp;
	int ret, t_ret;

	DB_ASSERT(env, fnp->txn_ref == 1);
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	if (fnp->id == DB_LOGFILEID_INVALID)
		return (__dbreg_revoke_id_int(env,
		     fnp, locked, 1, DB_LOGFILEID_INVALID));

	if (F_ISSET(fnp, DB_FNAME_RECOVER))
		return (__dbreg_close_file(env, fnp));
	/*
	 * If log_close fails then it will mark the name DB_FNAME_NOTLOGGED
	 * and the id must persist.
	 */
	if (!locked)
		MUTEX_LOCK(env, lp->mtx_filelist);
	if ((ret = __dbreg_log_close(env, fnp, NULL, op)) != 0)
		goto err;

	ret = __dbreg_revoke_id_int(env, fnp, 1, 1, DB_LOGFILEID_INVALID);

err:	if (!locked)
		MUTEX_UNLOCK(env, lp->mtx_filelist);

	if ((t_ret = __dbreg_teardown_int(env, fnp)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __dbreg_failchk --
 *
 * Look for entries that belong to dead processes and either close them
 * out or, if there are pending transactions, just remove the mutex which
 * will get discarded later.
 *
 * PUBLIC: int __dbreg_failchk __P((ENV *));
 */
int
__dbreg_failchk(env)
	ENV *env;
{
	DB_ENV *dbenv;
	DB_LOG *dblp;
	FNAME *fnp, *nnp;
	LOG *lp;
	int ret, t_ret;
	char buf[DB_THREADID_STRLEN];

	if ((dblp = env->lg_handle) == NULL)
		return (0);

	lp = dblp->reginfo.primary;
	dbenv = env->dbenv;
	ret = 0;

	MUTEX_LOCK(env, lp->mtx_filelist);
	for (fnp = SH_TAILQ_FIRST(&lp->fq, __fname); fnp != NULL; fnp = nnp) {
		nnp = SH_TAILQ_NEXT(fnp, q, __fname);
		if (dbenv->is_alive(dbenv, fnp->pid, 0, DB_MUTEX_PROCESS_ONLY))
			continue;
		MUTEX_LOCK(env, fnp->mutex);
		__db_msg(env,
		    "Freeing log information for process: %s, (ref %lu)",
		    dbenv->thread_id_string(dbenv, fnp->pid, 0, buf),
		    (u_long)fnp->txn_ref);
		if (fnp->txn_ref > 1 || F_ISSET(fnp, DB_FNAME_CLOSED)) {
			if (!F_ISSET(fnp, DB_FNAME_CLOSED)) {
				fnp->txn_ref--;
				F_SET(fnp, DB_FNAME_CLOSED);
			}
			MUTEX_UNLOCK(env, fnp->mutex);
			fnp->mutex = MUTEX_INVALID;
			fnp->pid = 0;
		} else {
			F_SET(fnp, DB_FNAME_CLOSED);
			if ((t_ret = __dbreg_close_id_int(env,
			    fnp, DBREG_CLOSE, 1)) && ret == 0)
				ret = t_ret;
		}
	}

	MUTEX_UNLOCK(env, lp->mtx_filelist);
	return (ret);
}
/*
 * __dbreg_log_close --
 *
 * Log a close of a database.  Called when closing a file or when a
 * replication client is becoming a master.  That closes all the
 * files it previously had open.
 *
 * Assumes caller holds the lp->mutex_filelist lock already.
 *
 * PUBLIC: int __dbreg_log_close __P((ENV *, FNAME *,
 * PUBLIC:    DB_TXN *, u_int32_t));
 */
int
__dbreg_log_close(env, fnp, txn, op)
	ENV *env;
	FNAME *fnp;
	DB_TXN *txn;
	u_int32_t op;
{
	DBT fid_dbt, r_name, *dbtp;
	DB_LOG *dblp;
	DB_LSN r_unused;
	int ret;

	dblp = env->lg_handle;
	ret = 0;

	if (fnp->fname_off == INVALID_ROFF)
		dbtp = NULL;
	else {
		memset(&r_name, 0, sizeof(r_name));
		r_name.data = R_ADDR(&dblp->reginfo, fnp->fname_off);
		r_name.size = (u_int32_t)strlen((char *)r_name.data) + 1;
		dbtp = &r_name;
	}
	memset(&fid_dbt, 0, sizeof(fid_dbt));
	fid_dbt.data = fnp->ufid;
	fid_dbt.size = DB_FILE_ID_LEN;
	if ((ret = __dbreg_register_log(env, txn, &r_unused,
	    F_ISSET(fnp, DB_FNAME_DURABLE) ? 0 : DB_LOG_NOT_DURABLE,
	    op, dbtp, &fid_dbt, fnp->id,
	    fnp->s_type, fnp->meta_pgno, TXN_INVALID)) != 0) {
		/*
		 * We are trying to close, but the log write failed.
		 * Unfortunately, close needs to plow forward, because
		 * the application can't do anything with the handle.
		 * Make the entry in the shared memory region so that
		 * when we close the environment, we know that this
		 * happened.  Also, make sure we remove this from the
		 * per-process table, so that we don't try to close it
		 * later.
		 */
		F_SET(fnp, DB_FNAME_NOTLOGGED);
		(void)__dbreg_rem_dbentry(dblp, fnp->id);
	}
	return (ret);
}

/*
 * __dbreg_push_id and __dbreg_pop_id --
 *	Dbreg ids from closed files are kept on a stack in shared memory
 * for recycling.  (We want to reuse them as much as possible because each
 * process keeps open files in an array by ID.)  Push them to the stack and
 * pop them from it, managing memory as appropriate.
 *
 * The stack is protected by the mtx_filelist, and both functions assume it
 * is already locked.
 */
static int
__dbreg_push_id(env, id)
	ENV *env;
	int32_t id;
{
	DB_LOG *dblp;
	LOG *lp;
	REGINFO *infop;
	int32_t *stack, *newstack;
	int ret;

	dblp = env->lg_handle;
	infop = &dblp->reginfo;
	lp = infop->primary;

	if (id == lp->fid_max - 1) {
		lp->fid_max--;
		return (0);
	}

	/* Check if we have room on the stack. */
	if (lp->free_fid_stack == INVALID_ROFF ||
	    lp->free_fids_alloced <= lp->free_fids + 1) {
		LOG_SYSTEM_LOCK(env);
		if ((ret = __env_alloc(infop,
		    (lp->free_fids_alloced + 20) * sizeof(u_int32_t),
		    &newstack)) != 0) {
			LOG_SYSTEM_UNLOCK(env);
			return (ret);
		}

		if (lp->free_fid_stack != INVALID_ROFF) {
			stack = R_ADDR(infop, lp->free_fid_stack);
			memcpy(newstack, stack,
			    lp->free_fids_alloced * sizeof(u_int32_t));
			__env_alloc_free(infop, stack);
		}
		lp->free_fid_stack = R_OFFSET(infop, newstack);
		lp->free_fids_alloced += 20;
		LOG_SYSTEM_UNLOCK(env);
	}

	stack = R_ADDR(infop, lp->free_fid_stack);
	stack[lp->free_fids++] = id;
	return (0);
}

static int
__dbreg_pop_id(env, id)
	ENV *env;
	int32_t *id;
{
	DB_LOG *dblp;
	LOG *lp;
	int32_t *stack;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	/* Do we have anything to pop? */
	if (lp->free_fid_stack != INVALID_ROFF && lp->free_fids > 0) {
		stack = R_ADDR(&dblp->reginfo, lp->free_fid_stack);
		*id = stack[--lp->free_fids];
	} else
		*id = DB_LOGFILEID_INVALID;

	return (0);
}

/*
 * __dbreg_pluck_id --
 *	Remove a particular dbreg id from the stack of free ids.  This is
 * used when we open a file, as in recovery, with a specific ID that might
 * be on the stack.
 *
 * Returns success whether or not the particular id was found, and like
 * push and pop, assumes that the mtx_filelist is locked.
 */
static int
__dbreg_pluck_id(env, id)
	ENV *env;
	int32_t id;
{
	DB_LOG *dblp;
	LOG *lp;
	int32_t *stack;
	u_int i;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	if (id >= lp->fid_max)
		return (0);

	/* Do we have anything to look at? */
	if (lp->free_fid_stack != INVALID_ROFF) {
		stack = R_ADDR(&dblp->reginfo, lp->free_fid_stack);
		for (i = 0; i < lp->free_fids; i++)
			if (id == stack[i]) {
				/*
				 * Found it.  Overwrite it with the top
				 * id (which may harmlessly be itself),
				 * and shorten the stack by one.
				 */
				stack[i] = stack[lp->free_fids - 1];
				lp->free_fids--;
				return (0);
			}
	}

	return (0);
}

/*
 * __dbreg_log_id --
 *	Used for in-memory named files.  They are created in mpool and
 * are given id's early in the open process so that we can read and
 * create pages in the mpool for the files.  However, at the time that
 * the mpf is created, the file may not be fully created and/or its
 * meta-data may not be fully known, so we can't do a full dbregister.
 * This is a routine exported that will log a complete dbregister
 * record that will allow for both recovery and replication.
 *
 * PUBLIC: int __dbreg_log_id __P((DB *, DB_TXN *, int32_t, int));
 */
int
__dbreg_log_id(dbp, txn, id, needlock)
	DB *dbp;
	DB_TXN *txn;
	int32_t id;
	int needlock;
{
	DBT fid_dbt, r_name;
	DB_LOG *dblp;
	DB_LSN unused;
	ENV *env;
	FNAME *fnp;
	LOG *lp;
	u_int32_t op;
	int i, ret;

	env = dbp->env;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	fnp = dbp->log_filename;

	/*
	 * Verify that the fnp has been initialized, by seeing if it
	 * has any non-zero bytes in it.
	 */
	for (i = 0; i < DB_FILE_ID_LEN; i++)
		if (fnp->ufid[i] != 0)
			break;
	if (i == DB_FILE_ID_LEN)
		memcpy(fnp->ufid, dbp->fileid, DB_FILE_ID_LEN);

	if (fnp->s_type == DB_UNKNOWN)
		fnp->s_type = dbp->type;

	/*
	 * Log the registry.  We should only request a new ID in situations
	 * where logging is reasonable.
	 */
	memset(&fid_dbt, 0, sizeof(fid_dbt));
	memset(&r_name, 0, sizeof(r_name));

	if (needlock)
		MUTEX_LOCK(env, lp->mtx_filelist);

	if (fnp->fname_off != INVALID_ROFF) {
		r_name.data = R_ADDR(&dblp->reginfo, fnp->fname_off);
		r_name.size = (u_int32_t)strlen((char *)r_name.data) + 1;
	}

	fid_dbt.data = dbp->fileid;
	fid_dbt.size = DB_FILE_ID_LEN;

	op = !F_ISSET(dbp, DB_AM_OPEN_CALLED) ? DBREG_PREOPEN :
	    (F_ISSET(dbp, DB_AM_INMEM) ? DBREG_REOPEN : DBREG_OPEN);
	ret = __dbreg_register_log(env, txn, &unused,
	    F_ISSET(dbp, DB_AM_NOT_DURABLE) ? DB_LOG_NOT_DURABLE : 0,
	    op, r_name.size == 0 ? NULL : &r_name, &fid_dbt, id,
	    fnp->s_type, fnp->meta_pgno, fnp->create_txnid);

	if (needlock)
		MUTEX_UNLOCK(env, lp->mtx_filelist);

	return (ret);
}
