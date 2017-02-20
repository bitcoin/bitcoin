/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2004-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/fop.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

/*
 * Context information needed for buffer management during the building of a
 * list of database files present in the environment.  When fully built, the
 * buffer is in the form of an UPDATE message: a (marshaled) update_args,
 * followed by some number of (marshaled) fileinfo_args.
 *
 * Note that the fileinfo for the first file in the list always appears at
 * (constant) offset __REP_UPDATE_SIZE in the buffer.
 */
typedef struct {
	u_int8_t	*buf;	/* Buffer base address. */
	size_t		size;	/* Total allocated buffer size. */
	u_int8_t	*fillptr; /* Pointer to first unused space. */
	u_int32_t	count;	/* Number of entries currently in list. */
} FILE_LIST_CTX;
#define	FIRST_FILE_PTR(buf)	((buf) + __REP_UPDATE_SIZE)

static int __rep_check_uid __P((ENV *, FILE_LIST_CTX *, u_int32_t,
    u_int8_t *));
static int __rep_clean_interrupted __P((ENV *));
static int __rep_cleanup_nimdbs __P((ENV *));
static int __rep_filedone __P((ENV *, DB_THREAD_INFO *ip, int,
     REP *, __rep_fileinfo_args *, u_int32_t));
static int __rep_find_dbs __P((ENV *, u_int32_t, FILE_LIST_CTX *));
static int __rep_get_fileinfo __P((ENV *, const char *,
    const char *, __rep_fileinfo_args *, u_int8_t *));
static int __rep_get_file_list __P((ENV *,
    DB_FH *, u_int32_t, u_int32_t *, DBT *));
static int __rep_log_setup __P((ENV *,
    REP *, u_int32_t, u_int32_t, DB_LSN *));
static int __rep_mpf_open __P((ENV *, DB_MPOOLFILE **,
    __rep_fileinfo_args *, u_int32_t));
static int __rep_nextfile __P((ENV *, int, REP *));
static int __rep_page_gap __P((ENV *,
     REP *, __rep_fileinfo_args *, u_int32_t));
static int __rep_page_sendpages __P((ENV *, DB_THREAD_INFO *, int,
    __rep_control_args *, __rep_fileinfo_args *, DB_MPOOLFILE *, DB *));
static int __rep_queue_filedone __P((ENV *,
    DB_THREAD_INFO *, REP *, __rep_fileinfo_args *));
static int __rep_remove_all __P((ENV *, u_int32_t, DBT *));
static int __rep_remove_by_list __P((ENV *, u_int32_t,
    u_int8_t *, u_int32_t, u_int32_t));
static int __rep_remove_by_prefix __P((ENV *, const char *, const char *,
    size_t, APPNAME));
static int __rep_remove_file __P((ENV *, u_int8_t *, const char *,
    u_int32_t, u_int32_t));
static int __rep_remove_logs __P((ENV *));
static int __rep_remove_nimdbs __P((ENV *));
static int __rep_rollback __P((ENV *, DB_LSN *));
static int __rep_unlink_by_list __P((ENV *, u_int32_t,
    u_int8_t *, u_int32_t, u_int32_t));
static int __rep_walk_dir __P((ENV *, const char *, u_int32_t, FILE_LIST_CTX*));
static int __rep_write_page __P((ENV *,
    DB_THREAD_INFO *, REP *, __rep_fileinfo_args *));

/*
 * __rep_update_req -
 *	Process an update_req and send the file information to the client.
 *
 * PUBLIC: int __rep_update_req __P((ENV *, __rep_control_args *, int));
 */
int
__rep_update_req(env, rp, eid)
	ENV *env;
	__rep_control_args *rp;
	int eid;
{
	DBT updbt, vdbt;
	DB_LOG *dblp;
	DB_LOGC *logc;
	DB_LSN lsn;
	__rep_update_args u_args;
	FILE_LIST_CTX context;
	size_t updlen;
	u_int32_t flag, version;
	int ret, t_ret;

	/*
	 * Start by allocating 1Meg, which ought to be plenty enough to describe
	 * all databases in the environment.  (If it's not, __rep_walk_dir can
	 * grow the size.)
	 *
	 * The data we send looks like this:
	 *	__rep_update_args
	 *	__rep_fileinfo_args
	 *	__rep_fileinfo_args
	 *	...
	 */
	dblp = env->lg_handle;
	logc = NULL;
	if ((ret = __os_calloc(env, 1, MEGABYTE, &context.buf)) != 0)
		return (ret);
	context.size = MEGABYTE;
	context.count = 0;

	/* Reserve space for the update_args, and fill in file info. */
	context.fillptr = FIRST_FILE_PTR(context.buf);
	if ((ret = __rep_find_dbs(env, rp->rep_version, &context)) != 0)
		goto err;

	/*
	 * Now get our first LSN.  We send the lsn of the first
	 * non-archivable log file.
	 */
	flag = DB_SET;
	if ((ret = __log_get_stable_lsn(env, &lsn)) != 0) {
		if (ret != DB_NOTFOUND)
			goto err;
		/*
		 * If ret is DB_NOTFOUND then there is no checkpoint
		 * in this log, that is okay, just start at the beginning.
		 */
		ret = 0;
		flag = DB_FIRST;
	}

	/*
	 * Now get the version number of the log file of that LSN.
	 */
	if ((ret = __log_cursor(env, &logc)) != 0)
		goto err;

	memset(&vdbt, 0, sizeof(vdbt));
	/*
	 * Set our log cursor on the LSN we are sending.  Or
	 * to the first LSN if we have no stable LSN.
	 */
	if ((ret = __logc_get(logc, &lsn, &vdbt, flag)) != 0) {
		/*
		 * We could be racing a fresh master starting up.  If we
		 * have no log records, assume an initial LSN and current
		 * log version.
		 */
		if (ret != DB_NOTFOUND)
			goto err;
		INIT_LSN(lsn);
		version = DB_LOGVERSION;
	} else {
		if ((ret = __logc_version(logc, &version)) != 0)
			goto err;
	}
	/*
	 * Package up the update information.
	 */
	u_args.first_lsn = lsn;
	u_args.first_vers = version;
	u_args.num_files = context.count;
	if ((ret = __rep_update_marshal(env, rp->rep_version,
	    &u_args, context.buf, __REP_UPDATE_SIZE, &updlen)) != 0)
		goto err;
	DB_ASSERT(env, updlen == __REP_UPDATE_SIZE);

	/*
	 * We have all the file information now.  Send it to the client.
	 */
	DB_INIT_DBT(updbt, context.buf, context.fillptr - context.buf);

	LOG_SYSTEM_LOCK(env);
	lsn = ((LOG *)dblp->reginfo.primary)->lsn;
	LOG_SYSTEM_UNLOCK(env);
	(void)__rep_send_message(
	    env, eid, REP_UPDATE, &lsn, &updbt, 0, 0);

err:	__os_free(env, context.buf);
	/*
	 * If we got here because the lower code could not get the page
	 * lock then we skipped sending the message, but we don't want
	 * to return an error to the user.
	 */
	if (ret == DB_REP_PAGELOCKED)
		ret = 0;
	if (logc != NULL && (t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_find_dbs -
 *	Walk through all the named files/databases including those in the
 *	environment or data_dirs and those that in named and in-memory.  We
 *	need to	open them, gather the necessary information and then close
 *	them.
 *
 * May be called either while holding REP_SYSTEM_LOCK or without.
 */
static int
__rep_find_dbs(env, version, context)
	ENV *env;
	u_int32_t version;
	FILE_LIST_CTX *context;
{
	DB_ENV *dbenv;
	int ret;
	char **ddir, *real_dir;

	dbenv = env->dbenv;
	ret = 0;
	real_dir = NULL;

	if (dbenv->db_data_dir == NULL) {
		/*
		 * If we don't have a data dir, we have just the
		 * env home dir.
		 */
		ret = __rep_walk_dir(env, env->db_home, version, context);
	} else {
		for (ddir = dbenv->db_data_dir; *ddir != NULL; ++ddir) {
			if ((ret = __db_appname(env,
			    DB_APP_NONE, *ddir, NULL, &real_dir)) != 0)
				break;
			if ((ret = __rep_walk_dir(env,
			    real_dir, version, context)) != 0)
				break;
			__os_free(env, real_dir);
			real_dir = NULL;
		}
	}

	/* Now, collect any in-memory named databases. */
	if (ret == 0)
		ret = __rep_walk_dir(env, NULL, version, context);

	if (real_dir != NULL)
		__os_free(env, real_dir);
	return (ret);
}

/*
 * __rep_walk_dir --
 *
 * This is the routine that walks a directory and fills in the structures
 * that we use to generate messages to the client telling it what
 * files are available.  If the directory name is NULL, then we should
 * walk the list of in-memory named files.
 */
static int
__rep_walk_dir(env, dir, version, context)
	ENV *env;
	const char *dir;
	u_int32_t version;
	FILE_LIST_CTX *context;
{
	__rep_fileinfo_args tmpfp;
	size_t avail, len;
	int cnt, first_file, i, ret;
	u_int8_t uid[DB_FILE_ID_LEN];
	char *file, **names, *subdb;

	if (dir == NULL) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Walk_dir: Getting info for in-memory named files"));
		if ((ret = __memp_inmemlist(env, &names, &cnt)) != 0)
			return (ret);
	} else {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Walk_dir: Getting info for dir: %s", dir));
		if ((ret = __os_dirlist(env, dir, 0, &names, &cnt)) != 0)
			return (ret);
	}
	RPRINT(env, DB_VERB_REP_SYNC, (env, "Walk_dir: Dir %s has %d files",
	    (dir == NULL) ? "INMEM" : dir, cnt));
	first_file = 1;
	for (i = 0; i < cnt; i++) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Walk_dir: File %d name: %s", i, names[i]));
		/*
		 * Skip DB-owned files: __db*, DB_CONFIG, log*
		 */
		if (strncmp(names[i],
		    DB_REGION_PREFIX, sizeof(DB_REGION_PREFIX) - 1) == 0) {
			/* Process partition files: "__dbp.*". */
			if (names[i][sizeof(DB_REGION_PREFIX) - 1] != 'p')
				continue;
		}
		if (strncmp(names[i], "DB_CONFIG", 9) == 0)
			continue;
		if (strncmp(names[i], "log.", 4) == 0)
			continue;

		/* We found a file to process. */
		if (dir == NULL) {
			file = NULL;
			subdb = names[i];
		} else {
			file = names[i];
			subdb = NULL;
		}
		if ((ret = __rep_get_fileinfo(env,
		    file, subdb, &tmpfp, uid)) != 0) {
			/*
			 * If we find a file that isn't a database, skip it.
			 */
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			    "Walk_dir: File %d %s: returned error %s",
			    i, names[i], db_strerror(ret)));
			if (ret == DB_REP_PAGELOCKED)
				goto err;
			ret = 0;
			continue;
		}
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Walk_dir: File %s at 0x%lx: pgsize %lu, max_pgno %lu",
		    names[i], P_TO_ULONG(context->fillptr),
		    (u_long)tmpfp.pgsize, (u_long)tmpfp.max_pgno));

		/*
		 * On the first time through the loop, check to see if the file
		 * we're about to add is already on the list.  If it is, it must
		 * have been added in a previous call, and that means the
		 * directory we're currently scanning has already been scanned
		 * before.  (This can happen if the user called
		 * env->set_data_dir() more than once for the same directory.)
		 * If that's the case, we're done: not only is it a waste of
		 * time to scan the same directory again, but doing so would
		 * result in the same files appearing in the list more than
		 * once.
		 */
		if (first_file && dir != NULL &&
		    (ret = __rep_check_uid(env, context, version, uid)) != 0) {
			if (ret == DB_KEYEXIST)
				ret = 0;
			goto err;
		}
		first_file = 0;

		/*
		 * Finally we know that this file is a suitable database file
		 * that we haven't yet included on our list.
		 */
		tmpfp.filenum = context->count++;

		DB_SET_DBT(tmpfp.info, names[i], strlen(names[i]) + 1);
		DB_SET_DBT(tmpfp.uid, uid, DB_FILE_ID_LEN);
retry:		avail = (size_t)(&context->buf[context->size] -
		    context->fillptr);
		ret = __rep_fileinfo_marshal(env, version,
		    &tmpfp, context->fillptr, avail, &len);
		if (ret == ENOMEM) {
			/*
			 * Here, 'len' is the total space in use in the buffer.
			 */
			len = (size_t)(context->fillptr - context->buf);
			context->size *= 2;

			if ((ret = __os_realloc(env,
			    context->size, &context->buf)) != 0)
				goto err;
			context->fillptr = context->buf + len;

			/*
			 * Now that we've reallocated the space, try to
			 * store it again.
			 */
			goto retry;
		}
		/*
		 * Here, 'len' (still) holds the length of the marshaled
		 * information about the current file (as filled in by the last
		 * call to  __rep_fileinfo_marshal()).
		 */
		context->fillptr += len;
	}
err:
	__os_dirfree(env, names, cnt);
	return (ret);
}

/*
 * Check whether the given uid is already present in the list of files being
 * built in the context buffer.  A return of DB_KEYEXIST means it is.
 */
static int
__rep_check_uid(env, context, version, uid)
	ENV *env;
	FILE_LIST_CTX *context;
	u_int32_t version;
	u_int8_t *uid;
{
	__rep_fileinfo_args *rfp;
	size_t max;
	u_int8_t *fp;
	u_int32_t i;
	int ret;

	ret = 0;
	rfp = NULL;
	fp = FIRST_FILE_PTR(context->buf);
	for (i = 0; i < context->count; i++) {
		max = (size_t)(context->fillptr - fp);
		if ((ret = __rep_fileinfo_unmarshal(env, version,
		    &rfp, fp, max, &fp)) != 0) {
			__db_errx(env, "rep_check_uid: Could not malloc");
			goto err;
		}
		if (memcmp(rfp->uid.data, uid, DB_FILE_ID_LEN) == 0) {
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			    "Check_uid: Found matching file."));
			ret = DB_KEYEXIST;
			goto err;
		}
		__os_free(env, rfp);
		rfp = NULL;
	}
err:
	if (rfp != NULL)
		__os_free(env, rfp);
	return (ret);

}

