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
#include "dbinc/txn.h"

/*
 * __memp_fget_pp --
 *	DB_MPOOLFILE->get pre/post processing.
 *
 * PUBLIC: int __memp_fget_pp
 * PUBLIC:     __P((DB_MPOOLFILE *, db_pgno_t *, DB_TXN *, u_int32_t, void *));
 */
int
__memp_fget_pp(dbmfp, pgnoaddr, txnp, flags, addrp)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t *pgnoaddr;
	DB_TXN *txnp;
	u_int32_t flags;
	void *addrp;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int rep_blocked, ret;

	env = dbmfp->env;

	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->get");

	/*
	 * Validate arguments.
	 *
	 * !!!
	 * Don't test for DB_MPOOL_CREATE and DB_MPOOL_NEW flags for readonly
	 * files here, and create non-existent pages in readonly files if the
	 * flags are set, later.  The reason is that the hash access method
	 * wants to get empty pages that don't really exist in readonly files.
	 * The only alternative is for hash to write the last "bucket" all the
	 * time, which we don't want to do because one of our big goals in life
	 * is to keep database files small.  It's sleazy as hell, but we catch
	 * any attempt to actually write the file in memp_fput().
	 */
#define	OKFLAGS		(DB_MPOOL_CREATE | DB_MPOOL_DIRTY | \
	    DB_MPOOL_EDIT | DB_MPOOL_LAST | DB_MPOOL_NEW)
	if (flags != 0) {
		if ((ret = __db_fchk(env, "memp_fget", flags, OKFLAGS)) != 0)
			return (ret);

		switch (flags) {
		case DB_MPOOL_DIRTY:
		case DB_MPOOL_CREATE:
		case DB_MPOOL_EDIT:
		case DB_MPOOL_LAST:
		case DB_MPOOL_NEW:
			break;
		default:
			return (__db_ferr(env, "memp_fget", 1));
		}
	}

	ENV_ENTER(env, ip);

	rep_blocked = 0;
	if (txnp == NULL && IS_ENV_REPLICATED(env)) {
		if ((ret = __op_rep_enter(env)) != 0)
			goto err;
		rep_blocked = 1;
	}
	ret = __memp_fget(dbmfp, pgnoaddr, ip, txnp, flags, addrp);
	/*
	 * We only decrement the count in op_rep_exit if the operation fails.
	 * Otherwise the count will be decremented when the page is no longer
	 * pinned in memp_fput.
	 */
	if (ret != 0 && rep_blocked)
		(void)__op_rep_exit(env);

	/* Similarly if an app has a page pinned it is ACTIVE. */
err:	if (ret != 0)
		ENV_LEAVE(env, ip);

	return (ret);
}

/*
 * __memp_fget --
 *	Get a page from the file.
 *
 * PUBLIC: int __memp_fget __P((DB_MPOOLFILE *,
 * PUBLIC:     db_pgno_t *, DB_THREAD_INFO *, DB_TXN *, u_int32_t, void *));
 */
