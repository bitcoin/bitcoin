/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"		/* Required for diagnostic code. */
#include "dbinc/mp.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

static int __memp_pgwrite
	       __P((ENV *, DB_MPOOLFILE *, DB_MPOOL_HASH *, BH *));

/*
 * __memp_bhwrite --
 *	Write the page associated with a given buffer header.
 *
 * PUBLIC: int __memp_bhwrite __P((DB_MPOOL *,
 * PUBLIC:      DB_MPOOL_HASH *, MPOOLFILE *, BH *, int));
 */
int
__memp_bhwrite(dbmp, hp, mfp, bhp, open_extents)
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOLFILE *mfp;
	BH *bhp;
	int open_extents;
{
	DB_MPOOLFILE *dbmfp;
	DB_MPREG *mpreg;
	ENV *env;
	int ret;

	env = dbmp->env;

	/*
	 * If the file has been removed or is a closed temporary file, we're
	 * done -- the page-write function knows how to handle the fact that
	 * we don't have (or need!) any real file descriptor information.
	 */
	if (mfp->deadfile)
		return (__memp_pgwrite(env, NULL, hp, bhp));

	/*
	 * Walk the process' DB_MPOOLFILE list and find a file descriptor for
	 * the file.  We also check that the descriptor is open for writing.
	 */
	MUTEX_LOCK(env, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q)
		if (dbmfp->mfp == mfp && !F_ISSET(dbmfp, MP_READONLY)) {
			++dbmfp->ref;
			break;
		}
	MUTEX_UNLOCK(env, dbmp->mutex);

	if (dbmfp != NULL) {
		/*
		 * Temporary files may not have been created.  We only handle
		 * temporary files in this path, because only the process that
		 * created a temporary file will ever flush buffers to it.
		 */
		if (dbmfp->fhp == NULL) {
			/* We may not be allowed to create backing files. */
			if (mfp->no_backing_file) {
				--dbmfp->ref;
				return (EPERM);
			}

			MUTEX_LOCK(env, dbmp->mutex);
			if (dbmfp->fhp == NULL) {
				ret = __db_tmp_open(env,
				    F_ISSET(env->dbenv, DB_ENV_DIRECT_DB) ?
				    DB_OSO_DIRECT : 0, &dbmfp->fhp);
			} else
				ret = 0;
			MUTEX_UNLOCK(env, dbmp->mutex);
			if (ret != 0) {
				__db_errx(env,
				    "unable to create temporary backing file");
				--dbmfp->ref;
				return (ret);
			}
		}

		goto pgwrite;
	}

	/*
	 * There's no file handle for this file in our process.
	 *
	 * !!!
	 * It's the caller's choice if we're going to open extent files.
	 */
	if (!open_extents && F_ISSET(mfp, MP_EXTENT))
		return (EPERM);

	/*
	 * !!!
	 * Don't try to attach to temporary files.  There are two problems in
	 * trying to do that.  First, if we have different privileges than the
	 * process that "owns" the temporary file, we might create the backing
	 * disk file such that the owning process couldn't read/write its own
	 * buffers, e.g., memp_trickle running as root creating a file owned
	 * as root, mode 600.  Second, if the temporary file has already been
	 * created, we don't have any way of finding out what its real name is,
	 * and, even if we did, it was already unlinked (so that it won't be
	 * left if the process dies horribly).  This decision causes a problem,
	 * however: if the temporary file consumes the entire buffer cache,
	 * and the owner doesn't flush the buffers to disk, we could end up
	 * with resource starvation, and the memp_trickle thread couldn't do
	 * anything about it.  That's a pretty unlikely scenario, though.
	 *
	 * Note we should never get here when the temporary file in question
	 * has already been closed in another process, in which case it should
	 * be marked dead.
	 */
	if (F_ISSET(mfp, MP_TEMP) || mfp->no_backing_file)
		return (EPERM);

	/*
	 * It's not a page from a file we've opened.  If the file requires
	 * application-specific input/output processing, see if this process
	 * has ever registered information as to how to write this type of
	 * file.  If not, there's nothing we can do.
	 */
	if (mfp->ftype != 0 && mfp->ftype != DB_FTYPE_SET) {
		MUTEX_LOCK(env, dbmp->mutex);
		LIST_FOREACH(mpreg, &dbmp->dbregq, q)
			if (mpreg->ftype == mfp->ftype)
				break;
		MUTEX_UNLOCK(env, dbmp->mutex);
		if (mpreg == NULL)
			return (EPERM);
	}

	/*
	 * Try and open the file, specifying the known underlying shared area.
	 *
	 * !!!
	 * There's no negative cache, so we may repeatedly try and open files
	 * that we have previously tried (and failed) to open.
	 */
	if ((ret = __memp_fcreate(env, &dbmfp)) != 0)
		return (ret);
	if ((ret = __memp_fopen(dbmfp, mfp,
	    NULL, NULL, DB_DURABLE_UNKNOWN, 0, mfp->stat.st_pagesize)) != 0) {
		(void)__memp_fclose(dbmfp, 0);

		/*
		 * Ignore any error if the file is marked dead, assume the file
		 * was removed from under us.
		 */
		if (!mfp->deadfile)
			return (ret);

		dbmfp = NULL;
	}

pgwrite:
	MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize,
	    PROT_READ | PROT_WRITE | PROT_EXEC);
	ret = __memp_pgwrite(env, dbmfp, hp, bhp);
	if (dbmfp == NULL)
		return (ret);

	/*
	 * Discard our reference, and, if we're the last reference, make sure
	 * the file eventually gets closed.
	 */
	MUTEX_LOCK(env, dbmp->mutex);
	if (dbmfp->ref == 1)
		F_SET(dbmfp, MP_FLUSH);
	else
		--dbmfp->ref;
	MUTEX_UNLOCK(env, dbmp->mutex);

	return (ret);
}