static int
__rep_get_fileinfo(env, file, subdb, rfp, uid)
	ENV *env;
	const char *file, *subdb;
	__rep_fileinfo_args *rfp;
	u_int8_t *uid;
{
	DB *dbp;
	DBC *dbc;
	DBMETA *dbmeta;
	DB_LOCK lk;
	DB_MPOOLFILE *mpf;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	PAGE *pagep;
	int lorder, ret, retry, t_ret;

	dbp = NULL;
	dbc = NULL;
	pagep = NULL;
	mpf = NULL;
	txn = NULL;
	LOCK_INIT(lk);

	ENV_GET_THREAD_INFO(env, ip);

	/*
	 * If the meta page is locked, try a few times.  If we cannot
	 * get it, return.
	 */
	for (retry = 0; retry < REP_META_RETRY; retry++) {
		if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
			goto err;
		if ((ret = __txn_begin(env, NULL, NULL, &txn,
		    DB_TXN_NOWAIT)) != 0)
			goto err;
		if ((ret = __db_open(dbp, ip, txn, file, subdb, DB_UNKNOWN,
		    DB_RDONLY | (F_ISSET(env, ENV_THREAD) ? DB_THREAD : 0),
		    0, PGNO_BASE_MD)) != 0) {
			RPRINT(env, DB_VERB_REP_SYNC,
			    (env, "get_fileinfo: open error %d", ret));
			(void)__txn_abort(txn);
			txn = NULL;
			(void)__db_close(dbp, NULL, DB_NOSYNC);
			dbp = NULL;
			if (ret == DB_LOCK_DEADLOCK ||
			    ret == DB_LOCK_NOTGRANTED) {
				__os_yield(env, 1, 0);
				RPRINT(env, DB_VERB_REP_SYNC,
    (env, "get_fileinfo: Try %d could not get meta lock for open", retry));
				continue;
			} else
				goto err;
		} else
			break;
	}
	if (retry == REP_META_RETRY) {
		ret = DB_REP_PAGELOCKED;
		goto err;
	}

	if ((ret = __db_cursor(dbp, ip, txn, &dbc, 0)) != 0)
		goto err;
	/*
	 * If the meta page is locked, try a few times.  If we cannot
	 * get it, return.
	 */
	for (retry = 0; retry < REP_META_RETRY; retry++) {
		if ((ret = __db_lget(dbc, 0, dbp->meta_pgno,
		    DB_LOCK_READ, DB_LOCK_NOWAIT, &lk)) != 0) {
			if (ret == DB_LOCK_DEADLOCK ||
			    ret == DB_LOCK_NOTGRANTED) {
				RPRINT(env, DB_VERB_REP_SYNC,
    (env, "get_fileinfo: Try %d could not get meta lock", retry));
				__os_yield(env, 1, 0);
				continue;
			} else
				goto err;
		} else
			break;
	}
	if (retry == REP_META_RETRY) {
		ret = DB_REP_PAGELOCKED;
		goto err;
	}
	if ((ret = __memp_fget(dbp->mpf, &dbp->meta_pgno, ip, dbc->txn,
	    0, &pagep)) != 0)
		goto err;
	/*
	 * We have the meta page.  Set up our information.
	 */
	dbmeta = (DBMETA *)pagep;
	rfp->pgno = 0;
	/*
	 * Queue is a special-case.  We need to set max_pgno to 0 so that
	 * the client can compute the pages from the meta-data.
	 */
	if (dbp->type == DB_QUEUE)
		rfp->max_pgno = 0;
	else
		rfp->max_pgno = dbmeta->last_pgno;
	rfp->pgsize = dbp->pgsize;
	memcpy(uid, dbp->fileid, DB_FILE_ID_LEN);
	rfp->type = (u_int32_t)dbp->type;
	rfp->db_flags = dbp->flags;
	rfp->finfo_flags = 0;
	/*
	 * Send the lorder of this database.
	 */
	(void)__db_get_lorder(dbp, &lorder);
	if (lorder == 1234)
		FLD_SET(rfp->finfo_flags, REPINFO_DB_LITTLEENDIAN);
	else
		FLD_CLR(rfp->finfo_flags, REPINFO_DB_LITTLEENDIAN);

	ret = __memp_fput(dbp->mpf, ip, pagep, dbc->priority);
	pagep = NULL;
	if ((t_ret = __LPUT(dbc, lk)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		goto err;
err:
	if ((t_ret = __LPUT(dbc, lk)) != 0 && ret == 0)
		ret = t_ret;
	if (pagep != NULL && (t_ret =
	    __memp_fput(mpf, ip, pagep, dbc->priority)) != 0 && ret == 0)
		ret = t_ret;
	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	if (txn != NULL)
		(void)__txn_abort(txn);
	if (dbp != NULL && (t_ret = __db_close(dbp, NULL, 0)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_page_req
 *	Process a page_req and send the page information to the client.
 *
 * PUBLIC: int __rep_page_req __P((ENV *,
 * PUBLIC:     DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
 */
int
__rep_page_req(env, ip, eid, rp, rec)
	ENV *env;
	DB_THREAD_INFO *ip;
	int eid;
	__rep_control_args *rp;
	DBT *rec;
{
	__rep_fileinfo_args *msgfp;
	DB_MPOOLFILE *mpf;
	DB_REP *db_rep;
	REP *rep;
	int ret, t_ret;
	u_int8_t *next;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	if ((ret = __rep_fileinfo_unmarshal(env, rp->rep_version,
	    &msgfp, rec->data, rec->size, &next)) != 0)
		return (ret);

	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "page_req: file %d page %lu to %lu",
	    msgfp->filenum, (u_long)msgfp->pgno, (u_long)msgfp->max_pgno));

	/*
	 * We need to open the file and then send its pages.
	 * If we cannot open the file, we send REP_FILE_FAIL.
	 */
	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "page_req: Open %d via mpf_open", msgfp->filenum));
	if ((ret = __rep_mpf_open(env, &mpf, msgfp, 0)) != 0) {
		RPRINT(env, DB_VERB_REP_SYNC,
		    (env, "page_req: Open %d failed", msgfp->filenum));
		if (F_ISSET(rep, REP_F_MASTER))
			(void)__rep_send_message(env, eid, REP_FILE_FAIL,
			    NULL, rec, 0, 0);
		else
			ret = DB_NOTFOUND;
		goto err;
	}

	ret = __rep_page_sendpages(env, ip, eid, rp, msgfp, mpf, NULL);
	t_ret = __memp_fclose(mpf, 0);
	if (ret == 0 && t_ret != 0)
		ret = t_ret;
err:
	__os_free(env, msgfp);
	return (ret);
}

static int
__rep_page_sendpages(env, ip, eid, rp, msgfp, mpf, dbp)
	ENV *env;
	DB_THREAD_INFO *ip;
	int eid;
	__rep_control_args *rp;
	__rep_fileinfo_args *msgfp;
	DB_MPOOLFILE *mpf;
	DB *dbp;
{
	DB *qdbp;
	DBC *qdbc;
	DBT lockdbt, msgdbt;
	DB_LOCK lock;
	DB_LOCKER *locker;
	DB_LOCK_ILOCK lock_obj;
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	PAGE *pagep;
	REP *rep;
	REP_BULK bulk;
	REP_THROTTLE repth;
	db_pgno_t p;
	uintptr_t bulkoff;
	size_t len, msgsz;
	u_int32_t bulkflags, use_bulk;
	int opened, ret, t_ret;
	u_int8_t *buf;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	locker = NULL;
	opened = 0;
	t_ret = 0;
	qdbp = NULL;
	qdbc = NULL;
	buf = NULL;
	bulk.addr = NULL;
	use_bulk = FLD_ISSET(rep->config, REP_C_BULK);
	if (msgfp->type == (u_int32_t)DB_QUEUE) {
		if (dbp == NULL) {
			if ((ret = __db_create_internal(&qdbp, env, 0)) != 0)
				goto err;
			/*
			 * We need to check whether this is in-memory so that
			 * we pass the name correctly as either the file or
			 * the database name.
			 */
			if ((ret = __db_open(qdbp, ip, NULL,
			    FLD_ISSET(msgfp->db_flags, DB_AM_INMEM) ?
			    NULL : msgfp->info.data,
			    FLD_ISSET(msgfp->db_flags, DB_AM_INMEM) ?
			    msgfp->info.data : NULL,
			    DB_UNKNOWN,
			DB_RDONLY | (F_ISSET(env, ENV_THREAD) ? DB_THREAD : 0),
			    0, PGNO_BASE_MD)) != 0)
				goto err;
			opened = 1;
		} else
			qdbp = dbp;
		if ((ret = __db_cursor(qdbp, ip, NULL, &qdbc, 0)) != 0)
			goto err;
	}
	msgsz = __REP_FILEINFO_SIZE + DB_FILE_ID_LEN + msgfp->pgsize;
	if ((ret = __os_calloc(env, 1, msgsz, &buf)) != 0)
		goto err;
	memset(&msgdbt, 0, sizeof(msgdbt));
	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "sendpages: file %d page %lu to %lu",
	    msgfp->filenum, (u_long)msgfp->pgno, (u_long)msgfp->max_pgno));
	memset(&repth, 0, sizeof(repth));
	/*
	 * If we're doing bulk transfer, allocate a bulk buffer to put our
	 * pages in.  We still need to initialize the throttle info
	 * because if we encounter a page larger than our entire bulk
	 * buffer, we need to send it as a singleton.
	 *
	 * Use a local var so that we don't need to worry if someone else
	 * turns on/off bulk in the middle of our call here.
	 */
	if (use_bulk && (ret = __rep_bulk_alloc(env, &bulk, eid,
	    &bulkoff, &bulkflags, REP_BULK_PAGE)) != 0)
		goto err;
	REP_SYSTEM_LOCK(env);
	repth.gbytes = rep->gbytes;
	repth.bytes = rep->bytes;
	repth.type = REP_PAGE;
	repth.data_dbt = &msgdbt;
	REP_SYSTEM_UNLOCK(env);

	/*
	 * Set up locking.
	 */
	LOCK_INIT(lock);
	memset(&lock_obj, 0, sizeof(lock_obj));
	if ((ret = __lock_id(env, NULL, &locker)) != 0)
		goto err;
	memcpy(lock_obj.fileid, mpf->fileid, DB_FILE_ID_LEN);
	lock_obj.type = DB_PAGE_LOCK;

	memset(&lockdbt, 0, sizeof(lockdbt));
	lockdbt.data = &lock_obj;
	lockdbt.size = sizeof(lock_obj);

	for (p = msgfp->pgno; p <= msgfp->max_pgno; p++) {
		/*
		 * We're not waiting for the lock, if we cannot get
		 * the lock for this page, skip it.  The gap
		 * code will rerequest it.
		 */
		lock_obj.pgno = p;
		if ((ret = __lock_get(env, locker, DB_LOCK_NOWAIT, &lockdbt,
		    DB_LOCK_READ, &lock)) != 0) {
			/*
			 * Continue if we couldn't get the lock.
			 */
			if (ret == DB_LOCK_DEADLOCK ||
			    ret == DB_LOCK_NOTGRANTED) {
				ret = 0;
				continue;
			}
			/*
			 * Otherwise we have an error.
			 */
			goto err;
		}
		if (msgfp->type == (u_int32_t)DB_QUEUE && p != 0)
#ifdef HAVE_QUEUE
			ret = __qam_fget(qdbc, &p, DB_MPOOL_CREATE, &pagep);
#else
			ret = DB_PAGE_NOTFOUND;
#endif
		else
			ret = __memp_fget(mpf, &p, ip, NULL,
			    DB_MPOOL_CREATE, &pagep);
		msgfp->pgno = p;
		if (ret == DB_PAGE_NOTFOUND) {
			ZERO_LSN(lsn);
			if (F_ISSET(rep, REP_F_MASTER)) {
				ret = 0;
				RPRINT(env, DB_VERB_REP_SYNC, (env,
				    "sendpages: PAGE_FAIL on page %lu",
				    (u_long)p));
				(void)__rep_send_message(env, eid,
				    REP_PAGE_FAIL, &lsn, &msgdbt, 0, 0);
			} else
				ret = DB_NOTFOUND;
			goto lockerr;
		} else if (ret != 0)
			goto lockerr;
		else
			DB_SET_DBT(msgfp->info, pagep, msgfp->pgsize);
		len = 0;
		/*
		 * Send along an indication of the byte order of this mpool
		 * page.  Since mpool always keeps pages in the native byte
		 * order of the local environment, this is simply my
		 * environment's byte order.
		 *
		 * Since pages can be served from a variety of sites when using
		 * client-to-client synchronization, the receiving client needs
		 * to know the byte order of each page independently.
		 */
		if (F_ISSET(env, ENV_LITTLEENDIAN))
			FLD_SET(msgfp->finfo_flags, REPINFO_PG_LITTLEENDIAN);
		else
			FLD_CLR(msgfp->finfo_flags, REPINFO_PG_LITTLEENDIAN);
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "sendpages: %lu, page lsn [%lu][%lu]", (u_long)p,
		    (u_long)pagep->lsn.file, (u_long)pagep->lsn.offset));
		ret = __rep_fileinfo_marshal(env, rp->rep_version,
		    msgfp, buf, msgsz, &len);
		if (msgfp->type != (u_int32_t)DB_QUEUE || p == 0)
			t_ret = __memp_fput(mpf,
			    ip, pagep, DB_PRIORITY_UNCHANGED);
#ifdef HAVE_QUEUE
		else
			/*
			 * We don't need an #else for HAVE_QUEUE here because if
			 * we're not compiled with queue, then we're guaranteed
			 * to have set REP_PAGE_FAIL above.
			 */
			t_ret = __qam_fput(qdbc, p, pagep, qdbp->priority);
#endif
		if (t_ret != 0 && ret == 0)
			ret = t_ret;
		if ((t_ret = __ENV_LPUT(env, lock)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;

		DB_ASSERT(env, len <= msgsz);
		DB_SET_DBT(msgdbt, buf, len);

		dblp = env->lg_handle;
		LOG_SYSTEM_LOCK(env);
		repth.lsn = ((LOG *)dblp->reginfo.primary)->lsn;
		LOG_SYSTEM_UNLOCK(env);
		/*
		 * If we are configured for bulk, try to send this as a bulk
		 * request.  If not configured, or it is too big for bulk
		 * then just send normally.
		 */
		if (use_bulk)
			ret = __rep_bulk_message(env, &bulk, &repth,
			    &repth.lsn, &msgdbt, 0);
		if (!use_bulk || ret == DB_REP_BULKOVF)
			ret = __rep_send_throttle(env, eid, &repth, 0, 0);
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "sendpages: %lu, lsn [%lu][%lu]", (u_long)p,
		    (u_long)repth.lsn.file, (u_long)repth.lsn.offset));
		/*
		 * If we have REP_PAGE_MORE we need to break this loop.
		 * Otherwise, with REP_PAGE, we keep going.
		 */
		if (repth.type == REP_PAGE_MORE || ret != 0) {
			/* Ignore send failure, except to break the loop. */
			if (ret == DB_REP_UNAVAIL)
				ret = 0;
			break;
		}
	}

	if (0) {
lockerr:	if ((t_ret = __ENV_LPUT(env, lock)) != 0 && ret == 0)
			ret = t_ret;
	}