int
__memp_fget(dbmfp, pgnoaddr, ip, txn, flags, addrp)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t *pgnoaddr;
	DB_THREAD_INFO *ip;
	DB_TXN *txn;
	u_int32_t flags;
	void *addrp;
{
	enum { FIRST_FOUND, FIRST_MISS, SECOND_FOUND, SECOND_MISS } state;
	BH *alloc_bhp, *bhp, *oldest_bhp;
	ENV *env;
	DB_LSN *read_lsnp, vlsn;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOL *c_mp;
	MPOOLFILE *mfp;
	PIN_LIST *list, *lp;
	REGENV *renv;
	REGINFO *infop, *t_infop, *reginfo;
	TXN_DETAIL *td;
	roff_t list_off, mf_offset;
	u_int32_t bucket, pinmax, st_hsearch;
	int b_incr, b_lock, h_locked, dirty, extending;
	int makecopy, mvcc, need_free, ret;

	*(void **)addrp = NULL;
	COMPQUIET(c_mp, NULL);
	COMPQUIET(infop, NULL);

	env = dbmfp->env;
	dbmp = env->mp_handle;

	mfp = dbmfp->mfp;
	mvcc = mfp->multiversion && (txn != NULL);
	mf_offset = R_OFFSET(dbmp->reginfo, mfp);
	alloc_bhp = bhp = oldest_bhp = NULL;
	read_lsnp = NULL;
	td = NULL;
	hp = NULL;
	b_incr = b_lock = h_locked = extending = makecopy = ret = 0;

	if (LF_ISSET(DB_MPOOL_DIRTY)) {
		if (F_ISSET(dbmfp, MP_READONLY)) {
			__db_errx(env,
			    "%s: dirty flag set for readonly file page",
			    __memp_fn(dbmfp));
			return (EINVAL);
		}
		if ((ret = __db_fcchk(env, "DB_MPOOLFILE->get",
		    flags, DB_MPOOL_DIRTY, DB_MPOOL_EDIT)) != 0)
			return (ret);
	}

	dirty = LF_ISSET(DB_MPOOL_DIRTY | DB_MPOOL_EDIT | DB_MPOOL_FREE);
	LF_CLR(DB_MPOOL_DIRTY | DB_MPOOL_EDIT);

	/*
	 * If the transaction is being used to update a multiversion database
	 * for the first time, set the read LSN.  In addition, if this is an
	 * update, allocate a mutex.  If no transaction has been supplied, that
	 * will be caught later, when we know whether one is required.
	 */
	if (mvcc && txn != NULL && txn->td != NULL) {
		/* We're only interested in the ultimate parent transaction. */
		while (txn->parent != NULL)
			txn = txn->parent;
		td = (TXN_DETAIL *)txn->td;
		if (F_ISSET(txn, TXN_SNAPSHOT)) {
			read_lsnp = &td->read_lsn;
			if (IS_MAX_LSN(*read_lsnp) &&
			    (ret = __log_current_lsn(env, read_lsnp,
			    NULL, NULL)) != 0)
				return (ret);
		}
		if ((dirty || LF_ISSET(DB_MPOOL_CREATE | DB_MPOOL_NEW)) &&
		    td->mvcc_mtx == MUTEX_INVALID && (ret =
		    __mutex_alloc(env, MTX_TXN_MVCC, 0, &td->mvcc_mtx)) != 0)
			return (ret);
	}

	switch (flags) {
	case DB_MPOOL_LAST:
		/* Get the last page number in the file. */
		MUTEX_LOCK(env, mfp->mutex);
		*pgnoaddr = mfp->last_pgno;
		MUTEX_UNLOCK(env, mfp->mutex);
		break;
	case DB_MPOOL_NEW:
		/*
		 * If always creating a page, skip the first search
		 * of the hash bucket.
		 */
		goto newpg;
	case DB_MPOOL_CREATE:
	default:
		break;
	}

	/*
	 * If mmap'ing the file and the page is not past the end of the file,
	 * just return a pointer.  We can't use R_ADDR here: this is an offset
	 * into an mmap'd file, not a shared region, and doesn't change for
	 * private environments.
	 *
	 * The page may be past the end of the file, so check the page number
	 * argument against the original length of the file.  If we previously
	 * returned pages past the original end of the file, last_pgno will
	 * have been updated to match the "new" end of the file, and checking
	 * against it would return pointers past the end of the mmap'd region.
	 *
	 * If another process has opened the file for writing since we mmap'd
	 * it, we will start playing the game by their rules, i.e. everything
	 * goes through the cache.  All pages previously returned will be safe,
	 * as long as the correct locking protocol was observed.
	 *
	 * We don't discard the map because we don't know when all of the
	 * pages will have been discarded from the process' address space.
	 * It would be possible to do so by reference counting the open
	 * pages from the mmap, but it's unclear to me that it's worth it.
	 */
	if (dbmfp->addr != NULL &&
	    F_ISSET(mfp, MP_CAN_MMAP) && *pgnoaddr <= mfp->orig_last_pgno) {
		*(void **)addrp = (u_int8_t *)dbmfp->addr +
		    (*pgnoaddr * mfp->stat.st_pagesize);
		STAT(++mfp->stat.st_map);
		return (0);
	}

	/*
	 * Determine the cache and hash bucket where this page lives and get
	 * local pointers to them.  Reset on each pass through this code, the
	 * page number can change.
	 */
	MP_GET_BUCKET(env, mfp, *pgnoaddr, &infop, hp, bucket, ret);
	if (ret != 0)
		return (ret);
	c_mp = infop->primary;

	if (0) {
		/* if we search again, get an exclusive lock. */
retry:		MUTEX_LOCK(env, hp->mtx_hash);
	}

	/* Search the hash chain for the page. */
	st_hsearch = 0;
	h_locked = 1;
	SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
		++st_hsearch;
		if (bhp->pgno != *pgnoaddr || bhp->mf_offset != mf_offset)
			continue;

		/* Snapshot reads -- get the version visible at read_lsn. */
		if (read_lsnp != NULL) {
			while (bhp != NULL &&
			    !BH_OWNED_BY(env, bhp, txn) &&
			    !BH_VISIBLE(env, bhp, read_lsnp, vlsn))
				bhp = SH_CHAIN_PREV(bhp, vc, __bh);
	
			/* 
			 * We can get a null bhp if we are looking for a 
			 * page that was created after the transaction was
			 * started so its not visible  (i.e. page added to 
			 * the BTREE in a subsequent txn).
			 */
			if (bhp == NULL) {
				ret = DB_PAGE_NOTFOUND;
				goto err;
			}
		}

		makecopy = mvcc && dirty && !BH_OWNED_BY(env, bhp, txn);

		/*
		 * Increment the reference count.  This signals that the
		 * buffer may not be discarded.  We must drop the hash
		 * mutex before we lock the buffer mutex.
		 */
		if (BH_REFCOUNT(bhp) == UINT16_MAX) {
			__db_errx(env,
			    "%s: page %lu: reference count overflow",
			    __memp_fn(dbmfp), (u_long)bhp->pgno);
			ret = __env_panic(env, EINVAL);
			goto err;
		}
		atomic_inc(env, &bhp->ref);
		b_incr = 1;

		/*
		 * Lock the buffer. If the page is being read in or modified it
		 * will be exclusively locked and we will block.
		 */
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		if (dirty || extending || makecopy || F_ISSET(bhp, BH_FROZEN)) {
xlatch:			if (LF_ISSET(DB_MPOOL_TRY)) {
				if ((ret =
				    MUTEX_TRYLOCK(env, bhp->mtx_buf)) != 0)
					goto err;
			} else
				MUTEX_LOCK(env, bhp->mtx_buf);
			F_SET(bhp, BH_EXCLUSIVE);
		} else if (LF_ISSET(DB_MPOOL_TRY)) {
			if ((ret = MUTEX_TRY_READLOCK(env, bhp->mtx_buf)) != 0)
				goto err;
		} else
			MUTEX_READLOCK(env, bhp->mtx_buf);

#ifdef HAVE_SHARED_LATCHES
		/*
		 * If buffer is still in transit once we have a shared latch,
		 * upgrade to an exclusive latch.
		 */
		if (F_ISSET(bhp, BH_FREED | BH_TRASH) &&
		    !F_ISSET(bhp, BH_EXCLUSIVE)) {
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			goto xlatch;
		}
#else
		F_SET(bhp, BH_EXCLUSIVE);
#endif
		b_lock = 1;

		/*
		 * If the buffer was frozen before we waited for any I/O to
		 * complete and is still frozen, we will need to thaw it.
		 * Otherwise, it was thawed while we waited, and we need to
		 * search again.
		 */
		if (F_ISSET(bhp, BH_THAWED)) {
thawed:			need_free = (atomic_dec(env, &bhp->ref) == 0);
			b_incr = 0;
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			if (need_free) {
				MPOOL_REGION_LOCK(env, infop);
				SH_TAILQ_INSERT_TAIL(&c_mp->free_frozen,
				    bhp, hq);
				MPOOL_REGION_UNLOCK(env, infop);
			}
			bhp = NULL;
			goto retry;
		}

		/*
		 * If the buffer we wanted was frozen or thawed while we
		 * waited, we need to start again.  That is indicated by
		 * a new buffer header in the version chain owned by the same
		 * transaction as the one we pinned.
		 *
		 * Also, if we're doing an unversioned read on a multiversion
		 * file, another thread may have dirtied this buffer while we
		 * swapped from the hash bucket lock to the buffer lock.
		 */
		if (SH_CHAIN_HASNEXT(bhp, vc) &&
		    (SH_CHAIN_NEXTP(bhp, vc, __bh)->td_off == bhp->td_off ||
		    (!dirty && read_lsnp == NULL))) {
			DB_ASSERT(env, b_incr && BH_REFCOUNT(bhp) != 0);
			atomic_dec(env, &bhp->ref);
			b_incr = 0;
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			bhp = NULL;
			goto retry;
		} else if (dirty && SH_CHAIN_HASNEXT(bhp, vc)) {
			ret = DB_LOCK_DEADLOCK;
			goto err;
		} else if (F_ISSET(bhp, BH_FREED) && flags != DB_MPOOL_CREATE &&
		    flags != DB_MPOOL_NEW && flags != DB_MPOOL_FREE) {
			ret = DB_PAGE_NOTFOUND;
			goto err;
		}

		STAT(++mfp->stat.st_cache_hit);
		break;
	}

