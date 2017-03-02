/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/hmac.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

static int	__log_init __P((ENV *, DB_LOG *));
static int	__log_recover __P((DB_LOG *));
static size_t	__log_region_size __P((ENV *));

/*
 * __log_open --
 *	Internal version of log_open: only called from ENV->open.
 *
 * PUBLIC: int __log_open __P((ENV *, int));
 */
int
__log_open(env, create_ok)
	ENV *env;
	int create_ok;
{
	DB_ENV *dbenv;
	DB_LOG *dblp;
	LOG *lp;
	u_int8_t *bulk;
	int region_locked, ret;

	dbenv = env->dbenv;
	region_locked = 0;

	/* Create/initialize the DB_LOG structure. */
	if ((ret = __os_calloc(env, 1, sizeof(DB_LOG), &dblp)) != 0)
		return (ret);
	dblp->env = env;

	/* Set the default buffer size, if not otherwise configured. */
	if (dbenv->lg_bsize == 0)
		dbenv->lg_bsize = FLD_ISSET(dbenv->lg_flags, DB_LOG_IN_MEMORY) ?
		    LG_BSIZE_INMEM : LG_BSIZE_DEFAULT;

	/* Join/create the log region. */
	dblp->reginfo.env = env;
	dblp->reginfo.type = REGION_TYPE_LOG;
	dblp->reginfo.id = INVALID_REGION_ID;
	dblp->reginfo.flags = REGION_JOIN_OK;

	if (create_ok)
		F_SET(&dblp->reginfo, REGION_CREATE_OK);
	if ((ret = __env_region_attach(
	    env, &dblp->reginfo, __log_region_size(env))) != 0)
		goto err;

	/* If we created the region, initialize it. */
	if (F_ISSET(&dblp->reginfo, REGION_CREATE))
		if ((ret = __log_init(env, dblp)) != 0)
			goto err;

	/* Set the local addresses. */
	lp = dblp->reginfo.primary =
	    R_ADDR(&dblp->reginfo, dblp->reginfo.rp->primary);
	dblp->bufp = R_ADDR(&dblp->reginfo, lp->buffer_off);

	/*
	 * If the region is threaded, we have to lock the DBREG list, and we
	 * need to allocate a mutex for that purpose.
	 */
	if ((ret = __mutex_alloc(env,
	    MTX_LOG_REGION, DB_MUTEX_PROCESS_ONLY, &dblp->mtx_dbreg)) != 0)
		goto err;

	/*
	 * Set the handle -- we may be about to run recovery, which allocates
	 * log cursors.  Log cursors require logging be already configured,
	 * and the handle being set is what demonstrates that.
	 *
	 * If we created the region, run recovery.  If that fails, make sure
	 * we reset the log handle before cleaning up, otherwise we will try
	 * and clean up again in the mainline ENV initialization code.
	 */
	env->lg_handle = dblp;

	if (F_ISSET(&dblp->reginfo, REGION_CREATE)) {
		/*
		 * We first take the log file size from the environment, if
		 * specified.  If that wasn't set, default it.  Regardless,
		 * recovery may set it from the persistent information in a
		 * log file header.
		 */
		if (lp->log_size == 0)
			lp->log_size =
			    FLD_ISSET(dbenv->lg_flags, DB_LOG_IN_MEMORY) ?
			    LG_MAX_INMEM : LG_MAX_DEFAULT;

		if ((ret = __log_recover(dblp)) != 0)
			goto err;

		/*
		 * If the next log file size hasn't been set yet, default it
		 * to the current log file size.
		 */
		if (lp->log_nsize == 0)
			lp->log_nsize = lp->log_size;

		/*
		 * If we haven't written any log files, write the first one
		 * so that checkpoint gets a valid ckp_lsn value.
		 */
		if (IS_INIT_LSN(lp->lsn) &&
		    (ret = __log_newfile(dblp, NULL, 0, 0)) != 0)
			goto err;

		/*
		 * Initialize replication's next-expected LSN value
		 * and replication's bulk buffer.  In __env_open, we
		 * always create/open the replication region before
		 * the log region so we're assured that our rep_handle
		 * is valid at this point, if replication is being used.
		 */
		lp->ready_lsn = lp->lsn;
		if (IS_ENV_REPLICATED(env)) {
			if ((ret =
			    __env_alloc(&dblp->reginfo, MEGABYTE, &bulk)) != 0)
				goto err;
			lp->bulk_buf = R_OFFSET(&dblp->reginfo, bulk);
			lp->bulk_len = MEGABYTE;
			lp->bulk_off = 0;
			lp->wait_ts = env->rep_handle->request_gap;
			__os_gettime(env, &lp->rcvd_ts, 1);
		} else {
			lp->bulk_buf = INVALID_ROFF;
			lp->bulk_len = 0;
			lp->bulk_off = 0;
		}
		dblp->reginfo.mtx_alloc = lp->mtx_region;
	} else {
		/*
		 * A process joining the region may have reset the log file
		 * size, too.  If so, it only affects the next log file we
		 * create.  We need to check that the size is reasonable given
		 * the buffer size in the region.
		 */
		LOG_SYSTEM_LOCK(env);
		region_locked = 1;

		 if (dbenv->lg_size != 0) {
			if ((ret =
			    __log_check_sizes(env, dbenv->lg_size, 0)) != 0)
				goto err;

			lp->log_nsize = dbenv->lg_size;
		 }

		LOG_SYSTEM_UNLOCK(env);
		region_locked = 0;
	}

	return (0);

err:	if (dblp->reginfo.addr != NULL) {
		if (region_locked)
			LOG_SYSTEM_UNLOCK(env);
		(void)__env_region_detach(env, &dblp->reginfo, 0);
	}
	env->lg_handle = NULL;

	(void)__mutex_free(env, &dblp->mtx_dbreg);
	__os_free(env, dblp);

	return (ret);
}

/*
 * __log_init --
 *	Initialize a log region in shared memory.
 */