/*
 * __memp_pgread --
 *	Read a page from a file.
 *
 * PUBLIC: int __memp_pgread __P((DB_MPOOLFILE *, BH *, int));
 */
int
__memp_pgread(dbmfp, bhp, can_create)
	DB_MPOOLFILE *dbmfp;
	BH *bhp;
	int can_create;
{
	ENV *env;
	MPOOLFILE *mfp;
	size_t len, nr;
	u_int32_t pagesize;
	int ret;

	env = dbmfp->env;
	mfp = dbmfp->mfp;
	pagesize = mfp->stat.st_pagesize;

	/* We should never be called with a dirty or unlocked buffer. */
	DB_ASSERT(env, !F_ISSET(bhp, BH_DIRTY_CREATE | BH_FROZEN));
	DB_ASSERT(env, can_create || !F_ISSET(bhp, BH_DIRTY));
	DB_ASSERT(env, F_ISSET(bhp, BH_EXCLUSIVE));

	/* Mark the buffer as in transistion. */
	F_SET(bhp, BH_TRASH);

	/*
	 * Temporary files may not yet have been created.  We don't create
	 * them now, we create them when the pages have to be flushed.
	 */
	nr = 0;
	if (dbmfp->fhp != NULL)
		if ((ret = __os_io(env, DB_IO_READ, dbmfp->fhp,
		    bhp->pgno, pagesize, 0, pagesize, bhp->buf, &nr)) != 0)
			goto err;

	/*
	 * The page may not exist; if it doesn't, nr may well be 0, but we
	 * expect the underlying OS calls not to return an error code in
	 * this case.
	 */
	if (nr < pagesize) {
		/*
		 * Don't output error messages for short reads.  In particular,
		 * DB recovery processing may request pages never written to
		 * disk or for which only some part have been written to disk,
		 * in which case we won't find the page.  The caller must know
		 * how to handle the error.
		 */
		if (!can_create) {
			ret = DB_PAGE_NOTFOUND;
			goto err;
		}

		/* Clear any bytes that need to be cleared. */
		len = mfp->clear_len == DB_CLEARLEN_NOTSET ?
		    pagesize : mfp->clear_len;
		memset(bhp->buf, 0, len);

#if defined(DIAGNOSTIC) || defined(UMRW)
		/*
		 * If we're running in diagnostic mode, corrupt any bytes on
		 * the page that are unknown quantities for the caller.
		 */
		if (len < pagesize)
			memset(bhp->buf + len, CLEAR_BYTE, pagesize - len);
#endif
#ifdef HAVE_STATISTICS
		++mfp->stat.st_page_create;
	} else
		++mfp->stat.st_page_in;
#else
	}
#endif

	/* Call any pgin function. */
	ret = mfp->ftype == 0 ? 0 : __memp_pg(dbmfp, bhp->pgno, bhp->buf, 1);

	/*
	 * If no errors occurred, the data is now valid, clear the BH_TRASH
	 * flag.
	 */
	if (ret == 0)
		F_CLR(bhp, BH_TRASH);
