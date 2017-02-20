/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/mp.h"
#include "dbinc/db_page.h"
#include "dbinc/hash.h"

/*
 * __memp_env_create --
 *	Mpool specific creation of the DB_ENV structure.
 *
 * PUBLIC: int __memp_env_create __P((DB_ENV *));
 */
int
__memp_env_create(dbenv)
	DB_ENV *dbenv;
{
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 *
	 * We default to 32 8K pages.  We don't default to a flat 256K, because
	 * some systems require significantly more memory to hold 32 pages than
	 * others.  For example, HP-UX with POSIX pthreads needs 88 bytes for
	 * a POSIX pthread mutex and almost 200 bytes per buffer header, while
	 * Solaris needs 24 and 52 bytes for the same structures.  The minimum
	 * number of hash buckets is 37.  These contain a mutex also.
	 */
	dbenv->mp_bytes = dbenv->mp_max_bytes =
	    32 * ((8 * 1024) + sizeof(BH)) + 37 * sizeof(DB_MPOOL_HASH);
	dbenv->mp_ncache = 1;

	return (0);
}

/*
 * __memp_env_destroy --
 *	Mpool specific destruction of the DB_ENV structure.
 *
 * PUBLIC: void __memp_env_destroy __P((DB_ENV *));
 */
void
__memp_env_destroy(dbenv)
	DB_ENV *dbenv;
{
	COMPQUIET(dbenv, NULL);
}

/*
 * __memp_get_cachesize --
 *	{DB_ENV,DB}->get_cachesize.
 *
 * PUBLIC: int __memp_get_cachesize
 * PUBLIC:         __P((DB_ENV *, u_int32_t *, u_int32_t *, int *));
 */
int
__memp_get_cachesize(dbenv, gbytesp, bytesp, ncachep)
	DB_ENV *dbenv;
	u_int32_t *gbytesp, *bytesp;
	int *ncachep;
{
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_cachesize", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		/* Cannot be set after open, no lock required to read. */
		mp = env->mp_handle->reginfo[0].primary;
		if (gbytesp != NULL)
			*gbytesp = mp->stat.st_gbytes;
		if (bytesp != NULL)
			*bytesp = mp->stat.st_bytes;
		if (ncachep != NULL)
			*ncachep = (int)mp->nreg;
	} else {
		if (gbytesp != NULL)
			*gbytesp = dbenv->mp_gbytes;
		if (bytesp != NULL)
			*bytesp = dbenv->mp_bytes;
		if (ncachep != NULL)
			*ncachep = (int)dbenv->mp_ncache;
	}
	return (0);
}

/*
 * __memp_set_cachesize --
 *	{DB_ENV,DB}->set_cachesize.
 *
 * PUBLIC: int __memp_set_cachesize __P((DB_ENV *, u_int32_t, u_int32_t, int));
 */
int
__memp_set_cachesize(dbenv, gbytes, bytes, arg_ncache)
	DB_ENV *dbenv;
	u_int32_t gbytes, bytes;
	int arg_ncache;
{
	ENV *env;
	u_int ncache;

	env = dbenv->env;

	/* Normalize the cache count. */
	ncache = arg_ncache <= 0 ? 1 : (u_int)arg_ncache;

	/*
	 * You can only store 4GB-1 in an unsigned 32-bit value, so correct for
	 * applications that specify 4GB cache sizes -- we know what they meant.
	 */
	if (sizeof(roff_t) == 4 && gbytes / ncache == 4 && bytes == 0) {
		--gbytes;
		bytes = GIGABYTE - 1;
	} else {
		gbytes += bytes / GIGABYTE;
		bytes %= GIGABYTE;
	}

	/*
	 * !!!
	 * With 32-bit region offsets, individual cache regions must be smaller
	 * than 4GB.  Also, cache sizes larger than 10TB would cause 32-bit
	 * wrapping in the calculation of the number of hash buckets.  See
	 * __memp_open for details.
	 */
	if (!F_ISSET(env, ENV_OPEN_CALLED)) {
		if (sizeof(roff_t) <= 4 && gbytes / ncache >= 4) {
			__db_errx(env,
			    "individual cache size too large: maximum is 4GB");
			return (EINVAL);
		}
		if (gbytes / ncache > 10000) {
			__db_errx(env,
			    "individual cache size too large: maximum is 10TB");
			return (EINVAL);
		}
	}

	/*
	 * If the application requested less than 500Mb, increase the cachesize
	 * by 25% and factor in the size of the hash buckets to account for our
	 * overhead.  (I'm guessing caches over 500Mb are specifically sized,
	 * that is, it's a large server and the application actually knows how
	 * much memory is available.  We only document the 25% overhead number,
	 * not the hash buckets, but I don't see a reason to confuse the issue,
	 * it shouldn't matter to an application.)
	 *
	 * There is a minimum cache size, regardless.
	 */
	if (gbytes == 0) {
		if (bytes < 500 * MEGABYTE)
			bytes += (bytes / 4) + 37 * sizeof(DB_MPOOL_HASH);
		if (bytes / ncache < DB_CACHESIZE_MIN)
			bytes = ncache * DB_CACHESIZE_MIN;
	}

	if (F_ISSET(env, ENV_OPEN_CALLED))
		return (__memp_resize(env->mp_handle, gbytes, bytes));

	dbenv->mp_gbytes = gbytes;
	dbenv->mp_bytes = bytes;
	dbenv->mp_ncache = ncache;

	return (0);
}

