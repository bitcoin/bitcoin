/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"

typedef struct {
	DB_MPOOL_HASH *track_hp;	/* Hash bucket. */

	roff_t	  track_off;		/* Page file offset. */
	db_pgno_t track_pgno;		/* Page number. */
} BH_TRACK;

static int __bhcmp __P((const void *, const void *));
static int __memp_close_flush_files __P((ENV *, int));
static int __memp_sync_files __P((ENV *));
static int __memp_sync_file __P((ENV *,
		MPOOLFILE *, void *, u_int32_t *, u_int32_t));

/*
 * __memp_walk_files --
 * PUBLIC: int __memp_walk_files __P((ENV *, MPOOL *,
 * PUBLIC:	int (*) __P((ENV *, MPOOLFILE *, void *,
 * PUBLIC:	u_int32_t *, u_int32_t)), void *, u_int32_t *, u_int32_t));
 */
int
__memp_walk_files(env, mp, func, arg, countp, flags)
	ENV *env;
	MPOOL *mp;
	int (*func)__P((ENV *, MPOOLFILE *, void *, u_int32_t *, u_int32_t));
	void *arg;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOLFILE *mfp;
	int i, ret, t_ret;

	dbmp = env->mp_handle;
	ret = 0;

	hp = R_ADDR(dbmp->reginfo, mp->ftab);
	for (i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(env, hp->mtx_hash);
		SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
			if ((t_ret = func(env,
			    mfp, arg, countp, flags)) != 0 && ret == 0)
				ret = t_ret;
			if (ret != 0 && !LF_ISSET(DB_STAT_MEMP_NOERROR))
				break;
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
		if (ret != 0 && !LF_ISSET(DB_STAT_MEMP_NOERROR))
			break;
	}
	return (ret);
}

/*
 * __memp_sync_pp --
 *	ENV->memp_sync pre/post processing.
 *
 * PUBLIC: int __memp_sync_pp __P((DB_ENV *, DB_LSN *));
 */
int
__memp_sync_pp(dbenv, lsnp)
	DB_ENV *dbenv;
	DB_LSN *lsnp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->mp_handle, "memp_sync", DB_INIT_MPOOL);

	/*
	 * If no LSN is provided, flush the entire cache (reasonable usage
	 * even if there's no log subsystem configured).
	 */
	if (lsnp != NULL)
		ENV_REQUIRES_CONFIG(env,
		    env->lg_handle, "memp_sync", DB_INIT_LOG);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_sync(env, DB_SYNC_CACHE, lsnp)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_sync --
 *	ENV->memp_sync.
 *
 * PUBLIC: int __memp_sync __P((ENV *, u_int32_t, DB_LSN *));
 */
int
__memp_sync(env, flags, lsnp)
	ENV *env;
	u_int32_t flags;
	DB_LSN *lsnp;
{
	DB_MPOOL *dbmp;
	MPOOL *mp;
	int interrupted, ret;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;

	/* If we've flushed to the requested LSN, return that information. */
	if (lsnp != NULL) {
		MPOOL_SYSTEM_LOCK(env);
		if (LOG_COMPARE(lsnp, &mp->lsn) <= 0) {
			*lsnp = mp->lsn;

			MPOOL_SYSTEM_UNLOCK(env);
			return (0);
		}
		MPOOL_SYSTEM_UNLOCK(env);
	}

	if ((ret =
	    __memp_sync_int(env, NULL, 0, flags, NULL, &interrupted)) != 0)
		return (ret);

	if (!interrupted && lsnp != NULL) {
		MPOOL_SYSTEM_LOCK(env);
		if (LOG_COMPARE(lsnp, &mp->lsn) > 0)
			mp->lsn = *lsnp;
		MPOOL_SYSTEM_UNLOCK(env);
	}

	return (0);
}

/*
 * __memp_fsync_pp --
 *	DB_MPOOLFILE->sync pre/post processing.
 *
 * PUBLIC: int __memp_fsync_pp __P((DB_MPOOLFILE *));
 */
int
__memp_fsync_pp(dbmfp)
	DB_MPOOLFILE *dbmfp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbmfp->env;

	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->sync");

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_fsync(dbmfp)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_fsync --
 *	DB_MPOOLFILE->sync.
 *
 * PUBLIC: int __memp_fsync __P((DB_MPOOLFILE *));
 */