err:
	/*
	 * We're done, force out whatever remains in the bulk buffer and
	 * free it.
	 */
	if (use_bulk && bulk.addr != NULL &&
	    (t_ret = __rep_bulk_free(env, &bulk, 0)) != 0 && ret == 0 &&
	    t_ret != DB_REP_UNAVAIL)
		ret = t_ret;
	if (qdbc != NULL && (t_ret = __dbc_close(qdbc)) != 0 && ret == 0)
		ret = t_ret;
	if (opened && (t_ret = __db_close(qdbp, NULL, DB_NOSYNC)) != 0 &&
	    ret == 0)
		ret = t_ret;
	if (buf != NULL)
		__os_free(env, buf);
	if (locker != NULL && (t_ret = __lock_id_free(env,
	    locker)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_update_setup
 *	Process and setup with this file information.
 *
 * PUBLIC: int __rep_update_setup __P((ENV *, int, __rep_control_args *,
 * PUBLIC:     DBT *, time_t));
 */
int
__rep_update_setup(env, eid, rp, rec, savetime)
	ENV *env;
	int eid;
	__rep_control_args *rp;
	DBT *rec;
	time_t savetime;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	DB_THREAD_INFO *ip;
	LOG *lp;
	REGENV *renv;
	REGINFO *infop;
	REP *rep;
	__rep_update_args *rup;
	__rep_fileinfo_args *finfo;
	DB_LSN verify_lsn;
	size_t max;
	int found, ret;
	u_int32_t count;
	u_int8_t *end, *next;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	ret = 0;

	MUTEX_LOCK(env, rep->mtx_clientdb);
	verify_lsn = lp->verify_lsn;
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	REP_SYSTEM_LOCK(env);
	if (!F_ISSET(rep, REP_F_RECOVER_UPDATE) || IN_ELECTION(rep)) {
		REP_SYSTEM_UNLOCK(env);
		return (0);
	}
	F_CLR(rep, REP_F_RECOVER_UPDATE);

	if ((ret = __rep_update_unmarshal(env, rp->rep_version,
	    &rup, rec->data, rec->size, &next)) != 0)
		return (ret);
	DB_ASSERT(env, next == FIRST_FILE_PTR((u_int8_t*)rec->data));
	end = &((u_int8_t*)rec->data)[rec->size];

	/*
	 * If we're doing an abbreviated internal init, it's because we found a
	 * sync point but we needed to materialize any NIMDBs.  However, if we
	 * now see that there are no NIMDBs we can just skip to verify_match,
	 * just as we would have done if we had already loaded the NIMDBs.  In
	 * other words, if there are no NIMDBs, then I can trivially say that
	 * I've already loaded all of them!  The whole abbreviated internal init
	 * turns out not to have been necessary after all.
	 */
	if (F_ISSET(rep, REP_F_ABBREVIATED)) {
		count = rup->num_files;
		found = 0;
		while (count-- > 0) {
			max = (size_t)(end - next);
			if ((ret = __rep_fileinfo_unmarshal(env,
			    rp->rep_version, &finfo, next, max, &next)) != 0)
				goto err;
			found = FLD_ISSET(finfo->db_flags, DB_AM_INMEM);
			__os_free(env, finfo);
			if (found)
				break;
		}
		if (!found) {
			/*
			 * Revert to VERIFY state, so that we can pick up where
			 * we left off, except that from now on (i.e., future
			 * master changes) we can skip checking for NIMDBs if we
			 * find a sync point.
			 */
			F_SET(rep, REP_F_NIMDBS_LOADED | REP_F_RECOVER_VERIFY);
			F_CLR(rep, REP_F_ABBREVIATED);

			REP_SYSTEM_UNLOCK(env);
			ret = __rep_verify_match(env, &verify_lsn, savetime);
			__os_free(env, rup);
			return (ret);
		}
	}

	/*
	 * We know we're the first to come in here due to the
	 * REP_F_RECOVER_UPDATE flag.
	 */
	F_SET(rep, REP_F_RECOVER_PAGE);
	/*
	 * We should not ever be in internal init with a lease granted.
	 */
	DB_ASSERT(env,
	    !IS_USING_LEASES(env) || __rep_islease_granted(env) == 0);

	/*
	 * We do not clear REP_F_READY_* in this code.
	 * We'll eventually call the normal __rep_verify_match recovery
	 * code and that will clear all the flags and allow others to
	 * proceed.  We lockout both the messages and API here.
	 * We lockout messages briefly because we are about to reset
	 * all our LSNs and we do not want another thread possibly
	 * using/needing those.  We have to lockout the API for
	 * the duration of internal init.
	 */
	if ((ret = __rep_lockout_msg(env, rep, 1)) != 0)
		goto err;

	if ((ret = __rep_lockout_api(env, rep)) != 0)
		goto err;
	/*
	 * We need to update the timestamp and kill any open handles
	 * on this client.  The files are changing completely.
	 */
	infop = env->reginfo;
	renv = infop->primary;
	(void)time(&renv->rep_timestamp);

	REP_SYSTEM_UNLOCK(env);
	MUTEX_LOCK(env, rep->mtx_clientdb);
	__os_gettime(env, &lp->rcvd_ts, 1);
	lp->wait_ts = rep->request_gap;
	ZERO_LSN(lp->ready_lsn);
	ZERO_LSN(lp->verify_lsn);
	ZERO_LSN(lp->prev_ckp);
	ZERO_LSN(lp->waiting_lsn);
	ZERO_LSN(lp->max_wait_lsn);
	ZERO_LSN(lp->max_perm_lsn);
	if (db_rep->rep_db == NULL)
		ret = __rep_client_dbinit(env, 0, REP_DB);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	if (ret != 0)
		goto err_nolock;

	/*
	 * We need to empty out any old log records that might be in the
	 * temp database.
	 */
	ENV_GET_THREAD_INFO(env, ip);
	if ((ret = __db_truncate(db_rep->rep_db, ip, NULL, &count)) != 0)
		goto err_nolock;
	rep->stat.st_log_queued = 0;

	REP_SYSTEM_LOCK(env);
	if (F_ISSET(rep, REP_F_ABBREVIATED)) {
		/*
		 * For an abbreviated internal init, the place from which we'll
		 * want to request master's logs after (NIMDB) pages are loaded
		 * is precisely the sync point we found during VERIFY.  We'll
		 * roll back to there in a moment.
		 *
		 * We don't need first_vers, because it's only used with
		 * __log_newfile, which only happens with non-ABBREVIATED
		 * internal init.
		 */
		rep->first_lsn = verify_lsn;
	} else {
		/*
		 * We will remove all logs we have so we need to request
		 * from the master's beginning.
		 */
		rep->first_lsn = rup->first_lsn;
		rep->first_vers = rup->first_vers;
	}
	rep->last_lsn = rp->lsn;
	rep->nfiles = rup->num_files;

	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "Update setup for %d files.", rep->nfiles));
	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "Update setup:  First LSN [%lu][%lu].",
	    (u_long)rep->first_lsn.file, (u_long)rep->first_lsn.offset));
	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "Update setup:  Last LSN [%lu][%lu]",
	    (u_long)rep->last_lsn.file, (u_long)rep->last_lsn.offset));

	if (rep->nfiles > 0) {
		rep->infoversion = rp->rep_version;
		rep->originfolen = rep->infolen =
		    rec->size - __REP_UPDATE_SIZE;
		if ((ret = __os_calloc(env, 1, rep->infolen,
		    &rep->originfo)) != 0)
			goto err;
		memcpy(rep->originfo,
		    FIRST_FILE_PTR((u_int8_t*)rec->data), rep->infolen);
		rep->nextinfo = rep->originfo;
	}

	/*
	 * Clear the decks to make room for the logs and databases that we will
	 * request as part of this internal init.  For a normal, full internal
	 * init, that means all logs and databases.  For an abbreviated internal
	 * init, it means only the NIMDBs, and only that portion of the log
	 * after the sync point.
	 */
	if (F_ISSET(rep, REP_F_ABBREVIATED)) {
		/*
		 * Note that in order to pare the log back to the sync point, we
		 * can't just crudely hack it off there.  We need to make sure
		 * that pages in regular databases get rolled back to a state
		 * consistent with that sync point.  So we have to do a real
		 * recovery step.
		 */
		if ((ret = __rep_rollback(env, &rep->first_lsn)) != 0)
			goto err;
		ret = __rep_remove_nimdbs(env);
	} else
		ret = __rep_remove_all(env, rp->rep_version, rec);
	if (ret != 0)
		goto err;
	F_CLR(rep, REP_F_READY_MSG);

	rep->curfile = 0;
	ret = __rep_nextfile(env, eid, rep);
	if (ret != 0)
		goto err;

	if (0) {
err_nolock:	REP_SYSTEM_LOCK(env);
	}

err:	/*
	 * If we get an error, we cannot leave ourselves in the RECOVER_PAGE
	 * state because we have no file information.  That also means undo'ing
	 * the rep_lockout.  We need to move back to the RECOVER_UPDATE stage.
	 * In the non-error path, we will have already cleared READY_MSG, but it
	 * doesn't hurt to clear it again.
	 */
	F_CLR(rep, REP_F_READY_MSG);
	if (ret != 0) {
		if (rep->originfo != NULL) {
			__os_free(env, rep->originfo);
			rep->originfo = NULL;
		}
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Update_setup: Error: Clear PAGE, set UPDATE again. %s",
		    db_strerror(ret)));
		F_CLR(rep, REP_F_RECOVER_PAGE | REP_F_READY_API |
		    REP_F_READY_OP);
		F_SET(rep, REP_F_RECOVER_UPDATE);
	}
	REP_SYSTEM_UNLOCK(env);
	__os_free(env, rup);
	return (ret);
}

/*
 * Removes any currently existing NIMDBs.  We do this at the beginning of
 * abbreviated internal init, when any existing NIMDBs should be intact, so
 * walk_dir should produce reliable results.
 */
static int
__rep_remove_nimdbs(env)
	ENV *env;
{
	__rep_fileinfo_args *finfo;
	FILE_LIST_CTX context;
	size_t max;
	u_int8_t *fp;
	int ret;

	finfo = NULL;

	if ((ret = __os_calloc(env, 1, MEGABYTE, &context.buf)) != 0)
		return (ret);
	context.size = MEGABYTE;
	context.count = 0;
	context.fillptr = context.buf;

	/* NB: "NULL" asks walk_dir to consider only in-memory DBs */
	if ((ret = __rep_walk_dir(env, NULL, DB_REPVERSION, &context)) != 0)
		goto out;

	if ((ret = __rep_closefiles(env)) != 0)
		goto out;

	fp = context.buf;
	while (context.count-- > 0) {
		max = (size_t)(context.fillptr - fp);
		if ((ret = __rep_fileinfo_unmarshal(env, DB_REPVERSION,
		    &finfo, fp, max, &fp)) != 0)
			goto out;
		if ((ret = __rep_remove_file(env, finfo->uid.data,
		    finfo->info.data, finfo->type, finfo->db_flags)) != 0)
			goto out;
		__os_free(env, finfo);
		finfo = NULL;
	}

out:
	if (finfo != NULL)
		__os_free(env, finfo);
	__os_free(env, context.buf);
	return (ret);
}

/*
 * Removes all existing logs and databases, at the start of internal init.  But
 * before we do, write a list of the databases onto the init file, so that in
 * case we crash in the middle, we'll know how to resume when we restart.
 * Finally, also write into the init file the UPDATE message from the master (in
 * the "rec" DBT), which includes the (new) list of databases we intend to
 * request copies of (again, so that we know what to do if we crash in the
 * middle).
 *
 * For the sake of simplicity, these database lists are in the form of an UPDATE
 * message (since we already have the mechanisms in place), even though strictly
 * speaking that contains more information than we really need to store.
 *
 * !!! Must be called with the REP_SYSTEM_LOCK held.
 */