/*
 * __memp_set_config --
 *	Set the cache subsystem configuration.
 *
 * PUBLIC: int __memp_set_config __P((DB_ENV *, u_int32_t, int));
 */
int
__memp_set_config(dbenv, which, on)
	DB_ENV *dbenv;
	u_int32_t which;
	int on;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->memp_set_config", DB_INIT_MPOOL);

	switch (which) {
	case DB_MEMP_SUPPRESS_WRITE:
	case DB_MEMP_SYNC_INTERRUPT:
		if (MPOOL_ON(env)) {
			dbmp = env->mp_handle;
			mp = dbmp->reginfo[0].primary;
			if (on)
				FLD_SET(mp->config_flags, which);
			else
				FLD_CLR(mp->config_flags, which);
		}
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
 * __memp_get_config --
 *	Return the cache subsystem configuration.
 *
 * PUBLIC: int __memp_get_config __P((DB_ENV *, u_int32_t, int *));
 */
int
__memp_get_config(dbenv, which, onp)
	DB_ENV *dbenv;
	u_int32_t which;
	int *onp;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->mp_handle, "DB_ENV->memp_get_config", DB_INIT_MPOOL);

	switch (which) {
	case DB_MEMP_SUPPRESS_WRITE:
	case DB_MEMP_SYNC_INTERRUPT:
		if (MPOOL_ON(env)) {
			dbmp = env->mp_handle;
			mp = dbmp->reginfo[0].primary;
			*onp = FLD_ISSET(mp->config_flags, which) ? 1 : 0;
		} else
			*onp = 0;
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
 * PUBLIC: int __memp_get_mp_max_openfd __P((DB_ENV *, int *));
 */
int
__memp_get_mp_max_openfd(dbenv, maxopenfdp)
	DB_ENV *dbenv;
	int *maxopenfdp;
{
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_openfd", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		*maxopenfdp = mp->mp_maxopenfd;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		*maxopenfdp = dbenv->mp_maxopenfd;
	return (0);
}

/*
 * __memp_set_mp_max_openfd --
 *	Set the maximum number of open fd's when flushing the cache.
 * PUBLIC: int __memp_set_mp_max_openfd __P((DB_ENV *, int));
 */
int
__memp_set_mp_max_openfd(dbenv, maxopenfd)
	DB_ENV *dbenv;
	int maxopenfd;
{
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->set_mp_max_openfd", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		mp->mp_maxopenfd = maxopenfd;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		dbenv->mp_maxopenfd = maxopenfd;
	return (0);
}

/*
 * PUBLIC: int __memp_get_mp_max_write __P((DB_ENV *, int *, db_timeout_t *));
 */
int
__memp_get_mp_max_write(dbenv, maxwritep, maxwrite_sleepp)
	DB_ENV *dbenv;
	int *maxwritep;
	db_timeout_t *maxwrite_sleepp;
{
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_write", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		*maxwritep = mp->mp_maxwrite;
		*maxwrite_sleepp = mp->mp_maxwrite_sleep;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		*maxwritep = dbenv->mp_maxwrite;
		*maxwrite_sleepp = dbenv->mp_maxwrite_sleep;
	}
	return (0);
}

/*
 * __memp_set_mp_max_write --
 *	Set the maximum continuous I/O count.
 *
 * PUBLIC: int __memp_set_mp_max_write __P((DB_ENV *, int, db_timeout_t));
 */
