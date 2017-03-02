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

static int __log_encrypt_record __P((ENV *, DBT *, HDR *, u_int32_t));
static int __log_file __P((ENV *, const DB_LSN *, char *, size_t));
static int __log_fill __P((DB_LOG *, DB_LSN *, void *, u_int32_t));
static int __log_flush_commit __P((ENV *, const DB_LSN *, u_int32_t));
static int __log_newfh __P((DB_LOG *, int));
static int __log_put_next __P((ENV *,
    DB_LSN *, const DBT *, HDR *, DB_LSN *));
static int __log_putr __P((DB_LOG *,
    DB_LSN *, const DBT *, u_int32_t, HDR *));
static int __log_write __P((DB_LOG *, void *, u_int32_t));

/*
 * __log_put_pp --
 *	ENV->log_put pre/post processing.
 *
 * PUBLIC: int __log_put_pp __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
 */
int
__log_put_pp(dbenv, lsnp, udbt, flags)
	DB_ENV *dbenv;
	DB_LSN *lsnp;
	const DBT *udbt;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_ENV->log_put", DB_INIT_LOG);

	/* Validate arguments: check for allowed flags. */
	if ((ret = __db_fchk(env, "DB_ENV->log_put", flags,
	    DB_LOG_CHKPNT | DB_LOG_COMMIT |
	    DB_FLUSH | DB_LOG_NOCOPY | DB_LOG_WRNOSYNC)) != 0)
		return (ret);

	/* DB_LOG_WRNOSYNC and DB_FLUSH are mutually exclusive. */
	if (LF_ISSET(DB_LOG_WRNOSYNC) && LF_ISSET(DB_FLUSH))
		return (__db_ferr(env, "DB_ENV->log_put", 1));

	/* Replication clients should never write log records. */
	if (IS_REP_CLIENT(env)) {
		__db_errx(env,
		    "DB_ENV->log_put is illegal on replication clients");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_put(env, lsnp, udbt, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __log_put --
 *	ENV->log_put.
 *
 * PUBLIC: int __log_put __P((ENV *, DB_LSN *, const DBT *, u_int32_t));
 */
int
__log_put(env, lsnp, udbt, flags)
	ENV *env;
	DB_LSN *lsnp;
	const DBT *udbt;
	u_int32_t flags;
{
	DBT *dbt, t;
	DB_CIPHER *db_cipher;
	DB_LOG *dblp;
	DB_LSN lsn, old_lsn;
	DB_REP *db_rep;
	HDR hdr;
	LOG *lp;
	REP *rep;
	int lock_held, need_free, ret;
	u_int8_t *key;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	db_cipher = env->crypto_handle;
	db_rep = env->rep_handle;
	if (db_rep != NULL)
		rep = db_rep->region;
	else
		rep = NULL;

	dbt = &t;
	t = *udbt;
	lock_held = need_free = 0;
	ZERO_LSN(old_lsn);
	hdr.len = hdr.prev = 0;

	/*
	 * In general, if we are not a rep application, but are sharing a master
	 * rep env, we should not be writing log records.  However, we can allow
	 * a non-replication-aware process to join a pre-existing repmgr
	 * environment, if env handle meets repmgr's DB_THREAD requirement.
	 */

	if (IS_REP_MASTER(env) && db_rep->send == NULL) {
#ifdef HAVE_REPLICATION_THREADS
		if (F_ISSET(env, ENV_THREAD) &&
		    rep->my_addr.host != INVALID_ROFF) {
			if ((ret = __repmgr_autostart(env)) != 0)
				return (ret);
		} else
#endif
		{
#if !defined(DEBUG_ROP) && !defined(DEBUG_WOP)
			__db_errx(env, "%s %s",
			    "Non-replication DB_ENV handle attempting",
			    "to modify a replicated environment");
			return (EINVAL);
#endif
		}
	}
	DB_ASSERT(env, !IS_REP_CLIENT(env));

	/*
	 * If we are coming from the logging code, we use an internal flag,
	 * DB_LOG_NOCOPY, because we know we can overwrite/encrypt the log
	 * record in place.  Otherwise, if a user called log_put then we
	 * must copy it to new memory so that we know we can write it.
	 *
	 * We also must copy it to new memory if we are a replication master
	 * so that we retain an unencrypted copy of the log record to send
	 * to clients.
	 */
	if (!LF_ISSET(DB_LOG_NOCOPY) || IS_REP_MASTER(env)) {
		if (CRYPTO_ON(env))
			t.size += db_cipher->adj_size(udbt->size);
		if ((ret = __os_calloc(env, 1, t.size, &t.data)) != 0)
			goto err;
		need_free = 1;
		memcpy(t.data, udbt->data, udbt->size);
	}
	if ((ret = __log_encrypt_record(env, dbt, &hdr, udbt->size)) != 0)
		goto err;
	if (CRYPTO_ON(env))
		key = db_cipher->mac_key;
	else
		key = NULL;

	__db_chksum(&hdr, dbt->data, dbt->size, key, hdr.chksum);

	LOG_SYSTEM_LOCK(env);
	lock_held = 1;

	if ((ret = __log_put_next(env, &lsn, dbt, &hdr, &old_lsn)) != 0)
		goto panic_check;

	/*
	 * Assign the return LSN before dropping the region lock.  Necessary
	 * in case the lsn is a begin_lsn from a TXN_DETAIL structure passed in
	 * by the logging routines.  We use atomic 32-bit operations because
	 * during commit this will be a TXN_DETAIL visible_lsn field, and MVCC
	 * relies on reading the fields atomically.
	 */
	lsnp->file = lsn.file;
	lsnp->offset = lsn.offset;

#ifdef HAVE_REPLICATION
	if (IS_REP_MASTER(env)) {
		__rep_newfile_args nf_args;
		DBT newfiledbt;
		REP_BULK bulk;
		size_t len;
		u_int32_t ctlflags;
		u_int8_t buf[__REP_NEWFILE_SIZE];

		/*
		 * Replication masters need to drop the lock to send messages,
		 * but want to drop and reacquire it a minimal number of times.
		 */
		ctlflags = LF_ISSET(DB_LOG_COMMIT | DB_LOG_CHKPNT) ?
		    REPCTL_PERM : 0;
		/*
		 * If using leases, keep track of our last PERM lsn.
		 * Set this on a master under the log lock.
		 */
		if (IS_USING_LEASES(env) &&
		    FLD_ISSET(ctlflags, REPCTL_PERM))
			lp->max_perm_lsn = lsn;
		LOG_SYSTEM_UNLOCK(env);
		lock_held = 0;
		if (LF_ISSET(DB_FLUSH))
			ctlflags |= REPCTL_FLUSH;

		/*
		 * If we changed files and we're in a replicated environment,
		 * we need to inform our clients now that we've dropped the
		 * region lock.
		 *
		 * Note that a failed NEWFILE send is a dropped message that
		 * our client can handle, so we can ignore it.  It's possible
		 * that the record we already put is a commit, so we don't just
		 * want to return failure.
		 */
		if (!IS_ZERO_LSN(old_lsn)) {
			memset(&newfiledbt, 0, sizeof(newfiledbt));
			nf_args.version = lp->persist.version;
			(void)__rep_newfile_marshal(env, &nf_args,
			    buf, __REP_NEWFILE_SIZE, &len);
			DB_INIT_DBT(newfiledbt, buf, len);
			(void)__rep_send_message(env, DB_EID_BROADCAST,
			    REP_NEWFILE, &old_lsn, &newfiledbt, 0, 0);
		}

		/*
		 * If we're doing bulk processing put it in the bulk buffer.
		 */
		ret = 0;
		if (FLD_ISSET(rep->config, REP_C_BULK)) {
			/*
			 * Bulk could have been turned on by another process.
			 * If so, set the address into the bulk region now.
			 */
			if (db_rep->bulk == NULL)
				db_rep->bulk = R_ADDR(&dblp->reginfo,
				    lp->bulk_buf);
			memset(&bulk, 0, sizeof(bulk));
			bulk.addr = db_rep->bulk;
			bulk.offp = &lp->bulk_off;
			bulk.len = lp->bulk_len;
			bulk.lsn = lsn;
			bulk.type = REP_BULK_LOG;
			bulk.eid = DB_EID_BROADCAST;
			bulk.flagsp = &lp->bulk_flags;
			ret = __rep_bulk_message(env, &bulk, NULL,
			    &lsn, udbt, ctlflags);
		}
		if (!FLD_ISSET(rep->config, REP_C_BULK) ||
		    ret == DB_REP_BULKOVF) {
			/*
			 * Then send the log record itself on to our clients.
			 */
			/*
			 * !!!
			 * In the crypto case, we MUST send the udbt, not the
			 * now-encrypted dbt.  Clients have no way to decrypt
			 * without the header.
			 */
			ret = __rep_send_message(env, DB_EID_BROADCAST,
			    REP_LOG, &lsn, udbt, ctlflags, 0);
		}
		/*
		 * If the send fails and we're a commit or checkpoint,
		 * there's nothing we can do;  the record's in the log.
		 * Flush it, even if we're running with TXN_NOSYNC,
		 * on the grounds that it should be in durable
		 * form somewhere.
		 */
		if (ret != 0 && FLD_ISSET(ctlflags, REPCTL_PERM))
			LF_SET(DB_FLUSH);
		/*
		 * We ignore send failures so reset 'ret' to 0 here.
		 * We needed to check special return values from
		 * bulk transfer and errors from either bulk or normal
		 * message sending need flushing on perm records.  But
		 * otherwise we need to ignore it and reset it now.
		 */
		ret = 0;
	}
#endif

	/*
	 * If needed, do a flush.  Note that failures at this point
	 * are only permissible if we know we haven't written a commit
	 * record;  __log_flush_commit is responsible for enforcing this.
	 *
	 * If a flush is not needed, see if WRITE_NOSYNC was set and we
	 * need to write out the log buffer.
	 */
	if (LF_ISSET(DB_FLUSH | DB_LOG_WRNOSYNC)) {
		if (!lock_held) {
			LOG_SYSTEM_LOCK(env);
			lock_held = 1;
		}
		if ((ret = __log_flush_commit(env, &lsn, flags)) != 0)
			goto panic_check;
	}

	/*
	 * If flushed a checkpoint record, reset the "bytes since the last
	 * checkpoint" counters.
	 */
	if (LF_ISSET(DB_LOG_CHKPNT))
		lp->stat.st_wc_bytes = lp->stat.st_wc_mbytes = 0;

	/* Increment count of records added to the log. */
	STAT(++lp->stat.st_record);

	if (0) {
panic_check:	/*
		 * Writing log records cannot fail if we're a replication
		 * master.  The reason is that once we send the record to
		 * replication clients, the transaction can no longer
		 * abort, otherwise the master would be out of sync with
		 * the rest of the replication group.  Panic the system.
		 */
		if (ret != 0 && IS_REP_MASTER(env))
			ret = __env_panic(env, ret);
	}

err:	if (lock_held)
		LOG_SYSTEM_UNLOCK(env);
	if (need_free)
		__os_free(env, dbt->data);

	/*
	 * If auto-remove is set and we switched files, remove unnecessary
	 * log files.
	 */
	if (ret == 0 && !IS_ZERO_LSN(old_lsn) && lp->db_log_autoremove)
		__log_autoremove(env);

	return (ret);
}

/*
 * __log_current_lsn --
 *	Return the current LSN.
 *
 * PUBLIC: int __log_current_lsn
 * PUBLIC:     __P((ENV *, DB_LSN *, u_int32_t *, u_int32_t *));
 */
int
__log_current_lsn(env, lsnp, mbytesp, bytesp)
	ENV *env;
	DB_LSN *lsnp;
	u_int32_t *mbytesp, *bytesp;
{
	DB_LOG *dblp;
	LOG *lp;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	LOG_SYSTEM_LOCK(env);

	/*
	 * We need the LSN of the last entry in the log.
	 *
	 * Typically, it's easy to get the last written LSN, you simply look
	 * at the current log pointer and back up the number of bytes of the
	 * last log record.  However, if the last thing we did was write the
	 * log header of a new log file, then, this doesn't work, so we return
	 * the first log record that will be written in this new file.
	 */
	*lsnp = lp->lsn;
	if (lp->lsn.offset > lp->len)
		lsnp->offset -= lp->len;

	/*
	 * Since we're holding the log region lock, return the bytes put into
	 * the log since the last checkpoint, transaction checkpoint needs it.
	 *
	 * We add the current buffer offset so as to count bytes that have not
	 * yet been written, but are sitting in the log buffer.
	 */
	if (mbytesp != NULL) {
		*mbytesp = lp->stat.st_wc_mbytes;
		*bytesp = (u_int32_t)(lp->stat.st_wc_bytes + lp->b_off);
	}

	LOG_SYSTEM_UNLOCK(env);

	return (0);
}

/*
 * __log_put_next --
 *	Put the given record as the next in the log, wherever that may
 * turn out to be.
 */
static int
__log_put_next(env, lsn, dbt, hdr, old_lsnp)
	ENV *env;
	DB_LSN *lsn;
	const DBT *dbt;
	HDR *hdr;
	DB_LSN *old_lsnp;
{
	DB_LOG *dblp;
	DB_LSN old_lsn;
	LOG *lp;
	int adv_file, newfile, ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	/*
	 * Save a copy of lp->lsn before we might decide to switch log
	 * files and change it.  If we do switch log files, and we're
	 * doing replication, we'll need to tell our clients about the
	 * switch, and they need to receive a NEWFILE message
	 * with this "would-be" LSN in order to know they're not
	 * missing any log records.
	 */
	old_lsn = lp->lsn;
	newfile = 0;
	adv_file = 0;
	/*
	 * If our current log is at an older version and we want to write
	 * a record then we need to advance the log.
	 */
	if (lp->persist.version != DB_LOGVERSION) {
		__log_set_version(env, DB_LOGVERSION);
		adv_file = 1;
	}

	/*
	 * If this information won't fit in the file, or if we're a
	 * replication client environment and have been told to do so,
	 * swap files.
	 */
	if (adv_file || lp->lsn.offset == 0 ||
	    lp->lsn.offset + hdr->size + dbt->size > lp->log_size) {
		if (hdr->size + sizeof(LOGP) + dbt->size > lp->log_size) {
			__db_errx(env,
	    "DB_ENV->log_put: record larger than maximum file size (%lu > %lu)",
			    (u_long)hdr->size + sizeof(LOGP) + dbt->size,
			    (u_long)lp->log_size);
			return (EINVAL);
		}

		if ((ret = __log_newfile(dblp, NULL, 0, 0)) != 0)
			return (ret);

		/*
		 * Flag that we switched files, in case we're a master
		 * and need to send this information to our clients.
		 * We postpone doing the actual send until we can
		 * safely release the log region lock and are doing so
		 * anyway.
		 */
		newfile = 1;
	}

	/* If we switched log files, let our caller know where. */
	if (newfile)
		*old_lsnp = old_lsn;

	/* Actually put the record. */
	return (__log_putr(dblp, lsn, dbt, lp->lsn.offset - lp->len, hdr));
}

/*
 * __log_flush_commit --
 *	Flush a record.
 */
static int
__log_flush_commit(env, lsnp, flags)
	ENV *env;
	const DB_LSN *lsnp;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_LSN flush_lsn;
	LOG *lp;
	int ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	flush_lsn = *lsnp;

	ret = 0;

	/*
	 * DB_FLUSH:
	 *	Flush a record for which the DB_FLUSH flag to log_put was set.
	 *
	 * DB_LOG_WRNOSYNC:
	 *	If there's anything in the current log buffer, write it out.
	 */
	if (LF_ISSET(DB_FLUSH))
		ret = __log_flush_int(dblp, &flush_lsn, 1);
	else if (!lp->db_log_inmemory && lp->b_off != 0)
		if ((ret = __log_write(dblp,
		    dblp->bufp, (u_int32_t)lp->b_off)) == 0)
			lp->b_off = 0;

	/*
	 * If a flush supporting a transaction commit fails, we must abort the
	 * transaction.  (If we aren't doing a commit, return the failure; if
	 * if the commit we care about made it to disk successfully, we just
	 * ignore the failure, because there's no way to undo the commit.)
	 */
	if (ret == 0 || !LF_ISSET(DB_LOG_COMMIT))
		return (ret);

	if (flush_lsn.file != lp->lsn.file || flush_lsn.offset < lp->w_off)
		return (0);

	/*
	 * Else, make sure that the commit record does not get out after we
	 * abort the transaction.  Do this by overwriting the commit record
	 * in the buffer.  (Note that other commits in this buffer will wait
	 * until a successful write happens, we do not wake them.)  We point
	 * at the right part of the buffer and write an abort record over the
	 * commit.  We must then try and flush the buffer again, since the
	 * interesting part of the buffer may have actually made it out to
	 * disk before there was a failure, we can't know for sure.
	 */
	if (__txn_force_abort(env,
	    dblp->bufp + flush_lsn.offset - lp->w_off) == 0)
		(void)__log_flush_int(dblp, &flush_lsn, 0);

	return (ret);
}

/*
 * __log_newfile --
 *	Initialize and switch to a new log file.  (Note that this is
 * called both when no log yet exists and when we fill a log file.)
 *
 * PUBLIC: int __log_newfile __P((DB_LOG *, DB_LSN *, u_int32_t, u_int32_t));
 */
int
__log_newfile(dblp, lsnp, logfile, version)
	DB_LOG *dblp;
	DB_LSN *lsnp;
	u_int32_t logfile;
	u_int32_t version;
{
	DBT t;
	DB_CIPHER *db_cipher;
	DB_LSN lsn;
	ENV *env;
	HDR hdr;
	LOG *lp;
	LOGP *tpersist;
	int need_free, ret;
	u_int32_t lastoff;
	size_t tsize;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	/*
	 * If we're not specifying a specific log file number and we're
	 * not at the beginning of a file already, start a new one.
	 */
	if (logfile == 0 && lp->lsn.offset != 0) {
		/*
		 * Flush the log so this file is out and can be closed.  We
		 * cannot release the region lock here because we need to
		 * protect the end of the file while we switch.  In
		 * particular, a thread with a smaller record than ours
		 * could detect that there is space in the log. Even
		 * blocking that event by declaring the file full would
		 * require all threads to wait here so that the lsn.file
		 * can be moved ahead after the flush completes.  This
		 * probably can be changed if we had an lsn for the
		 * previous file and one for the current, but it does not
		 * seem like this would get much more throughput, if any.
		 */
		if ((ret = __log_flush_int(dblp, NULL, 0)) != 0)
			return (ret);

		/*
		 * Save the last known offset from the previous file, we'll
		 * need it to initialize the persistent header information.
		 */
		lastoff = lp->lsn.offset;

		/* Point the current LSN to the new file. */
		++lp->lsn.file;
		lp->lsn.offset = 0;

		/* Reset the file write offset. */
		lp->w_off = 0;
	} else
		lastoff = 0;

	/*
	 * Replication may require we reset the log file name space entirely.
	 * In that case we also force a file switch so that replication can
	 * clean up old files.
	 */
	if (logfile != 0) {
		lp->lsn.file = logfile;
		lp->lsn.offset = 0;
		lp->w_off = 0;
		if (lp->db_log_inmemory) {
			lsn = lp->lsn;
			(void)__log_zero(env, &lsn);
		} else {
			lp->s_lsn = lp->lsn;
			if ((ret = __log_newfh(dblp, 1)) != 0)
				return (ret);
		}
	}

	DB_ASSERT(env, lp->db_log_inmemory || lp->b_off == 0);
	if (lp->db_log_inmemory &&
	    (ret = __log_inmem_newfile(dblp, lp->lsn.file)) != 0)
		return (ret);

	/*
	 * Insert persistent information as the first record in every file.
	 * Note that the previous length is wrong for the very first record
	 * of the log, but that's okay, we check for it during retrieval.
	 */
	memset(&t, 0, sizeof(t));
	memset(&hdr, 0, sizeof(HDR));

	need_free = 0;
	tsize = sizeof(LOGP);
	db_cipher = env->crypto_handle;
	if (CRYPTO_ON(env))
		tsize += db_cipher->adj_size(tsize);
	if ((ret = __os_calloc(env, 1, tsize, &tpersist)) != 0)
		return (ret);
	need_free = 1;
	/*
	 * If we're told what version to make this file, then we
	 * need to be at that version.  Update here.
	 */
	if (version != 0) {
		__log_set_version(env, version);
		if ((ret = __env_init_rec(env, version)) != 0)
			goto err;
	}
	lp->persist.log_size = lp->log_size = lp->log_nsize;
	memcpy(tpersist, &lp->persist, sizeof(LOGP));
	DB_SET_DBT(t, tpersist, tsize);
	if (LOG_SWAPPED(env))
		__log_persistswap(tpersist);

	if ((ret =
	    __log_encrypt_record(env, &t, &hdr, (u_int32_t)tsize)) != 0)
		goto err;
	if (lp->persist.version != DB_LOGVERSION)
		__db_chksum(NULL, t.data, t.size,
		    (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr.chksum);
	else
		__db_chksum(&hdr, t.data, t.size,
		    (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr.chksum);

	if ((ret = __log_putr(dblp, &lsn,
	    &t, lastoff == 0 ? 0 : lastoff - lp->len, &hdr)) != 0)
		goto err;

	/* Update the LSN information returned to the caller. */
	if (lsnp != NULL)
		*lsnp = lp->lsn;

err:	if (need_free)
		__os_free(env, tpersist);
	return (ret);
}

/*
 * __log_putr --
 *	Actually put a record into the log.
 */
static int
__log_putr(dblp, lsn, dbt, prev, h)
	DB_LOG *dblp;
	DB_LSN *lsn;
	const DBT *dbt;
	u_int32_t prev;
	HDR *h;
{
	DB_CIPHER *db_cipher;
	DB_LSN f_lsn;
	ENV *env;
	HDR tmp, *hdr;
	LOG *lp;
	int ret, t_ret;
	size_t b_off, nr;
	u_int32_t w_off;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	/*
	 * If we weren't given a header, use a local one.
	 */
	db_cipher = env->crypto_handle;
	if (h == NULL) {
		hdr = &tmp;
		memset(hdr, 0, sizeof(HDR));
		if (CRYPTO_ON(env))
			hdr->size = HDR_CRYPTO_SZ;
		else
			hdr->size = HDR_NORMAL_SZ;
	} else
		hdr = h;

	/* Save our position in case we fail. */
	b_off = lp->b_off;
	w_off = lp->w_off;
	f_lsn = lp->f_lsn;

	/*
	 * Initialize the header.  If we just switched files, lsn.offset will
	 * be 0, and what we really want is the offset of the previous record
	 * in the previous file.  Fortunately, prev holds the value we want.
	 */
	hdr->prev = prev;
	hdr->len = (u_int32_t)hdr->size + dbt->size;

	/*
	 * If we were passed in a nonzero checksum, our caller calculated
	 * the checksum before acquiring the log mutex, as an optimization.
	 *
	 * If our caller calculated a real checksum of 0, we'll needlessly
	 * recalculate it.  C'est la vie;  there's no out-of-bounds value
	 * here.
	 */
	if (hdr->chksum[0] == 0)
		if (lp->persist.version != DB_LOGVERSION)
			__db_chksum(NULL, dbt->data, dbt->size,
			    (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL,
			    hdr->chksum);
		else
			__db_chksum(hdr, dbt->data, dbt->size,
			    (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL,
			    hdr->chksum);
	else if (lp->persist.version == DB_LOGVERSION) {
		/*
		 * We need to correct for prev and len since they are not
		 * set before here.
		 */
		LOG_HDR_SUM(CRYPTO_ON(env), hdr, hdr->chksum);
	}

	if (lp->db_log_inmemory && (ret = __log_inmem_chkspace(dblp,
	    (u_int32_t)hdr->size + dbt->size)) != 0)
		goto err;

	/*
	 * The offset into the log file at this point is the LSN where
	 * we're about to put this record, and is the LSN the caller wants.
	 */
	*lsn = lp->lsn;

	nr = hdr->size;
	if (LOG_SWAPPED(env))
		__log_hdrswap(hdr, CRYPTO_ON(env));

	 /* nr can't overflow a 32 bit value - header size is internal. */
	ret = __log_fill(dblp, lsn, hdr, (u_int32_t)nr);

	if (LOG_SWAPPED(env))
		__log_hdrswap(hdr, CRYPTO_ON(env));

	if (ret != 0)
		goto err;

	if ((ret = __log_fill(dblp, lsn, dbt->data, dbt->size)) != 0)
		goto err;

	lp->len = (u_int32_t)(hdr->size + dbt->size);
	lp->lsn.offset += lp->len;
	return (0);
err:
	/*
	 * If we wrote more than one buffer before failing, get the
	 * first one back.  The extra buffers will fail the checksums
	 * and be ignored.
	 */
	if (w_off + lp->buffer_size < lp->w_off) {
		DB_ASSERT(env, !lp->db_log_inmemory);
		if ((t_ret = __os_seek(env, dblp->lfhp, 0, 0, w_off)) != 0 ||
		    (t_ret = __os_read(env, dblp->lfhp, dblp->bufp,
		    b_off, &nr)) != 0)
			return (__env_panic(env, t_ret));
		if (nr != b_off) {
			__db_errx(env, "Short read while restoring log");
			return (__env_panic(env, EIO));
		}
	}

	/* Reset to where we started. */
	lp->w_off = w_off;
	lp->b_off = b_off;
	lp->f_lsn = f_lsn;

	return (ret);
}

/*
 * __log_flush_pp --
 *	ENV->log_flush pre/post processing.
 *
 * PUBLIC: int __log_flush_pp __P((DB_ENV *, const DB_LSN *));
 */
int
__log_flush_pp(dbenv, lsn)
	DB_ENV *dbenv;
	const DB_LSN *lsn;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_ENV->log_flush", DB_INIT_LOG);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_flush(env, lsn)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * See if we need to wait.  s_lsn is not locked so some care is needed.
 * The sync point can only move forward.  The lsnp->file cannot be
 * greater than the s_lsn.file.  If the file we want is in the past
 * we are done.  If the file numbers are the same check the offset.
 * This all assumes we can read an 32-bit quantity in one state or
 * the other, not in transition.
 */
#define	ALREADY_FLUSHED(lp, lsnp)					\
	(((lp)->s_lsn.file > (lsnp)->file) ||				\
	((lp)->s_lsn.file == (lsnp)->file &&				\
	    (lp)->s_lsn.offset > (lsnp)->offset))

/*
 * __log_flush --
 *	ENV->log_flush
 *
 * PUBLIC: int __log_flush __P((ENV *, const DB_LSN *));
 */
int
__log_flush(env, lsn)
	ENV *env;
	const DB_LSN *lsn;
{
	DB_LOG *dblp;
	LOG *lp;
	int ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;
	if (lsn != NULL && ALREADY_FLUSHED(lp, lsn))
		return (0);
	LOG_SYSTEM_LOCK(env);
	ret = __log_flush_int(dblp, lsn, 1);
	LOG_SYSTEM_UNLOCK(env);
	return (ret);
}

/*
 * __log_flush_int --
 *	Write all records less than or equal to the specified LSN; internal
 *	version.
 *
 * PUBLIC: int __log_flush_int __P((DB_LOG *, const DB_LSN *, int));
 */
int
__log_flush_int(dblp, lsnp, release)
	DB_LOG *dblp;
	const DB_LSN *lsnp;
	int release;
{
	struct __db_commit *commit;
	ENV *env;
	DB_LSN flush_lsn, f_lsn;
	LOG *lp;
	size_t b_off;
	u_int32_t ncommit, w_off;
	int do_flush, first, ret;

	env = dblp->env;
	lp = dblp->reginfo.primary;
	ncommit = 0;
	ret = 0;

	if (lp->db_log_inmemory) {
		lp->s_lsn = lp->lsn;
		STAT(++lp->stat.st_scount);
		return (0);
	}

	/*
	 * If no LSN specified, flush the entire log by setting the flush LSN
	 * to the last LSN written in the log.  Otherwise, check that the LSN
	 * isn't a non-existent record for the log.
	 */
	if (lsnp == NULL) {
		flush_lsn.file = lp->lsn.file;
		flush_lsn.offset = lp->lsn.offset - lp->len;
	} else if (lsnp->file > lp->lsn.file ||
	    (lsnp->file == lp->lsn.file &&
	    lsnp->offset > lp->lsn.offset - lp->len)) {
		__db_errx(env,
    "DB_ENV->log_flush: LSN of %lu/%lu past current end-of-log of %lu/%lu",
		    (u_long)lsnp->file, (u_long)lsnp->offset,
		    (u_long)lp->lsn.file, (u_long)lp->lsn.offset);
		__db_errx(env, "%s %s %s",
		    "Database environment corrupt; the wrong log files may",
		    "have been removed or incompatible database files imported",
		    "from another environment");
		return (__env_panic(env, DB_RUNRECOVERY));
	} else {
		if (ALREADY_FLUSHED(lp, lsnp))
			return (0);
		flush_lsn = *lsnp;
	}

	/*
	 * If a flush is in progress and we're allowed to do so, drop
	 * the region lock and block waiting for the next flush.
	 */
	if (release && lp->in_flush != 0) {
		if ((commit = SH_TAILQ_FIRST(
		    &lp->free_commits, __db_commit)) == NULL) {
			if ((ret = __env_alloc(&dblp->reginfo,
			    sizeof(struct __db_commit), &commit)) != 0)
				goto flush;
			memset(commit, 0, sizeof(*commit));
			if ((ret = __mutex_alloc(env, MTX_TXN_COMMIT,
			    DB_MUTEX_SELF_BLOCK, &commit->mtx_txnwait)) != 0) {
				__env_alloc_free(&dblp->reginfo, commit);
				return (ret);
			}
			MUTEX_LOCK(env, commit->mtx_txnwait);
		} else
			SH_TAILQ_REMOVE(
			    &lp->free_commits, commit, links, __db_commit);

		lp->ncommit++;

		/*
		 * Flushes may be requested out of LSN order;  be
		 * sure we only move lp->t_lsn forward.
		 */
		if (LOG_COMPARE(&lp->t_lsn, &flush_lsn) < 0)
			lp->t_lsn = flush_lsn;

		commit->lsn = flush_lsn;
		SH_TAILQ_INSERT_HEAD(
		    &lp->commits, commit, links, __db_commit);
		LOG_SYSTEM_UNLOCK(env);
		/* Wait here for the in-progress flush to finish. */
		MUTEX_LOCK(env, commit->mtx_txnwait);
		LOG_SYSTEM_LOCK(env);

		lp->ncommit--;
		/*
		 * Grab the flag before freeing the struct to see if
		 * we need to flush the log to commit.  If so,
		 * use the maximal lsn for any committing thread.
		 */
		do_flush = F_ISSET(commit, DB_COMMIT_FLUSH);
		F_CLR(commit, DB_COMMIT_FLUSH);
		SH_TAILQ_INSERT_HEAD(
		    &lp->free_commits, commit, links, __db_commit);
		if (do_flush) {
			lp->in_flush--;
			flush_lsn = lp->t_lsn;
		} else
			return (0);
	}

	/*
	 * Protect flushing with its own mutex so we can release
	 * the region lock except during file switches.
	 */
flush:	MUTEX_LOCK(env, lp->mtx_flush);

	/*
	 * If the LSN is less than or equal to the last-sync'd LSN, we're done.
	 * Note, the last-sync LSN saved in s_lsn is the LSN of the first byte
	 * after the byte we absolutely know was written to disk, so the test
	 * is <, not <=.
	 */
	if (flush_lsn.file < lp->s_lsn.file ||
	    (flush_lsn.file == lp->s_lsn.file &&
	    flush_lsn.offset < lp->s_lsn.offset)) {
		MUTEX_UNLOCK(env, lp->mtx_flush);
		goto done;
	}

	/*
	 * We may need to write the current buffer.  We have to write the
	 * current buffer if the flush LSN is greater than or equal to the
	 * buffer's starting LSN.
	 *
	 * Otherwise, it's still possible that this thread may never have
	 * written to this log file.  Acquire a file descriptor if we don't
	 * already have one.
	 */
	if (lp->b_off != 0 && LOG_COMPARE(&flush_lsn, &lp->f_lsn) >= 0) {
		if ((ret = __log_write(dblp,
		    dblp->bufp, (u_int32_t)lp->b_off)) != 0) {
			MUTEX_UNLOCK(env, lp->mtx_flush);
			goto done;
		}

		lp->b_off = 0;
	} else if (dblp->lfhp == NULL || dblp->lfname != lp->lsn.file)
		if ((ret = __log_newfh(dblp, 0)) != 0) {
			MUTEX_UNLOCK(env, lp->mtx_flush);
			goto done;
		}

	/*
	 * We are going to flush, release the region.
	 * First get the current state of the buffer since
	 * another write may come in, but we may not flush it.
	 */
	b_off = lp->b_off;
	w_off = lp->w_off;
	f_lsn = lp->f_lsn;
	lp->in_flush++;
	if (release)
		LOG_SYSTEM_UNLOCK(env);

	/* Sync all writes to disk. */
	if ((ret = __os_fsync(env, dblp->lfhp)) != 0) {
		MUTEX_UNLOCK(env, lp->mtx_flush);
		if (release)
			LOG_SYSTEM_LOCK(env);
		ret = __env_panic(env, ret);
		return (ret);
	}

	/*
	 * Set the last-synced LSN.
	 * This value must be set to the LSN past the last complete
	 * record that has been flushed.  This is at least the first
	 * lsn, f_lsn.  If the buffer is empty, b_off == 0, then
	 * we can move up to write point since the first lsn is not
	 * set for the new buffer.
	 */
	lp->s_lsn = f_lsn;
	if (b_off == 0)
		lp->s_lsn.offset = w_off;

	MUTEX_UNLOCK(env, lp->mtx_flush);
	if (release)
		LOG_SYSTEM_LOCK(env);

	lp->in_flush--;
	STAT(++lp->stat.st_scount);

	/*
	 * How many flush calls (usually commits) did this call actually sync?
	 * At least one, if it got here.
	 */
	ncommit = 1;
done:
	if (lp->ncommit != 0) {
		first = 1;
		SH_TAILQ_FOREACH(commit, &lp->commits, links, __db_commit)
			if (LOG_COMPARE(&lp->s_lsn, &commit->lsn) > 0) {
				MUTEX_UNLOCK(env, commit->mtx_txnwait);
				SH_TAILQ_REMOVE(
				    &lp->commits, commit, links, __db_commit);
				ncommit++;
			} else if (first == 1) {
				F_SET(commit, DB_COMMIT_FLUSH);
				MUTEX_UNLOCK(env, commit->mtx_txnwait);
				SH_TAILQ_REMOVE(
				    &lp->commits, commit, links, __db_commit);
				/*
				 * This thread will wake and flush.
				 * If another thread commits and flushes
				 * first we will waste a trip trough the
				 * mutex.
				 */
				lp->in_flush++;
				first = 0;
			}
	}
#ifdef HAVE_STATISTICS
	if (lp->stat.st_maxcommitperflush < ncommit)
		lp->stat.st_maxcommitperflush = ncommit;
	if (lp->stat.st_mincommitperflush > ncommit ||
	    lp->stat.st_mincommitperflush == 0)
		lp->stat.st_mincommitperflush = ncommit;
#endif

	return (ret);
}

/*
 * __log_fill --
 *	Write information into the log.
 */
static int
__log_fill(dblp, lsn, addr, len)
	DB_LOG *dblp;
	DB_LSN *lsn;
	void *addr;
	u_int32_t len;
{
	LOG *lp;
	u_int32_t bsize, nrec;
	size_t nw, remain;
	int ret;

	lp = dblp->reginfo.primary;
	bsize = lp->buffer_size;

	if (lp->db_log_inmemory) {
		__log_inmem_copyin(dblp, lp->b_off, addr, len);
		lp->b_off = (lp->b_off + len) % lp->buffer_size;
		return (0);
	}

	while (len > 0) {			/* Copy out the data. */
		/*
		 * If we're beginning a new buffer, note the user LSN to which
		 * the first byte of the buffer belongs.  We have to know this
		 * when flushing the buffer so that we know if the in-memory
		 * buffer needs to be flushed.
		 */
		if (lp->b_off == 0)
			lp->f_lsn = *lsn;

		/*
		 * If we're on a buffer boundary and the data is big enough,
		 * copy as many records as we can directly from the data.
		 */
		if (lp->b_off == 0 && len >= bsize) {
			nrec = len / bsize;
			if ((ret = __log_write(dblp, addr, nrec * bsize)) != 0)
				return (ret);
			addr = (u_int8_t *)addr + nrec * bsize;
			len -= nrec * bsize;
			STAT(++lp->stat.st_wcount_fill);
			continue;
		}

		/* Figure out how many bytes we can copy this time. */
		remain = bsize - lp->b_off;
		nw = remain > len ? len : remain;
		memcpy(dblp->bufp + lp->b_off, addr, nw);
		addr = (u_int8_t *)addr + nw;
		len -= (u_int32_t)nw;
		lp->b_off += nw;

		/* If we fill the buffer, flush it. */
		if (lp->b_off == bsize) {
			if ((ret = __log_write(dblp, dblp->bufp, bsize)) != 0)
				return (ret);
			lp->b_off = 0;
			STAT(++lp->stat.st_wcount_fill);
		}
	}
	return (0);
}

/*
 * __log_write --
 *	Write the log buffer to disk.
 */
static int
__log_write(dblp, addr, len)
	DB_LOG *dblp;
	void *addr;
	u_int32_t len;
{
	ENV *env;
	LOG *lp;
	size_t nw;
	int ret;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	DB_ASSERT(env, !lp->db_log_inmemory);

	/*
	 * If we haven't opened the log file yet or the current one has
	 * changed, acquire a new log file.  We are creating the file if we're
	 * about to write to the start of it, in other words, if the write
	 * offset is zero.
	 */
	if (dblp->lfhp == NULL || dblp->lfname != lp->lsn.file ||
	    dblp->lf_timestamp != lp->timestamp)
		if ((ret = __log_newfh(dblp, lp->w_off == 0)) != 0)
			return (ret);

	/*
	 * If we're writing the first block in a log file on a filesystem that
	 * guarantees unwritten blocks are zero-filled, we set the size of the
	 * file in advance.  This increases sync performance on some systems,
	 * because they don't need to update metadata on every sync.
	 *
	 * Ignore any error -- we may have run out of disk space, but that's no
	 * reason to quit.
	 */
#ifdef HAVE_FILESYSTEM_NOTZERO
	if (lp->w_off == 0 && !__os_fs_notzero()) {
#else
	if (lp->w_off == 0) {
#endif
		(void)__db_file_extend(env, dblp->lfhp, lp->log_size);
		if (F_ISSET(dblp, DBLOG_ZERO))
			(void)__db_zero_extend(env, dblp->lfhp,
			     0, lp->log_size/lp->buffer_size, lp->buffer_size);

	}

	/*
	 * Seek to the offset in the file (someone may have written it
	 * since we last did).
	 */
	if ((ret = __os_io(env, DB_IO_WRITE,
	    dblp->lfhp, 0, 0, lp->w_off, len, addr, &nw)) != 0)
		return (ret);

	/* Reset the buffer offset and update the seek offset. */
	lp->w_off += len;

	/* Update written statistics. */
	if ((lp->stat.st_wc_bytes += len) >= MEGABYTE) {
		lp->stat.st_wc_bytes -= MEGABYTE;
		++lp->stat.st_wc_mbytes;
	}
#ifdef HAVE_STATISTICS
	if ((lp->stat.st_w_bytes += len) >= MEGABYTE) {
		lp->stat.st_w_bytes -= MEGABYTE;
		++lp->stat.st_w_mbytes;
	}
	++lp->stat.st_wcount;
#endif

	return (0);
}

/*
 * __log_file_pp --
 *	ENV->log_file pre/post processing.
 *
 * PUBLIC: int __log_file_pp __P((DB_ENV *, const DB_LSN *, char *, size_t));
 */
int
__log_file_pp(dbenv, lsn, namep, len)
	DB_ENV *dbenv;
	const DB_LSN *lsn;
	char *namep;
	size_t len;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret, set;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->lg_handle, "DB_ENV->log_file", DB_INIT_LOG);

	if ((ret = __log_get_config(dbenv, DB_LOG_IN_MEMORY, &set)) != 0)
		return (ret);
	if (set) {
		__db_errx(env,
		    "DB_ENV->log_file is illegal with in-memory logs");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__log_file(env, lsn, namep, len)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __log_file --
 *	ENV->log_file.
 */
static int
__log_file(env, lsn, namep, len)
	ENV *env;
	const DB_LSN *lsn;
	char *namep;
	size_t len;
{
	DB_LOG *dblp;
	int ret;
	char *name;

	dblp = env->lg_handle;
	LOG_SYSTEM_LOCK(env);
	ret = __log_name(dblp, lsn->file, &name, NULL, 0);
	LOG_SYSTEM_UNLOCK(env);
	if (ret != 0)
		return (ret);

	/* Check to make sure there's enough room and copy the name. */
	if (len < strlen(name) + 1) {
		*namep = '\0';
		__db_errx(env, "DB_ENV->log_file: name buffer is too short");
		return (EINVAL);
	}
	(void)strcpy(namep, name);
	__os_free(env, name);

	return (0);
}

/*
 * __log_newfh --
 *	Acquire a file handle for the current log file.
 */
static int
__log_newfh(dblp, create)
	DB_LOG *dblp;
	int create;
{
	ENV *env;
	LOG *lp;
	u_int32_t flags;
	int ret;
	logfile_validity status;

	env = dblp->env;
	lp = dblp->reginfo.primary;

	/* Close any previous file descriptor. */
	if (dblp->lfhp != NULL) {
		(void)__os_closehandle(env, dblp->lfhp);
		dblp->lfhp = NULL;
	}

	flags = DB_OSO_SEQ |
	    (create ? DB_OSO_CREATE : 0) |
	    (F_ISSET(dblp, DBLOG_DIRECT) ? DB_OSO_DIRECT : 0) |
	    (F_ISSET(dblp, DBLOG_DSYNC) ? DB_OSO_DSYNC : 0);

	/* Get the path of the new file and open it. */
	dblp->lfname = lp->lsn.file;
	if ((ret = __log_valid(dblp, dblp->lfname, 0, &dblp->lfhp,
	    flags, &status, NULL)) != 0)
		__db_err(env, ret,
		    "DB_ENV->log_newfh: %lu", (u_long)lp->lsn.file);
	else if (status != DB_LV_NORMAL && status != DB_LV_INCOMPLETE &&
	    status != DB_LV_OLD_READABLE)
		ret = DB_NOTFOUND;

	return (ret);
}

/*
 * __log_name --
 *	Return the log name for a particular file, and optionally open it.
 *
 * PUBLIC: int __log_name __P((DB_LOG *,
 * PUBLIC:     u_int32_t, char **, DB_FH **, u_int32_t));
 */
int
__log_name(dblp, filenumber, namep, fhpp, flags)
	DB_LOG *dblp;
	u_int32_t filenumber, flags;
	char **namep;
	DB_FH **fhpp;
{
	ENV *env;
	LOG *lp;
	int mode, ret;
	char *oname;
	char old[sizeof(LFPREFIX) + 5 + 20], new[sizeof(LFPREFIX) + 10 + 20];

	env = dblp->env;
	lp = dblp->reginfo.primary;

	DB_ASSERT(env, !lp->db_log_inmemory);

	/*
	 * !!!
	 * The semantics of this routine are bizarre.
	 *
	 * The reason for all of this is that we need a place where we can
	 * intercept requests for log files, and, if appropriate, check for
	 * both the old-style and new-style log file names.  The trick is
	 * that all callers of this routine that are opening the log file
	 * read-only want to use an old-style file name if they can't find
	 * a match using a new-style name.  The only down-side is that some
	 * callers may check for the old-style when they really don't need
	 * to, but that shouldn't mess up anything, and we only check for
	 * the old-style name when we've already failed to find a new-style
	 * one.
	 *
	 * Create a new-style file name, and if we're not going to open the
	 * file, return regardless.
	 */
	(void)snprintf(new, sizeof(new), LFNAME, filenumber);
	if ((ret = __db_appname(env,
	    DB_APP_LOG, new, NULL, namep)) != 0 || fhpp == NULL)
		return (ret);

	/* The application may have specified an absolute file mode. */
	if (lp->filemode == 0)
		mode = env->db_mode;
	else {
		LF_SET(DB_OSO_ABSMODE);
		mode = lp->filemode;
	}

	/* Open the new-style file -- if we succeed, we're done. */
	dblp->lf_timestamp = lp->timestamp;
	if ((ret = __os_open(env, *namep, 0, flags, mode, fhpp)) == 0)
		return (0);

	/*
	 * If the open failed for reason other than the file
	 * not being there, complain loudly, the wrong user
	 * probably started up the application.
	 */
	if (ret != ENOENT) {
		__db_err(env, ret, "%s: log file unreadable", *namep);
		return (__env_panic(env, ret));
	}

	/*
	 * The open failed... if the DB_RDONLY flag isn't set, we're done,
	 * the caller isn't interested in old-style files.
	 */
	if (!LF_ISSET(DB_OSO_RDONLY)) {
		__db_err(env, ret, "%s: log file open failed", *namep);
		return (__env_panic(env, ret));
	}

	/* Create an old-style file name. */
	(void)snprintf(old, sizeof(old), LFNAME_V1, filenumber);
	if ((ret = __db_appname(env,
	    DB_APP_LOG, old, NULL, &oname)) != 0)
		goto err;

	/*
	 * Open the old-style file -- if we succeed, we're done.  Free the
	 * space allocated for the new-style name and return the old-style
	 * name to the caller.
	 */
	if ((ret = __os_open(env, oname, 0, flags, mode, fhpp)) == 0) {
		__os_free(env, *namep);
		*namep = oname;
		return (0);
	}

	/*
	 * Couldn't find either style of name -- return the new-style name
	 * for the caller's error message.  If it's an old-style name that's
	 * actually missing we're going to confuse the user with the error
	 * message, but that implies that not only were we looking for an
	 * old-style name, but we expected it to exist and we weren't just
	 * looking for any log file.  That's not a likely error.
	 */
err:	__os_free(env, oname);
	return (ret);
}

/*
 * __log_rep_put --
 *	Short-circuit way for replication clients to put records into the
 * log.  Replication clients' logs need to be laid out exactly as their masters'
 * are, so we let replication take responsibility for when the log gets
 * flushed, when log switches files, etc.  This is just a thin PUBLIC wrapper
 * for __log_putr with a slightly prettier interface.
 *
 * Note that the REP->mtx_clientdb should be held when this is called.
 * Note that we acquire the log region mutex while holding mtx_clientdb.
 *
 * PUBLIC: int __log_rep_put __P((ENV *, DB_LSN *, const DBT *, u_int32_t));
 */
int
__log_rep_put(env, lsnp, rec, flags)
	ENV *env;
	DB_LSN *lsnp;
	const DBT *rec;
	u_int32_t flags;
{
	DBT *dbt, t;
	DB_CIPHER *db_cipher;
	DB_LOG *dblp;
	HDR hdr;
	LOG *lp;
	int need_free, ret;

	dblp = env->lg_handle;
	lp = dblp->reginfo.primary;

	LOG_SYSTEM_LOCK(env);
	memset(&hdr, 0, sizeof(HDR));
	t = *rec;
	dbt = &t;
	need_free = 0;
	db_cipher = env->crypto_handle;
	if (CRYPTO_ON(env))
		t.size += db_cipher->adj_size(rec->size);
	if ((ret = __os_calloc(env, 1, t.size, &t.data)) != 0)
		goto err;
	need_free = 1;
	memcpy(t.data, rec->data, rec->size);

	if ((ret = __log_encrypt_record(env, dbt, &hdr, rec->size)) != 0)
		goto err;
	__db_chksum(&hdr, t.data, t.size,
	    (CRYPTO_ON(env)) ? db_cipher->mac_key : NULL, hdr.chksum);

	DB_ASSERT(env, LOG_COMPARE(lsnp, &lp->lsn) == 0);
	ret = __log_putr(dblp, lsnp, dbt, lp->lsn.offset - lp->len, &hdr);
err:
	/*
	 * !!! Assume caller holds REP->mtx_clientdb to modify ready_lsn.
	 */
	lp->ready_lsn = lp->lsn;

	if (LF_ISSET(DB_LOG_CHKPNT))
		lp->stat.st_wc_bytes = lp->stat.st_wc_mbytes = 0;

	/* Increment count of records added to the log. */
	STAT(++lp->stat.st_record);
	LOG_SYSTEM_UNLOCK(env);
	if (need_free)
		__os_free(env, t.data);
	return (ret);
}

static int
__log_encrypt_record(env, dbt, hdr, orig)
	ENV *env;
	DBT *dbt;
	HDR *hdr;
	u_int32_t orig;
{
	DB_CIPHER *db_cipher;
	int ret;

	if (CRYPTO_ON(env)) {
		db_cipher = env->crypto_handle;
		hdr->size = HDR_CRYPTO_SZ;
		hdr->orig_size = orig;
		if ((ret = db_cipher->encrypt(env, db_cipher->data,
		    hdr->iv, dbt->data, dbt->size)) != 0)
			return (ret);
	} else {
		hdr->size = HDR_NORMAL_SZ;
	}
	return (0);
}