int
__memp_fsync(dbmfp)
	DB_MPOOLFILE *dbmfp;
{
	MPOOLFILE *mfp;

	mfp = dbmfp->mfp;

	/*
	 * If this handle doesn't have a file descriptor that's open for
	 * writing, or if the file is a temporary, or if the file hasn't
	 * been written since it was flushed, there's no reason to proceed
	 * further.
	 */
	if (F_ISSET(dbmfp, MP_READONLY))
		return (0);

	if (F_ISSET(dbmfp->mfp, MP_TEMP) || dbmfp->mfp->no_backing_file)
		return (0);

	if (mfp->file_written == 0)
		return (0);

	return (__memp_sync_int(
	    dbmfp->env, dbmfp, 0, DB_SYNC_FILE, NULL, NULL));
}

/*
 * __mp_xxx_fh --
 *	Return a file descriptor for DB 1.85 compatibility locking.
 *
 * PUBLIC: int __mp_xxx_fh __P((DB_MPOOLFILE *, DB_FH **));
 */
int
__mp_xxx_fh(dbmfp, fhp)
	DB_MPOOLFILE *dbmfp;
	DB_FH **fhp;
{
	int ret;

	/*
	 * This is a truly spectacular layering violation, intended ONLY to
	 * support compatibility for the DB 1.85 DB->fd call.
	 *
	 * Sync the database file to disk, creating the file as necessary.
	 *
	 * We skip the MP_READONLY and MP_TEMP tests done by memp_fsync(3).
	 * The MP_READONLY test isn't interesting because we will either
	 * already have a file descriptor (we opened the database file for
	 * reading) or we aren't readonly (we created the database which
	 * requires write privileges).  The MP_TEMP test isn't interesting
	 * because we want to write to the backing file regardless so that
	 * we get a file descriptor to return.
	 */
	if ((*fhp = dbmfp->fhp) != NULL)
		return (0);

	if ((ret = __memp_sync_int(
	    dbmfp->env, dbmfp, 0, DB_SYNC_FILE, NULL, NULL)) == 0)
		*fhp = dbmfp->fhp;
	return (ret);
}

/*
 * __memp_sync_int --
 *	Mpool sync internal function.
 *
 * PUBLIC: int __memp_sync_int __P((ENV *,
 * PUBLIC:     DB_MPOOLFILE *, u_int32_t, u_int32_t, u_int32_t *, int *));
 */