int
__memp_set_mp_max_write(dbenv, maxwrite, maxwrite_sleep)
	DB_ENV *dbenv;
	int maxwrite;
	db_timeout_t maxwrite_sleep;
{
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_write", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		mp->mp_maxwrite = maxwrite;
		mp->mp_maxwrite_sleep = maxwrite_sleep;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else {
		dbenv->mp_maxwrite = maxwrite;
		dbenv->mp_maxwrite_sleep = maxwrite_sleep;
	}
	return (0);
}

/*
 * PUBLIC: int __memp_get_mp_mmapsize __P((DB_ENV *, size_t *));
 */
int
__memp_get_mp_mmapsize(dbenv, mp_mmapsizep)
	DB_ENV *dbenv;
	size_t *mp_mmapsizep;
{
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		*mp_mmapsizep = mp->mp_mmapsize;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		*mp_mmapsizep = dbenv->mp_mmapsize;
	return (0);
}

/*
 * __memp_set_mp_mmapsize --
 *	DB_ENV->set_mp_mmapsize.
 *
 * PUBLIC: int __memp_set_mp_mmapsize __P((DB_ENV *, size_t));
 */
int
__memp_set_mp_mmapsize(dbenv, mp_mmapsize)
	DB_ENV *dbenv;
	size_t mp_mmapsize;
{
	DB_MPOOL *dbmp;
	DB_THREAD_INFO *ip;
	ENV *env;
	MPOOL *mp;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->set_mp_max_mmapsize", DB_INIT_MPOOL);

	if (MPOOL_ON(env)) {
		dbmp = env->mp_handle;
		mp = dbmp->reginfo[0].primary;
		ENV_ENTER(env, ip);
		MPOOL_SYSTEM_LOCK(env);
		mp->mp_mmapsize = mp_mmapsize;
		MPOOL_SYSTEM_UNLOCK(env);
		ENV_LEAVE(env, ip);
	} else
		dbenv->mp_mmapsize = mp_mmapsize;
	return (0);
}

/*
 * PUBLIC: int __memp_get_mp_pagesize __P((DB_ENV *, u_int32_t *));
 */
int
__memp_get_mp_pagesize(dbenv, mp_pagesizep)
	DB_ENV *dbenv;
	u_int32_t *mp_pagesizep;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_pagesize", DB_INIT_MPOOL);

	*mp_pagesizep = dbenv->mp_pagesize;
	return (0);
}

/*
 * __memp_set_mp_pagesize --
 *	DB_ENV->set_mp_pagesize.
 *
 * PUBLIC: int __memp_set_mp_pagesize __P((DB_ENV *, u_int32_t));
 */
int
__memp_set_mp_pagesize(dbenv, mp_pagesize)
	DB_ENV *dbenv;
	u_int32_t mp_pagesize;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mp_pagesize");

	dbenv->mp_pagesize = mp_pagesize;
	return (0);
}

/*
 * PUBLIC: int __memp_get_mp_tablesize __P((DB_ENV *, u_int32_t *));
 */
int
__memp_get_mp_tablesize(dbenv, mp_tablesizep)
	DB_ENV *dbenv;
	u_int32_t *mp_tablesizep;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_tablesize", DB_INIT_MPOOL);

	*mp_tablesizep = dbenv->mp_tablesize;
	return (0);
}

/*
 * __memp_set_mp_tablesize --
 *	DB_ENV->set_mp_tablesize.
 *
 * PUBLIC: int __memp_set_mp_tablesize __P((DB_ENV *, u_int32_t));
 */
int
__memp_set_mp_tablesize(dbenv, mp_tablesize)
	DB_ENV *dbenv;
	u_int32_t mp_tablesize;
{
	ENV *env;

	env = dbenv->env;

	ENV_NOT_CONFIGURED(env,
	    env->mp_handle, "DB_ENV->get_mp_max_mmapsize", DB_INIT_MPOOL);
	ENV_ILLEGAL_AFTER_OPEN(env, "DB_ENV->set_mp_tablesize");

	dbenv->mp_tablesize = mp_tablesize;
	return (0);
}

/*
 * __memp_nameop
 *	Remove or rename a file in the pool.
 *
 * PUBLIC: int __memp_nameop __P((ENV *,
 * PUBLIC:     u_int8_t *, const char *, const char *, const char *, int));
 *
 * XXX
 * Undocumented interface: DB private.
 */