#ifdef HAVE_STATISTICS
	/*
	 * Update the hash bucket search statistics -- do now because our next
	 * search may be for a different bucket.
	 */
	++c_mp->stat.st_hash_searches;
	if (st_hsearch > c_mp->stat.st_hash_longest)
		c_mp->stat.st_hash_longest = st_hsearch;
	c_mp->stat.st_hash_examined += st_hsearch;
#endif

	/*
	 * There are 4 possible paths to this location:
	 *
	 * FIRST_MISS:
	 *	Didn't find the page in the hash bucket on our first pass:
	 *	bhp == NULL, alloc_bhp == NULL
	 *
	 * FIRST_FOUND:
	 *	Found the page in the hash bucket on our first pass:
	 *	bhp != NULL, alloc_bhp == NULL
	 *
	 * SECOND_FOUND:
	 *	Didn't find the page in the hash bucket on the first pass,
	 *	allocated space, and found the page in the hash bucket on
	 *	our second pass:
	 *	bhp != NULL, alloc_bhp != NULL
	 *
	 * SECOND_MISS:
	 *	Didn't find the page in the hash bucket on the first pass,
	 *	allocated space, and didn't find the page in the hash bucket
	 *	on our second pass:
	 *	bhp == NULL, alloc_bhp != NULL
	 */
	state = bhp == NULL ?
	    (alloc_bhp == NULL ? FIRST_MISS : SECOND_MISS) :
	    (alloc_bhp == NULL ? FIRST_FOUND : SECOND_FOUND);

	switch (state) {
	case FIRST_FOUND:
		/*
		 * If we are to free the buffer, then this had better be the
		 * only reference. If so, just free the buffer.  If not,
		 * complain and get out.
		 */
		if (flags == DB_MPOOL_FREE) {
freebuf:		MUTEX_LOCK(env, hp->mtx_hash);
			h_locked = 1;
			if (F_ISSET(bhp, BH_DIRTY)) {
				F_CLR(bhp, BH_DIRTY | BH_DIRTY_CREATE);
				DB_ASSERT(env,
				   atomic_read(&hp->hash_page_dirty) > 0);
				atomic_dec(env, &hp->hash_page_dirty);
			}

			/*
			 * If the buffer we found is already freed, we're done.
			 * If the ref count is not 1 then someone may be
			 * peeking at the buffer.  We cannot free it until they
			 * determine that it is not what they want.  Clear the
			 * buffer so that waiting threads get an empty page.
			 */
			if (F_ISSET(bhp, BH_FREED))
				goto done;
			else if (F_ISSET(bhp, BH_FROZEN))
				makecopy = 1;

			if (makecopy)
				break;
			else if (BH_REFCOUNT(bhp) != 1 ||
			    !SH_CHAIN_SINGLETON(bhp, vc)) {
				/*
				 * Create an empty page in the chain for
				 * subsequent gets.  Otherwise, a thread that
				 * re-creates this page while it is still in
				 * cache will see stale data.
				 */
				F_SET(bhp, BH_FREED);
				F_CLR(bhp, BH_TRASH);
			} else {
				ret = __memp_bhfree(dbmp, infop, mfp,
				    hp, bhp, BH_FREE_FREEMEM);
				bhp = NULL;
				b_incr = b_lock = h_locked = 0;
			}
			goto done;
		} else if (F_ISSET(bhp, BH_FREED)) {
revive:			DB_ASSERT(env,
			    flags == DB_MPOOL_CREATE || flags == DB_MPOOL_NEW);
			makecopy = makecopy ||
			    (mvcc && !BH_OWNED_BY(env, bhp, txn)) ||
			    F_ISSET(bhp, BH_FROZEN);
			if (flags == DB_MPOOL_CREATE) {
				MUTEX_LOCK(env, mfp->mutex);
				if (*pgnoaddr > mfp->last_pgno)
					mfp->last_pgno = *pgnoaddr;
				MUTEX_UNLOCK(env, mfp->mutex);
			}
		}
		if (mvcc) {
			/*
			 * With multiversion databases, we might need to
			 * allocate a new buffer into which we can copy the one
			 * that we found.  In that case, check the last buffer
			 * in the chain to see whether we can reuse an obsolete
			 * buffer.
			 *
			 * To provide snapshot isolation, we need to make sure
			 * that we've seen a buffer older than the oldest
			 * snapshot read LSN.
			 */
reuse:			if ((makecopy || F_ISSET(bhp, BH_FROZEN)) &&
			    !h_locked) {
				MUTEX_LOCK(env, hp->mtx_hash);
				h_locked = 1;
			}
			if ((makecopy || F_ISSET(bhp, BH_FROZEN)) &&
			    SH_CHAIN_HASPREV(bhp, vc)) {
				oldest_bhp = SH_CHAIN_PREVP(bhp, vc, __bh);
				while (SH_CHAIN_HASPREV(oldest_bhp, vc))
					oldest_bhp = SH_CHAIN_PREVP(
					    oldest_bhp, vc, __bh);

				if (BH_REFCOUNT(oldest_bhp) == 0 &&
				    !BH_OBSOLETE(
				    oldest_bhp, hp->old_reader, vlsn) &&
				    (ret = __txn_oldest_reader(env,
				    &hp->old_reader)) != 0)
					goto err;

				if (BH_OBSOLETE(
				    oldest_bhp, hp->old_reader, vlsn) &&
				    BH_REFCOUNT(oldest_bhp) == 0) {
					DB_ASSERT(env,
					    !F_ISSET(oldest_bhp, BH_DIRTY));
					atomic_inc(env, &oldest_bhp->ref);
					if (F_ISSET(oldest_bhp, BH_FROZEN)) {
						/*
						 * This call will release the
						 * hash bucket mutex.
						 */
						ret = __memp_bh_thaw(dbmp,
						    infop, hp, oldest_bhp,
						    NULL);
						h_locked = 0;
						if (ret != 0)
							goto err;
						goto reuse;
					}
					if ((ret = __memp_bhfree(dbmp,
					    infop, mfp, hp, oldest_bhp,
					    BH_FREE_REUSE)) != 0)
						goto err;
					alloc_bhp = oldest_bhp;
					h_locked = 0;
				}

				DB_ASSERT(env, alloc_bhp == NULL ||
				    !F_ISSET(alloc_bhp, BH_FROZEN));
			}
		}

		/* We found the buffer or we're ready to copy -- we're done. */
		if (!(makecopy || F_ISSET(bhp, BH_FROZEN)) || alloc_bhp != NULL)
			break;

		/* FALLTHROUGH */
	case FIRST_MISS:
		/*
		 * We didn't find the buffer in our first check.  Figure out
		 * if the page exists, and allocate structures so we can add
		 * the page to the buffer pool.
		 */
		if (h_locked)
			MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;

		/*
		 * The buffer is not in the pool, so we don't need to free it.
		 */
		if (LF_ISSET(DB_MPOOL_FREE) &&
		    (bhp == NULL || F_ISSET(bhp, BH_FREED) || !makecopy))
			goto done;

		if (bhp != NULL)
			goto alloc;

newpg:		/*
		 * If DB_MPOOL_NEW is set, we have to allocate a page number.
		 * If neither DB_MPOOL_CREATE or DB_MPOOL_NEW is set, then
		 * it's an error to try and get a page past the end of file.
		 */
		DB_ASSERT(env, !h_locked);
		MUTEX_LOCK(env, mfp->mutex);
		switch (flags) {
		case DB_MPOOL_NEW:
			extending = 1;
			if (mfp->maxpgno != 0 &&
			    mfp->last_pgno >= mfp->maxpgno) {
				__db_errx(env, "%s: file limited to %lu pages",
				    __memp_fn(dbmfp), (u_long)mfp->maxpgno);
				ret = ENOSPC;
			} else
				*pgnoaddr = mfp->last_pgno + 1;
			break;
		case DB_MPOOL_CREATE:
			if (mfp->maxpgno != 0 && *pgnoaddr > mfp->maxpgno) {
				__db_errx(env, "%s: file limited to %lu pages",
				    __memp_fn(dbmfp), (u_long)mfp->maxpgno);
				ret = ENOSPC;
			} else if (!extending)
				extending = *pgnoaddr > mfp->last_pgno;
			break;
		default:
			ret = *pgnoaddr > mfp->last_pgno ? DB_PAGE_NOTFOUND : 0;
			break;
		}
		MUTEX_UNLOCK(env, mfp->mutex);
		if (ret != 0)
			goto err;

		/*
		 * !!!
		 * In the DB_MPOOL_NEW code path, hp, infop and c_mp have
		 * not yet been initialized.
		 */
		if (hp == NULL) {
			MP_GET_BUCKET(env,
			    mfp, *pgnoaddr, &infop, hp, bucket, ret);
			if (ret != 0)
				goto err;
			MUTEX_UNLOCK(env, hp->mtx_hash);
			c_mp = infop->primary;
		}

alloc:		/* Allocate a new buffer header and data space. */
		if (alloc_bhp == NULL && (ret =
		    __memp_alloc(dbmp, infop, mfp, 0, NULL, &alloc_bhp)) != 0)
			goto err;

		/* Initialize enough so we can call __memp_bhfree. */
		alloc_bhp->flags = 0;
		atomic_init(&alloc_bhp->ref, 1);
#ifdef DIAGNOSTIC
		if ((uintptr_t)alloc_bhp->buf & (sizeof(size_t) - 1)) {
			__db_errx(env,
		    "DB_MPOOLFILE->get: buffer data is NOT size_t aligned");
			ret = __env_panic(env, EINVAL);
			goto err;
		}
#endif

		/*
		 * If we're doing copy-on-write, we will already have the
		 * buffer header.  In that case, we don't need to search again.
		 */
		if (bhp != NULL)
			break;

		/*
		 * If we are extending the file, we'll need the mfp lock
		 * again.
		 */
		if (extending)
			MUTEX_LOCK(env, mfp->mutex);

		/*
		 * DB_MPOOL_NEW does not guarantee you a page unreferenced by
		 * any other thread of control.  (That guarantee is interesting
		 * for DB_MPOOL_NEW, unlike DB_MPOOL_CREATE, because the caller
		 * did not specify the page number, and so, may reasonably not
		 * have any way to lock the page outside of mpool.) Regardless,
		 * if we allocate the page, and some other thread of control
		 * requests the page by number, we will not detect that and the
		 * thread of control that allocated using DB_MPOOL_NEW may not
		 * have a chance to initialize the page.  (Note: we *could*
		 * detect this case if we set a flag in the buffer header which
		 * guaranteed that no gets of the page would succeed until the
		 * reference count went to 0, that is, until the creating page
		 * put the page.)  What we do guarantee is that if two threads
		 * of control are both doing DB_MPOOL_NEW calls, they won't
		 * collide, that is, they won't both get the same page.
		 *
		 * There's a possibility that another thread allocated the page
		 * we were planning to allocate while we were off doing buffer
		 * allocation.  We can do that by making sure the page number
		 * we were going to use is still available.  If it's not, then
		 * we check to see if the next available page number hashes to
		 * the same mpool region as the old one -- if it does, we can
		 * continue, otherwise, we have to start over.
		 */
		if (flags == DB_MPOOL_NEW && *pgnoaddr != mfp->last_pgno + 1) {
			*pgnoaddr = mfp->last_pgno + 1;
			MP_GET_REGION(dbmfp, *pgnoaddr, &t_infop, ret);
			if (ret != 0)
				goto err;
			if (t_infop != infop) {
				/*
				 * flags == DB_MPOOL_NEW, so extending is set
				 * and we're holding the mfp locked.
				 */
				MUTEX_UNLOCK(env, mfp->mutex);
				goto newpg;
			}
		}

		/*
		 * We released the mfp lock, so another thread might have
		 * extended the file.  Update the last_pgno and initialize
		 * the file, as necessary, if we extended the file.
		 */
		if (extending) {
			if (*pgnoaddr > mfp->last_pgno)
				mfp->last_pgno = *pgnoaddr;
			MUTEX_UNLOCK(env, mfp->mutex);
			if (ret != 0)
				goto err;
		}
		goto retry;
	case SECOND_FOUND:
		/*
		 * We allocated buffer space for the requested page, but then
		 * found the page in the buffer cache on our second check.
		 * That's OK -- we can use the page we found in the pool,
		 * unless DB_MPOOL_NEW is set.  If we're about to copy-on-write,
		 * this is exactly the situation we want.
		 *
		 * For multiversion files, we may have left some pages in cache
		 * beyond the end of a file after truncating.  In that case, we
		 * would get to here with extending set.  If so, we need to
		 * insert the new page in the version chain similar to when
		 * we copy on write.
		 */
		if (F_ISSET(bhp, BH_FREED) &&
		    (flags == DB_MPOOL_NEW || flags == DB_MPOOL_CREATE))
			goto revive;
		else if (flags == DB_MPOOL_FREE)
			goto freebuf;
		else if (makecopy || F_ISSET(bhp, BH_FROZEN))
			break;

		/*
		 * We can't use the page we found in the pool if DB_MPOOL_NEW
		 * was set.  (For details, see the above comment beginning
		 * "DB_MPOOL_NEW does not guarantee you a page unreferenced by
		 * any other thread of control".)  If DB_MPOOL_NEW is set, we
		 * release our pin on this particular buffer, and try to get
		 * another one.
		 */
		if (flags == DB_MPOOL_NEW) {
			DB_ASSERT(env, b_incr && BH_REFCOUNT(bhp) != 0);
			atomic_dec(env, &bhp->ref);
			b_incr = 0;
			if (F_ISSET(bhp, BH_EXCLUSIVE))
				F_CLR(bhp, BH_EXCLUSIVE);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			bhp = NULL;
			goto newpg;
		}

		break;
	case SECOND_MISS:
		/*
		 * We allocated buffer space for the requested page, and found
		 * the page still missing on our second pass through the buffer
		 * cache.  Instantiate the page.
		 */
		DB_ASSERT(env, alloc_bhp != NULL);
		bhp = alloc_bhp;
		alloc_bhp = NULL;

		/*
		 * Initialize all the BH and hash bucket fields so we can call
		 * __memp_bhfree if an error occurs.
		 *
		 * Append the buffer to the tail of the bucket list.
		 */
		bhp->priority = UINT32_MAX;
		bhp->pgno = *pgnoaddr;
		bhp->mf_offset = mf_offset;
		bhp->bucket = bucket;
		bhp->region = (int)(infop - dbmp->reginfo);
		bhp->td_off = INVALID_ROFF;
		SH_CHAIN_INIT(bhp, vc);
		bhp->flags = 0;

		/*
		 * Reference the buffer and lock exclusive.  We either
		 * need to read the buffer or create it from scratch
		 * and don't want anyone looking at it till we do.
		 */
		MUTEX_LOCK(env, bhp->mtx_buf);
		b_lock = 1;
		F_SET(bhp, BH_EXCLUSIVE);
		b_incr = 1;

		/* We created a new page, it starts dirty. */
		if (extending) {
			atomic_inc(env, &hp->hash_page_dirty);
			F_SET(bhp, BH_DIRTY | BH_DIRTY_CREATE);
		}

		MUTEX_REQUIRED(env, hp->mtx_hash);
		SH_TAILQ_INSERT_HEAD(&hp->hash_bucket, bhp, hq, __bh);
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;

		/*
		 * If we created the page, zero it out.  If we didn't create
		 * the page, read from the backing file.
		 *
		 * !!!
		 * DB_MPOOL_NEW doesn't call the pgin function.
		 *
		 * If DB_MPOOL_CREATE is used, then the application's pgin
		 * function has to be able to handle pages of 0's -- if it
		 * uses DB_MPOOL_NEW, it can detect all of its page creates,
		 * and not bother.
		 *
		 * If we're running in diagnostic mode, smash any bytes on the
		 * page that are unknown quantities for the caller.
		 *
		 * Otherwise, read the page into memory, optionally creating it
		 * if DB_MPOOL_CREATE is set.
		 */
		if (extending) {
			MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize,
			    PROT_READ | PROT_WRITE);
			memset(bhp->buf, 0,
			    (mfp->clear_len == DB_CLEARLEN_NOTSET) ?
			    mfp->stat.st_pagesize : mfp->clear_len);
#if defined(DIAGNOSTIC) || defined(UMRW)
			if (mfp->clear_len != DB_CLEARLEN_NOTSET)
				memset(bhp->buf + mfp->clear_len, CLEAR_BYTE,
				    mfp->stat.st_pagesize - mfp->clear_len);
#endif

			if (flags == DB_MPOOL_CREATE && mfp->ftype != 0 &&
			    (ret = __memp_pg(dbmfp,
			    bhp->pgno, bhp->buf, 1)) != 0)
				goto err;

			STAT(++mfp->stat.st_page_create);
		} else {
			F_SET(bhp, BH_TRASH);
			STAT(++mfp->stat.st_cache_miss);
		}

		makecopy = mvcc && dirty && !extending;

		/* Increment buffer count referenced by MPOOLFILE. */
		MUTEX_LOCK(env, mfp->mutex);
		++mfp->block_cnt;
		MUTEX_UNLOCK(env, mfp->mutex);
	}

	DB_ASSERT(env, bhp != NULL && BH_REFCOUNT(bhp) != 0 && b_lock);
	DB_ASSERT(env, !F_ISSET(bhp, BH_FROZEN) || !F_ISSET(bhp, BH_FREED) ||
	    makecopy);

	/* We've got a buffer header we're re-instantiating. */
	if (F_ISSET(bhp, BH_FROZEN) && !F_ISSET(bhp, BH_FREED)) {
		if (alloc_bhp == NULL)
			goto reuse;

		/*
		 * To thaw the buffer, we must hold the hash bucket mutex,
		 * and the call to __memp_bh_thaw will release it.
		 */
		if (h_locked == 0)
			MUTEX_LOCK(env, hp->mtx_hash);
		h_locked = 1;

		/*
		 * If the empty buffer has been filled in the meantime, don't
		 * overwrite it.
		 */
		if (F_ISSET(bhp, BH_THAWED)) {
			MUTEX_UNLOCK(env, hp->mtx_hash);
			h_locked = 0;
			goto thawed;
		}

		ret = __memp_bh_thaw(dbmp, infop, hp, bhp, alloc_bhp);
		bhp = NULL;
		b_lock = h_locked = 0;
		if (ret != 0)
			goto err;
		bhp = alloc_bhp;
		alloc_bhp = NULL;
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		b_incr = b_lock = 1;
	}

	/*
	 * BH_TRASH --
	 * The buffer we found may need to be filled from the disk.
	 *
	 * It's possible for the read function to fail, which means we fail
	 * as well.  Discard the buffer on failure unless another thread
	 * is waiting on our I/O to complete.  It's OK to leave the buffer
	 * around, as the waiting thread will see the BH_TRASH flag set,
	 * and will also attempt to discard it.  If there's a waiter,
	 * we need to decrement our reference count.
	 */
	if (F_ISSET(bhp, BH_TRASH) &&
	    flags != DB_MPOOL_FREE && !F_ISSET(bhp, BH_FREED)) {
		if ((ret = __memp_pgread(dbmfp,
		    bhp, LF_ISSET(DB_MPOOL_CREATE) ? 1 : 0)) != 0)
			goto err;
		DB_ASSERT(env, read_lsnp != NULL || !SH_CHAIN_HASNEXT(bhp, vc));
	}

	/* Copy-on-write. */
	if (makecopy) {
		/*
		 * If we read a page from disk that we want to modify, we now
		 * need to make copy, so we now need to allocate another buffer
		 * to hold the new copy.
		 */
		if (alloc_bhp == NULL)
			goto reuse;

		DB_ASSERT(env, bhp != NULL && alloc_bhp != bhp);
		DB_ASSERT(env, txn != NULL ||
		    (F_ISSET(bhp, BH_FROZEN) && F_ISSET(bhp, BH_FREED)));
		DB_ASSERT(env, (extending || flags == DB_MPOOL_FREE ||
		    F_ISSET(bhp, BH_FREED)) ||
		    !F_ISSET(bhp, BH_FROZEN | BH_TRASH));
		MUTEX_REQUIRED(env, bhp->mtx_buf);

		if (BH_REFCOUNT(bhp) == 1)
			MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize,
			    PROT_READ);

		atomic_init(&alloc_bhp->ref, 1);
		MUTEX_LOCK(env, alloc_bhp->mtx_buf);
		alloc_bhp->priority = bhp->priority;
		alloc_bhp->pgno = bhp->pgno;
		alloc_bhp->bucket = bhp->bucket;
		alloc_bhp->region = bhp->region;
		alloc_bhp->mf_offset = bhp->mf_offset;
		alloc_bhp->td_off = INVALID_ROFF;
		if (txn == NULL) {
			DB_ASSERT(env,
			    F_ISSET(bhp, BH_FROZEN) && F_ISSET(bhp, BH_FREED));
			if (bhp->td_off != INVALID_ROFF && (ret =
			    __memp_bh_settxn(dbmp, mfp, alloc_bhp,
			    BH_OWNER(env, bhp))) != 0)
				goto err;
		} else if ((ret =
		    __memp_bh_settxn(dbmp, mfp, alloc_bhp, td)) != 0)
			goto err;
		MVCC_MPROTECT(alloc_bhp->buf, mfp->stat.st_pagesize,
		    PROT_READ | PROT_WRITE);
		if (extending ||
		    F_ISSET(bhp, BH_FREED) || flags == DB_MPOOL_FREE) {
			memset(alloc_bhp->buf, 0,
			    (mfp->clear_len == DB_CLEARLEN_NOTSET) ?
			    mfp->stat.st_pagesize : mfp->clear_len);
#if defined(DIAGNOSTIC) || defined(UMRW)
			if (mfp->clear_len != DB_CLEARLEN_NOTSET)
				memset(alloc_bhp->buf + mfp->clear_len,
				    CLEAR_BYTE,
				    mfp->stat.st_pagesize - mfp->clear_len);
#endif
		} else
			memcpy(alloc_bhp->buf, bhp->buf, mfp->stat.st_pagesize);
		MVCC_MPROTECT(alloc_bhp->buf, mfp->stat.st_pagesize, 0);

		if (h_locked == 0)
			MUTEX_LOCK(env, hp->mtx_hash);
		MUTEX_REQUIRED(env, hp->mtx_hash);
		h_locked = 1;

		alloc_bhp->flags = BH_EXCLUSIVE |
		    ((flags == DB_MPOOL_FREE) ? BH_FREED :
		    F_ISSET(bhp, BH_DIRTY | BH_DIRTY_CREATE));
		DB_ASSERT(env, flags != DB_MPOOL_FREE ||
		    !F_ISSET(bhp, BH_DIRTY));
		F_CLR(bhp, BH_DIRTY | BH_DIRTY_CREATE);
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		SH_CHAIN_INSERT_AFTER(bhp, alloc_bhp, vc, __bh);
		SH_TAILQ_INSERT_BEFORE(&hp->hash_bucket,
		    bhp, alloc_bhp, hq, __bh);
		SH_TAILQ_REMOVE(&hp->hash_bucket, bhp, hq, __bh);
		MUTEX_UNLOCK(env, hp->mtx_hash);
		h_locked = 0;
		DB_ASSERT(env, b_incr && BH_REFCOUNT(bhp) > 0);
		if (atomic_dec(env, &bhp->ref) == 0) {
			bhp->priority = c_mp->lru_count;
			MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize, 0);
		}
		F_CLR(bhp, BH_EXCLUSIVE);
		MUTEX_UNLOCK(env, bhp->mtx_buf);

		bhp = alloc_bhp;
		DB_ASSERT(env, BH_REFCOUNT(bhp) > 0);
		b_incr = 1;
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		b_lock = 1;

		if (alloc_bhp != oldest_bhp) {
			MUTEX_LOCK(env, mfp->mutex);
			++mfp->block_cnt;
			MUTEX_UNLOCK(env, mfp->mutex);
		}

		alloc_bhp = NULL;
	} else if (mvcc && extending &&
	    (ret = __memp_bh_settxn(dbmp, mfp, bhp, td)) != 0)
		goto err;

	if (flags == DB_MPOOL_FREE) {
		DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
		/* If we have created an empty buffer, it is not returned. */
		if (!F_ISSET(bhp, BH_FREED))
			goto freebuf;
		goto done;
	}

	/*
	 * Free the allocated memory, we no longer need it.
	 */
	if (alloc_bhp != NULL) {
		if ((ret = __memp_bhfree(dbmp, infop, NULL,
		     NULL, alloc_bhp, BH_FREE_FREEMEM | BH_FREE_UNLOCKED)) != 0)
			goto err;
		alloc_bhp = NULL;
	}

	if (dirty || extending ||
	    (F_ISSET(bhp, BH_FREED) &&
	    (flags == DB_MPOOL_CREATE || flags == DB_MPOOL_NEW))) {
		MUTEX_REQUIRED(env, bhp->mtx_buf);
		if (F_ISSET(bhp, BH_FREED)) {
			memset(bhp->buf, 0,
			    (mfp->clear_len == DB_CLEARLEN_NOTSET) ?
			    mfp->stat.st_pagesize : mfp->clear_len);
			F_CLR(bhp, BH_FREED);
		}
		if (!F_ISSET(bhp, BH_DIRTY)) {
#ifdef DIAGNOSTIC
			MUTEX_LOCK(env, hp->mtx_hash);
#endif
			DB_ASSERT(env, !SH_CHAIN_HASNEXT(bhp, vc));
			atomic_inc(env, &hp->hash_page_dirty);
			F_SET(bhp, BH_DIRTY);
#ifdef DIAGNOSTIC
			MUTEX_UNLOCK(env, hp->mtx_hash);
#endif
		}
	} else if (F_ISSET(bhp, BH_EXCLUSIVE)) {
		F_CLR(bhp, BH_EXCLUSIVE);
#ifdef HAVE_SHARED_LATCHES
		MUTEX_UNLOCK(env, bhp->mtx_buf);
		MUTEX_READLOCK(env, bhp->mtx_buf);
		/*
		 * If another thread has dirtied the page while we
		 * switched locks, we have to go through it all again.
		 */
		if (SH_CHAIN_HASNEXT(bhp, vc) && read_lsnp == NULL) {
			atomic_dec(env, &bhp->ref);
			b_incr = 0;
			MUTEX_UNLOCK(env, bhp->mtx_buf);
			b_lock = 0;
			bhp = NULL;
			goto retry;
		}
#endif
	}

	MVCC_MPROTECT(bhp->buf, mfp->stat.st_pagesize, PROT_READ |
	    (dirty || extending || F_ISSET(bhp, BH_DIRTY) ?
	    PROT_WRITE : 0));