int
__memp_sync_int(env, dbmfp, trickle_max, flags, wrote_totalp, interruptedp)
	ENV *env;
	DB_MPOOLFILE *dbmfp;
	u_int32_t trickle_max, flags, *wrote_totalp;
	int *interruptedp;
{
	BH *bhp;
	BH_TRACK *bharray;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOL *c_mp, *mp;
	MPOOLFILE *mfp;
	db_mutex_t mutex;
	roff_t last_mf_offset;
	u_int32_t ar_cnt, ar_max, i, n_cache, remaining, wrote_total;
	int dirty, filecnt, maxopenfd, required_write, ret, t_ret;
	int wrote_cnt;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;
	last_mf_offset = INVALID_ROFF;
	filecnt = wrote_total = 0;

	if (wrote_totalp != NULL)
		*wrote_totalp = 0;
	if (interruptedp != NULL)
		*interruptedp = 0;

	/*
	 * If we're flushing the cache, it's a checkpoint or we're flushing a
	 * specific file, we really have to write the blocks and we have to
	 * confirm they made it to disk.  Otherwise, we can skip a block if
	 * it's hard to get.
	 */
	required_write = LF_ISSET(DB_SYNC_CACHE |
	    DB_SYNC_CHECKPOINT | DB_SYNC_FILE | DB_SYNC_QUEUE_EXTENT);

	/* Get shared configuration information. */
	MPOOL_SYSTEM_LOCK(env);
	maxopenfd = mp->mp_maxopenfd;
	MPOOL_SYSTEM_UNLOCK(env);

	/* Assume one dirty page per bucket. */
	ar_max = mp->nreg * mp->htab_buckets;
	if ((ret =
	    __os_malloc(env, ar_max * sizeof(BH_TRACK), &bharray)) != 0)
		return (ret);

	/*
	 * Walk each cache's list of buffers and mark all dirty buffers to be
	 * written and all dirty buffers to be potentially written, depending
	 * on our flags.
	 */
	for (ar_cnt = 0, n_cache = 0; n_cache < mp->nreg; ++n_cache) {
		c_mp = dbmp->reginfo[n_cache].primary;

		hp = R_ADDR(&dbmp->reginfo[n_cache], c_mp->htab);
		for (i = 0; i < c_mp->htab_buckets; i++, hp++) {
			/*
			 * We can check for empty buckets before locking as
			 * we only care if the pointer is zero or non-zero.
			 * We can ignore empty or clean buckets because we
			 * only need write buffers that were dirty before
			 * we started.
			 */
#ifdef DIAGNOSTIC
			if (SH_TAILQ_FIRST(&hp->hash_bucket, __bh) == NULL)
#else
			if (atomic_read(&hp->hash_page_dirty) == 0)
#endif
				continue;

			dirty = 0;
			MUTEX_LOCK(env, hp->mtx_hash);
			SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
				/* Always ignore clean pages. */
				if (!F_ISSET(bhp, BH_DIRTY))
					continue;

				dirty++;
				mfp = R_ADDR(dbmp->reginfo, bhp->mf_offset);

				/*
				 * Ignore in-memory files, unless the file is
				 * specifically being flushed.
				 */
				if (mfp->no_backing_file)
					continue;
				if (!LF_ISSET(DB_SYNC_FILE) &&
				    F_ISSET(mfp, MP_TEMP))
					continue;

				/*
				 * Ignore files that aren't involved in DB's
				 * transactional operations during checkpoints.
				 */
				if (LF_ISSET(DB_SYNC_CHECKPOINT) &&
				    mfp->lsn_off == DB_LSN_OFF_NOTSET)
					continue;

				/*
				 * Ignore files that aren't Queue extent files
				 * if we're flushing a Queue file with extents.
				 */
				if (LF_ISSET(DB_SYNC_QUEUE_EXTENT) &&
				    !F_ISSET(mfp, MP_EXTENT))
					continue;

				/*
				 * If we're flushing a specific file, see if
				 * this page is from that file.
				 */
				if (dbmfp != NULL && mfp != dbmfp->mfp)
					continue;

				/* Track the buffer, we want it. */
				bharray[ar_cnt].track_hp = hp;
				bharray[ar_cnt].track_pgno = bhp->pgno;
				bharray[ar_cnt].track_off = bhp->mf_offset;
				ar_cnt++;

				/*
				 * If we run out of space, double and continue.
				 * Don't stop at trickle_max, we want to sort
				 * as large a sample set as possible in order
				 * to minimize disk seeks.
				 */
				if (ar_cnt >= ar_max) {
					if ((ret = __os_realloc(env,
					    (ar_max * 2) * sizeof(BH_TRACK),
					    &bharray)) != 0)
						break;
					ar_max *= 2;
				}
			}

			if (ret != 0)
				goto err;
			/*
			 * We are only checking this in diagnostic mode
			 * since it requires extra latching to keep the count
			 * in sync with the number of bits counted.
			 */
			DB_ASSERT(env,
			    dirty == (int)atomic_read(&hp->hash_page_dirty));
			MUTEX_UNLOCK(env, hp->mtx_hash);

			/* Check if the call has been interrupted. */
			if (LF_ISSET(DB_SYNC_INTERRUPT_OK) && FLD_ISSET(
			    mp->config_flags, DB_MEMP_SYNC_INTERRUPT)) {
				STAT(++mp->stat.st_sync_interrupted);
				if (interruptedp != NULL)
					*interruptedp = 1;
				goto err;
			}
		}
	}

	/* If there no buffers to write, we're done. */
	if (ar_cnt == 0)
		goto done;

	/*
	 * Write the buffers in file/page order, trying to reduce seeks by the
	 * filesystem and, when pages are smaller than filesystem block sizes,
	 * reduce the actual number of writes.
	 */
	if (ar_cnt > 1)
		qsort(bharray, ar_cnt, sizeof(BH_TRACK), __bhcmp);

	/*
	 * If we're trickling buffers, only write enough to reach the correct
	 * percentage.
	 */
	if (LF_ISSET(DB_SYNC_TRICKLE) && ar_cnt > trickle_max)
		ar_cnt = trickle_max;

	/*
	 * Flush the log.  We have to ensure the log records reflecting the
	 * changes on the database pages we're writing have already made it
	 * to disk.  We still have to check the log each time we write a page
	 * (because pages we are about to write may be modified after we have
	 * flushed the log), but in general this will at least avoid any I/O
	 * on the log's part.
	 */
	if (LOGGING_ON(env) && (ret = __log_flush(env, NULL)) != 0)
		goto err;

	/*
	 * Walk the array, writing buffers.  When we write a buffer, we NULL
	 * out its hash bucket pointer so we don't process a slot more than
	 * once.
	 */
	for (i = wrote_cnt = 0, remaining = ar_cnt; remaining > 0; ++i) {
		if (i >= ar_cnt) {
			i = 0;
			__os_yield(env, 1, 0);
		}
		if ((hp = bharray[i].track_hp) == NULL)
			continue;

		/* Lock the hash bucket and find the buffer. */
		mutex = hp->mtx_hash;
		MUTEX_READLOCK(env, mutex);
		SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh)
			if (bhp->pgno == bharray[i].track_pgno &&
			    bhp->mf_offset == bharray[i].track_off)
				break;

		/*
		 * If we can't find the buffer we're done, somebody else had
		 * to have written it.
		 *
		 * If the buffer isn't dirty, we're done, there's no work
		 * needed.
		 */
		if (bhp == NULL || !F_ISSET(bhp, BH_DIRTY)) {
			MUTEX_UNLOCK(env, mutex);
			--remaining;
			bharray[i].track_hp = NULL;
			continue;
		}

		/*
		 * If the buffer is locked by another thread, ignore it, we'll
		 * come back to it.
		 */
		if (F_ISSET(bhp, BH_EXCLUSIVE)) {
			MUTEX_UNLOCK(env, mutex);
			if (!required_write) {
				--remaining;
				bharray[i].track_hp = NULL;
			}
			continue;
		}

		/* Pin the buffer into memory. */
		atomic_inc(env, &bhp->ref);
		MUTEX_UNLOCK(env, mutex);
		MUTEX_READLOCK(env, bhp->mtx_buf);
		DB_ASSERT(env, !F_ISSET(bhp, BH_EXCLUSIVE));

		/*
		 * When swapping the hash bucket mutex for the buffer mutex,
		 * we may have raced with an MVCC update.  In that case, we
		 * no longer have the most recent version, and need to retry
		 * (the buffer header we have pinned will no longer be marked
		 * dirty, so we can't just write it).
		 */
		if (SH_CHAIN_HASNEXT(bhp, vc)) {
			atomic_dec(env, &bhp->ref);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			continue;
		}

		/* we will dispose of this buffer. */
		--remaining;
		bharray[i].track_hp = NULL;

		/*
		 * If we've switched files, check to see if we're configured
		 * to close file descriptors.
		 */
		if (maxopenfd != 0 && bhp->mf_offset != last_mf_offset) {
			if (++filecnt >= maxopenfd) {
				filecnt = 0;
				if ((t_ret = __memp_close_flush_files(
				    env, 1)) != 0 && ret == 0)
					ret = t_ret;
			}
			last_mf_offset = bhp->mf_offset;
		}

		/*
		 * If the buffer is dirty, we write it.  We only try to
		 * write the buffer once.
		 */
		if (F_ISSET(bhp, BH_DIRTY)) {
			mfp = R_ADDR(dbmp->reginfo, bhp->mf_offset);
			if ((t_ret =
			    __memp_bhwrite(dbmp, hp, mfp, bhp, 1)) == 0) {
				++wrote_cnt;
				++wrote_total;
			} else {
				if (ret == 0)
					ret = t_ret;
				__db_errx
				    (env, "%s: unable to flush page: %lu",
				    __memp_fns(dbmp, mfp), (u_long)bhp->pgno);

			}
		}

		/* Discard our buffer reference. */
		DB_ASSERT(env, atomic_read(&bhp->ref) > 0);
		atomic_dec(env, &bhp->ref);
		MUTEX_UNLOCK(env, bhp->mtx_buf);

		/* Check if the call has been interrupted. */
		if (LF_ISSET(DB_SYNC_INTERRUPT_OK) &&
		    FLD_ISSET(mp->config_flags, DB_MEMP_SYNC_INTERRUPT)) {
			STAT(++mp->stat.st_sync_interrupted);
			if (interruptedp != NULL)
				*interruptedp = 1;
			goto err;
		}

		/*
		 * Sleep after some number of writes to avoid disk saturation.
		 * Don't cache the max writes value, an application shutting
		 * down might reset the value in order to do a fast flush or
		 * checkpoint.
		 */
		if (!LF_ISSET(DB_SYNC_SUPPRESS_WRITE) &&
		    !FLD_ISSET(mp->config_flags, DB_MEMP_SUPPRESS_WRITE) &&
		    mp->mp_maxwrite != 0 && wrote_cnt >= mp->mp_maxwrite) {
			wrote_cnt = 0;
			__os_yield(env, 0, (u_long)mp->mp_maxwrite_sleep);
		}
	}