int
__memp_nameop(env, fileid, newname, fullold, fullnew, inmem)
	ENV *env;
	u_int8_t *fileid;
	const char *newname, *fullold, *fullnew;
	int inmem;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp, *nhp;
	MPOOL *mp;
	MPOOLFILE *mfp;
	roff_t newname_off;
	u_int32_t bucket;
	int locked, ret;
	size_t nlen;
	void *p;

#undef	op_is_remove
#define	op_is_remove	(newname == NULL)

	COMPQUIET(bucket, 0);
	COMPQUIET(hp, NULL);
	COMPQUIET(newname_off, 0);
	COMPQUIET(nlen, 0);

	dbmp = NULL;
	mfp = NULL;
	nhp = NULL;
	p = NULL;
	locked = ret = 0;

	if (!MPOOL_ON(env))
		goto fsop;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;
	hp = R_ADDR(dbmp->reginfo, mp->ftab);

	if (!op_is_remove) {
		nlen = strlen(newname);
		if ((ret = __memp_alloc(dbmp, dbmp->reginfo,
		    NULL,  nlen + 1, &newname_off, &p)) != 0)
			return (ret);
		memcpy(p, newname, nlen + 1);
	}

	/*
	 * Remove or rename a file that the mpool might know about.  We assume
	 * that the fop layer has the file locked for exclusive access, so we
	 * don't worry about locking except for the mpool mutexes.  Checkpoint
	 * can happen at any time, independent of file locking, so we have to
	 * do the actual unlink or rename system call while holding
	 * all affected buckets locked.
	 *
	 * If this is a rename and this is a memory file then we need
	 * to make sure that the new name does not exist.  Since we
	 * are locking two buckets lock them in ascending order.
	 */
	if (inmem) {
		DB_ASSERT(env, fullold != NULL);
		hp += FNBUCKET(fullold, strlen(fullold));
		if (!op_is_remove) {
			bucket = FNBUCKET(newname, nlen);
			nhp = R_ADDR(dbmp->reginfo, mp->ftab);
			nhp += bucket;
		}
	} else
		hp += FNBUCKET(fileid, DB_FILE_ID_LEN);

	if (nhp != NULL && nhp < hp)
		MUTEX_LOCK(env, nhp->mtx_hash);
	MUTEX_LOCK(env, hp->mtx_hash);
	if (nhp != NULL && nhp > hp)
		MUTEX_LOCK(env, nhp->mtx_hash);
	locked = 1;

	if (!op_is_remove && inmem) {
		SH_TAILQ_FOREACH(mfp, &nhp->hash_bucket, q, __mpoolfile)
			if (!mfp->deadfile &&
			    mfp->no_backing_file && strcmp(newname,
			    R_ADDR(dbmp->reginfo, mfp->path_off)) == 0)
				break;
		if (mfp != NULL) {
			ret = EEXIST;
			goto err;
		}
	}

	/*
	 * Find the file -- if mpool doesn't know about this file, that may
	 * not be an error.
	 */
	SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
		/* Ignore non-active files. */
		if (mfp->deadfile || F_ISSET(mfp, MP_TEMP))
			continue;

		/* Try to match on fileid. */
		if (memcmp(fileid, R_ADDR(
		    dbmp->reginfo, mfp->fileid_off), DB_FILE_ID_LEN) != 0)
			continue;

		break;
	}

	if (mfp == NULL) {
		if (inmem) {
			ret = ENOENT;
			goto err;
		}
		goto fsop;
	}

	if (op_is_remove) {
		MUTEX_LOCK(env, mfp->mutex);
		/*
		 * In-memory dbs have an artificially incremented ref count so
		 * they do not get reclaimed as long as they exist.  Since we
		 * are now deleting the database, we need to dec that count.
		 */
		if (mfp->no_backing_file)
			mfp->mpf_cnt--;
		mfp->deadfile = 1;
		MUTEX_UNLOCK(env, mfp->mutex);
	} else {
		/*
		 * Else, it's a rename.  We've allocated memory for the new
		 * name.  Swap it with the old one.  If it's in memory we
		 * need to move it the right bucket.
		 */
		p = R_ADDR(dbmp->reginfo, mfp->path_off);
		mfp->path_off = newname_off;

		if (inmem && hp != nhp) {
			DB_ASSERT(env, nhp != NULL);
			SH_TAILQ_REMOVE(&hp->hash_bucket, mfp, q, __mpoolfile);
			mfp->bucket = bucket;
			SH_TAILQ_INSERT_TAIL(&nhp->hash_bucket, mfp, q);
		}
	}