static int
__rep_remove_all(env, msg_version, rec)
	ENV *env;
	u_int32_t msg_version;
	DBT *rec;
{
	FILE_LIST_CTX context;
	__rep_fileinfo_args *finfo;
	__rep_update_args u_args;
	DB_FH *fhp;
	DB_REP *db_rep;
	REP *rep;
	size_t cnt, max, updlen;
	u_int32_t bufsz, fvers, mvers, zero;
	u_int8_t *fp;
	int ret, t_ret;
	char *fname;

	finfo = NULL;
	fname = NULL;
	fhp = NULL;
	db_rep = env->rep_handle;
	rep = db_rep->region;

	/*
	 * 1. Get list of databases currently present at this client, which we
	 *    intend to remove.
	 */
	if ((ret = __os_calloc(env, 1, MEGABYTE, &context.buf)) != 0)
		return (ret);
	context.size = MEGABYTE;
	context.count = 0;

	/* Reserve space for the marshaled update_args. */
	context.fillptr = FIRST_FILE_PTR(context.buf);

	if ((ret = __rep_find_dbs(env, DB_REPVERSION, &context)) != 0)
		goto out;
	ZERO_LSN(u_args.first_lsn);
	u_args.first_vers = 0;
	u_args.num_files = context.count;
	if ((ret = __rep_update_marshal(env, DB_REPVERSION,
	    &u_args, context.buf, __REP_UPDATE_SIZE, &updlen)) != 0)
		goto out;
	DB_ASSERT(env, updlen == __REP_UPDATE_SIZE);

	/*
	 * 2. Before removing anything, safe-store the database list, so that in
	 *    case we crash before we've removed them all, when we restart we
	 *    can clean up what we were doing. Only write database list to
	 *    file if not running in-memory replication.
	 *
	 * The original version of the file contains:
	 * data1 size (4 bytes)
	 * data1
	 * data2 size (possibly) (4 bytes)
	 * data2 (possibly)
	 *
	 * As of 4.7 the file has the following form:
	 * 0 (4 bytes - to indicate a new style file)
	 * file version (4 bytes)
	 * data1 version (4 bytes)
	 * data1 size (4 bytes)
	 * data1
	 * data2 version (possibly) (4 bytes)
	 * data2 size (possibly) (4 bytes)
	 * data2 (possibly)
	 */
	if (!FLD_ISSET(rep->config, REP_C_INMEM)) {
		if ((ret = __db_appname(env,
		    DB_APP_NONE, REP_INITNAME, NULL, &fname)) != 0)
			goto out;
		/* Sanity check that the write size fits into 32 bits. */
		DB_ASSERT(env, (size_t)(context.fillptr - context.buf) ==
		    (u_int32_t)(context.fillptr - context.buf));
		bufsz = (u_int32_t)(context.fillptr - context.buf);

		/*
		 * (Short writes aren't possible, so we don't have to verify
		 * 'cnt'.) This first list is generated internally, so it is
		 * always in the form of the current message version.
		 */
		zero = 0;
		fvers = REP_INITVERSION;
		mvers = DB_REPVERSION;
		if ((ret = __os_open(env, fname, 0,
		    DB_OSO_CREATE | DB_OSO_TRUNC, DB_MODE_600, &fhp)) != 0 ||
		    (ret =
		    __os_write(env, fhp, &zero, sizeof(zero), &cnt)) != 0 ||
		    (ret =
		    __os_write(env, fhp, &fvers, sizeof(fvers), &cnt)) != 0 ||
		    (ret =
		    __os_write(env, fhp, &mvers, sizeof(mvers), &cnt)) != 0 ||
		    (ret =
		    __os_write(env, fhp, &bufsz, sizeof(bufsz), &cnt)) != 0 ||
		    (ret =
		    __os_write(env, fhp, context.buf, bufsz, &cnt)) != 0 ||
		    (ret = __os_fsync(env, fhp)) != 0) {
			__db_err(env, ret, "%s", fname);
			goto out;
		}
	}

	/*
	 * 3. Go ahead and remove logs and databases.  The databases get removed
	 *    according to the list we just finished safe-storing.
	 *
	 * Clearing NIMDBS_LOADED might not really be necessary, since once
	 * we've committed to removing all there's no chance of doing an
	 * abbreviated internal init.  This just keeps us honest.
	 */
	if ((ret = __rep_remove_logs(env)) != 0)
		goto out;
	if ((ret = __rep_closefiles(env)) != 0)
		goto out;
	F_CLR(rep, REP_F_NIMDBS_LOADED);
	fp = FIRST_FILE_PTR(context.buf);
	while (context.count-- > 0) {
		max = (size_t)(context.fillptr - fp);
		if ((ret = __rep_fileinfo_unmarshal(env, DB_REPVERSION,
		    &finfo, fp, max, &fp)) != 0)
			goto out;
		if ((ret = __rep_remove_file(env, finfo->uid.data,
		    finfo->info.data, finfo->type, finfo->db_flags)) != 0)
			goto out;
		__os_free(env, finfo);
		finfo = NULL;
	}

	/*
	 * 4. Safe-store the (new) list of database files we intend to copy from
	 *    the master (again, so that in case we crash before we're finished
	 *    doing so, we'll have enough information to clean up and start over
	 *    again).  This list is the list from the master, so it uses
	 *    the message version. Only write to file if not running
	 *    in-memory replication.
	 */
	if (!FLD_ISSET(rep->config, REP_C_INMEM)) {
		mvers = msg_version;
		if ((ret =
		    __os_write(env, fhp, &mvers, sizeof(mvers), &cnt)) != 0 ||
		    (ret = __os_write(env, fhp,
		    &rec->size, sizeof(rec->size), &cnt)) != 0 ||
		    (ret =
		    __os_write(env, fhp, rec->data, rec->size, &cnt)) != 0 ||
		    (ret = __os_fsync(env, fhp)) != 0) {
			__db_err(env, ret, "%s", fname);
			goto out;
		}
	}

out:
	if (fhp != NULL && (t_ret = __os_closehandle(env, fhp)) && ret == 0)
		ret = t_ret;
	if (fname != NULL)
		__os_free(env, fname);
	if (finfo != NULL)
		__os_free(env, finfo);
	__os_free(env, context.buf);
	return (ret);
}

/*
 * __rep_remove_logs -
 *	Remove our logs to prepare for internal init.
 */
static int
__rep_remove_logs(env)
	ENV *env;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	LOG *lp;
	u_int32_t fnum, lastfile;
	int ret;
	char *name;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	ret = 0;

	/*
	 * Call memp_sync to flush any pages that might be in the log buffers
	 * and not on disk before we remove files on disk.  If there were no
	 * dirty pages, the log isn't flushed.  Yet the log buffers could still
	 * be dirty: __log_flush should take care of this rare situation.
	 */
	if ((ret = __memp_sync_int(env,
	    NULL, 0, DB_SYNC_CACHE | DB_SYNC_INTERRUPT_OK, NULL, NULL)) != 0)
		return (ret);
	if ((ret = __log_flush(env, NULL)) != 0)
		return (ret);
	/*
	 * Forcibly remove existing log files or reset
	 * the in-memory log space.
	 */
	if (lp->db_log_inmemory) {
		ZERO_LSN(lsn);
		if ((ret = __log_zero(env, &lsn)) != 0)
			return (ret);
	} else {
		lastfile = lp->lsn.file;
		for (fnum = 1; fnum <= lastfile; fnum++) {
			if ((ret = __log_name(dblp, fnum, &name, NULL, 0)) != 0)
				return (ret);
			(void)time(&lp->timestamp);
			(void)__os_unlink(env, name, 0);
			__os_free(env, name);
		}
	}
	return (0);
}

/*
 * Removes a file during internal init.  Assumes underlying subsystems are
 * active; therefore, this can't be used for internal init crash recovery.
 */
static int
__rep_remove_file(env, uid, name, type, flags)
	ENV *env;
	u_int8_t *uid;
	const char *name;
	u_int32_t type, flags;
{
	DB *dbp;
#ifdef HAVE_QUEUE
	DB_THREAD_INFO *ip;
#endif
	int ret, t_ret;

	dbp = NULL;

	/*
	 * Calling __fop_remove will both purge any matching
	 * fileid from mpool and unlink it on disk.
	 */
#ifdef HAVE_QUEUE
	/*
	 * Handle queue separately.  __fop_remove will not
	 * remove extent files.  Use __qam_remove to remove
	 * extent files that might exist under this name.  Note that
	 * in-memory queue databases can't have extent files.
	 */
	if (type == (u_int32_t)DB_QUEUE && !LF_ISSET(DB_AM_INMEM)) {
		if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
			return (ret);

		/*
		 * At present, qam_remove expects the passed-in dbp to have a
		 * locker allocated, and if not, db_open allocates a locker
		 * which qam_remove then leaks.
		 *
		 * TODO: it would be better to avoid cobbling together this
		 * sequence of low-level operations, if fileops provided some
		 * API to allow us to remove a database without write-locking
		 * its handle.
		 */
		if ((ret = __lock_id(env, NULL, &dbp->locker)) != 0)
			goto out;

		ENV_GET_THREAD_INFO(env, ip);
		RPRINT(env, DB_VERB_REP_SYNC,
		    (env, "QAM: Unlink %s via __qam_remove", name));
		if ((ret = __qam_remove(dbp, ip, NULL, name, NULL, 0)) != 0) {
			RPRINT(env, DB_VERB_REP_SYNC,
			    (env, "qam_remove returned %d", ret));
			goto out;
		}
	}
#else
	COMPQUIET(type, 0);
#endif
	/*
	 * We call fop_remove even if we've called qam_remove.
	 * That will only have removed extent files.  Now
	 * we need to deal with the actual file itself.
	 */
	if (LF_ISSET(DB_AM_INMEM)) {
		if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
			return (ret);
		MAKE_INMEM(dbp);
		F_SET(dbp, DB_AM_RECOVER); /* Skirt locking. */
		ret = __db_inmem_remove(dbp, NULL, name);
	} else
		ret = __fop_remove(env, NULL, uid, name, NULL, DB_APP_DATA, 0);
#ifdef HAVE_QUEUE
out:
#endif
	if (dbp != NULL &&
	    (t_ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_bulk_page
 *	Process a bulk page message.
 *
 * PUBLIC: int __rep_bulk_page __P((ENV *,
 * PUBLIC:     DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
 */
int
__rep_bulk_page(env, ip, eid, rp, rec)
	ENV *env;
	DB_THREAD_INFO *ip;
	int eid;
	__rep_control_args *rp;
	DBT *rec;
{
	__rep_control_args tmprp;
	__rep_bulk_args b_args;
	int ret;
	u_int8_t *p, *ep;

	/*
	 * We're going to be modifying the rp LSN contents so make
	 * our own private copy to play with.  We need to set the
	 * rectype to REP_PAGE because we're calling through __rep_page
	 * to process each page, and lower functions make decisions
	 * based on the rectypes (for throttling/gap processing)
	 */
	memcpy(&tmprp, rp, sizeof(tmprp));
	tmprp.rectype = REP_PAGE;
	ret = 0;
	for (ep = (u_int8_t *)rec->data + rec->size, p = (u_int8_t *)rec->data;
	    p < ep;) {
		/*
		 * First thing in the buffer is the length.  Then the LSN
		 * of this page, then the page info itself.
		 */
		if ((ret = __rep_bulk_unmarshal(env,
		    &b_args, p, rec->size, &p)) != 0)
			return (ret);
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "rep_bulk_page: Processing LSN [%lu][%lu]",
		    (u_long)tmprp.lsn.file, (u_long)tmprp.lsn.offset));
		RPRINT(env, DB_VERB_REP_SYNC, (env,
    "rep_bulk_page: p %#lx ep %#lx pgrec data %#lx, size %lu (%#lx)",
		    P_TO_ULONG(p), P_TO_ULONG(ep),
		    P_TO_ULONG(b_args.bulkdata.data),
		    (u_long)b_args.bulkdata.size,
		    (u_long)b_args.bulkdata.size));
		/*
		 * Now send the page info DBT to the page processing function.
		 */
		ret = __rep_page(env, ip, eid, &tmprp, &b_args.bulkdata);
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "rep_bulk_page: rep_page ret %d", ret));

		/*
		 * If this set of pages is already done just return.
		 */
		if (ret != 0) {
			if (ret == DB_REP_PAGEDONE)
				ret = 0;
			break;
		}
	}
	return (ret);
}

/*
 * __rep_page
 *	Process a page message.
 *
 * PUBLIC: int __rep_page __P((ENV *,
 * PUBLIC:     DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
 */
int
__rep_page(env, ip, eid, rp, rec)
	ENV *env;
	DB_THREAD_INFO *ip;
	int eid;
	__rep_control_args *rp;
	DBT *rec;
{

	DB_REP *db_rep;
	DBT key, data;
	REP *rep;
	__rep_fileinfo_args *msgfp;
	db_recno_t recno;
	int ret;

	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;

	if (!F_ISSET(rep, REP_F_RECOVER_PAGE))
		return (DB_REP_PAGEDONE);
	/*
	 * If we restarted internal init, it is possible to receive
	 * an old REP_PAGE message, while we're in the current
	 * stage of recovering pages.  Until we have some sort of
	 * an init generation number, ignore any message that has
	 * a message LSN that is before this internal init's first_lsn.
	 */
	if (LOG_COMPARE(&rp->lsn, &rep->first_lsn) < 0) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "PAGE: Old page: msg LSN [%lu][%lu] first_lsn [%lu][%lu]",
		    (u_long)rp->lsn.file, (u_long)rp->lsn.offset,
		    (u_long)rep->first_lsn.file,
		    (u_long)rep->first_lsn.offset));
		return (DB_REP_PAGEDONE);
	}
	if ((ret = __rep_fileinfo_unmarshal(env, rp->rep_version,
	    &msgfp, rec->data, rec->size, NULL)) != 0)
		return (ret);
	MUTEX_LOCK(env, rep->mtx_clientdb);
	REP_SYSTEM_LOCK(env);
	/*
	 * Check if the world changed.
	 */
	if (!F_ISSET(rep, REP_F_RECOVER_PAGE)) {
		ret = DB_REP_PAGEDONE;
		goto err;
	}
	/*
	 * We should not ever be in internal init with a lease granted.
	 */
	DB_ASSERT(env,
	    !IS_USING_LEASES(env) || __rep_islease_granted(env) == 0);

	RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "PAGE: Received page %lu from file %d",
	    (u_long)msgfp->pgno, msgfp->filenum));
	/*
	 * Check if this page is from the file we're expecting.
	 * This may be an old or delayed page message.
	 */
	/*
	 * !!!
	 * If we allow dbrename/dbremove on the master while a client
	 * is updating, then we'd have to verify the file's uid here too.
	 */
	if (msgfp->filenum != rep->curfile) {
		RPRINT(env, DB_VERB_REP_SYNC,
		    (env, "Msg file %d != curfile %d",
		    msgfp->filenum, rep->curfile));
		ret = DB_REP_PAGEDONE;
		goto err;
	}
	/*
	 * We want to create/open our dbp to the database
	 * where we'll keep our page information.
	 */
	if ((ret = __rep_client_dbinit(env, 1, REP_PG)) != 0) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "PAGE: Client_dbinit %s", db_strerror(ret)));
		goto err;
	}

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	recno = (db_recno_t)(msgfp->pgno + 1);
	key.data = &recno;
	key.ulen = key.size = sizeof(db_recno_t);
	key.flags = DB_DBT_USERMEM;

	/*
	 * If we already have this page, then we don't want to bother
	 * rewriting it into the file.  Otherwise, any other error
	 * we want to return.
	 */
	ret = __db_put(rep->file_dbp, ip, NULL, &key, &data, DB_NOOVERWRITE);
	if (ret == DB_KEYEXIST) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "PAGE: Received duplicate page %lu from file %d",
		    (u_long)msgfp->pgno, msgfp->filenum));
		STAT(rep->stat.st_pg_duplicated++);
		ret = 0;
		goto err;
	}
	if (ret != 0)
		goto err;

	RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "PAGE: Write page %lu into mpool", (u_long)msgfp->pgno));
	/*
	 * We put the page in the database file itself.
	 */
	ret = __rep_write_page(env, ip, rep, msgfp);
	if (ret != 0) {
		/*
		 * We got an error storing the page, therefore, we need
		 * remove this page marker from the page database too.
		 * !!!
		 * I'm ignoring errors from the delete because we want to
		 * return the original error.  If we cannot write the page
		 * and we cannot delete the item we just put, what should
		 * we do?  Panic the env and return DB_RUNRECOVERY?
		 */
		(void)__db_del(rep->file_dbp, NULL, NULL, &key, 0);
		goto err;
	}
	STAT(rep->stat.st_pg_records++);
	rep->npages++;

	/*
	 * Now check the LSN on the page and save it if it is later
	 * than the one we have.
	 */
	if (LOG_COMPARE(&rp->lsn, &rep->last_lsn) > 0)
		rep->last_lsn = rp->lsn;

	/*
	 * We've successfully written the page.  Now we need to see if
	 * we're done with this file.  __rep_filedone will check if we
	 * have all the pages expected and if so, set up for the next
	 * file and send out a page request for the next file's pages.
	 */
	ret = __rep_filedone(env, ip, eid, rep, msgfp, rp->rectype);