done:	/*
	 * If a write is required, we have to force the pages to disk.  We
	 * don't do this as we go along because we want to give the OS as
	 * much time as possible to lazily flush, and because we have to flush
	 * files that might not even have had dirty buffers in the cache, so
	 * we have to walk the files list.
	 */
	if (ret == 0 && required_write) {
		if (dbmfp == NULL)
			ret = __memp_sync_files(env);
		else
			ret = __os_fsync(env, dbmfp->fhp);
	}

	/* If we've opened files to flush pages, close them. */
	if ((t_ret = __memp_close_flush_files(env, 0)) != 0 && ret == 0)
		ret = t_ret;

err:	__os_free(env, bharray);
	if (wrote_totalp != NULL)
		*wrote_totalp = wrote_total;

	return (ret);
}

static int
__memp_sync_file(env, mfp, argp, countp, flags)
	ENV *env;
	MPOOLFILE *mfp;
	void *argp;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	int ret, t_ret;

	COMPQUIET(countp, NULL);
	COMPQUIET(flags, 0);

	if (!mfp->file_written || mfp->no_backing_file ||
	    mfp->deadfile || F_ISSET(mfp, MP_TEMP))
		return (0);
	/*
	 * Pin the MPOOLFILE structure into memory, and release the
	 * region mutex allowing us to walk the linked list.  We'll
	 * re-acquire that mutex to move to the next entry in the list.
	 *
	 * This works because we only need to flush current entries,
	 * we don't care about new entries being added, and the linked
	 * list is never re-ordered, a single pass is sufficient.  It
	 * requires MPOOLFILE structures removed before we get to them
	 * be flushed to disk, but that's nothing new, they could have
	 * been removed while checkpoint was running, too.
	 *
	 * Once we have the MPOOLFILE lock, re-check the MPOOLFILE is
	 * not being discarded.  (A thread removing the MPOOLFILE
	 * will: hold the MPOOLFILE mutex, set deadfile, drop the
	 * MPOOLFILE mutex and then acquire the region MUTEX to walk
	 * the linked list and remove the MPOOLFILE structure.  Make
	 * sure the MPOOLFILE wasn't marked dead while we waited for
	 * the mutex.
	 */
	MUTEX_LOCK(env, mfp->mutex);
	if (!mfp->file_written || mfp->deadfile) {
		MUTEX_UNLOCK(env, mfp->mutex);
		return (0);
	}
	++mfp->mpf_cnt;
	MUTEX_UNLOCK(env, mfp->mutex);

	/*
	 * Look for an already open, writeable handle (fsync doesn't
	 * work on read-only Windows handles).
	 */
	dbmp = env->mp_handle;
	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q) {
		if (dbmfp->mfp != mfp || F_ISSET(dbmfp, MP_READONLY))
			continue;
		/*
		 * We don't want to hold the mutex while calling sync.
		 * Increment the DB_MPOOLFILE handle ref count to pin
		 * it into memory.
		 */
		++dbmfp->ref;
		break;
	}
	MUTEX_UNLOCK(env, dbmp->mutex);

	/* If we don't find a handle we can use, open one. */
	if (dbmfp == NULL) {
		if ((ret = __memp_mf_sync(dbmp, mfp, 1)) != 0) {
			__db_err(env, ret,
			    "%s: unable to flush", (char *)
			    R_ADDR(dbmp->reginfo, mfp->path_off));
		}
	} else
		ret = __os_fsync(env, dbmfp->fhp);

	/*
	 * Re-acquire the MPOOLFILE mutex, we need it to modify the
	 * reference count.
	 */
	MUTEX_LOCK(env, mfp->mutex);

	/*
	 * If we wrote the file and there are no other references (or there
	 * is a single reference, and it's the one we opened to write
	 * buffers during checkpoint), clear the file_written flag.  We
	 * do this so that applications opening thousands of files don't
	 * loop here opening and flushing those files during checkpoint.
	 *
	 * The danger here is if a buffer were to be written as part of
	 * a checkpoint, and then not be flushed to disk.  This cannot
	 * happen because we only clear file_written when there are no
	 * other users of the MPOOLFILE in the system, and, as we hold
	 * the region lock, no possibility of another thread of control
	 * racing with us to open a MPOOLFILE.
	 */
	if (mfp->mpf_cnt == 1 || (mfp->mpf_cnt == 2 &&
	    dbmfp != NULL && F_ISSET(dbmfp, MP_FLUSH))) {
		mfp->file_written = 0;

		/*
		 * We may be the last reference for a MPOOLFILE, as we
		 * weren't holding the MPOOLFILE mutex when flushing
		 * it's buffers to disk.  If we can discard it, set
		 * a flag to schedule a clean-out pass.   (Not likely,
		 * I mean, what are the chances that there aren't any
		 * buffers in the pool?  Regardless, it might happen.)
		 */
		if (mfp->mpf_cnt == 1 && mfp->block_cnt == 0)
			*(int *)argp = 1;
	}

	/*
	 * If we found the file we must close it in case we are the last
	 * reference to the dbmfp.  NOTE: since we have incremented
	 * mfp->mpf_cnt this cannot be the last reference to the mfp.
	 * This is important since we are called with the hash bucket
	 * locked.  The mfp will get freed via the cleanup pass.
	 */
	if (dbmfp != NULL &&
	    (t_ret = __memp_fclose(dbmfp, DB_MPOOL_NOLOCK)) != 0 && ret == 0)
		ret = t_ret;

	--mfp->mpf_cnt;

	/* Unlock the MPOOLFILE. */
	MUTEX_UNLOCK(env, mfp->mutex);
	return (ret);
}