fsop:	/*
	 * If this is a real file, then mfp could be NULL, because
	 * mpool isn't turned on, and we still need to do the file ops.
	 */
	if (mfp == NULL || !mfp->no_backing_file) {
		if (op_is_remove) {
			/*
			 * !!!
			 * Replication may ask us to unlink a file that's been
			 * renamed.  Don't complain if it doesn't exist.
			 */
			if ((ret = __os_unlink(env, fullold, 0)) == ENOENT)
				ret = 0;
		} else {
			/*
			 * Defensive only, fullnew should never be
			 * NULL.
			 */
			DB_ASSERT(env, fullnew != NULL);
			if (fullnew == NULL) {
				ret = EINVAL;
				goto err;
			}
			ret = __os_rename(env, fullold, fullnew, 1);
		}
	}

	/* Delete the memory we no longer need. */
err:	if (p != NULL) {
		MPOOL_REGION_LOCK(env, &dbmp->reginfo[0]);
		__memp_free(&dbmp->reginfo[0], p);
		MPOOL_REGION_UNLOCK(env, &dbmp->reginfo[0]);
	}

	/* If we have buckets locked, unlock them when done moving files. */
	if (locked == 1) {
		MUTEX_UNLOCK(env, hp->mtx_hash);
		if (nhp != NULL && nhp != hp)
			MUTEX_UNLOCK(env, nhp->mtx_hash);
	}
	return (ret);
}

/*
 * __memp_ftruncate __
 *	Truncate the file.
 *
 * PUBLIC: int __memp_ftruncate __P((DB_MPOOLFILE *, DB_TXN *,
 * PUBLIC:     DB_THREAD_INFO *, db_pgno_t, u_int32_t));
 */
int
__memp_ftruncate(dbmfp, txn, ip, pgno, flags)
	DB_MPOOLFILE *dbmfp;
	DB_TXN *txn;
	DB_THREAD_INFO *ip;
	db_pgno_t pgno;
	u_int32_t flags;
{
	ENV *env;
	MPOOLFILE *mfp;
	void *pagep;
	db_pgno_t last_pgno, pg;
	int ret;

	env = dbmfp->env;
	mfp = dbmfp->mfp;
	ret = 0;

	MUTEX_LOCK(env, mfp->mutex);
	last_pgno = mfp->last_pgno;
	MUTEX_UNLOCK(env, mfp->mutex);

	if (pgno > last_pgno) {
		if (LF_ISSET(MP_TRUNC_RECOVER))
			return (0);
		__db_errx(env, "Truncate beyond the end of file");
		return (EINVAL);
	}

	pg = pgno;
	do {
		if (mfp->block_cnt == 0)
			break;
		if ((ret = __memp_fget(dbmfp, &pg,
		    ip, txn, DB_MPOOL_FREE, &pagep)) != 0)
			return (ret);
	} while (pg++ < last_pgno);

	/*
	 * If we are aborting an extend of a file, the call to __os_truncate
	 * could extend the file if the new page(s) had not yet been
	 * written to disk.  We do not want to extend the file to pages
	 * whose log records are not yet flushed [#14031].  In addition if
	 * we are out of disk space we can generate an error [#12743].
	 */
	MUTEX_LOCK(env, mfp->mutex);
	if (!F_ISSET(mfp, MP_TEMP) &&
	    !mfp->no_backing_file && pgno <= mfp->last_flushed_pgno)
#ifdef HAVE_FTRUNCATE
		ret = __os_truncate(env,
		    dbmfp->fhp, pgno, mfp->stat.st_pagesize);
#else
		ret = __db_zero_extend(env,
		    dbmfp->fhp, pgno, mfp->last_pgno, mfp->stat.st_pagesize);
#endif

	/*
	 * This set could race with another thread of control that extending
	 * the file.  It's not a problem because we should have the page
	 * locked at a higher level of the system.
	 */
	if (ret == 0) {
		mfp->last_pgno = pgno - 1;
		if (mfp->last_flushed_pgno > mfp->last_pgno)
			mfp->last_flushed_pgno = mfp->last_pgno;
	}
	MUTEX_UNLOCK(env, mfp->mutex);

	return (ret);
}

#ifdef HAVE_FTRUNCATE
/*
 * Support routines for maintaining a sorted freelist while we try to rearrange
 * and truncate the file.
 */

/*
 * __memp_alloc_freelist --
 *	Allocate mpool space for the freelist.
 *
 * PUBLIC: int __memp_alloc_freelist __P((DB_MPOOLFILE *,
 * PUBLIC:	 u_int32_t, db_pgno_t **));
 */