#ifdef DIAGNOSTIC
	MUTEX_LOCK(env, hp->mtx_hash);
	{
	BH *next_bhp = SH_CHAIN_NEXT(bhp, vc, __bh);

	DB_ASSERT(env, !mfp->multiversion || read_lsnp != NULL ||
	    next_bhp == NULL);
	DB_ASSERT(env, !mvcc || read_lsnp == NULL ||
	    bhp->td_off == INVALID_ROFF || BH_OWNED_BY(env, bhp, txn) ||
	    (BH_VISIBLE(env, bhp, read_lsnp, vlsn) &&
	    (next_bhp == NULL || F_ISSET(next_bhp, BH_FROZEN) ||
	    (next_bhp->td_off != INVALID_ROFF &&
	    (BH_OWNER(env, next_bhp)->status != TXN_COMMITTED ||
	    IS_ZERO_LSN(BH_OWNER(env, next_bhp)->last_lsn) ||
	    !BH_VISIBLE(env, next_bhp, read_lsnp, vlsn))))));
	}
	MUTEX_UNLOCK(env, hp->mtx_hash);
#endif

	/*
	 * Record this pin for this thread.  Holding the page pinned
	 * without recording the pin is ok since we do not recover from
	 * a death from within the library itself.
	 */
	if (ip != NULL) {
		reginfo = env->reginfo;
		if (ip->dbth_pincount == ip->dbth_pinmax) {
			pinmax = ip->dbth_pinmax;
			renv = reginfo->primary;
			MUTEX_LOCK(env, renv->mtx_regenv);
			if ((ret = __env_alloc(reginfo,
			    2 * pinmax * sizeof(PIN_LIST), &list)) != 0) {
				MUTEX_UNLOCK(env, renv->mtx_regenv);
				goto err;
			}

			memcpy(list, R_ADDR(reginfo, ip->dbth_pinlist),
			    pinmax * sizeof(PIN_LIST));
			memset(&list[pinmax], 0, pinmax * sizeof(PIN_LIST));
			list_off = R_OFFSET(reginfo, list);
			list = R_ADDR(reginfo, ip->dbth_pinlist);
			ip->dbth_pinmax = 2 * pinmax;
			ip->dbth_pinlist = list_off;
			if (list != ip->dbth_pinarray)
				__env_alloc_free(reginfo, list);
			MUTEX_UNLOCK(env, renv->mtx_regenv);
		}
		list = R_ADDR(reginfo, ip->dbth_pinlist);
		for (lp = list; lp < &list[ip->dbth_pinmax]; lp++)
			if (lp->b_ref == INVALID_ROFF)
				break;

		ip->dbth_pincount++;
		lp->b_ref = R_OFFSET(infop, bhp);
		lp->region = (int)(infop - dbmp->reginfo);
	}