err:	REP_SYSTEM_UNLOCK(env);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);

	__os_free(env, msgfp);
	return (ret);
}

/*
 * __rep_page_fail
 *	Process a page fail message.
 *
 * PUBLIC: int __rep_page_fail __P((ENV *,
 * PUBLIC:     DB_THREAD_INFO *, int, __rep_control_args *, DBT *));
 */
int
__rep_page_fail(env, ip, eid, rp, rec)
	ENV *env;
	DB_THREAD_INFO *ip;
	int eid;
	__rep_control_args *rp;
	DBT *rec;
{

	DB_REP *db_rep;
	REP *rep;
	__rep_fileinfo_args *msgfp, *rfp;
	int ret;

	ret = 0;
	db_rep = env->rep_handle;
	rep = db_rep->region;

	if (!F_ISSET(rep, REP_F_RECOVER_PAGE))
		return (0);
	if ((ret = __rep_fileinfo_unmarshal(env, rp->rep_version,
	    &msgfp, rec->data, rec->size, NULL)) != 0)
		return (ret);
	/*
	 * Check if this page is from the file we're expecting.
	 * This may be an old or delayed page message.
	 */
	/*
	 * !!!
	 * If we allow dbrename/dbremove on the master while a client
	 * is updating, then we'd have to verify the file's uid here too.
	 */
	MUTEX_LOCK(env, rep->mtx_clientdb);
	REP_SYSTEM_LOCK(env);
	/*
	 * We should not ever be in internal init with a lease granted.
	 */
	DB_ASSERT(env,
	    !IS_USING_LEASES(env) || __rep_islease_granted(env) == 0);

	if (msgfp->filenum != rep->curfile) {
		RPRINT(env, DB_VERB_REP_SYNC,
		    (env, "Msg file %d != curfile %d",
		    msgfp->filenum, rep->curfile));
		goto out;
	}
	rfp = rep->curinfo;
	if (rfp->type != (u_int32_t)DB_QUEUE)
		--rfp->max_pgno;
	else {
		/*
		 * Queue is special.  Pages at the beginning of the queue
		 * may disappear, as well as at the end.  Use msgfp->pgno
		 * to adjust accordingly.
		 */
		RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "page_fail: BEFORE page %lu failed. ready %lu, max %lu, npages %d",
		    (u_long)msgfp->pgno, (u_long)rep->ready_pg,
		    (u_long)rfp->max_pgno, rep->npages));
		if (msgfp->pgno == rfp->max_pgno)
			--rfp->max_pgno;
		if (msgfp->pgno >= rep->ready_pg) {
			rep->ready_pg = msgfp->pgno + 1;
			rep->npages = rep->ready_pg;
		}
		RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "page_fail: AFTER page %lu failed. ready %lu, max %lu, npages %d",
		    (u_long)msgfp->pgno, (u_long)rep->ready_pg,
		    (u_long)rfp->max_pgno, rep->npages));
	}

	/*
	 * We've lowered the number of pages expected.  It is possible that
	 * this was the last page we were expecting.  Now we need to see if
	 * we're done with this file.  __rep_filedone will check if we have
	 * all the pages expected and if so, set up for the next file and
	 * send out a page request for the next file's pages.
	 */
	ret = __rep_filedone(env, ip, eid, rep, msgfp, REP_PAGE_FAIL);
out:
	REP_SYSTEM_UNLOCK(env);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	__os_free(env, msgfp);
	return (ret);
}

/*
 * __rep_write_page -
 *	Write this page into a database.
 */
static int
__rep_write_page(env, ip, rep, msgfp)
	ENV *env;
	DB_THREAD_INFO *ip;
	REP *rep;
	__rep_fileinfo_args *msgfp;
{
	DB db;
	DBT pgcookie;
	DB_MPOOLFILE *mpf;
	DB_PGINFO *pginfo;
	__rep_fileinfo_args *rfp;
	int ret;
	void *dst;

	rfp = NULL;

	/*
	 * If this is the first page we're putting in this database, we need
	 * to create the mpool file.  Otherwise call memp_fget to create the
	 * page in mpool.  Then copy the data to the page, and memp_fput the
	 * page to give it back to mpool.
	 *
	 * We need to create the file, removing any existing file and associate
	 * the correct file ID with the new one.
	 */
	rfp = rep->curinfo;
	if (rep->file_mpf == NULL) {
		if (!FLD_ISSET(rfp->db_flags, DB_AM_INMEM)) {
			/*
			 * Recreate the file on disk.  We'll be putting
			 * the data into the file via mpool.
			 */
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			    "rep_write_page: Calling fop_create for %s",
			    (char *)rfp->info.data));
			if ((ret = __fop_create(env, NULL, NULL,
			    rfp->info.data, NULL, DB_APP_DATA,
			    env->db_mode, 0)) != 0)
				goto err;
		}

		if ((ret =
		    __rep_mpf_open(env, &rep->file_mpf, rep->curinfo,
		    FLD_ISSET(rfp->db_flags, DB_AM_INMEM) ?
		    DB_CREATE : 0)) != 0)
			goto err;
	}
	/*
	 * Handle queue specially.  If we're a QUEUE database, we need to
	 * use the __qam_fget/put calls.  We need to use rep->queue_dbc for
	 * that.  That dbp is opened after getting the metapage for the
	 * queue database.  Since the meta-page is always in the queue file,
	 * we'll use the normal path for that first page.  After that we
	 * can assume the dbp is opened.
	 */
	if (msgfp->type == (u_int32_t)DB_QUEUE && msgfp->pgno != 0) {
#ifdef HAVE_QUEUE
		ret = __qam_fget(rep->queue_dbc, &msgfp->pgno,
		    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &dst);
#else
		/*
		 * This always returns an error.
		 */
		ret = __db_no_queue_am(env);
#endif
	} else
		ret = __memp_fget(rep->file_mpf, &msgfp->pgno, ip, NULL,
		    DB_MPOOL_CREATE | DB_MPOOL_DIRTY, &dst);

	if (ret != 0)
		goto err;

	/*
	 * Before writing this page into our local mpool, see if its byte order
	 * needs to be swapped.  When in mpool the page should be in the native
	 * byte order of our local environment.  But the page image we've
	 * received may be in the opposite order (as indicated in finfo_flags).
	 */
	if ((F_ISSET(env, ENV_LITTLEENDIAN) &&
	    !FLD_ISSET(msgfp->finfo_flags, REPINFO_PG_LITTLEENDIAN)) ||
	    (!F_ISSET(env, ENV_LITTLEENDIAN) &&
	    FLD_ISSET(msgfp->finfo_flags, REPINFO_PG_LITTLEENDIAN))) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "write_page: Page %d needs to be swapped", msgfp->pgno));
		/*
		 * Set up a dbp to pass into the swap functions.  We need
		 * only a few things:  The environment and any special
		 * dbp flags and some obvious basics like db type and
		 * pagesize.  Those flags were set back in rep_mpf_open
		 * and are available in the pgcookie set up with the
		 * mpoolfile associated with this database.
		 */
		memset(&db, 0, sizeof(db));
		db.env = env;
		db.type = (DBTYPE)msgfp->type;
		db.pgsize = msgfp->pgsize;
		mpf = rep->file_mpf;
		if ((ret = __memp_get_pgcookie(mpf, &pgcookie)) != 0)
			goto err;
		pginfo = (DB_PGINFO *)pgcookie.data;
		db.flags = pginfo->flags;
		if ((ret = __db_pageswap(&db, msgfp->info.data, msgfp->pgsize,
		    NULL, 1)) != 0)
			goto err;
	}

	memcpy(dst, msgfp->info.data, msgfp->pgsize);
#ifdef HAVE_QUEUE
	if (msgfp->type == (u_int32_t)DB_QUEUE && msgfp->pgno != 0)
		ret = __qam_fput(rep->queue_dbc,
		     msgfp->pgno, dst, rep->queue_dbc->priority);
	else
#endif
		ret = __memp_fput(rep->file_mpf,
		    ip, dst, rep->file_dbp->priority);

err:	return (ret);
}

/*
 * __rep_page_gap -
 *	After we've put the page into the database, we need to check if
 *	we have a page gap and whether we need to request pages.
 */
static int
__rep_page_gap(env, rep, msgfp, type)
	ENV *env;
	REP *rep;
	__rep_fileinfo_args *msgfp;
	u_int32_t type;
{
	DBC *dbc;
	DBT data, key;
	DB_LOG *dblp;
	DB_THREAD_INFO *ip;
	LOG *lp;
	__rep_fileinfo_args *rfp;
	db_recno_t recno;
	int ret, t_ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	ret = 0;
	dbc = NULL;

	/*
	 * We've successfully put this page into our file.
	 * Now we need to account for it and re-request new pages
	 * if necessary.
	 */
	/*
	 * We already hold both the db mutex and rep mutex.
	 */
	rfp = rep->curinfo;

	/*
	 * Make sure we're still talking about the same file.
	 * If not, we're done here.
	 */
	if (rfp->filenum != msgfp->filenum) {
		ret = DB_REP_PAGEDONE;
		goto err;
	}

	/*
	 * We have 3 possible states:
	 * 1.  We receive a page we already have accounted for.
	 *	msg pgno < ready pgno
	 * 2.  We receive a page that is beyond a gap.
	 *	msg pgno > ready pgno
	 * 3.  We receive the page we're expecting next.
	 *	msg pgno == ready pgno
	 */
	/*
	 * State 1.  This can happen once we put our page record into the
	 * database, but by the time we acquire the mutex other
	 * threads have already accounted for this page and moved on.
	 * We just want to return.
	 */
	if (msgfp->pgno < rep->ready_pg) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "PAGE_GAP: pgno %lu < ready %lu, waiting %lu",
		    (u_long)msgfp->pgno, (u_long)rep->ready_pg,
		    (u_long)rep->waiting_pg));
		goto err;
	}

	/*
	 * State 2.  This page is beyond the page we're expecting.
	 * We need to update waiting_pg if this page is less than
	 * (earlier) the current waiting_pg.  There is nothing
	 * to do but see if we need to request.
	 */
	RPRINT(env, DB_VERB_REP_SYNC, (env,
    "PAGE_GAP: pgno %lu, max_pg %lu ready %lu, waiting %lu max_wait %lu",
	    (u_long)msgfp->pgno, (u_long)rfp->max_pgno, (u_long)rep->ready_pg,
	    (u_long)rep->waiting_pg, (u_long)rep->max_wait_pg));
	if (msgfp->pgno > rep->ready_pg) {
		if (rep->waiting_pg == PGNO_INVALID ||
		    msgfp->pgno < rep->waiting_pg)
			rep->waiting_pg = msgfp->pgno;
	} else {
		/*
		 * We received the page we're expecting.
		 */
		rep->ready_pg++;
		__os_gettime(env, &lp->rcvd_ts, 1);
		if (rep->ready_pg == rep->waiting_pg) {
			/*
			 * If we get here we know we just filled a gap.
			 * Move the cursor to that place and then walk
			 * forward looking for the next gap, if it exists.
			 */
			lp->wait_ts = rep->request_gap;
			rep->max_wait_pg = PGNO_INVALID;
			/*
			 * We need to walk the recno database looking for the
			 * next page we need or expect.
			 */
			memset(&key, 0, sizeof(key));
			memset(&data, 0, sizeof(data));
			ENV_GET_THREAD_INFO(env, ip);
			if ((ret = __db_cursor(rep->file_dbp, ip, NULL,
			    &dbc, 0)) != 0)
				goto err;
			/*
			 * Set cursor to the first waiting page.
			 * Page numbers/record numbers are offset by 1.
			 */
			recno = (db_recno_t)rep->waiting_pg + 1;
			key.data = &recno;
			key.ulen = key.size = sizeof(db_recno_t);
			key.flags = DB_DBT_USERMEM;
			/*
			 * We know that page is there, this should
			 * find the record.
			 */
			ret = __dbc_get(dbc, &key, &data, DB_SET);
			if (ret != 0)
				goto err;
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			    "PAGE_GAP: Set cursor for ready %lu, waiting %lu",
			    (u_long)rep->ready_pg, (u_long)rep->waiting_pg));
		}
		while (ret == 0 && rep->ready_pg == rep->waiting_pg) {
			rep->ready_pg++;
			ret = __dbc_get(dbc, &key, &data, DB_NEXT);
			/*
			 * If we get to the end of the list, there are no
			 * more gaps.  Reset waiting_pg.
			 */
			if (ret == DB_NOTFOUND || ret == DB_KEYEMPTY) {
				rep->waiting_pg = PGNO_INVALID;
				RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "PAGE_GAP: Next cursor No next - ready %lu, waiting %lu",
				    (u_long)rep->ready_pg,
				    (u_long)rep->waiting_pg));
				break;
			}
			/*
			 * Subtract 1 from waiting_pg because record numbers
			 * are 1-based and pages are 0-based and we added 1
			 * into the page number when we put it into the db.
			 */
			rep->waiting_pg = *(db_pgno_t *)key.data;
			rep->waiting_pg--;
			RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "PAGE_GAP: Next cursor ready %lu, waiting %lu",
			    (u_long)rep->ready_pg, (u_long)rep->waiting_pg));
		}
	}

	/*
	 * If we filled a gap and now have the entire file, there's
	 * nothing to do.  We're done when ready_pg is > max_pgno
	 * because ready_pg is larger than the last page we received.
	 */
	if (rep->ready_pg > rfp->max_pgno)
		goto err;

	/*
	 * Check if we need to ask for more pages.
	 */
	if ((rep->waiting_pg != PGNO_INVALID &&
	    rep->ready_pg != rep->waiting_pg) || type == REP_PAGE_MORE) {
		/*
		 * We got a page but we may still be waiting for more.
		 * If we got REP_PAGE_MORE we always want to ask for more.
		 * We need to set rfp->pgno to the current page number
		 * we will use to ask for more pages.
		 */
		if (type == REP_PAGE_MORE)
			rfp->pgno = msgfp->pgno;
		if ((__rep_check_doreq(env, rep) || type == REP_PAGE_MORE) &&
		    ((ret = __rep_pggap_req(env, rep, rfp,
		    (type == REP_PAGE_MORE) ? REP_GAP_FORCE : 0)) != 0))
			goto err;
	} else {
		lp->wait_ts = rep->request_gap;
		rep->max_wait_pg = PGNO_INVALID;
	}