int
__memp_alloc_freelist(dbmfp, nelems, listp)
	DB_MPOOLFILE *dbmfp;
	u_int32_t nelems;
	db_pgno_t **listp;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOLFILE *mfp;
	void *retp;
	int ret;

	env = dbmfp->env;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;

	*listp = NULL;

	/*
	 * These fields are protected because the database layer
	 * has the metapage locked while manipulating them.
	 */
	mfp->free_ref++;
	if (mfp->free_size != 0)
		return (EBUSY);

	/* Allocate at least a few slots. */
	mfp->free_cnt = nelems;
	if (nelems == 0)
		nelems = 50;

	if ((ret = __memp_alloc(dbmp, dbmp->reginfo,
	    NULL, nelems * sizeof(db_pgno_t), &mfp->free_list, &retp)) != 0)
		return (ret);

	mfp->free_size = nelems * sizeof(db_pgno_t);
	*listp = retp;
	return (0);
}

/*
 * __memp_free_freelist --
 *	Free the list.
 *
 * PUBLIC: int __memp_free_freelist __P((DB_MPOOLFILE *));
 */
int
__memp_free_freelist(dbmfp)
	DB_MPOOLFILE *dbmfp;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOLFILE *mfp;

	env = dbmfp->env;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;

	DB_ASSERT(env, mfp->free_ref > 0);
	if (--mfp->free_ref > 0)
		return (0);

	DB_ASSERT(env, mfp->free_size != 0);

	MPOOL_SYSTEM_LOCK(env);
	__memp_free(dbmp->reginfo, R_ADDR(dbmp->reginfo, mfp->free_list));
	MPOOL_SYSTEM_UNLOCK(env);

	mfp->free_cnt = 0;
	mfp->free_list = 0;
	mfp->free_size = 0;
	return (0);
}

/*
 * __memp_get_freelst --
 *	Return current list.
 *
 * PUBLIC: int __memp_get_freelist __P((
 * PUBLIC:	DB_MPOOLFILE *, u_int32_t *, db_pgno_t **));
 */
int
__memp_get_freelist(dbmfp, nelemp, listp)
	DB_MPOOLFILE *dbmfp;
	u_int32_t *nelemp;
	db_pgno_t **listp;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOLFILE *mfp;

	env = dbmfp->env;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;

	if (mfp->free_size == 0) {
		*nelemp = 0;
		*listp = NULL;
	} else {
		*nelemp = mfp->free_cnt;
		*listp = R_ADDR(dbmp->reginfo, mfp->free_list);
	}

	return (0);
}

/*
 * __memp_extend_freelist --
 *	Extend the list.
 *
 * PUBLIC: int __memp_extend_freelist __P((
 * PUBLIC:	DB_MPOOLFILE *, u_int32_t , db_pgno_t **));
 */
int
__memp_extend_freelist(dbmfp, count, listp)
	DB_MPOOLFILE *dbmfp;
	u_int32_t count;
	db_pgno_t **listp;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOLFILE *mfp;
	int ret;
	void *retp;

	env = dbmfp->env;
	dbmp = env->mp_handle;
	mfp = dbmfp->mfp;

	if (mfp->free_size == 0)
		return (EINVAL);

	if (count * sizeof(db_pgno_t) > mfp->free_size) {
		mfp->free_size =
		     (size_t)DB_ALIGN(count * sizeof(db_pgno_t), 512);
		*listp = R_ADDR(dbmp->reginfo, mfp->free_list);
		if ((ret = __memp_alloc(dbmp, dbmp->reginfo,
		    NULL, mfp->free_size, &mfp->free_list, &retp)) != 0)
			return (ret);

		memcpy(retp, *listp, mfp->free_cnt * sizeof(db_pgno_t));

		MPOOL_SYSTEM_LOCK(env);
		__memp_free(dbmp->reginfo, *listp);
		MPOOL_SYSTEM_UNLOCK(env);
	}

	mfp->free_cnt = count;
	*listp = R_ADDR(dbmp->reginfo, mfp->free_list);

	return (0);
}
#endif

/*
 * __memp_set_last_pgno -- set the last page of the file
 *
 * PUBLIC: void __memp_set_last_pgno __P((DB_MPOOLFILE *, db_pgno_t));
 */
void
__memp_set_last_pgno(dbmfp, pgno)
	DB_MPOOLFILE *dbmfp;
	db_pgno_t pgno;
{
	dbmfp->mfp->last_pgno = pgno;
}