/*
 * __memp_sync_files --
 *	Sync all the files in the environment, open or not.
 */
static int
__memp_sync_files(env)
	ENV *env;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOL *mp;
	MPOOLFILE *mfp, *next_mfp;
	int i, need_discard_pass, ret;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;
	need_discard_pass = ret = 0;

	ret = __memp_walk_files(env,
	    mp, __memp_sync_file, &need_discard_pass, 0, DB_STAT_MEMP_NOERROR);

	/*
	 * We may need to do a last pass through the MPOOLFILE list -- if we
	 * were the last reference to an MPOOLFILE, we need to clean it out.
	 */
	if (!need_discard_pass)
		return (ret);

	hp = R_ADDR(dbmp->reginfo, mp->ftab);
	for (i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
retry:		MUTEX_LOCK(env, hp->mtx_hash);
		for (mfp = SH_TAILQ_FIRST(&hp->hash_bucket,
		    __mpoolfile); mfp != NULL; mfp = next_mfp) {
			next_mfp = SH_TAILQ_NEXT(mfp, q, __mpoolfile);
			/*
			 * Do a fast check -- we can check for zero/non-zero
			 * without a mutex on the MPOOLFILE.  If likely to
			 * succeed, lock the MPOOLFILE down and look for real.
			 */
			if (mfp->deadfile ||
			    mfp->block_cnt != 0 || mfp->mpf_cnt != 0)
				continue;

			MUTEX_LOCK(env, mfp->mutex);
			if (!mfp->deadfile &&
			    mfp->block_cnt == 0 && mfp->mpf_cnt == 0) {
				MUTEX_UNLOCK(env, hp->mtx_hash);
				(void)__memp_mf_discard(dbmp, mfp);
				goto retry;
			} else
				MUTEX_UNLOCK(env, mfp->mutex);
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	return (ret);
}

/*
 * __memp_mf_sync --
 *	Flush an MPOOLFILE, when no currently open handle is available.
 *
 * PUBLIC: int __memp_mf_sync __P((DB_MPOOL *, MPOOLFILE *, int));
 */
int
__memp_mf_sync(dbmp, mfp, locked)
	DB_MPOOL *dbmp;
	MPOOLFILE *mfp;
	int locked;
{
	DB_FH *fhp;
	DB_MPOOL_HASH *hp;
	ENV *env;
	MPOOL *mp;
	int ret, t_ret;
	char *rpath;

	COMPQUIET(hp, NULL);
	env = dbmp->env;

	/*
	 * We need to be holding the hash lock: we're using the path name
	 * and __memp_nameop might try and rename the file.
	 */
	if (!locked) {
		mp = dbmp->reginfo[0].primary;
		hp = R_ADDR(dbmp->reginfo, mp->ftab);
		hp += FNBUCKET(
		    R_ADDR(dbmp->reginfo, mfp->fileid_off), DB_FILE_ID_LEN);
		MUTEX_LOCK(env, hp->mtx_hash);
	}

	if ((ret = __db_appname(env, DB_APP_DATA,
	    R_ADDR(dbmp->reginfo, mfp->path_off), NULL, &rpath)) == 0) {
		if ((ret = __os_open(env, rpath, 0, 0, 0, &fhp)) == 0) {
			ret = __os_fsync(env, fhp);
			if ((t_ret =
			    __os_closehandle(env, fhp)) != 0 && ret == 0)
				ret = t_ret;
		}
		__os_free(env, rpath);
	}

	if (!locked)
		MUTEX_UNLOCK(env, hp->mtx_hash);

	return (ret);
}

/*
 * __memp_close_flush_files --
 *	Close files opened only to flush buffers.
 */
static int
__memp_close_flush_files(env, dosync)
	ENV *env;
	int dosync;
{
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	MPOOLFILE *mfp;
	int ret;

	dbmp = env->mp_handle;

	/*
	 * The routine exists because we must close files opened by sync to
	 * flush buffers.  There are two cases: first, extent files have to
	 * be closed so they may be removed when empty.  Second, regular
	 * files have to be closed so we don't run out of descriptors (for
	 * example, an application partitioning its data into databases
	 * based on timestamps, so there's a continually increasing set of
	 * files).
	 *
	 * We mark files opened in the __memp_bhwrite() function with the
	 * MP_FLUSH flag.  Here we walk through our file descriptor list,
	 * and, if a file was opened by __memp_bhwrite(), we close it.
	 */
retry:	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q)
		if (F_ISSET(dbmfp, MP_FLUSH)) {
			F_CLR(dbmfp, MP_FLUSH);
			MUTEX_UNLOCK(env, dbmp->mutex);
			if (dosync) {
				/*
				 * If we have the only open handle on the file,
				 * clear the dirty flag so we don't re-open and
				 * sync it again when discarding the MPOOLFILE
				 * structure.  Clear the flag before the sync
				 * so can't race with a thread writing the file.
				 */
				mfp = dbmfp->mfp;
				if (mfp->mpf_cnt == 1) {
					MUTEX_LOCK(env, mfp->mutex);
					if (mfp->mpf_cnt == 1)
						mfp->file_written = 0;
					MUTEX_UNLOCK(env, mfp->mutex);
				}
				if ((ret = __os_fsync(env, dbmfp->fhp)) != 0)
					return (ret);
			}
			if ((ret = __memp_fclose(dbmfp, 0)) != 0)
				return (ret);
			goto retry;
		}
	MUTEX_UNLOCK(env, dbmp->mutex);

	return (0);
}

static int
__bhcmp(p1, p2)
	const void *p1, *p2;
{
	BH_TRACK *bhp1, *bhp2;

	bhp1 = (BH_TRACK *)p1;
	bhp2 = (BH_TRACK *)p2;

	/* Sort by file (shared memory pool offset). */
	if (bhp1->track_off < bhp2->track_off)
		return (-1);
	if (bhp1->track_off > bhp2->track_off)
		return (1);

	/*
	 * !!!
	 * Defend against badly written quicksort code calling the comparison
	 * function with two identical pointers (e.g., WATCOM C++ (Power++)).
	 */
	if (bhp1->track_pgno < bhp2->track_pgno)
		return (-1);
	if (bhp1->track_pgno > bhp2->track_pgno)
		return (1);
	return (0);
}