err:
	if (dbc != NULL && (t_ret = __dbc_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __rep_init_cleanup -
 *	Clean up internal initialization pieces.
 *
 * !!!
 * Caller must hold client database mutex (mtx_clientdb) and REP_SYSTEM_LOCK.
 *
 * PUBLIC: int __rep_init_cleanup __P((ENV *, REP *, int));
 */
int
__rep_init_cleanup(env, rep, force)
	ENV *env;
	REP *rep;
	int force;
{
	DB *queue_dbp;
	int ret, t_ret;

	ret = 0;
	/*
	 * 1.  Close up the file data pointer we used.
	 * 2.  Close/reset the page database.
	 * 3.  Close/reset the queue database if we're forcing a cleanup.
	 * 4.  Free current file info.
	 * 5.  If we have all files or need to force, free original file info.
	 */
	if (rep->file_mpf != NULL) {
		ret = __memp_fclose(rep->file_mpf, 0);
		rep->file_mpf = NULL;
	}
	if (rep->file_dbp != NULL) {
		t_ret = __db_close(rep->file_dbp, NULL, DB_NOSYNC);
		rep->file_dbp = NULL;
		if (ret == 0)
			ret = t_ret;
	}
	if (force && rep->queue_dbc != NULL) {
		queue_dbp = rep->queue_dbc->dbp;
		if ((t_ret = __dbc_close(rep->queue_dbc)) != 0 && ret == 0)
			ret = t_ret;
		rep->queue_dbc = NULL;
		if ((t_ret = __db_close(queue_dbp, NULL, DB_NOSYNC)) != 0 &&
		    ret == 0)
			ret = t_ret;
	}
	if (rep->curinfo != NULL) {
		__os_free(env, rep->curinfo);
		rep->curinfo = NULL;
	}
	if (IN_INTERNAL_INIT(rep) && force) {
		RPRINT(env, DB_VERB_REP_SYNC,
		    (env, "clean up interrupted internal init"));
		t_ret = F_ISSET(rep, REP_F_ABBREVIATED) ?
		    __rep_cleanup_nimdbs(env) :
		    __rep_clean_interrupted(env);
		if (ret == 0)
			ret = t_ret;

		if (rep->originfo != NULL) {
			__os_free(env, rep->originfo);
			rep->originfo = NULL;
		}
	}

	return (ret);
}

/*
 * Remove NIMDBs that may have been fully or partially loaded during an
 * abbreviated internal init, when the init gets interrupted.  At this point,
 * we know that any databases we have processed are listed in originfo.
 */
static int
__rep_cleanup_nimdbs(env)
	ENV *env;
{
	REP *rep;
	DB *dbp;
	__rep_fileinfo_args *rfp;
	u_int8_t *filelist, *new_fp;
	char *namep;
	u_int32_t count, filesz, version;
	int ret, t_ret;

	/* Use the saved file list from the original UPDATE message. */
	rep = env->rep_handle->region;
	version = rep->infoversion;
	filelist = rep->originfo;
	filesz = rep->originfolen;
	count = rep->nfiles;

	ret = 0;
	rfp = NULL;
	dbp = NULL;
	while (count-- > 0) {
		if ((ret = __rep_fileinfo_unmarshal(env, version,
		    &rfp, filelist, filesz, &new_fp)) != 0)
			goto out;
		filesz -= (u_int32_t)(new_fp - filelist);
		filelist = new_fp;

		if (FLD_ISSET(rfp->db_flags, DB_AM_INMEM)) {
			namep = rfp->info.data;

			if ((ret = __db_create_internal(&dbp, env, 0)) != 0)
				goto out;
			MAKE_INMEM(dbp);
			F_SET(dbp, DB_AM_RECOVER); /* Skirt locking. */

			/*
			 * Some of these "files" (actually NIMDBs) may not exist
			 * yet, simply because the interrupted abbreviated
			 * internal init had not yet progressed far enough to
			 * retrieve them.  So ENOENT is an acceptable outcome.
			 */
			if ((ret = __db_inmem_remove(dbp, NULL, namep)) != 0 &&
			    ret != ENOENT)
				goto out;
			ret = __db_close(dbp, NULL, DB_NOSYNC);
			dbp = NULL;
			if (ret != 0)
				goto out;
		}

		__os_free(env, rfp);
		rfp = NULL;
	}

out:
	if (rfp != NULL)
		__os_free(env, rfp);
	if (dbp != NULL &&
	    (t_ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * Clean up files involved in an interrupted internal init.
 */
static int
__rep_clean_interrupted(env)
	ENV *env;
{
	REP *rep;
	DB_LOG *dblp;
	LOG *lp;
	int ret, t_ret;

	rep = env->rep_handle->region;

	/*
	 * 1. logs
	 *   a) remove old log files
	 *   b) set up initial log file #1
	 * 2. database files
	 * 3. the "init file"
	 *
	 * Steps 1 and 2 can be attempted independently.  Step 1b is
	 * dependent on successful completion of 1a.
	 */

	/* Step 1a. */
	if ((ret = __rep_remove_logs(env)) == 0) {
		/*
		 * Since we have no logs, recover by making it look like
		 * the case when a new client first starts up, namely we
		 * have nothing but a fresh log file #1.  This is a
		 * little wasteful, since we may soon remove this log
		 * file again.  But it's insignificant in the context of
		 * interrupted internal init.
		 */
		dblp = env->lg_handle;
		lp = dblp->reginfo.primary;

		/* Step 1b. */
		ret = __rep_log_setup(env,
		    rep, 1, DB_LOGVERSION, &lp->ready_lsn);
	}

	/* Step 2. */
	if ((t_ret = __rep_remove_by_list(env, rep->infoversion,
	    rep->originfo, rep->originfolen, rep->nfiles)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Step 3 must not be done if anything fails along the way, because the
	 * init file's raison d'etre is to show that some files remain to be
	 * cleaned up.
	 */
	if (ret == 0)
		ret = __rep_remove_init_file(env);

	return (ret);
}

/*
 * __rep_filedone -
 *	We need to check if we're done with the current file after
 *	processing the current page.  Stat the database to see if
 *	we have all the pages.  If so, we need to clean up/close
 *	this one, set up for the next one, and ask for its pages,
 *	or if this is the last file, request the log records and
 *	move to the REP_RECOVER_LOG state.
 */
static int
__rep_filedone(env, ip, eid, rep, msgfp, type)
	ENV *env;
	DB_THREAD_INFO *ip;
	int eid;
	REP *rep;
	__rep_fileinfo_args *msgfp;
	u_int32_t type;
{
	__rep_fileinfo_args *rfp;
	int ret;

	/*
	 * We've put our page, now we need to do any gap processing
	 * that might be needed to re-request pages.
	 */
	ret = __rep_page_gap(env, rep, msgfp, type);
	/*
	 * The world changed while we were doing gap processing.
	 * We're done here.
	 */
	if (ret == DB_REP_PAGEDONE)
		return (0);

	rfp = rep->curinfo;
	/*
	 * max_pgno is 0-based and npages is 1-based, so we don't have
	 * all the pages until npages is > max_pgno.
	 */
	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "FILEDONE: have %lu pages. Need %lu.",
	    (u_long)rep->npages, (u_long)rfp->max_pgno + 1));
	if (rep->npages <= rfp->max_pgno)
		return (0);

	/*
	 * If we're queue and we think we have all the pages for this file,
	 * we need to do special queue processing.  Queue is handled in
	 * several stages.
	 */
	if (rfp->type == (u_int32_t)DB_QUEUE &&
	    ((ret = __rep_queue_filedone(env, ip, rep, rfp)) !=
	    DB_REP_PAGEDONE))
		return (ret);
	/*
	 * We have all the pages for this file.  Clean up.
	 */
	if ((ret = __rep_init_cleanup(env, rep, 0)) != 0)
		goto err;

	rep->curfile++;
	ret = __rep_nextfile(env, eid, rep);
err:
	return (ret);
}

/*
 * Starts requesting pages for the next file in the list (if any), or if not,
 * proceeds to the next stage: requesting logs.
 *
 * !!!
 * Called with REP_SYSTEM_LOCK held or both clientdb_mutex and REP_SYSTEM,
 * though we may drop REP_SYSTEM_LOCK momentarily in order to send
 * a LOG_REQ (but not a PAGE_REQ).
 */
static int
__rep_nextfile(env, eid, rep)
	ENV *env;
	int eid;
	REP *rep;
{
	DBT dbt;
	__rep_logreq_args lr_args;
	int ret;
	u_int8_t *buf, *info_ptr, lrbuf[__REP_LOGREQ_SIZE];
	size_t len, msgsz;

	/*
	 * Always direct the next request to the master (at least nominally),
	 * regardless of where the current response came from.  The application
	 * can always still redirect it to another client.
	 */
	if (rep->master_id != DB_EID_INVALID)
		eid = rep->master_id;

	while (rep->curfile < rep->nfiles) {
		/* Set curinfo to next file and examine it. */
		info_ptr = rep->nextinfo;
		if ((ret = __rep_fileinfo_unmarshal(env,
		    rep->infoversion, &rep->curinfo,
		    info_ptr, rep->infolen, &rep->nextinfo)) != 0) {
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			    "NEXTINFO: Fileinfo read: %s", db_strerror(ret)));
			return (ret);
		}
		rep->infolen -= (u_int32_t)(rep->nextinfo - info_ptr);

		/* Skip over regular DB's in "abbreviated" internal inits. */
		if (F_ISSET(rep, REP_F_ABBREVIATED) &&
		    !FLD_ISSET(rep->curinfo->db_flags, DB_AM_INMEM)) {
			RPRINT(env, DB_VERB_REP_SYNC, (env,
			    "Skipping file %d in abbreviated internal init",
			    rep->curinfo->filenum));
			__os_free(env, rep->curinfo);
			rep->curinfo = NULL;
			rep->curfile++;
			continue;
		}

		/* Request this file's pages. */
		DB_ASSERT(env, rep->curinfo->pgno == 0);
		rep->ready_pg = 0;
		rep->npages = 0;
		rep->waiting_pg = PGNO_INVALID;
		rep->max_wait_pg = PGNO_INVALID;
		memset(&dbt, 0, sizeof(dbt));
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Next file %d: pgsize %lu, maxpg %lu",
		    rep->curinfo->filenum, (u_long)rep->curinfo->pgsize,
		    (u_long)rep->curinfo->max_pgno));
		msgsz = __REP_FILEINFO_SIZE +
		    rep->curinfo->uid.size + rep->curinfo->info.size;
		if ((ret = __os_calloc(env, 1, msgsz, &buf)) != 0)
			return (ret);
		if ((ret = __rep_fileinfo_marshal(env, rep->infoversion,
		    rep->curinfo, buf, msgsz, &len)) != 0)
			return (ret);
		DB_INIT_DBT(dbt, buf, len);
		(void)__rep_send_message(env, eid, REP_PAGE_REQ,
		    NULL, &dbt, 0, DB_REP_ANYWHERE);
		__os_free(env, buf);

		return (0);
	}

	RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "NEXTFILE: have %d files.  RECOVER_LOG now", rep->nfiles));
	/*
	 * Move to REP_RECOVER_LOG state.
	 * Request logs.
	 */
	/*
	 * We need to do a sync here so that any later opens
	 * can find the file and file id.  We need to do it
	 * before we clear REP_F_RECOVER_PAGE so that we do not
	 * try to flush the log.
	 */
	if ((ret = __memp_sync_int(env, NULL, 0,
	    DB_SYNC_CACHE | DB_SYNC_INTERRUPT_OK, NULL, NULL)) != 0)
		return (ret);
	F_CLR(rep, REP_F_RECOVER_PAGE);
	F_SET(rep, REP_F_RECOVER_LOG);
	memset(&dbt, 0, sizeof(dbt));
	lr_args.endlsn = rep->last_lsn;
	if ((ret = __rep_logreq_marshal(env, &lr_args, lrbuf,
	    __REP_LOGREQ_SIZE, &len)) != 0)
		return (ret);
	DB_INIT_DBT(dbt, lrbuf, len);

	/*
	 * Get the logging subsystem ready to receive the first log record we
	 * are going to ask for.  In the case of a normal internal init, this is
	 * pretty simple, since we only deal in whole log files.  In the
	 * ABBREVIATED case we've already taken care of this, back when we
	 * processed the UPDATE message, because we had to do it by rolling back
	 * to a sync point at an arbitrary LSN.
	 */
	if (!F_ISSET(rep, REP_F_ABBREVIATED) &&
	    (ret = __rep_log_setup(env, rep,
	    rep->first_lsn.file, rep->first_vers, NULL)) != 0)
		return (ret);
	RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "NEXTFILE: LOG_REQ from LSN [%lu][%lu] to [%lu][%lu]",
	    (u_long)rep->first_lsn.file, (u_long)rep->first_lsn.offset,
	    (u_long)rep->last_lsn.file, (u_long)rep->last_lsn.offset));
	REP_SYSTEM_UNLOCK(env);
	(void)__rep_send_message(env, eid,
	    REP_LOG_REQ, &rep->first_lsn, &dbt, REPCTL_INIT, DB_REP_ANYWHERE);
	REP_SYSTEM_LOCK(env);
	return (0);
}

/*
 * Run a recovery, for the purpose of rolling back the client environment to a
 * specific sync point, in preparation for doing an abbreviated internal init
 * (materializing only NIMDBs, when we already have the on-disk DBs).
 *
 * REP_SYSTEM_LOCK should be held on entry, and will be held on exit, but we
 * drop it momentarily during the call.
 */