#ifdef DIAGNOSTIC
	/* Update the file's pinned reference count. */
	MPOOL_SYSTEM_LOCK(env);
	++dbmfp->pinref;
	MPOOL_SYSTEM_UNLOCK(env);

	/*
	 * We want to switch threads as often as possible, and at awkward
	 * times.  Yield every time we get a new page to ensure contention.
	 */
	if (F_ISSET(env->dbenv, DB_ENV_YIELDCPU))
		__os_yield(env, 0, 0);
#endif

	DB_ASSERT(env, alloc_bhp == NULL);
	DB_ASSERT(env, !(dirty || extending) ||
	    atomic_read(&hp->hash_page_dirty) > 0);
	DB_ASSERT(env, BH_REFCOUNT(bhp) > 0 &&
	    !F_ISSET(bhp, BH_FREED | BH_FROZEN | BH_TRASH));

	*(void **)addrp = bhp->buf;
	return (0);

done:
err:	/*
	 * We should only get to here with ret == 0 if freeing a buffer.
	 * In that case, check that it has in fact been freed.
	 */
	DB_ASSERT(env, ret != 0 || flags != DB_MPOOL_FREE || bhp == NULL ||
	    (F_ISSET(bhp, BH_FREED) && !SH_CHAIN_HASNEXT(bhp, vc)));

	if (bhp != NULL) {
		if (b_incr)
			atomic_dec(env, &bhp->ref);
		if (b_lock) {
			F_CLR(bhp, BH_EXCLUSIVE);
			MUTEX_UNLOCK(env, bhp->mtx_buf);
		}
	}

	if (h_locked)
		MUTEX_UNLOCK(env, hp->mtx_hash);

	/* If alloc_bhp is set, free the memory. */
	if (alloc_bhp != NULL)
		(void)__memp_bhfree(dbmp, infop, NULL,
		     NULL, alloc_bhp, BH_FREE_FREEMEM | BH_FREE_UNLOCKED);

	return (ret);
}