static int
__log_init(env, dblp)
	ENV *env;
	DB_LOG *dblp;
{
	DB_ENV *dbenv;
	LOG *lp;
	int ret;
	void *p;

	dbenv = env->dbenv;

	/*
	 * This is the first point where we can validate the buffer size,
	 * because we know all three settings have been configured (file size,
	 * buffer size and the in-memory flag).
	 */
	if ((ret =
	   __log_check_sizes(env, dbenv->lg_size, dbenv->lg_bsize)) != 0)
		return (ret);

	if ((ret = __env_alloc(&dblp->reginfo,
	    sizeof(*lp), &dblp->reginfo.primary)) != 0)
		goto mem_err;
	dblp->reginfo.rp->primary =
	    R_OFFSET(&dblp->reginfo, dblp->reginfo.primary);
	lp = dblp->reginfo.primary;
	memset(lp, 0, sizeof(*lp));

	if ((ret =
	    __mutex_alloc(env, MTX_LOG_REGION, 0, &lp->mtx_region)) != 0)
		return (ret);

	lp->fid_max = 0;
	SH_TAILQ_INIT(&lp->fq);
	lp->free_fid_stack = INVALID_ROFF;
	lp->free_fids = lp->free_fids_alloced = 0;

	/* Initialize LOG LSNs. */
	INIT_LSN(lp->lsn);
	INIT_LSN(lp->t_lsn);

	/*
	 * It's possible to be waiting for an LSN of [1][0], if a replication
	 * client gets the first log record out of order.  An LSN of [0][0]
	 * signifies that we're not waiting.
	 */
	ZERO_LSN(lp->waiting_lsn);

	/*
	 * Log makes note of the fact that it ran into a checkpoint on
	 * startup if it did so, as a recovery optimization.  A zero
	 * LSN signifies that it hasn't found one [yet].
	 */
	ZERO_LSN(lp->cached_ckp_lsn);

	if ((ret =
	    __mutex_alloc(env, MTX_LOG_FILENAME, 0, &lp->mtx_filelist)) != 0)
		return (ret);
	if ((ret = __mutex_alloc(env, MTX_LOG_FLUSH, 0, &lp->mtx_flush)) != 0)
		return (ret);

	/* Initialize the buffer. */
	if ((ret = __env_alloc(&dblp->reginfo, dbenv->lg_bsize, &p)) != 0) {
mem_err:	__db_errx( env, "unable to allocate log region memory");
		return (ret);
	}
	lp->regionmax = dbenv->lg_regionmax;
	lp->buffer_off = R_OFFSET(&dblp->reginfo, p);
	lp->buffer_size = dbenv->lg_bsize;
	lp->filemode = dbenv->lg_filemode;
	lp->log_size = lp->log_nsize = dbenv->lg_size;

	/* Initialize the commit Queue. */
	SH_TAILQ_INIT(&lp->free_commits);
	SH_TAILQ_INIT(&lp->commits);
	lp->ncommit = 0;

	/* Initialize the logfiles list for in-memory logs. */
	SH_TAILQ_INIT(&lp->logfiles);
	SH_TAILQ_INIT(&lp->free_logfiles);

	/*
	 * Fill in the log's persistent header.  Don't fill in the log file
	 * sizes, as they may change at any time and so have to be filled in
	 * as each log file is created.
	 */
	lp->persist.magic = DB_LOGMAGIC;
	/*
	 * Don't use __log_set_version because env->dblp isn't set up yet.
	 */
	lp->persist.version = DB_LOGVERSION;
	lp->persist.notused = 0;
	env->lg_handle = dblp;

	/* Migrate persistent flags from the ENV into the region. */
	if (dbenv->lg_flags != 0 &&
	    (ret = __log_set_config_int(dbenv, dbenv->lg_flags, 1, 1)) != 0)
		return (ret);

	(void)time(&lp->timestamp);
	return (0);
}

/*
 * __log_recover --
 *	Recover a log.
 */
static int
__log_recover(dblp)
	DB_LOG *dblp;
{
	DBT dbt;
	DB_ENV *dbenv;
	DB_LOGC *logc;
	DB_LSN lsn;
	ENV *env;
	LOG *lp;
	u_int32_t cnt, rectype;
	int ret;
	logfile_validity status;

	env = dblp->env;
	dbenv = env->dbenv;
	logc = NULL;
	lp = dblp->reginfo.primary;

	/*
	 * Find a log file.  If none exist, we simply return, leaving
	 * everything initialized to a new log.
	 */
	if ((ret = __log_find(dblp, 0, &cnt, &status)) != 0)
		return (ret);
	if (cnt == 0)
		return (0);

	/*
	 * If the last file is an old, unreadable version, start a new
	 * file.  Don't bother finding the end of the last log file;
	 * we assume that it's valid in its entirety, since the user
	 * should have shut down cleanly or run recovery before upgrading.
	 */
	if (status == DB_LV_OLD_UNREADABLE) {
		lp->lsn.file = lp->s_lsn.file = cnt + 1;
		lp->lsn.offset = lp->s_lsn.offset = 0;
		goto skipsearch;
	}
	DB_ASSERT(env,
	    (status == DB_LV_NORMAL || status == DB_LV_OLD_READABLE));

	/*
	 * We have the last useful log file and we've loaded any persistent
	 * information.  Set the end point of the log past the end of the last
	 * file. Read the last file, looking for the last checkpoint and
	 * the log's end.
	 */
	lp->lsn.file = cnt + 1;
	lp->lsn.offset = 0;
	lsn.file = cnt;
	lsn.offset = 0;

	/*
	 * Allocate a cursor and set it to the first record.  This shouldn't
	 * fail, leave error messages on.
	 */
	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);
	F_SET(logc, DB_LOG_LOCKED);
	memset(&dbt, 0, sizeof(dbt));
	if ((ret = __logc_get(logc, &lsn, &dbt, DB_SET)) != 0)
		goto err;

	/*
	 * Read to the end of the file.  This may fail at some point, so
	 * turn off error messages.
	 */
	F_SET(logc, DB_LOG_SILENT_ERR);
	while (__logc_get(logc, &lsn, &dbt, DB_NEXT) == 0) {
		if (dbt.size < sizeof(u_int32_t))
			continue;
		LOGCOPY_32(env, &rectype, dbt.data);
		if (rectype == DB___txn_ckp)
			/*
			 * If we happen to run into a checkpoint, cache its
			 * LSN so that the transaction system doesn't have
			 * to walk this log file again looking for it.
			 */
			lp->cached_ckp_lsn = lsn;
	}
	F_CLR(logc, DB_LOG_SILENT_ERR);

	/*
	 * We now know where the end of the log is.  Set the first LSN that
	 * we want to return to an application and the LSN of the last known
	 * record on disk.
	 */
	lp->lsn = lsn;
	lp->s_lsn = lsn;
	lp->lsn.offset += logc->len;
	lp->s_lsn.offset += logc->len;

	/* Set up the current buffer information, too. */
	lp->len = logc->len;
	lp->a_off = 0;
	lp->b_off = 0;
	lp->w_off = lp->lsn.offset;

skipsearch:
	if (FLD_ISSET(dbenv->verbose, DB_VERB_RECOVERY))
		__db_msg(env,
		    "Finding last valid log LSN: file: %lu offset %lu",
		    (u_long)lp->lsn.file, (u_long)lp->lsn.offset);