static int
__rep_rollback(env, lsnp)
	ENV *env;
	DB_LSN *lsnp;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	DB_THREAD_INFO *ip;
	DB_LSN trunclsn;
	int ret;
	u_int32_t unused;

	db_rep = env->rep_handle;
	rep = db_rep->region;
	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	ENV_GET_THREAD_INFO(env, ip);

	DB_ASSERT(env, F_ISSET(rep,
	    REP_F_READY_API | REP_F_READY_MSG | REP_F_READY_OP));

	REP_SYSTEM_UNLOCK(env);

	if ((ret = __rep_dorecovery(env, lsnp, &trunclsn)) != 0)
		goto errlock;

	MUTEX_LOCK(env, rep->mtx_clientdb);
	lp->ready_lsn = trunclsn;
	ZERO_LSN(lp->waiting_lsn);
	ZERO_LSN(lp->max_wait_lsn);
	lp->max_perm_lsn = *lsnp;
	lp->wait_ts = rep->request_gap;
	__os_gettime(env, &lp->rcvd_ts, 1);
	ZERO_LSN(lp->verify_lsn);

	if (db_rep->rep_db == NULL &&
	    (ret = __rep_client_dbinit(env, 0, REP_DB)) != 0) {
		MUTEX_UNLOCK(env, rep->mtx_clientdb);
		goto errlock;
	}

	F_SET(db_rep->rep_db, DB_AM_RECOVER);
	MUTEX_UNLOCK(env, rep->mtx_clientdb);
	ret = __db_truncate(db_rep->rep_db, ip, NULL, &unused);
	MUTEX_LOCK(env, rep->mtx_clientdb);
	F_CLR(db_rep->rep_db, DB_AM_RECOVER);
	rep->stat.st_log_queued = 0;
	MUTEX_UNLOCK(env, rep->mtx_clientdb);

errlock:
	REP_SYSTEM_LOCK(env);

	return (ret);
}

/*
 * __rep_mpf_open -
 *	Create and open the mpool file for a database.
 *	Used by both master and client to bring files into mpool.
 */
static int
__rep_mpf_open(env, mpfp, rfp, flags)
	ENV *env;
	DB_MPOOLFILE **mpfp;
	__rep_fileinfo_args *rfp;
	u_int32_t flags;
{
	DB db;
	int ret;

	if ((ret = __memp_fcreate(env, mpfp)) != 0)
		return (ret);

	/*
	 * We need a dbp to pass into to __env_mpool.  Set up
	 * only the parts that it needs.
	 */
	memset(&db, 0, sizeof(db));
	db.env = env;
	db.type = (DBTYPE)rfp->type;
	db.pgsize = rfp->pgsize;
	memcpy(db.fileid, rfp->uid.data, DB_FILE_ID_LEN);
	db.flags = rfp->db_flags;
	/* We need to make sure the dbp isn't marked open. */
	F_CLR(&db, DB_AM_OPEN_CALLED);
	/*
	 * The byte order of this database may be different from my local native
	 * byte order.  If so, set the swap bit so that the necessary swapping
	 * will be done during file I/O.
	 */
	if ((F_ISSET(env, ENV_LITTLEENDIAN) &&
	    !FLD_ISSET(rfp->finfo_flags, REPINFO_DB_LITTLEENDIAN)) ||
	    (!F_ISSET(env, ENV_LITTLEENDIAN) &&
	    FLD_ISSET(rfp->finfo_flags, REPINFO_DB_LITTLEENDIAN))) {
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "rep_mpf_open: Different endian database.  Set swap bit."));
		F_SET(&db, DB_AM_SWAP);
	} else
		F_CLR(&db, DB_AM_SWAP);

	db.mpf = *mpfp;
	if (F_ISSET(&db, DB_AM_INMEM))
		(void)__memp_set_flags(db.mpf, DB_MPOOL_NOFILE, 1);
	if ((ret = __env_mpool(&db, rfp->info.data, flags)) != 0) {
		(void)__memp_fclose(db.mpf, 0);
		*mpfp = NULL;
	}
	return (ret);
}

/*
 * __rep_pggap_req -
 *	Request a page gap.  Assumes the caller holds the rep_mutex.
 *
 * PUBLIC: int __rep_pggap_req __P((ENV *, REP *, __rep_fileinfo_args *,
 * PUBLIC:     u_int32_t));
 */
int
__rep_pggap_req(env, rep, reqfp, gapflags)
	ENV *env;
	REP *rep;
	__rep_fileinfo_args *reqfp;
	u_int32_t gapflags;
{
	DBT max_pg_dbt;
	__rep_fileinfo_args *tmpfp, t;
	size_t len, msgsz;
	u_int32_t flags;
	int alloc, master, ret;
	u_int8_t *buf;

	ret = 0;
	alloc = 0;
	/*
	 * There is a window where we have to set REP_RECOVER_PAGE when
	 * we receive the update information to transition from getting
	 * file information to getting page information.  However, that
	 * thread does release and then reacquire mutexes.  So, we might
	 * try re-requesting before the original thread can get curinfo
	 * setup.  If curinfo isn't set up there is nothing to do.
	 */
	if (rep->curinfo == NULL)
		return (0);
	if (reqfp == NULL) {
		if ((ret = __rep_finfo_alloc(env, rep->curinfo, &tmpfp)) != 0)
			return (ret);
		alloc = 1;
	} else {
		t = *reqfp;
		tmpfp = &t;
	}

	/*
	 * If we've never requested this page, then
	 * request everything between it and the first
	 * page we have.  If we have requested this page
	 * then only request this record, not the entire gap.
	 */
	flags = 0;
	memset(&max_pg_dbt, 0, sizeof(max_pg_dbt));
	/*
	 * If this is a PAGE_MORE and we're forcing then we want to
	 * force the request to ask for the next page after this one.
	 */
	if (FLD_ISSET(gapflags, REP_GAP_FORCE))
		tmpfp->pgno++;
	else
		tmpfp->pgno = rep->ready_pg;
	msgsz = __REP_FILEINFO_SIZE +
	    tmpfp->uid.size + tmpfp->info.size;
	if ((ret = __os_calloc(env, 1, msgsz, &buf)) != 0)
		goto err;
	if (rep->max_wait_pg == PGNO_INVALID ||
	    FLD_ISSET(gapflags, REP_GAP_FORCE | REP_GAP_REREQUEST)) {
		/*
		 * Request the gap - set max to waiting_pg - 1 or if
		 * there is no waiting_pg, just ask for one.
		 */
		if (rep->waiting_pg == PGNO_INVALID) {
			if (FLD_ISSET(gapflags,
			    REP_GAP_FORCE | REP_GAP_REREQUEST))
				rep->max_wait_pg = rep->curinfo->max_pgno;
			else
				rep->max_wait_pg = rep->ready_pg;
		} else {
			/*
			 * If we're forcing, and waiting_pg is less than
			 * the page we want to start this request at, then
			 * we set max_wait_pg to the max pgno in the file.
			 */
			if (FLD_ISSET(gapflags, REP_GAP_FORCE) &&
			  rep->waiting_pg < tmpfp->pgno)
				rep->max_wait_pg = rep->curinfo->max_pgno;
			else
				rep->max_wait_pg = rep->waiting_pg - 1;
		}
		tmpfp->max_pgno = rep->max_wait_pg;
		/*
		 * Gap requests are "new" and can go anywhere.
		 */
		if (FLD_ISSET(gapflags, REP_GAP_REREQUEST))
			flags = DB_REP_REREQUEST;
		else
			flags = DB_REP_ANYWHERE;
	} else {
		/*
		 * Request 1 page - set max to ready_pg.
		 */
		rep->max_wait_pg = rep->ready_pg;
		tmpfp->max_pgno = rep->ready_pg;
		/*
		 * If we're dropping to singletons, this is a rerequest.
		 */
		flags = DB_REP_REREQUEST;
	}
	if ((master = rep->master_id) != DB_EID_INVALID) {
		STAT(rep->stat.st_pg_requested++);
		/*
		 * We need to request the pages, but we need to get the
		 * new info into rep->finfo.  Assert that the sizes never
		 * change.  The only thing this should do is change
		 * the pgno field.  Everything else remains the same.
		 */
		if ((ret = __rep_fileinfo_marshal(env, rep->infoversion,
		    tmpfp, buf, msgsz, &len)) == 0) {
			DB_INIT_DBT(max_pg_dbt, buf, len);
			DB_ASSERT(env, len == max_pg_dbt.size);
			(void)__rep_send_message(env, master,
			    REP_PAGE_REQ, NULL, &max_pg_dbt, 0, flags);
		}
	} else
		(void)__rep_send_message(env, DB_EID_BROADCAST,
		    REP_MASTER_REQ, NULL, NULL, 0, 0);

	__os_free(env, buf);
err:
	if (alloc)
		__os_free(env, tmpfp);
	return (ret);
}

/*
 * __rep_finfo_alloc -
 *	Allocate and initialize a fileinfo structure.
 *
 * PUBLIC: int __rep_finfo_alloc __P((ENV *, __rep_fileinfo_args *,
 * PUBLIC:     __rep_fileinfo_args **));
 */
int
__rep_finfo_alloc(env, rfpsrc, rfpp)
	ENV *env;
	__rep_fileinfo_args *rfpsrc, **rfpp;
{
	__rep_fileinfo_args *rfp;
	size_t size;
	int ret;
	void *uidp, *infop;

	/*
	 * Allocate enough for the structure and the two DBT data areas.
	 */
	size = sizeof(__rep_fileinfo_args) + rfpsrc->uid.size +
	    rfpsrc->info.size;
	if ((ret = __os_malloc(env, size, &rfp)) != 0)
		return (ret);

	/*
	 * Copy the structure itself, and then set the DBT data pointers
	 * to their space and copy the data itself as well.
	 */
	memcpy(rfp, rfpsrc, sizeof(__rep_fileinfo_args));
	uidp = (u_int8_t *)rfp + sizeof(__rep_fileinfo_args);
	rfp->uid.data = uidp;
	memcpy(uidp, rfpsrc->uid.data, rfpsrc->uid.size);

	infop = (u_int8_t *)uidp + rfpsrc->uid.size;
	rfp->info.data = infop;
	memcpy(infop, rfpsrc->info.data, rfpsrc->info.size);
	*rfpp = rfp;
	return (ret);
}

/*
 * __rep_log_setup -
 *	We know our first LSN and need to reset the log subsystem
 *	to get our logs set up for the proper file.
 */
static int
__rep_log_setup(env, rep, file, version, lsnp)
	ENV *env;
	REP *rep;
	u_int32_t file;
	u_int32_t version;
	DB_LSN *lsnp;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	LOG *lp;
	int ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	mgr = env->tx_handle;
	region = mgr->reginfo.primary;

	/*
	 * Set up the log starting at the file number of the first LSN we
	 * need to get from the master.
	 */
	LOG_SYSTEM_LOCK(env);
	if ((ret = __log_newfile(dblp, &lsn, file, version)) == 0 &&
	    lsnp != NULL)
		*lsnp = lsn;
	LOG_SYSTEM_UNLOCK(env);

	/*
	 * We reset first_lsn to the lp->lsn.  We were given the LSN of
	 * the checkpoint and we now need the LSN for the beginning of
	 * the file, which __log_newfile conveniently set up for us
	 * in lp->lsn.
	 */
	rep->first_lsn = lp->lsn;
	TXN_SYSTEM_LOCK(env);
	ZERO_LSN(region->last_ckp);
	TXN_SYSTEM_UNLOCK(env);
	return (ret);
}

/*
 * __rep_queue_filedone -
 *	Determine if we're really done getting the pages for a queue file.
 *	Queue is handled in several steps.
 *	1.  First we get the meta page only.
 *	2.  We use the meta-page information to figure out first and last
 *	    page numbers (and if queue wraps, first can be > last.
 *	3.  If first < last, we do a REP_PAGE_REQ for all pages.
 *	4.  If first > last, we REP_PAGE_REQ from first -> max page number.
 *	    Then we'll ask for page 1 -> last.
 *
 * This function can return several things:
 *	DB_REP_PAGEDONE - if we're done with this file.
 *	0 - if we're not done with this file.
 *	error - if we get an error doing some operations.
 *
 * This function will open a dbp handle to the queue file.  This is needed
 * by most of the QAM macros.  We'll open it on the first pass through
 * here and we'll close it whenever we decide we're done.
 */
static int
__rep_queue_filedone(env, ip, rep, rfp)
	ENV *env;
	DB_THREAD_INFO *ip;
	REP *rep;
	__rep_fileinfo_args *rfp;
{
#ifndef HAVE_QUEUE
	COMPQUIET(ip, NULL);
	COMPQUIET(rep, NULL);
	COMPQUIET(rfp, NULL);
	return (__db_no_queue_am(env));
#else
	DB *queue_dbp;
	db_pgno_t first, last;
	u_int32_t flags;
	int empty, ret, t_ret;

	ret = 0;
	queue_dbp = NULL;
	if (rep->queue_dbc == NULL) {
		/*
		 * We need to do a sync here so that the open
		 * can find the file and file id.
		 */
		if ((ret = __memp_sync_int(env, NULL, 0,
		    DB_SYNC_CACHE | DB_SYNC_INTERRUPT_OK, NULL, NULL)) != 0)
			goto out;
		if ((ret =
		    __db_create_internal(&queue_dbp, env, 0)) != 0)
			goto out;
		flags = DB_NO_AUTO_COMMIT |
		    (F_ISSET(env, ENV_THREAD) ? DB_THREAD : 0);
		/*
		 * We need to check whether this is in-memory so that we pass
		 * the name correctly as either the file or the database name.
		 */
		if ((ret = __db_open(queue_dbp, ip, NULL,
		    FLD_ISSET(rfp->db_flags, DB_AM_INMEM) ? NULL :
			rfp->info.data,
		    FLD_ISSET(rfp->db_flags, DB_AM_INMEM) ? rfp->info.data :
			NULL,
		    DB_QUEUE, flags, 0, PGNO_BASE_MD)) != 0)
			goto out;

		if ((ret = __db_cursor(queue_dbp,
		    ip, NULL, &rep->queue_dbc, 0)) != 0)
			goto out;
	} else
		queue_dbp = rep->queue_dbc->dbp;

	if ((ret = __queue_pageinfo(queue_dbp,
	    &first, &last, &empty, 0, 0)) != 0)
		goto out;
	RPRINT(env, DB_VERB_REP_SYNC, (env,
	    "Queue fileinfo: first %lu, last %lu, empty %d",
	    (u_long)first, (u_long)last, empty));
	/*
	 * We can be at the end of 3 possible states.
	 * 1.  We have received the meta-page and now need to get the
	 *     rest of the pages in the database.
	 * 2.  We have received from first -> max_pgno.  We might be done,
	 *     or we might need to ask for wrapped pages.
	 * 3.  We have received all pages in the file.  We're done.
	 */
	if (rfp->max_pgno == 0) {
		/*
		 * We have just received the meta page.  Set up the next
		 * pages to ask for and check if the file is empty.
		 */
		if (empty)
			goto out;
		if (first > last) {
			rfp->max_pgno =
			    QAM_RECNO_PAGE(rep->queue_dbc->dbp, UINT32_MAX);
		} else
			rfp->max_pgno = last;
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Queue fileinfo: First req: first %lu, last %lu",
		    (u_long)first, (u_long)rfp->max_pgno));
		goto req;
	} else if (rfp->max_pgno != last) {
		/*
		 * If max_pgno != last that means we're dealing with a
		 * wrapped situation.  Request next batch of pages.
		 * Set npages to 1 because we already have page 0, the
		 * meta-page, now we need pages 1-max_pgno.
		 */
		first = 1;
		rfp->max_pgno = last;
		RPRINT(env, DB_VERB_REP_SYNC, (env,
		    "Queue fileinfo: Wrap req: first %lu, last %lu",
		    (u_long)first, (u_long)last));
req:
		/*
		 * Since we're simulating a "gap" to resend new PAGE_REQ
		 * for this file, we need to set waiting page to last + 1
		 * so that we'll ask for all from ready_pg -> last.
		 */
		rep->npages = first;
		rep->ready_pg = first;
		rep->waiting_pg = rfp->max_pgno + 1;
		rep->max_wait_pg = PGNO_INVALID;
		ret = __rep_pggap_req(env, rep, rfp, 0);
		return (ret);
	}
	/*
	 * max_pgno == last
	 * If we get here, we have all the pages we need.
	 * Close the dbp and return.
	 */