err:	return (ret);
}

/*
 * __memp_pgwrite --
 *	Write a page to a file.
 */
static int
__memp_pgwrite(env, dbmfp, hp, bhp)
	ENV *env;
	DB_MPOOLFILE *dbmfp;
	DB_MPOOL_HASH *hp;
	BH *bhp;
{
	DB_LSN lsn;
	MPOOLFILE *mfp;
	size_t nw;
	int ret;
	void * buf;

	/*
	 * Since writing does not require exclusive access, another thread
	 * could have already written this buffer.
	 */
	if (!F_ISSET(bhp, BH_DIRTY))
		return (0);

	mfp = dbmfp == NULL ? NULL : dbmfp->mfp;
	ret = 0;
	buf = NULL;

	/* We should never be called with a frozen or trashed buffer. */
	DB_ASSERT(env, !F_ISSET(bhp, BH_FROZEN | BH_TRASH));

	/*
	 * It's possible that the underlying file doesn't exist, either
	 * because of an outright removal or because it was a temporary
	 * file that's been closed.
	 *
	 * !!!
	 * Once we pass this point, we know that dbmfp and mfp aren't NULL,
	 * and that we have a valid file reference.
	 */
	if (mfp == NULL || mfp->deadfile)
		goto file_dead;

	/*
	 * If the page is in a file for which we have LSN information, we have
	 * to ensure the appropriate log records are on disk.
	 */
	if (LOGGING_ON(env) && mfp->lsn_off != DB_LSN_OFF_NOTSET &&
	    !IS_CLIENT_PGRECOVER(env)) {
		memcpy(&lsn, bhp->buf + mfp->lsn_off, sizeof(DB_LSN));
		if (!IS_NOT_LOGGED_LSN(lsn) &&
		    (ret = __log_flush(env, &lsn)) != 0)
			goto err;
	}

#ifdef DIAGNOSTIC
	/*
	 * Verify write-ahead logging semantics.
	 *
	 * !!!
	 * Two special cases.  There is a single field on the meta-data page,
	 * the last-page-number-in-the-file field, for which we do not log
	 * changes.  If the page was originally created in a database that
	 * didn't have logging turned on, we can see a page marked dirty but
	 * for which no corresponding log record has been written.  However,
	 * the only way that a page can be created for which there isn't a
	 * previous log record and valid LSN is when the page was created
	 * without logging turned on, and so we check for that special-case
	 * LSN value.
	 *
	 * Second, when a client is reading database pages from a master
	 * during an internal backup, we may get pages modified after
	 * the current end-of-log.
	 */
	if (LOGGING_ON(env) && !IS_NOT_LOGGED_LSN(LSN(bhp->buf)) &&
	    !IS_CLIENT_PGRECOVER(env)) {
		/*
		 * There is a potential race here.  If we are in the midst of
		 * switching log files, it's possible we could test against the
		 * old file and the new offset in the log region's LSN.  If we
		 * fail the first test, acquire the log mutex and check again.
		 */
		DB_LOG *dblp;
		LOG *lp;

		dblp = env->lg_handle;
		lp = dblp->reginfo.primary;
		if (!lp->db_log_inmemory &&
		    LOG_COMPARE(&lp->s_lsn, &LSN(bhp->buf)) <= 0) {
			MUTEX_LOCK(env, lp->mtx_flush);
			DB_ASSERT(env,
			    LOG_COMPARE(&lp->s_lsn, &LSN(bhp->buf)) > 0);
			MUTEX_UNLOCK(env, lp->mtx_flush);
		}
	}
#endif

	/*
	 * Call any pgout function.  If we have the page exclusive then
	 * we are going to reuse it otherwise make a copy of the page so
	 * that others can continue looking at the page while we write it.
	 */
	buf = bhp->buf;
	if (mfp->ftype != 0) {
		if (F_ISSET(bhp, BH_EXCLUSIVE))
			F_SET(bhp, BH_TRASH);
		else {
			if ((ret =
			    __os_malloc(env, mfp->stat.st_pagesize, &buf)) != 0)
				goto err;
			memcpy(buf, bhp->buf, mfp->stat.st_pagesize);
		}
		if ((ret = __memp_pg(dbmfp, bhp->pgno, buf, 0)) != 0)
			goto err;
	}

	/* Write the page. */
	if ((ret = __os_io(
	    env, DB_IO_WRITE, dbmfp->fhp, bhp->pgno, mfp->stat.st_pagesize,
	    0, mfp->stat.st_pagesize, buf, &nw)) != 0) {
		__db_errx(env, "%s: write failed for page %lu",
		    __memp_fn(dbmfp), (u_long)bhp->pgno);
		goto err;
	}
	STAT(++mfp->stat.st_page_out);
	if (bhp->pgno > mfp->last_flushed_pgno) {
		MUTEX_LOCK(env, mfp->mutex);
		if (bhp->pgno > mfp->last_flushed_pgno)
			mfp->last_flushed_pgno = bhp->pgno;
		MUTEX_UNLOCK(env, mfp->mutex);
	}

err:
file_dead:
	if (buf != NULL && buf != bhp->buf)
		__os_free(env, buf);
	/*
	 * !!!
	 * Once we pass this point, dbmfp and mfp may be NULL, we may not have
	 * a valid file reference.
	 */

	/*
	 * Update the hash bucket statistics, reset the flags.  If we were
	 * successful, the page is no longer dirty.  Someone else may have
	 * also written the page so we need to latch the hash bucket here
	 * to get the accounting correct.  Since we have the buffer
	 * shared it cannot be marked dirty again till we release it.
	 * This is the only place we update the flags field only holding
	 * a shared latch.
	 */
	if (F_ISSET(bhp, BH_DIRTY | BH_TRASH)) {
		MUTEX_LOCK(env, hp->mtx_hash);
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		if (ret == 0 && F_ISSET(bhp, BH_DIRTY)) {
			F_CLR(bhp, BH_DIRTY | BH_DIRTY_CREATE);
			DB_ASSERT(env, atomic_read(&hp->hash_page_dirty) > 0);
			atomic_dec(env, &hp->hash_page_dirty);
		}

		/* put the page back if necessary. */
		if ((ret != 0 || BH_REFCOUNT(bhp) > 1) &&
		    F_ISSET(bhp, BH_TRASH)) {
			ret = __memp_pg(dbmfp, bhp->pgno, bhp->buf, 1);
			F_CLR(bhp, BH_TRASH);
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}

	return (ret);
}

/*
 * __memp_pg --
 *	Call the pgin/pgout routine.
 *
 * PUBLIC: int __memp_pg __P((DB_MPOOLFILE *, db_pgno_t, void *, int));
 */
int
__memp_pg(dbmfp, pgno, buf, is_pgin)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t pgno;
	void *buf;
	int is_pgin;
{
	DBT dbt, *dbtp;
	DB_MPOOL *dbmp;
	DB_MPREG *mpreg;
	ENV *env;
	MPOOLFILE *mfp;
	int ftype, ret;

	env = dbmfp->env;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;

	if ((ftype = mfp->ftype) == DB_FTYPE_SET)
		mpreg = dbmp->pg_inout;
	else {
		MUTEX_LOCK(env, dbmp->mutex);
		LIST_FOREACH(mpreg, &dbmp->dbregq, q)
			if (ftype == mpreg->ftype)
				break;
		MUTEX_UNLOCK(env, dbmp->mutex);
	}
	if (mpreg == NULL)
		return (0);

	if (mfp->pgcookie_len == 0)
		dbtp = NULL;
	else {
		DB_SET_DBT(dbt, R_ADDR(
		    dbmp->reginfo, mfp->pgcookie_off), mfp->pgcookie_len);
		dbtp = &dbt;
	}

	if (is_pgin) {
		if (mpreg->pgin != NULL && (ret =
		    mpreg->pgin(env->dbenv, pgno, buf, dbtp)) != 0)
			goto err;
	} else
		if (mpreg->pgout != NULL && (ret =
		    mpreg->pgout(env->dbenv, pgno, buf, dbtp)) != 0)
			goto err;

	return (0);

err:	__db_errx(env, "%s: %s failed for page %lu",
	    __memp_fn(dbmfp), is_pgin ? "pgin" : "pgout", (u_long)pgno);
	return (ret);
}

/*
 * __memp_bhfree --
 *	Free a bucket header and its referenced data.
 *
 * PUBLIC: int __memp_bhfree __P((DB_MPOOL *,
 * PUBLIC:	REGINFO *, MPOOLFILE *, DB_MPOOL_HASH *, BH *, u_int32_t));
 */
int
__memp_bhfree(dbmp, infop, mfp, hp, bhp, flags)
	DB_MPOOL *dbmp;
	REGINFO *infop;
	MPOOLFILE *mfp;
	DB_MPOOL_HASH *hp;
	BH *bhp;
	u_int32_t flags;
{
	ENV *env;
#ifdef DIAGNOSTIC
	DB_LSN vlsn;
#endif
	BH *prev_bhp;
	MPOOL *c_mp;
	int ret, t_ret;
#ifdef DIAG_MVCC
	size_t pagesize;
#endif

	ret = 0;

	/*
	 * Assumes the hash bucket is locked and the MPOOL is not.
	 */
	env = dbmp->env;
#ifdef DIAG_MVCC
	if (mfp != NULL)
		pagesize = mfp->stat.st_pagesize;
#endif

	DB_ASSERT(env, LF_ISSET(BH_FREE_UNLOCKED) ||
	    (hp != NULL && MUTEX_IS_OWNED(env, hp->mtx_hash)));
	DB_ASSERT(env, BH_REFCOUNT(bhp) == 1 &&
	    !F_ISSET(bhp, BH_DIRTY | BH_FROZEN));
	DB_ASSERT(env, LF_ISSET(BH_FREE_UNLOCKED) ||
	    SH_CHAIN_SINGLETON(bhp, vc) || (SH_CHAIN_HASNEXT(bhp, vc) &&
	    (SH_CHAIN_NEXTP(bhp, vc, __bh)->td_off == bhp->td_off ||
	    bhp->td_off == INVALID_ROFF ||
	    IS_MAX_LSN(*VISIBLE_LSN(env, bhp)) ||
	    BH_OBSOLETE(bhp, hp->old_reader, vlsn))));

	/*
	 * Delete the buffer header from the hash bucket queue or the
	 * version chain.
	 */
	if (hp == NULL)
		goto no_hp;
	prev_bhp = SH_CHAIN_PREV(bhp, vc, __bh);
	if (!SH_CHAIN_HASNEXT(bhp, vc)) {
		if (prev_bhp != NULL)
			SH_TAILQ_INSERT_AFTER(&hp->hash_bucket,
			    bhp, prev_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
	}
	SH_CHAIN_REMOVE(bhp, vc, __bh);

	/*
	 * Remove the reference to this buffer from the transaction that
	 * created it, if any.  When the BH_FREE_UNLOCKED flag is set, we're
	 * discarding the environment, so the transaction region is already
	 * gone.
	 */
	if (bhp->td_off != INVALID_ROFF && !LF_ISSET(BH_FREE_UNLOCKED)) {
		ret = __txn_remove_buffer(
		    env, BH_OWNER(env, bhp), hp->mtx_hash);
		bhp->td_off = INVALID_ROFF;
	}

	/*
	 * We're going to use the memory for something else -- it had better be
	 * accessible.
	 */
no_hp:	MVCC_MPROTECT(bhp->buf, pagesize, PROT_READ | PROT_WRITE | PROT_EXEC);

	/*
	 * Discard the hash bucket's mutex, it's no longer needed, and
	 * we don't want to be holding it when acquiring other locks.
	 */
	if (!LF_ISSET(BH_FREE_UNLOCKED))
		MUTEX_UNLOCK(env, hp->mtx_hash);

	/*
	 * If we're only removing this header from the chain for reuse, we're
	 * done.
	 */
	if (LF_ISSET(BH_FREE_REUSE))
		return (ret);

	/*
	 * If we're not reusing the buffer immediately, free the buffer for
	 * real.
	 */
	if (!LF_ISSET(BH_FREE_UNLOCKED))
		MUTEX_UNLOCK(env, bhp->mtx_buf);
	if (LF_ISSET(BH_FREE_FREEMEM)) {
		if ((ret = __mutex_free(env, &bhp->mtx_buf)) != 0)
			return (ret);
		MPOOL_REGION_LOCK(env, infop);

		MVCC_BHUNALIGN(bhp);
		__memp_free(infop, bhp);
		c_mp = infop->primary;
		c_mp->stat.st_pages--;

		MPOOL_REGION_UNLOCK(env, infop);
	}

	if (mfp == NULL)
		return (ret);

	/*
	 * Decrement the reference count of the underlying MPOOLFILE.
	 * If this is its last reference, remove it.
	 */
	MUTEX_LOCK(env, mfp->mutex);
	if (--mfp->block_cnt == 0 && mfp->mpf_cnt == 0) {
		if ((t_ret = __memp_mf_discard(dbmp, mfp)) != 0 && ret == 0)
			ret = t_ret;
	} else
		MUTEX_UNLOCK(env, mfp->mutex);

	return (ret);
}