err:	if (logc != NULL)
		(void)__logc_close(logc);

	return (ret);
}

/*
 * __log_find --
 *	Try to find a log file.  If find_first is set, valp will contain
 * the number of the first readable log file, else it will contain the number
 * of the last log file (which may be too old to read).
 *
 * PUBLIC: int __log_find __P((DB_LOG *, int, u_int32_t *, logfile_validity *));
 */
int
__log_find(dblp, find_first, valp, statusp)
	DB_LOG *dblp;
	int find_first;
	u_int32_t *valp;
	logfile_validity *statusp;
{
	ENV *env;
	LOG *lp;
	logfile_validity logval_status, status;
	struct __db_filestart *filestart;
	u_int32_t clv, logval;
	int cnt, fcnt, ret;
	const char *dir;
	char *c, **names, *p, *q;

	env = dblp->env;
	lp = dblp->reginfo.primary;
	logval_status = status = DB_LV_NONEXISTENT;

	/* Return a value of 0 as the log file number on failure. */
	*valp = 0;

	if (lp->db_log_inmemory) {
		filestart = find_first ?
		    SH_TAILQ_FIRST(&lp->logfiles, __db_filestart) :
		    SH_TAILQ_LAST(&lp->logfiles, links, __db_filestart);
		if (filestart != NULL) {
			*valp = filestart->file;
			logval_status = DB_LV_NORMAL;
		}
		*statusp = logval_status;
		return (0);
	}

	/* Find the directory name. */
	if ((ret = __log_name(dblp, 1, &p, NULL, 0)) != 0) {
		__os_free(env, p);
		return (ret);
	}
	if ((q = __db_rpath(p)) == NULL)
		dir = PATH_DOT;
	else {
		*q = '\0';
		dir = p;
	}

	/* Get the list of file names. */
retry:	if ((ret = __os_dirlist(env, dir, 0, &names, &fcnt)) != 0) {
		__db_err(env, ret, "%s", dir);
		__os_free(env, p);
		return (ret);
	}

	/* Search for a valid log file name. */
	for (cnt = fcnt, clv = logval = 0; --cnt >= 0;) {
		if (strncmp(names[cnt], LFPREFIX, sizeof(LFPREFIX) - 1) != 0)
			continue;

		/*
		 * Names of the form log\.[0-9]* are reserved for DB.  Other
		 * names sharing LFPREFIX, such as "log.db", are legal.
		 */
		for (c = names[cnt] + sizeof(LFPREFIX) - 1; *c != '\0'; c++)
			if (!isdigit((int)*c))
				break;
		if (*c != '\0')
			continue;

		/*
		 * Use atol, not atoi; if an "int" is 16-bits, the largest
		 * log file name won't fit.
		 */
		clv = (u_int32_t)atol(names[cnt] + (sizeof(LFPREFIX) - 1));

		/*
		 * If searching for the first log file, we want to return the
		 * oldest log file we can read, or, if no readable log files
		 * exist, the newest log file we can't read (the crossover
		 * point between the old and new versions of the log file).
		 *
		 * If we're searching for the last log file, we want to return
		 * the newest log file, period.
		 *
		 * Readable log files should never precede unreadable log
		 * files, that would mean the admin seriously screwed up.
		 */
		if (find_first) {
			if (logval != 0 &&
			    status != DB_LV_OLD_UNREADABLE && clv > logval)
				continue;
		} else
			if (logval != 0 && clv < logval)
				continue;

		if ((ret = __log_valid(dblp, clv, 1, NULL, 0,
		    &status, NULL)) != 0) {
			/*
			 * If we have raced with removal of a log file since
			 * the call to __os_dirlist, it may no longer exist.
			 * In that case, just go on to the next one.  If we're
			 * at the end of the list, all of the log files we saw
			 * initially are gone and we need to get the list again.
			 */
			if (ret == ENOENT) {
				ret = 0;
				if (cnt == 0) {
					__os_dirfree(env, names, fcnt);
					goto retry;
				}
				continue;
			}
			__db_err(
			    env, ret, "Invalid log file: %s", names[cnt]);
			goto err;
		}
		switch (status) {
		case DB_LV_NONEXISTENT:
			/* __log_valid never returns DB_LV_NONEXISTENT. */
			DB_ASSERT(env, 0);
			break;
		case DB_LV_INCOMPLETE:
			/*
			 * The last log file may not have been initialized --
			 * it's possible to create a log file but not write
			 * anything to it.  If performing recovery (that is,
			 * if find_first isn't set), ignore the file, it's
			 * not interesting.  If we're searching for the first
			 * log record, return the file (assuming we don't find
			 * something better), as the "real" first log record
			 * is likely to be in the log buffer, and we want to
			 * set the file LSN for our return.
			 */
			if (find_first)
				goto found;
			break;
		case DB_LV_OLD_UNREADABLE:
			/*
			 * If we're searching for the first log file, then we
			 * only want this file if we don't yet have a file or
			 * already have an unreadable file and this one is
			 * newer than that one.  If we're searching for the
			 * last log file, we always want this file because we
			 * wouldn't be here if it wasn't newer than our current
			 * choice.
			 */
			if (!find_first || logval == 0 ||
			    (status == DB_LV_OLD_UNREADABLE && clv > logval))
				goto found;
			break;
		case DB_LV_NORMAL:
		case DB_LV_OLD_READABLE:
found:			logval = clv;
			logval_status = status;
			break;
		}
	}

	*valp = logval;

err:	__os_dirfree(env, names, fcnt);
	__os_free(env, p);
	*statusp = logval_status;

	return (ret);
}

/*
 * log_valid --
 *	Validate a log file.  Returns an error code in the event of
 *	a fatal flaw in a the specified log file;  returns success with
 *	a code indicating the currentness and completeness of the specified
 *	log file if it is not unexpectedly flawed (that is, if it's perfectly
 *	normal, if it's zero-length, or if it's an old version).
 *
 * PUBLIC: int __log_valid __P((DB_LOG *, u_int32_t, int,
 * PUBLIC:     DB_FH **, u_int32_t, logfile_validity *, u_int32_t *));
 */