out:
	if (rep->queue_dbc != NULL &&
	    (t_ret = __dbc_close(rep->queue_dbc)) != 0 && ret == 0)
		ret = t_ret;
	rep->queue_dbc = NULL;

	if (queue_dbp != NULL &&
	    (t_ret = __db_close(queue_dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
		ret = t_ret;
	if (ret == 0)
		ret = DB_REP_PAGEDONE;
	return (ret);
#endif
}

/*
 * PUBLIC: int __rep_remove_init_file __P((ENV *));
 */
int
__rep_remove_init_file(env)
	ENV *env;
{
	DB_REP *db_rep;
	REP *rep;
	int ret;
	char *name;

	db_rep = env->rep_handle;
	rep = db_rep->region;

	/*
	 * If running in-memory replication, return without any file
	 * operations.
	 */
	if (FLD_ISSET(rep->config, REP_C_INMEM))
		return (0);

	/* Abbreviated internal init doesn't use an init file. */
	if (F_ISSET(rep, REP_F_ABBREVIATED))
		return (0);

	if ((ret = __db_appname(env,
	    DB_APP_NONE, REP_INITNAME, NULL, &name)) != 0)
		return (ret);
	(void)__os_unlink(env, name, 0);
	__os_free(env, name);
	return (0);
}

/*
 * Checks for the existence of the internal init flag file.  If it exists, we
 * remove all logs and databases, and then remove the flag file.  This is
 * intended to force the internal init to start over again, and thus affords
 * protection against a client crashing during internal init.  This function
 * must be called before normal recovery in order to be properly effective.
 *
 * !!!
 * This function should only be called during initial set-up of the environment,
 * before various subsystems are initialized.  It doesn't rely on the
 * subsystems' code having been initialized, and it summarily deletes files "out
 * from under" them, which might disturb the subsystems if they were up.
 *
 * PUBLIC: int __rep_reset_init __P((ENV *));
 */
int
__rep_reset_init(env)
	ENV *env;
{
	DB_FH *fhp;
	__rep_update_args *rup;
	DBT dbt;
	char *allocated_dir, *dir, *init_name;
	size_t cnt;
	u_int32_t dbtvers, fvers, zero;
	u_int8_t *next;
	int ret, t_ret;

	allocated_dir = NULL;
	rup = NULL;
	dbt.data = NULL;

	if ((ret = __db_appname(env,
	    DB_APP_NONE, REP_INITNAME, NULL, &init_name)) != 0)
		return (ret);

	if ((ret = __os_open(
	    env, init_name, 0, DB_OSO_RDONLY, DB_MODE_600, &fhp)) != 0) {
		if (ret == ENOENT)
			ret = 0;
		goto out;
	}

	RPRINT(env, DB_VERB_REP_SYNC,
	    (env, "Cleaning up interrupted internal init"));

	/* There are a few possibilities:
	 *   1. no init file, or less than 1 full file list
	 *   2. exactly one full file list
	 *   3. more than one, less then a second full file list
	 *   4. second file list in full
	 *
	 * In cases 2 or 4, we need to remove all logs, and then remove files
	 * according to the (most recent) file list.  (In case 1 or 3, we don't
	 * have to do anything.)
	 *
	 * The __rep_get_file_list function takes care of folding these cases
	 * into two simple outcomes.
	 *
	 * As of 4.7, the first 4 bytes are 0.  Read the first 4 bytes now.
	 * If they are non-zero it means we have an old-style init file.
	 * Otherwise, pass the file version in to rep_get_file_list.
	 */
	if ((ret = __os_read(env, fhp, &zero, sizeof(zero), &cnt)) != 0)
		goto out;
	/*
	 * If we read successfully, but not enough, then unlink the file.
	 */
	if (cnt != sizeof(zero))
		goto rm;
	if (zero != 0) {
		/*
		 * Old style file.  We have to set fvers to the 4.6
		 * version of the file and also rewind the file so
		 * that __rep_get_file_list can read out the length itself.
		 */
		if ((ret = __os_seek(env, fhp, 0, 0, 0)) != 0)
			goto out;
		fvers = REP_INITVERSION_46;
	} else if ((ret = __os_read(env,
	    fhp, &fvers, sizeof(fvers), &cnt)) != 0)
		goto out;
	else if (cnt != sizeof(fvers))
		goto rm;
	ret = __rep_get_file_list(env, fhp, fvers, &dbtvers, &dbt);
	if ((t_ret = __os_closehandle(env, fhp)) != 0 || ret != 0) {
		if (ret == 0)
			ret = t_ret;
		goto out;
	}
	if (dbt.data == NULL) {
		/*
		 * The init file did not end with an intact file list.  Since we
		 * never start log/db removal without an intact file list
		 * sync'ed to the init file, this must mean we don't have any
		 * partial set of files to clean up.  So all we need to do is
		 * remove the init file.
		 */
		goto rm;
	}

	/* Remove all log files. */
	if (env->dbenv->db_log_dir == NULL)
		dir = env->db_home;
	else {
		if ((ret = __db_appname(env,
		    DB_APP_NONE, env->dbenv->db_log_dir, NULL, &dir)) != 0)
			goto out;
		allocated_dir = dir;
	}

	if ((ret = __rep_remove_by_prefix(env,
	    dir, LFPREFIX, sizeof(LFPREFIX)-1, DB_APP_LOG)) != 0)
		goto out;

	/*
	 * Remove databases according to the list, and queue extent files by
	 * searching them out on a walk through the data_dir's.
	 */
	if ((ret = __rep_update_unmarshal(env, dbtvers,
	    &rup, dbt.data, dbt.size, &next)) != 0)
		goto out;
	if ((ret = __rep_unlink_by_list(env, dbtvers,
	    next, dbt.size, rup->num_files)) != 0)
		goto out;

	/* Here, we've established that the file exists. */
rm:	(void)__os_unlink(env, init_name, 0);
out:	if (rup != NULL)
		__os_free(env, rup);
	if (allocated_dir != NULL)
		__os_free(env, allocated_dir);
	if (dbt.data != NULL)
		__os_free(env, dbt.data);

	__os_free(env, init_name);
	return (ret);
}

/*
 * Reads the last fully intact file list from the init file.  If the file ends
 * with a partial list (or is empty), we're not interested in it.  Lack of a
 * full file list is indicated by a NULL dbt->data.  On success, the list is
 * returned in allocated space, which becomes the responsibility of the caller.
 *
 * The file format is a u_int32_t buffer length, in native format, followed by
 * the file list itself, in the same format as in an UPDATE message (though
 * many parts of it in this case are meaningless).
 */
static int
__rep_get_file_list(env, fhp, fvers, dbtvers, dbt)
	ENV *env;
	DB_FH *fhp;
	u_int32_t fvers;
	u_int32_t *dbtvers;
	DBT *dbt;
{
	u_int32_t length, mvers;
	size_t cnt;
	int i, ret;

	/* At most 2 file lists: old and new. */
	dbt->data = NULL;
	mvers = DB_REPVERSION_46;
	length = 0;
	for (i = 1; i <= 2; i++) {
		if (fvers >= REP_INITVERSION_47) {
			if ((ret = __os_read(env, fhp, &mvers,
			    sizeof(mvers), &cnt)) != 0)
				goto err;
			if (cnt == 0 && dbt->data != NULL)
				break;
			if (cnt != sizeof(mvers))
				goto err;
		}
		if ((ret = __os_read(env,
		    fhp, &length, sizeof(length), &cnt)) != 0)
			goto err;

		/*
		 * Reaching the end here is fine, if we've been through at least
		 * once already.
		 */
		if (cnt == 0 && dbt->data != NULL)
			break;
		if (cnt != sizeof(length))
			goto err;

		if ((ret = __os_realloc(env,
		    (size_t)length, &dbt->data)) != 0)
			goto err;

		if ((ret = __os_read(
		    env, fhp, dbt->data, length, &cnt)) != 0 ||
		    cnt != (size_t)length)
			goto err;
	}

	*dbtvers = mvers;
	dbt->size = length;
	return (0);

err:
	/*
	 * Note that it's OK to get here with a zero value in 'ret': it means we
	 * read less than we expected, and dbt->data == NULL indicates to the
	 * caller that we don't have an intact list.
	 */
	if (dbt->data != NULL)
		__os_free(env, dbt->data);
	dbt->data = NULL;
	return (ret);
}

/*
 * Removes every file in a given directory that matches a given prefix.  Notice
 * how similar this is to __rep_walk_dir.
 */
static int
__rep_remove_by_prefix(env, dir, prefix, pref_len, appname)
	ENV *env;
	const char *dir;
	const char *prefix;
	size_t pref_len;
	APPNAME appname;	/* What kind of name. */
{
	char *namep, **names;
	int cnt, i, ret;

	if ((ret = __os_dirlist(env, dir, 0, &names, &cnt)) != 0)
		return (ret);
	for (i = 0; i < cnt; i++) {
		if (strncmp(names[i], prefix, pref_len) == 0) {
			if ((ret = __db_appname(env,
			    appname, names[i], NULL, &namep)) != 0)
				goto out;
			(void)__os_unlink(env, namep, 0);
			__os_free(env, namep);
		}
	}
out:	__os_dirfree(env, names, cnt);
	return (ret);
}

/*
 * Removes database files according to the contents of a list.
 *
 * This function must support removal either during environment creation, or
 * when an internal init is reset in the middle.  This means it must work
 * regardless of whether underlying subsystems are initialized.  However, it may
 * assume that databases are not open.  That means there is no REP!
 */
static int
__rep_unlink_by_list(env, version, filelist, filesz, count)
	ENV *env;
	u_int32_t version;
	u_int8_t *filelist;
	u_int32_t filesz;
	u_int32_t count;
{
	DB_ENV *dbenv;
	__rep_fileinfo_args *rfp;
	char **ddir, *dir, *namep;
	u_int8_t *new_fp;
	int ret;

	dbenv = env->dbenv;
	ret = 0;
	rfp = NULL;
	while (count-- > 0) {
		if ((ret = __rep_fileinfo_unmarshal(env, version,
		    &rfp, filelist, filesz, &new_fp)) != 0)
			goto out;
		filesz -= (u_int32_t)(new_fp - filelist);
		filelist = new_fp;

		if ((ret = __db_appname(env,
		    DB_APP_DATA, rfp->info.data, NULL, &namep)) != 0)
			goto out;
		(void)__os_unlink(env, namep, 0);
		__os_free(env, namep);
		__os_free(env, rfp);
		rfp = NULL;
	}

	/* Notice how similar this code is to __rep_find_dbs. */
	if (dbenv->db_data_dir == NULL)
		ret = __rep_remove_by_prefix(env, env->db_home,
		    QUEUE_EXTENT_PREFIX, sizeof(QUEUE_EXTENT_PREFIX) - 1,
		    DB_APP_DATA);
	else {
		for (ddir = dbenv->db_data_dir; *ddir != NULL; ++ddir) {
			if ((ret = __db_appname(env,
			    DB_APP_NONE, *ddir, NULL, &dir)) != 0)
				break;
			ret = __rep_remove_by_prefix(env, dir,
			    QUEUE_EXTENT_PREFIX, sizeof(QUEUE_EXTENT_PREFIX)-1,
			    DB_APP_DATA);
			__os_free(env, dir);
			if (ret != 0)
				break;
		}
	}

out:
	if (rfp != NULL)
		__os_free(env, rfp);
	return (ret);
}

static int
__rep_remove_by_list(env, version, filelist, filesz, count)
	ENV *env;
	u_int32_t version;
	u_int8_t *filelist;
	u_int32_t filesz;
	u_int32_t count;
{
	__rep_fileinfo_args *rfp;
	u_int8_t *new_fp;
	int ret;

	ret = 0;
	rfp = NULL;
	while (count-- > 0) {
		if ((ret = __rep_fileinfo_unmarshal(env, version,
		    &rfp, filelist, filesz, &new_fp)) != 0)
			break;
		filesz -= (u_int32_t)(new_fp - filelist);
		filelist = new_fp;

		if ((ret = __rep_remove_file(env, rfp->uid.data,
		    rfp->info.data, rfp->type, rfp->db_flags)) != 0) {
			/*
			 * If the file already doesn't exist, that's perfectly
			 * OK.  This can easily happen if we're cleaning up an
			 * interrupted internal init, and we only got part-way
			 * through the list of files.
			 */
			if (ret == ENOENT)
				ret = 0;
			else
				break;
		}
		__os_free(env, rfp);
		rfp = NULL;
	}

	if (rfp != NULL)
		__os_free(env, rfp);
	return (ret);
}