int
__log_valid(dblp, number, set_persist, fhpp, flags, statusp, versionp)
	DB_LOG *dblp;
	u_int32_t number;
	int set_persist;
	DB_FH **fhpp;
	u_int32_t flags;
	logfile_validity *statusp;
	u_int32_t *versionp;
{
	DB_CIPHER *db_cipher;
	DB_FH *fhp;
	ENV *env;
	HDR *hdr;
	LOG *lp;
	LOGP *persist;
	logfile_validity status;
	size_t hdrsize, nr, recsize;
	int is_hmac, ret;
	u_int8_t *tmp;
	char *fname;

	env = dblp->env;
	db_cipher = env->crypto_handle;
	fhp = NULL;
	persist = NULL;
	status = DB_LV_NORMAL;
	tmp = NULL;

	/* Return the file handle to our caller, on request */
	if (fhpp != NULL)
		*fhpp = NULL;

	if (flags == 0)
		flags = DB_OSO_RDONLY | DB_OSO_SEQ;
	/* Try to open the log file. */
	if ((ret = __log_name(dblp, number, &fname, &fhp, flags)) != 0) {
		__os_free(env, fname);
		return (ret);
	}

	hdrsize = HDR_NORMAL_SZ;
	is_hmac = 0;
	recsize = sizeof(LOGP);
	if (CRYPTO_ON(env)) {
		hdrsize = HDR_CRYPTO_SZ;
		recsize = sizeof(LOGP);
		recsize += db_cipher->adj_size(recsize);
		is_hmac = 1;
	}
	if ((ret = __os_calloc(env, 1, recsize + hdrsize, &tmp)) != 0)
		goto err;

	hdr = (HDR *)tmp;
	persist = (LOGP *)(tmp + hdrsize);

	/*
	 * Try to read the header.  This can fail if the log is truncated, or
	 * if we find a preallocated log file where the header has not yet been
	 * written, so we need to check whether the header is zero-filled.
	 */
	if ((ret = __os_read(env, fhp, tmp, recsize + hdrsize, &nr)) != 0 ||
	    nr != recsize + hdrsize ||
	    (hdr->len == 0 && persist->magic == 0 && persist->log_size == 0)) {
		if (ret == 0)
			status = DB_LV_INCOMPLETE;
		else
			/*
			 * The error was a fatal read error, not just an
			 * incompletely initialized log file.
			 */
			__db_err(env, ret, "ignoring log file: %s", fname);
		goto err;
	}

	if (LOG_SWAPPED(env))
		__log_hdrswap(hdr, CRYPTO_ON(env));

	/*
	 * Now we have to validate the persistent record.  We have
	 * several scenarios we have to deal with:
	 *
	 * 1.  User has crypto turned on:
	 *	- They're reading an old, unencrypted log file
	 *	  .  We will fail the record size match check below.
	 *	- They're reading a current, unencrypted log file
	 *	  .  We will fail the record size match check below.
	 *	- They're reading an old, encrypted log file [NOT YET]
	 *	  .  After decryption we'll fail the version check.  [NOT YET]
	 *	- They're reading a current, encrypted log file
	 *	  .  We should proceed as usual.
	 * 2.  User has crypto turned off:
	 *	- They're reading an old, unencrypted log file
	 *	  .  We will fail the version check.
	 *	- They're reading a current, unencrypted log file
	 *	  .  We should proceed as usual.
	 *	- They're reading an old, encrypted log file [NOT YET]
	 *	  .  We'll fail the magic number check (it is encrypted).
	 *	- They're reading a current, encrypted log file
	 *	  .  We'll fail the magic number check (it is encrypted).
	 */
	if (CRYPTO_ON(env)) {
		/*
		 * If we are trying to decrypt an unencrypted log
		 * we can only detect that by having an unreasonable
		 * data length for our persistent data.
		 */
		if ((hdr->len - hdrsize) != sizeof(LOGP)) {
			__db_errx(env, "log record size mismatch");
			goto err;
		}
		/* Check the checksum and decrypt. */
		if ((ret = __db_check_chksum(env, hdr, db_cipher,
		    &hdr->chksum[0], (u_int8_t *)persist,
		    hdr->len - hdrsize, is_hmac)) != 0) {
			__db_errx(env, "log record checksum mismatch");
			goto err;
		}

		if ((ret = db_cipher->decrypt(env, db_cipher->data,
		    &hdr->iv[0], (u_int8_t *)persist, hdr->len - hdrsize)) != 0)
			goto err;
	}

	/* Swap the header, if necessary. */
	if (LOG_SWAPPED(env)) {
		/*
		 * If the magic number is not byte-swapped, we're looking at an
		 * old log that we can no longer read.
		 */
		if (persist->magic == DB_LOGMAGIC) {
			__db_errx(env,
			    "Ignoring log file: %s historic byte order", fname);
			status = DB_LV_OLD_UNREADABLE;
			goto err;
		}

		__log_persistswap(persist);
	}

	/* Validate the header. */
	if (persist->magic != DB_LOGMAGIC) {
		__db_errx(env,
		    "Ignoring log file: %s: magic number %lx, not %lx",
		    fname, (u_long)persist->magic, (u_long)DB_LOGMAGIC);
		ret = EINVAL;
		goto err;
	}

	/*
	 * Set our status code to indicate whether the log file belongs to an
	 * unreadable or readable old version; leave it alone if and only if
	 * the log file version is the current one.
	 */
	if (persist->version > DB_LOGVERSION) {
		/* This is a fatal error--the log file is newer than DB. */
		__db_errx(env,
		    "Unacceptable log file %s: unsupported log version %lu",
		    fname, (u_long)persist->version);
		ret = EINVAL;
		goto err;
	} else if (persist->version < DB_LOGOLDVER) {
		status = DB_LV_OLD_UNREADABLE;
		/* This is a non-fatal error, but give some feedback. */
		__db_errx(env,
		    "Skipping log file %s: historic log version %lu",
		    fname, (u_long)persist->version);
		/*
		 * We don't want to set persistent info based on an unreadable
		 * region, so jump to "err".
		 */
		goto err;
	} else if (persist->version < DB_LOGVERSION)
		status = DB_LV_OLD_READABLE;

	/*
	 * Only if we have a current log do we verify the checksum.  We could
	 * not check the checksum before checking the magic and version because
	 * old log headers put the length and checksum in a different location.
	 * The checksum was calculated with the swapped byte order, so we need
	 * to check it with the same bytes.
	 */
	if (!CRYPTO_ON(env)) {
		if (LOG_SWAPPED(env))
			__log_persistswap(persist);

		if ((ret = __db_check_chksum(env,
		    hdr, db_cipher, &hdr->chksum[0], (u_int8_t *)persist,
		    hdr->len - hdrsize, is_hmac)) != 0) {
			__db_errx(env, "log record checksum mismatch");
			goto err;
		}

		if (LOG_SWAPPED(env))
			__log_persistswap(persist);
	}

	/*
	 * If the log is readable so far and we're doing system initialization,
	 * set the region's persistent information based on the headers.
	 *
	 * Override the current log file size.
	 */
	if (set_persist) {
		lp = dblp->reginfo.primary;
		lp->log_size = persist->log_size;
		lp->persist.version = persist->version;
	}
	if (versionp != NULL)
		*versionp = persist->version;

err:	if (fname != NULL)
		__os_free(env, fname);
	if (ret == 0 && fhpp != NULL)
		*fhpp = fhp;
	else
		/* Must close on error or if we only used it locally. */
		(void)__os_closehandle(env, fhp);
	if (tmp != NULL)
		__os_free(env, tmp);

	if (statusp != NULL)
		*statusp = status;

	return (ret);
}

/*
 * __log_env_refresh --
 *	Clean up after the log system on a close or failed open.
 *
 * PUBLIC: int __log_env_refresh __P((ENV *));
 */
int
__log_env_refresh(env)
	ENV *env;
{
	DB_LOG *dblp;
	LOG *lp;
	REGINFO *reginfo;
	struct __fname *fnp;
	struct __db_commit *commit;
	struct __db_filestart *filestart;
	int ret, t_ret;

	dblp = env->lg_handle;
	reginfo = &dblp->reginfo;
	lp = reginfo->primary;
	ret = 0;

	/*
	 * Flush the log if it's private -- there's no Berkeley DB guarantee
	 * that this gets done, but in case the application has forgotten to
	 * flush for durability, it's the polite thing to do.
	 */
	if (F_ISSET(env, ENV_PRIVATE) &&
	    (t_ret = __log_flush(env, NULL)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __dbreg_close_files(env, 0)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * After we close the files, check for any unlogged closes left in
	 * the shared memory queue.  If we find any, try to log it, otherwise
	 * return the error.  We cannot say the environment was closed
	 * cleanly.
	 */
	MUTEX_LOCK(env, lp->mtx_filelist);
	SH_TAILQ_FOREACH(fnp, &lp->fq, q, __fname)
		if (F_ISSET(fnp, DB_FNAME_NOTLOGGED) &&
		    (t_ret = __dbreg_close_id_int(
		    env, fnp, DBREG_CLOSE, 1)) != 0)
			ret = t_ret;
	MUTEX_UNLOCK(env, lp->mtx_filelist);

	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if (F_ISSET(env, ENV_PRIVATE)) {
		reginfo->mtx_alloc = MUTEX_INVALID;
		/* Discard the flush mutex. */
		if ((t_ret =
		    __mutex_free(env, &lp->mtx_flush)) != 0 && ret == 0)
			ret = t_ret;

		/* Discard the buffer. */
		__env_alloc_free(reginfo, R_ADDR(reginfo, lp->buffer_off));

		/* Discard stack of free file IDs. */
		if (lp->free_fid_stack != INVALID_ROFF)
			__env_alloc_free(reginfo,
			    R_ADDR(reginfo, lp->free_fid_stack));

		/* Discard the list of in-memory log file markers. */
		while ((filestart = SH_TAILQ_FIRST(&lp->logfiles,
		    __db_filestart)) != NULL) {
			SH_TAILQ_REMOVE(&lp->logfiles, filestart, links,
			    __db_filestart);
			__env_alloc_free(reginfo, filestart);
		}

		while ((filestart = SH_TAILQ_FIRST(&lp->free_logfiles,
		    __db_filestart)) != NULL) {
			SH_TAILQ_REMOVE(&lp->free_logfiles, filestart, links,
			    __db_filestart);
			__env_alloc_free(reginfo, filestart);
		}

		/* Discord commit queue elements. */
		while ((commit = SH_TAILQ_FIRST(&lp->free_commits,
		    __db_commit)) != NULL) {
			SH_TAILQ_REMOVE(&lp->free_commits, commit, links,
			    __db_commit);
			__env_alloc_free(reginfo, commit);
		}

		/* Discard replication bulk buffer. */
		if (lp->bulk_buf != INVALID_ROFF) {
			__env_alloc_free(reginfo,
			    R_ADDR(reginfo, lp->bulk_buf));
			lp->bulk_buf = INVALID_ROFF;
		}
	}

	/* Discard the per-thread DBREG mutex. */
	if ((t_ret = __mutex_free(env, &dblp->mtx_dbreg)) != 0 && ret == 0)
		ret = t_ret;

	/* Detach from the region. */
	if ((t_ret = __env_region_detach(env, reginfo, 0)) != 0 && ret == 0)
		ret = t_ret;

	/* Close open files, release allocated memory. */
	if (dblp->lfhp != NULL) {
		if ((t_ret =
		    __os_closehandle(env, dblp->lfhp)) != 0 && ret == 0)
			ret = t_ret;
		dblp->lfhp = NULL;
	}
	if (dblp->dbentry != NULL)
		__os_free(env, dblp->dbentry);

	__os_free(env, dblp);

	env->lg_handle = NULL;
	return (ret);
}

/*
 * __log_get_cached_ckp_lsn --
 *	Retrieve any last checkpoint LSN that we may have found on startup.
 *
 * PUBLIC: int __log_get_cached_ckp_lsn __P((ENV *, DB_LSN *));
 */
int
__log_get_cached_ckp_lsn(env, ckp_lsnp)
	ENV *env;
	DB_LSN *ckp_lsnp;
{
	DB_LOG *dblp;
	LOG *lp;

	dblp = env->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;

	LOG_SYSTEM_LOCK(env);
	*ckp_lsnp = lp->cached_ckp_lsn;
	LOG_SYSTEM_UNLOCK(env);

	return (0);
}

/*
 * __log_region_mutex_count --
 *	Return the number of mutexes the log region will need.
 *
 * PUBLIC: u_int32_t __log_region_mutex_count __P((ENV *));
 */
u_int32_t
__log_region_mutex_count(env)
	ENV *env;
{
	/*
	 * We need a few assorted mutexes, and one per transaction waiting
	 * on the group commit list.  We can't know how many that will be,
	 * but it should be bounded by the maximum active transactions.
	 */
	return (env->dbenv->tx_max + 5);
}

/*
 * __log_region_size --
 *	Return the amount of space needed for the log region.
 *	Make the region large enough to hold txn_max transaction
 *	detail structures  plus some space to hold thread handles
 *	and the beginning of the alloc region and anything we
 *	need for mutex system resource recording.
 */
static size_t
__log_region_size(env)
	ENV *env;
{
	DB_ENV *dbenv;
	size_t s;

	dbenv = env->dbenv;

	s = dbenv->lg_regionmax + dbenv->lg_bsize;

	/*
	 * If running with replication, add in space for bulk buffer.
	 * Allocate a megabyte and a little bit more space.
	 */
	if (IS_ENV_REPLICATED(env))
		s += MEGABYTE;

	return (s);
}

/*
 * __log_vtruncate
 *	This is a virtual truncate.  We set up the log indicators to
 * make everyone believe that the given record is the last one in the
 * log.  Returns with the next valid LSN (i.e., the LSN of the next
 * record to be written). This is used in replication to discard records
 * in the log file that do not agree with the master.
 *
 * PUBLIC: int __log_vtruncate __P((ENV *, DB_LSN *, DB_LSN *, DB_LSN *));
 */
int
__log_vtruncate(env, lsn, ckplsn, trunclsn)
	ENV *env;
	DB_LSN *lsn, *ckplsn, *trunclsn;
{
	DBT log_dbt;
	DB_LOG *dblp;
	DB_LOGC *logc;
	LOG *lp;
	u_int32_t bytes, len;
	int ret, t_ret;

	/* Need to find out the length of this soon-to-be-last record. */
	if ((ret = __log_cursor(env, &logc)) != 0)
		return (ret);
	memset(&log_dbt, 0, sizeof(log_dbt));
	ret = __logc_get(logc, lsn, &log_dbt, DB_SET);
	len = logc->len;
	if ((t_ret = __logc_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		return (ret);

	/* Now do the truncate. */
	dblp = env->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;

	LOG_SYSTEM_LOCK(env);

	/*
	 * Flush the log so we can simply initialize the in-memory buffer
	 * after the truncate.
	 */
	if ((ret = __log_flush_int(dblp, NULL, 0)) != 0)
		goto err;

	lp->lsn = *lsn;
	lp->len = len;
	lp->lsn.offset += lp->len;

	if (lp->db_log_inmemory &&
	    (ret = __log_inmem_lsnoff(dblp, &lp->lsn, &lp->b_off)) != 0)
		goto err;

	/*
	 * I am going to assume that the number of bytes written since
	 * the last checkpoint doesn't exceed a 32-bit number.
	 */
	DB_ASSERT(env, lp->lsn.file >= ckplsn->file);
	bytes = 0;
	if (ckplsn->file != lp->lsn.file) {
		bytes = lp->log_size - ckplsn->offset;
		if (lp->lsn.file > ckplsn->file + 1)
			bytes += lp->log_size *
			    ((lp->lsn.file - ckplsn->file) - 1);
		bytes += lp->lsn.offset;
	} else
		bytes = lp->lsn.offset - ckplsn->offset;

	lp->stat.st_wc_mbytes += bytes / MEGABYTE;
	lp->stat.st_wc_bytes += bytes % MEGABYTE;

	/*
	 * If the synced lsn is greater than our new end of log, reset it
	 * to our current end of log.
	 */
	MUTEX_LOCK(env, lp->mtx_flush);
	if (LOG_COMPARE(&lp->s_lsn, lsn) > 0)
		lp->s_lsn = lp->lsn;
	MUTEX_UNLOCK(env, lp->mtx_flush);

	/* Initialize the in-region buffer to a pristine state. */
	ZERO_LSN(lp->f_lsn);
	lp->w_off = lp->lsn.offset;

	if (trunclsn != NULL)
		*trunclsn = lp->lsn;

	/* Truncate the log to the new point. */
	if ((ret = __log_zero(env, &lp->lsn)) != 0)
		goto err;

err:	LOG_SYSTEM_UNLOCK(env);
	return (ret);
}

/*
 * __log_is_outdated --
 *	Used by the replication system to identify if a client's logs are too
 *	old.
 *
 * PUBLIC: int __log_is_outdated __P((ENV *, u_int32_t, int *));
 */
int
__log_is_outdated(env, fnum, outdatedp)
	ENV *env;
	u_int32_t fnum;
	int *outdatedp;
{
	DB_LOG *dblp;
	LOG *lp;
	char *name;
	int ret;
	u_int32_t cfile;
	struct __db_filestart *filestart;

	dblp = env->lg_handle;

	/*
	 * The log represented by env is compared to the file number passed
	 * in fnum.  If the log file fnum does not exist and is lower-numbered
	 * than the current logs, return *outdatedp non-zero, else we return 0.
	 */
	if (FLD_ISSET(env->dbenv->lg_flags, DB_LOG_IN_MEMORY)) {
		LOG_SYSTEM_LOCK(env);
		lp = (LOG *)dblp->reginfo.primary;
		filestart = SH_TAILQ_FIRST(&lp->logfiles, __db_filestart);
		*outdatedp = filestart == NULL ? 0 : (fnum < filestart->file);
		LOG_SYSTEM_UNLOCK(env);
		return (0);
	}

	*outdatedp = 0;
	if ((ret = __log_name(dblp, fnum, &name, NULL, 0)) != 0) {
		__os_free(env, name);
		return (ret);
	}

	/* If the file exists, we're just fine. */
	if (__os_exists(env, name, NULL) == 0)
		goto out;

	/*
	 * It didn't exist, decide if the file number is too big or
	 * too little.  If it's too little, then we need to indicate
	 * that the LSN is outdated.
	 */
	LOG_SYSTEM_LOCK(env);
	lp = (LOG *)dblp->reginfo.primary;
	cfile = lp->lsn.file;
	LOG_SYSTEM_UNLOCK(env);

	if (cfile > fnum)
		*outdatedp = 1;
out:	__os_free(env, name);
	return (ret);
}

/*
 * __log_zero --
 *	Zero out the tail of a log after a truncate.
 *
 * PUBLIC: int __log_zero __P((ENV *, DB_LSN *));
 */
int
__log_zero(env, from_lsn)
	ENV *env;
	DB_LSN *from_lsn;
{
	DB_FH *fhp;
	DB_LOG *dblp;
	LOG *lp;
	struct __db_filestart *filestart, *nextstart;
	size_t nbytes, len, nw;
	u_int32_t fn, mbytes, bytes;
	u_int8_t buf[4096];
	int ret;
	char *fname;

	dblp = env->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;
	DB_ASSERT(env, LOG_COMPARE(from_lsn, &lp->lsn) <= 0);
	if (LOG_COMPARE(from_lsn, &lp->lsn) > 0) {
		__db_errx(env,
		    "Warning: truncating to point beyond end of log");
		return (0);
	}

	if (lp->db_log_inmemory) {
		/*
		 * Remove the files that are invalidated by this truncate.
		 */
		for (filestart = SH_TAILQ_FIRST(&lp->logfiles, __db_filestart);
		    filestart != NULL; filestart = nextstart) {
			nextstart = SH_TAILQ_NEXT(filestart,
			    links, __db_filestart);
			if (filestart->file > from_lsn->file) {
				SH_TAILQ_REMOVE(&lp->logfiles,
				    filestart, links, __db_filestart);
				SH_TAILQ_INSERT_HEAD(&lp->free_logfiles,
				    filestart, links, __db_filestart);
			}
		}

		return (0);
	}

	/* Close any open file handles so unlinks don't fail. */
	if (dblp->lfhp != NULL) {
		(void)__os_closehandle(env, dblp->lfhp);
		dblp->lfhp = NULL;
	}

	/* Throw away any extra log files that we have around. */
	for (fn = from_lsn->file + 1;; fn++) {
		if (__log_name(dblp, fn, &fname, &fhp, DB_OSO_RDONLY) != 0) {
			__os_free(env, fname);
			break;
		}
		(void)__os_closehandle(env, fhp);
		(void)time(&lp->timestamp);
		ret = __os_unlink(env, fname, 0);
		__os_free(env, fname);
		if (ret != 0)
			return (ret);
	}

	/* We removed some log files; have to 0 to end of file. */
	if ((ret =
	    __log_name(dblp, from_lsn->file, &fname, &dblp->lfhp, 0)) != 0) {
		__os_free(env, fname);
		return (ret);
	}
	__os_free(env, fname);
	if ((ret = __os_ioinfo(env,
	    NULL, dblp->lfhp, &mbytes, &bytes, NULL)) != 0)
		goto err;
	DB_ASSERT(env, (mbytes * MEGABYTE + bytes) >= from_lsn->offset);
	len = (mbytes * MEGABYTE + bytes) - from_lsn->offset;

	memset(buf, 0, sizeof(buf));

	/* Initialize the write position. */
	if ((ret = __os_seek(env, dblp->lfhp, 0, 0, from_lsn->offset)) != 0)
		goto err;

	while (len > 0) {
		nbytes = len > sizeof(buf) ? sizeof(buf) : len;
		if ((ret =
		    __os_write(env, dblp->lfhp, buf, nbytes, &nw)) != 0)
			goto err;
		len -= nbytes;
	}

err:	(void)__os_closehandle(env, dblp->lfhp);
	dblp->lfhp = NULL;

	return (ret);
}

/*
 * __log_inmem_lsnoff --
 *	Find the offset in the buffer of a given LSN.
 *
 * PUBLIC: int __log_inmem_lsnoff __P((DB_LOG *, DB_LSN *, size_t *));
 */
int
__log_inmem_lsnoff(dblp, lsnp, offsetp)
	DB_LOG *dblp;
	DB_LSN *lsnp;
	size_t *offsetp;
{
	LOG *lp;
	struct __db_filestart *filestart;

	lp = (LOG *)dblp->reginfo.primary;

	SH_TAILQ_FOREACH(filestart, &lp->logfiles, links, __db_filestart)
		if (filestart->file == lsnp->file) {
			*offsetp =
			    (filestart->b_off + lsnp->offset) % lp->buffer_size;
			return (0);
		}

	return (DB_NOTFOUND);
}

/*
 * __log_inmem_newfile --
 *	Records the offset of the beginning of a new file in the in-memory
 *	buffer.
 *
 * PUBLIC: int __log_inmem_newfile __P((DB_LOG *, u_int32_t));
 */
int
__log_inmem_newfile(dblp, file)
	DB_LOG *dblp;
	u_int32_t file;
{
	HDR hdr;
	LOG *lp;
	struct __db_filestart *filestart;
	int ret;
#ifdef DIAGNOSTIC
	struct __db_filestart *first, *last;
#endif

	lp = (LOG *)dblp->reginfo.primary;

	/*
	 * If the log buffer is empty, reuse the filestart entry.
	 */
	filestart = SH_TAILQ_FIRST(&lp->logfiles, __db_filestart);
	if (filestart != NULL &&
	    RINGBUF_LEN(lp, filestart->b_off, lp->b_off) <=
	    sizeof(HDR) + sizeof(LOGP)) {
		filestart->file = file;
		filestart->b_off = lp->b_off;
		return (0);
	}

	/*
	 * We write an empty header at the end of every in-memory log file.
	 * This is used during cursor traversal to indicate when to switch the
	 * LSN to the next file.
	 */
	if (file > 1) {
		memset(&hdr, 0, sizeof(HDR));
		__log_inmem_copyin(dblp, lp->b_off, &hdr, sizeof(HDR));
		lp->b_off = (lp->b_off + sizeof(HDR)) % lp->buffer_size;
	}

	filestart = SH_TAILQ_FIRST(&lp->free_logfiles, __db_filestart);
	if (filestart == NULL) {
		if ((ret = __env_alloc(&dblp->reginfo,
		    sizeof(struct __db_filestart), &filestart)) != 0)
			return (ret);
		memset(filestart, 0, sizeof(*filestart));
	} else
		SH_TAILQ_REMOVE(&lp->free_logfiles, filestart,
		    links, __db_filestart);

	filestart->file = file;
	filestart->b_off = lp->b_off;

#ifdef DIAGNOSTIC
	first = SH_TAILQ_FIRST(&lp->logfiles, __db_filestart);
	last = SH_TAILQ_LAST(&(lp)->logfiles, links, __db_filestart);

	/* Check that we don't wrap. */
	DB_ASSERT(dblp->env, !first || first == last ||
	    RINGBUF_LEN(lp, first->b_off, lp->b_off) ==
	    RINGBUF_LEN(lp, first->b_off, last->b_off) +
	    RINGBUF_LEN(lp, last->b_off, lp->b_off));
#endif

	SH_TAILQ_INSERT_TAIL(&lp->logfiles, filestart, links);
	return (0);
}

/*
 * __log_inmem_chkspace --
 *	Ensure that the requested amount of space is available in the buffer,
 *	and invalidate the region.
 *      Note: assumes that the region lock is held on entry.
 *
 * PUBLIC: int __log_inmem_chkspace __P((DB_LOG *, size_t));
 */
int
__log_inmem_chkspace(dblp, len)
	DB_LOG *dblp;
	size_t len;
{
	DB_LSN active_lsn, old_active_lsn;
	ENV *env;
	LOG *lp;
	struct __db_filestart *filestart;
	int ret;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	DB_ASSERT(env, lp->db_log_inmemory);

	/*
	 * Allow room for an extra header so that we don't need to check for
	 * space when switching files.
	 */
	len += sizeof(HDR);

	/*
	 * If transactions are enabled and we're about to fill available space,
	 * update the active LSN and recheck.  If transactions aren't enabled,
	 * don't even bother checking: in that case we can always overwrite old
	 * log records, because we're never going to abort.
	 */
	while (TXN_ON(env) &&
	    RINGBUF_LEN(lp, lp->b_off, lp->a_off) <= len) {
		old_active_lsn = lp->active_lsn;
		active_lsn = lp->lsn;

		/*
		 * Drop the log region lock so we don't hold it while
		 * taking the transaction region lock.
		 */
		LOG_SYSTEM_UNLOCK(env);
		ret = __txn_getactive(env, &active_lsn);
		LOG_SYSTEM_LOCK(env);
		if (ret != 0)
			return (ret);
		active_lsn.offset = 0;

		/* If we didn't make any progress, give up. */
		if (LOG_COMPARE(&active_lsn, &old_active_lsn) == 0) {
			__db_errx(env,
      "In-memory log buffer is full (an active transaction spans the buffer)");
			return (DB_LOG_BUFFER_FULL);
		}

		/* Make sure we're moving the region LSN forwards. */
		if (LOG_COMPARE(&active_lsn, &lp->active_lsn) > 0) {
			lp->active_lsn = active_lsn;
			(void)__log_inmem_lsnoff(dblp, &active_lsn,
			    &lp->a_off);
		}
	}

	/*
	 * Remove the first file if it is invalidated by this write.
	 * Log records can't be bigger than a file, so we only need to
	 * check the first file.
	 */
	filestart = SH_TAILQ_FIRST(&lp->logfiles, __db_filestart);
	if (filestart != NULL &&
	    RINGBUF_LEN(lp, lp->b_off, filestart->b_off) <= len) {
		SH_TAILQ_REMOVE(&lp->logfiles, filestart,
		    links, __db_filestart);
		SH_TAILQ_INSERT_HEAD(&lp->free_logfiles, filestart,
		    links, __db_filestart);
		lp->f_lsn.file = filestart->file + 1;
	}

	return (0);
}

/*
 * __log_inmem_copyout --
 *	Copies the given number of bytes from the buffer -- no checking.
 *      Note: assumes that the region lock is held on entry.
 *
 * PUBLIC: void __log_inmem_copyout __P((DB_LOG *, size_t, void *, size_t));
 */
void
__log_inmem_copyout(dblp, offset, buf, size)
	DB_LOG *dblp;
	size_t offset;
	void *buf;
	size_t size;
{
	LOG *lp;
	size_t nbytes;

	lp = (LOG *)dblp->reginfo.primary;
	nbytes = (offset + size < lp->buffer_size) ?
	    size : lp->buffer_size - offset;
	memcpy(buf, dblp->bufp + offset, nbytes);
	if (nbytes < size)
		memcpy((u_int8_t *)buf + nbytes, dblp->bufp, size - nbytes);
}

/*
 * __log_inmem_copyin --
 *	Copies the given number of bytes into the buffer -- no checking.
 *      Note: assumes that the region lock is held on entry.
 *
 * PUBLIC: void __log_inmem_copyin __P((DB_LOG *, size_t, void *, size_t));
 */
void
__log_inmem_copyin(dblp, offset, buf, size)
	DB_LOG *dblp;
	size_t offset;
	void *buf;
	size_t size;
{
	LOG *lp;
	size_t nbytes;

	lp = (LOG *)dblp->reginfo.primary;
	nbytes = (offset + size < lp->buffer_size) ?
	    size : lp->buffer_size - offset;
	memcpy(dblp->bufp + offset, buf, nbytes);
	if (nbytes < size)
		memcpy(dblp->bufp, (u_int8_t *)buf + nbytes, size - nbytes);
}

/*
 * __log_set_version --
 *	Sets the current version of the log subsystem to the given version.
 *	Essentially this modifies the lp->persist.version field in the
 *	shared memory region.  Called when region is initially created
 *	and when replication is starting up or finds a new master.
 *
 * PUBLIC: void __log_set_version __P((ENV *, u_int32_t));
 */
void
__log_set_version(env, newver)
	ENV *env;
	u_int32_t newver;
{
	DB_LOG *dblp;
	LOG *lp;

	dblp = env->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;
	/*
	 * We should be able to update this atomically without locking.
	 */
	lp->persist.version = newver;
}

/*
 * __log_get_oldversion --
 *	Returns the last version of log that this environment was working
 *	with.  Since there could be several versions of log files, if
 *	the user upgraded and didn't log archive, we check the version
 *	of the first log file, compare it to the last log file.  If those
 *	are different, then there is an older log existing, and we then
 *	walk backward in the log files looking for the version of the
 *	most recent older log file.
 *
 * PUBLIC: int __log_get_oldversion __P((ENV *, u_int32_t *));
 */
int
__log_get_oldversion(env, ver)
	ENV *env;
	u_int32_t *ver;
{
	DBT rec;
	DB_LOG *dblp;
	DB_LOGC *logc;
	DB_LSN lsn;
	LOG *lp;
	u_int32_t firstfnum, fnum, lastver, oldver;
	int ret, t_ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	logc = NULL;
	ret = 0;
	oldver = DB_LOGVERSION;
	/*
	 * If we're in-memory logs we're always the current version.
	 */
	if (lp->db_log_inmemory) {
		*ver = oldver;
		return (0);
	}
	memset(&rec, 0, sizeof(rec));
	if ((ret = __log_cursor(env, &logc)) != 0)
		goto err;
	/*
	 * Get the version numbers of the first and last log files.
	 */
	if ((ret = __logc_get(logc, &lsn, &rec, DB_FIRST)) != 0) {
		/*
		 * If there is no log file, we'll get DB_NOTFOUND.
		 * If we get that, set the version to the current.
		 */
		if (ret == DB_NOTFOUND)
			ret = 0;
		goto err;
	}
	firstfnum = lsn.file;
	if ((ret = __logc_get(logc, &lsn, &rec, DB_LAST)) != 0)
		goto err;
	if ((ret = __log_valid(dblp, firstfnum, 0, NULL, 0,
	    NULL, &oldver)) != 0)
		goto err;
	/*
	 * If the first and last LSN are in the same file, then we
	 * already have the version in oldver.  Return it.
	 */
	if (firstfnum == lsn.file)
		goto err;

	/*
	 * Otherwise they're in different files and we call __log_valid
	 * to get the version numbers in both files.
	 */
	if ((ret = __log_valid(dblp, lsn.file, 0, NULL, 0,
	    NULL, &lastver)) != 0)
		goto err;
	/*
	 * If the version numbers are different, walk backward getting
	 * the version of each log file until we find one that is
	 * different than the last.
	 */
	if (oldver != lastver) {
		for (fnum = lsn.file - 1; fnum >= firstfnum; fnum--) {
			if ((ret = __log_valid(dblp, fnum, 0, NULL, 0,
			    NULL, &oldver)) != 0)
				goto err;
			if (oldver != lastver)
				break;
		}
	}
err:	if (logc != NULL && ((t_ret = __logc_close(logc)) != 0) && ret == 0)
		ret = t_ret;
	if (ret == 0 && ver != NULL)
		*ver = oldver;
	return (ret);
}
