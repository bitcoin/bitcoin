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

static int __memp_mpf_alloc __P((DB_MPOOL *,
    DB_MPOOLFILE *, const char *, u_int32_t, u_int32_t, MPOOLFILE **));
static int __memp_mpf_find __P((ENV *,
    DB_MPOOLFILE *, DB_MPOOL_HASH *, const char *, u_int32_t, MPOOLFILE **));

/*
 * __memp_fopen_pp --
 *	DB_MPOOLFILE->open pre/post processing.
 *
 * PUBLIC: int __memp_fopen_pp
 * PUBLIC:     __P((DB_MPOOLFILE *, const char *, u_int32_t, int, size_t));
 */
int
__memp_fopen_pp(dbmfp, path, flags, mode, pagesize)
	DB_MPOOLFILE *dbmfp;
	const char *path;
	u_int32_t flags;
	int mode;
	size_t pagesize;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbmfp->env;

	/* Validate arguments. */
	if ((ret = __db_fchk(env, "DB_MPOOLFILE->open", flags,
	    DB_CREATE | DB_DIRECT | DB_EXTENT | DB_MULTIVERSION |
	    DB_NOMMAP | DB_ODDFILESIZE | DB_RDONLY | DB_TRUNCATE)) != 0)
		return (ret);

	/*
	 * Require a non-zero, power-of-two pagesize, smaller than the
	 * clear length.
	 */
	if (pagesize == 0 || !POWER_OF_TWO(pagesize)) {
		__db_errx(env,
		    "DB_MPOOLFILE->open: page sizes must be a power-of-2");
		return (EINVAL);
	}
	if (dbmfp->clear_len > pagesize) {
		__db_errx(env,
		    "DB_MPOOLFILE->open: clear length larger than page size");
		return (EINVAL);
	}

	/* Read-only checks, and local flag. */
	if (LF_ISSET(DB_RDONLY) && path == NULL) {
		__db_errx(env,
		    "DB_MPOOLFILE->open: temporary files can't be readonly");
		return (EINVAL);
	}

	if (LF_ISSET(DB_MULTIVERSION) && !TXN_ON(env)) {
		__db_errx(env,
		   "DB_MPOOLFILE->open: DB_MULTIVERSION requires transactions");
		return (EINVAL);
	}

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env,
	    (__memp_fopen(dbmfp, NULL,
	    path, NULL, flags, mode, pagesize)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_fopen --
 *	DB_MPOOLFILE->open.
 *
 * PUBLIC: int __memp_fopen __P((DB_MPOOLFILE *, MPOOLFILE *,
 * PUBLIC:     const char *, const char **, u_int32_t, int, size_t));
 */
int
__memp_fopen(dbmfp, mfp, path, dirp, flags, mode, pgsize)
	DB_MPOOLFILE *dbmfp;
	MPOOLFILE *mfp;
	const char *path;
	const char **dirp;
	u_int32_t flags;
	int mode;
	size_t pgsize;
{
	DB_ENV *dbenv;
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *tmp_dbmfp;
	DB_MPOOL_HASH *hp;
	ENV *env;
	MPOOL *mp;
	MPOOLFILE *alloc_mfp;
	size_t maxmap;
	db_pgno_t last_pgno;
	u_int32_t bucket, mbytes, bytes, oflags, pagesize;
	int refinc, ret;
	char *rpath;

	/* If this handle is already open, return. */
	if (F_ISSET(dbmfp, MP_OPEN_CALLED))
		return (0);

	env = dbmfp->env;
	dbmp = env->mp_handle;
	dbenv = env->dbenv;
	mp = dbmp->reginfo[0].primary;
	alloc_mfp = NULL;
	mbytes = bytes = 0;
	refinc = ret = 0;
	rpath = NULL;

	/*
	 * We're keeping the page size as a size_t in the public API, but
	 * it's a u_int32_t everywhere internally.
	 */
	pagesize = (u_int32_t)pgsize;

	/*
	 * We're called internally with a specified mfp, in which case the
	 * path is NULL, but we'll get the path from the underlying region
	 * information.  Otherwise, if the path is NULL, it's a temporary
	 * file -- we know we can't join any existing files, and we'll delay
	 * the open until we actually need to write the file. All temporary
	 * files will go into the first hash bucket.
	 */
	DB_ASSERT(env, mfp == NULL || path == NULL);

	bucket = 0;
	hp = R_ADDR(dbmp->reginfo, mp->ftab);
	if (mfp == NULL) {
		if (path == NULL)
			goto alloc;

		/*
		 * Hash to the proper file table entry and walk it.
		 *
		 * The fileID is a filesystem unique number (e.g., a
		 * UNIX dev/inode pair) plus a timestamp.  If files are
		 * removed and created in less than a second, the fileID
		 * can be repeated.  The problem with repetition happens
		 * when the file that previously had the fileID value still
		 * has pages in the pool, since we don't want to use them
		 * to satisfy requests for the new file. Because the
		 * DB_TRUNCATE flag reuses the dev/inode pair, repeated
		 * opens with that flag set guarantees matching fileIDs
		 * when the machine can open a file and then re-open
		 * with truncate within a second.  For this reason, we
		 * pass that flag down, and, if we find a matching entry,
		 * we ensure that it's never found again, and we create
		 * a new entry for the current request.
		 */

		if (FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
			bucket = FNBUCKET(path, strlen(path));
		else
			bucket = FNBUCKET(dbmfp->fileid, DB_FILE_ID_LEN);
		hp += bucket;

		/*
		 * If we are passed a FILEID find the MPOOLFILE and inc
		 * its ref count.  That way it cannot go away while we
		 * open it.
		 */
		if (F_ISSET(dbmfp, MP_FILEID_SET)) {
			MUTEX_LOCK(env, hp->mtx_hash);
			ret =
			    __memp_mpf_find(env, dbmfp, hp, path, flags,&mfp);
			MUTEX_UNLOCK(env, hp->mtx_hash);
			if (ret != 0)
				goto err;
			if (mfp != NULL)
				refinc = 1;
		}
	} else {
		/*
		 * Deadfile can only be set if mpf_cnt goes to zero (or if we
		 * failed creating the file DB_AM_DISCARD).  Increment the ref
		 * count so the file cannot become dead and be unlinked.
		 */
		MUTEX_LOCK(env, mfp->mutex);
		if (!mfp->deadfile) {
			++mfp->mpf_cnt;
			refinc = 1;
		}
		MUTEX_UNLOCK(env, mfp->mutex);

		/*
		 * Test one last time to see if the file is dead -- it may have
		 * been removed.  This happens when a checkpoint trying to open
		 * the file to flush a buffer races with the Db::remove method.
		 * The error will be ignored, so don't output an error message.
		 */
		if (mfp->deadfile)
			return (EINVAL);
	}

	/*
	 * If there's no backing file, we can join existing files in the cache,
	 * but there's nothing to read from disk.
	 */
	if (!FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE)) {
		/* Convert MP open flags to DB OS-layer open flags. */
		oflags = 0;
		if (LF_ISSET(DB_CREATE))
			oflags |= DB_OSO_CREATE;
		if (LF_ISSET(DB_DIRECT))
			oflags |= DB_OSO_DIRECT;
		if (LF_ISSET(DB_RDONLY)) {
			F_SET(dbmfp, MP_READONLY);
			oflags |= DB_OSO_RDONLY;
		}

		/*
		 * XXX
		 * A grievous layering violation, the DB_DSYNC_DB flag
		 * was left in the ENV structure and not driven through
		 * the cache API.  This needs to be fixed when the general
		 * API configuration is fixed.
		 */
		if (F_ISSET(env->dbenv, DB_ENV_DSYNC_DB))
			oflags |= DB_OSO_DSYNC;

		/*
		 * Get the real name for this file and open it.
		 *
		 * Supply a page size so os_open can decide whether to
		 * turn buffering off if the DB_DIRECT_DB flag is set.
		 *
		 * Acquire the region lock if we're using a path from
		 * an underlying MPOOLFILE -- there's a race in accessing
		 * the path name stored in the region, __memp_nameop may
		 * be simultaneously renaming the file.
		 */
		if (mfp != NULL) {
			MPOOL_SYSTEM_LOCK(env);
			path = R_ADDR(dbmp->reginfo, mfp->path_off);
		}
		if ((ret = __db_appname(env,
		     DB_APP_DATA, path, dirp, &rpath)) == 0)
			ret = __os_open(env, rpath,
			     (u_int32_t)pagesize, oflags, mode, &dbmfp->fhp);
		if (mfp != NULL)
			MPOOL_SYSTEM_UNLOCK(env);
		if (ret != 0)
			goto err;

		/*
		 * Cache file handles are shared, and have mutexes to
		 * protect the underlying file handle across seek and
		 * read/write calls.
		 */
		dbmfp->fhp->ref = 1;
		if ((ret = __mutex_alloc(env, MTX_MPOOL_FH,
		     DB_MUTEX_PROCESS_ONLY, &dbmfp->fhp->mtx_fh)) != 0)
			goto err;

		/*
		 * Figure out the file's size.
		 *
		 * !!!
		 * We can't use off_t's here, or in any code in the mainline
		 * library for that matter.  (We have to use them in the
		 * os stubs, of course, as there are system calls that
		 * take them as arguments.)  The reason is some customers
		 * build in environments where an off_t is 32-bits, but
		 * still run where offsets are 64-bits, and they pay us
		 * a lot of money.
		 */
		if ((ret = __os_ioinfo(
		    env, rpath, dbmfp->fhp, &mbytes, &bytes, NULL)) != 0) {
			__db_err(env, ret, "%s", rpath);
			goto err;
		}

		/*
		 * Don't permit files that aren't a multiple of the pagesize,
		 * and find the number of the last page in the file, all the
		 * time being careful not to overflow 32 bits.
		 *
		 * During verify or recovery, we might have to cope with a
		 * truncated file; if the file size is not a multiple of the
		 * page size, round down to a page, we'll take care of the
		 * partial page outside the mpool system.
		 */
		DB_ASSERT(env, pagesize != 0);
		if (bytes % pagesize != 0) {
			if (LF_ISSET(DB_ODDFILESIZE))
				bytes -= (u_int32_t)(bytes % pagesize);
			else {
				__db_errx(env,
		    "%s: file size not a multiple of the pagesize", rpath);
				ret = EINVAL;
				goto err;
			}
		}

		/*
		 * Get the file id if we weren't given one.  Generated file id's
		 * don't use timestamps, otherwise there'd be no chance of any
		 * other process joining the party.  Don't bother looking for
		 * this id in the hash table, its new.
		 */
		if (mfp == NULL && !F_ISSET(dbmfp, MP_FILEID_SET)) {
			if  ((ret =
			     __os_fileid(env, rpath, 0, dbmfp->fileid)) != 0)
				goto err;
			F_SET(dbmfp, MP_FILEID_SET);
			goto alloc;
		}
	}

	if (mfp != NULL)
		goto have_mfp;

	/*
	 * We can race with another process opening the same file when
	 * we allocate the mpoolfile structure.  We will come back
	 * here and check the hash table again to see if it has appeared.
	 * For most files this is not a problem, since the name is locked
	 * at a higher layer but QUEUE extent files are not locked.
	 */
check:	MUTEX_LOCK(env, hp->mtx_hash);
	if ((ret = __memp_mpf_find(env, dbmfp, hp, path, flags, &mfp) != 0))
		goto err;

	if (alloc_mfp != NULL && mfp == NULL) {
		mfp = alloc_mfp;
		alloc_mfp = NULL;
		SH_TAILQ_INSERT_HEAD(&hp->hash_bucket, mfp, q, __mpoolfile);
	} else if (mfp != NULL) {
		/*
		 * Some things about a file cannot be changed: the clear length,
		 * page size, or LSN location.  However, if this is an attempt
		 * to open a named in-memory file, we may not yet have that
		 * information. so accept uninitialized entries.
		 *
		 * The file type can change if the application's pre- and post-
		 * processing needs change.  For example, an application that
		 * created a hash subdatabase in a database that was previously
		 * all btree.
		 *
		 * !!!
		 * We do not check to see if the pgcookie information changed,
		 * or update it if it is.
		 */
		if ((dbmfp->clear_len != DB_CLEARLEN_NOTSET &&
		    mfp->clear_len != DB_CLEARLEN_NOTSET &&
		    dbmfp->clear_len != mfp->clear_len) ||
		    (pagesize != 0 && pagesize != mfp->stat.st_pagesize) ||
		    (dbmfp->lsn_offset != DB_LSN_OFF_NOTSET &&
		    mfp->lsn_off != DB_LSN_OFF_NOTSET &&
		    dbmfp->lsn_offset != mfp->lsn_off)) {
			__db_errx(env,
		    "%s: clear length, page size or LSN location changed",
			    path);
			MUTEX_UNLOCK(env, hp->mtx_hash);
			ret = EINVAL;
			goto err;
		}
	}

	MUTEX_UNLOCK(env, hp->mtx_hash);
	if (alloc_mfp != NULL) {
		MUTEX_LOCK(env, alloc_mfp->mutex);
		if ((ret = __memp_mf_discard(dbmp, alloc_mfp)) != 0)
			goto err;
	}

	if (mfp == NULL) {
		/*
		 * If we didn't find the file and this is an in-memory file,
		 * then the create flag should be set.
		 */
		if (FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE) &&
		    !LF_ISSET(DB_CREATE)) {
			ret = ENOENT;
			goto err;
		}

alloc:		/*
		 * Get the file ID if we weren't given one.  Generated file
		 * ID's don't use timestamps, otherwise there'd be no
		 * chance of any other process joining the party.
		 */
		if (path != NULL &&
		     !FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE) &&
		     !F_ISSET(dbmfp, MP_FILEID_SET) && (ret =
			    __os_fileid(env, rpath, 0, dbmfp->fileid)) != 0)
				goto err;

		if ((ret = __memp_mpf_alloc(dbmp,
		     dbmfp, path, pagesize, flags, &alloc_mfp)) != 0)
			goto err;

		/*
		 * If the user specifies DB_MPOOL_LAST or DB_MPOOL_NEW on a
		 * page get, we have to increment the last page in the file.
		 * Figure it out and save it away.
		 *
		 * Note correction: page numbers are zero-based, not 1-based.
		 */
		DB_ASSERT(env, pagesize != 0);
		last_pgno = (db_pgno_t)(mbytes * (MEGABYTE / pagesize));
		last_pgno += (db_pgno_t)(bytes / pagesize);
		if (last_pgno != 0)
			--last_pgno;

		alloc_mfp->last_flushed_pgno = alloc_mfp->orig_last_pgno =
		    alloc_mfp->last_pgno = last_pgno;

		alloc_mfp->bucket = bucket;

		/* Go back and see if someone else has opened the file. */
		if (path != NULL)
			goto check;

		mfp = alloc_mfp;
		/* This is a temp, noone else can see it, put it at the end. */
		MUTEX_LOCK(env, hp->mtx_hash);
		SH_TAILQ_INSERT_TAIL(&hp->hash_bucket, mfp, q);
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
have_mfp:
	/*
	 * We need to verify that all handles open a file either durable or not
	 * durable.  This needs to be cross process and cross sub-databases, so
	 * mpool is the place to do it.
	 */
	if (!LF_ISSET(DB_DURABLE_UNKNOWN | DB_RDONLY)) {
		if (F_ISSET(mfp, MP_DURABLE_UNKNOWN)) {
			if (LF_ISSET(DB_TXN_NOT_DURABLE))
				F_SET(mfp, MP_NOT_DURABLE);
			F_CLR(mfp, MP_DURABLE_UNKNOWN);
		} else if (!LF_ISSET(DB_TXN_NOT_DURABLE) !=
		    !F_ISSET(mfp, MP_NOT_DURABLE)) {
			__db_errx(env,
	     "Cannot open DURABLE and NOT DURABLE handles in the same file");
			ret = EINVAL;
			goto err;
		}
	}

	if (LF_ISSET(DB_MULTIVERSION)) {
		++mfp->multiversion;
		F_SET(dbmfp, MP_MULTIVERSION);
	}

	/*
	 * All paths to here have initialized the mfp variable to reference
	 * the selected (or allocated) MPOOLFILE.
	 */
	dbmfp->mfp = mfp;

	/*
	 * Check to see if we can mmap the file.  If a file:
	 *	+ isn't temporary
	 *	+ is read-only
	 *	+ doesn't require any pgin/pgout support
	 *	+ the DB_NOMMAP flag wasn't set (in either the file open or
	 *	  the environment in which it was opened)
	 *	+ and is less than mp_mmapsize bytes in size
	 *
	 * we can mmap it instead of reading/writing buffers.  Don't do error
	 * checking based on the mmap call failure.  We want to do normal I/O
	 * on the file if the reason we failed was because the file was on an
	 * NFS mounted partition, and we can fail in buffer I/O just as easily
	 * as here.
	 *
	 * We'd like to test to see if the file is too big to mmap.  Since we
	 * don't know what size or type off_t's or size_t's are, or the largest
	 * unsigned integral type is, or what random insanity the local C
	 * compiler will perpetrate, doing the comparison in a portable way is
	 * flatly impossible.  Hope that mmap fails if the file is too large.
	 */
#define	DB_MAXMMAPSIZE	(10 * 1024 * 1024)	/* 10 MB. */
	if (F_ISSET(mfp, MP_CAN_MMAP)) {
		maxmap = dbenv->mp_mmapsize == 0 ?
		    DB_MAXMMAPSIZE : dbenv->mp_mmapsize;
		if (path == NULL ||
		    FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
			F_CLR(mfp, MP_CAN_MMAP);
		else if (!F_ISSET(dbmfp, MP_READONLY))
			F_CLR(mfp, MP_CAN_MMAP);
		else if (dbmfp->ftype != 0)
			F_CLR(mfp, MP_CAN_MMAP);
		else if (LF_ISSET(DB_NOMMAP) || F_ISSET(dbenv, DB_ENV_NOMMAP))
			F_CLR(mfp, MP_CAN_MMAP);
		else {
			MPOOL_SYSTEM_LOCK(env);
			maxmap = mp->mp_mmapsize == 0 ?
			    DB_MAXMMAPSIZE : mp->mp_mmapsize;
			MPOOL_SYSTEM_UNLOCK(env);
			if (mbytes > maxmap / MEGABYTE ||
			    (mbytes == maxmap / MEGABYTE &&
			    bytes >= maxmap % MEGABYTE))
				F_CLR(mfp, MP_CAN_MMAP);
		}

		dbmfp->addr = NULL;
		if (F_ISSET(mfp, MP_CAN_MMAP)) {
			dbmfp->len = (size_t)mbytes * MEGABYTE + bytes;
			if (__os_mapfile(env, rpath,
			    dbmfp->fhp, dbmfp->len, 1, &dbmfp->addr) != 0) {
				dbmfp->addr = NULL;
				F_CLR(mfp, MP_CAN_MMAP);
			}
		}
	}

	F_SET(dbmfp, MP_OPEN_CALLED);

	/*
	 * Share the underlying file descriptor if that's possible.
	 *
	 * Add the file to the process' list of DB_MPOOLFILEs.
	 */
	MUTEX_LOCK(env, dbmp->mutex);

	if (dbmfp->fhp != NULL)
		TAILQ_FOREACH(tmp_dbmfp, &dbmp->dbmfq, q)
			if (dbmfp->mfp == tmp_dbmfp->mfp &&
			    (F_ISSET(dbmfp, MP_READONLY) ||
			    !F_ISSET(tmp_dbmfp, MP_READONLY))) {
				(void)__mutex_free(env, &dbmfp->fhp->mtx_fh);
				(void)__os_closehandle(env, dbmfp->fhp);
				++tmp_dbmfp->fhp->ref;
				dbmfp->fhp = tmp_dbmfp->fhp;
				break;
			}

	TAILQ_INSERT_TAIL(&dbmp->dbmfq, dbmfp, q);

	MUTEX_UNLOCK(env, dbmp->mutex);

	if (0) {
err:		if (refinc) {
			/*
			 * If mpf_cnt goes to zero here and unlink_on_close is
			 * set, then we missed the last close, but there was an
			 * error trying to open the file, so we probably cannot
			 * unlink it anyway.
			 */
			MUTEX_LOCK(env, mfp->mutex);
			--mfp->mpf_cnt;
			MUTEX_UNLOCK(env, mfp->mutex);
		}

	}
	if (rpath != NULL)
		__os_free(env, rpath);
	return (ret);
}

/*
 * __memp_mpf_find --
 *	Search a hash bucket for a MPOOLFILE.
 */
static int
__memp_mpf_find(env, dbmfp, hp, path, flags, mfpp)
	ENV *env;
	DB_MPOOLFILE *dbmfp;
	DB_MPOOL_HASH *hp;
	const char *path;
	u_int32_t flags;
	MPOOLFILE **mfpp;
{
	DB_MPOOL *dbmp;
	MPOOLFILE *mfp;

	dbmp = env->mp_handle;

	SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
		/* Skip dead files and temporary files. */
		if (mfp->deadfile || F_ISSET(mfp, MP_TEMP))
			continue;

		/*
		 * Any remaining DB_MPOOL_NOFILE databases are in-memory
		 * named databases and need only match other in-memory
		 * databases with the same name.
		 */
		if (FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE)) {
			if (!mfp->no_backing_file)
				continue;

			if (strcmp(path, R_ADDR(dbmp->reginfo, mfp->path_off)))
				continue;

			/*
			 * We matched an in-memory file; grab the fileid if
			 * it is set in the region, but not in the dbmfp.
			 */
			if (!F_ISSET(dbmfp, MP_FILEID_SET))
				(void)__memp_set_fileid(dbmfp,
				    R_ADDR(dbmp->reginfo, mfp->fileid_off));
		} else
			if (memcmp(dbmfp->fileid, R_ADDR(dbmp->reginfo,
			    mfp->fileid_off), DB_FILE_ID_LEN) != 0)
				continue;

		/*
		 * If the file is being truncated, remove it from the system
		 * and create a new entry.
		 *
		 * !!!
		 * We should be able to set mfp to NULL and break out of the
		 * loop, but I like the idea of checking all the entries.
		 */
		if (LF_ISSET(DB_TRUNCATE)) {
			MUTEX_LOCK(env, mfp->mutex);
			mfp->deadfile = 1;
			MUTEX_UNLOCK(env, mfp->mutex);
			continue;
		}

		/*
		 * Check to see if this file has died while we waited.
		 *
		 * We normally don't lock the deadfile field when we read it as
		 * we only care if the field is zero or non-zero.  We do lock
		 * on read when searching for a matching MPOOLFILE so that two
		 * threads of control don't race between setting the deadfile
		 * bit and incrementing the reference count, that is, a thread
		 * of control decrementing the reference count and then setting
		 * deadfile because the reference count is 0 blocks us finding
		 * the file without knowing it's about to be marked dead.
		 */
		MUTEX_LOCK(env, mfp->mutex);
		if (mfp->deadfile) {
			MUTEX_UNLOCK(env, mfp->mutex);
			continue;
		}
		++mfp->mpf_cnt;
		MUTEX_UNLOCK(env, mfp->mutex);

		/* Initialize any fields that are not yet set. */
		if (dbmfp->ftype != 0)
			mfp->ftype = dbmfp->ftype;
		if (dbmfp->clear_len != DB_CLEARLEN_NOTSET)
			mfp->clear_len = dbmfp->clear_len;
		if (dbmfp->lsn_offset != -1)
			mfp->lsn_off = dbmfp->lsn_offset;

		break;
	}

	*mfpp = mfp;
	return (0);
}

static int
__memp_mpf_alloc(dbmp, dbmfp, path, pagesize, flags, retmfp)
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	const char *path;
	u_int32_t pagesize;
	u_int32_t flags;
	MPOOLFILE **retmfp;
{
	ENV *env;
	MPOOLFILE *mfp;
	int ret;
	void *p;

	env = dbmp->env;
	ret = 0;
	/* Allocate and initialize a new MPOOLFILE. */
	if ((ret = __memp_alloc(dbmp,
	     dbmp->reginfo, NULL, sizeof(MPOOLFILE), NULL, &mfp)) != 0)
		goto err;
	memset(mfp, 0, sizeof(MPOOLFILE));
	mfp->mpf_cnt = 1;
	mfp->ftype = dbmfp->ftype;
	mfp->stat.st_pagesize = pagesize;
	mfp->lsn_off = dbmfp->lsn_offset;
	mfp->clear_len = dbmfp->clear_len;
	mfp->priority = dbmfp->priority;
	if (dbmfp->gbytes != 0 || dbmfp->bytes != 0) {
		mfp->maxpgno = (db_pgno_t)
		    (dbmfp->gbytes * (GIGABYTE / mfp->stat.st_pagesize));
		mfp->maxpgno += (db_pgno_t)
		    ((dbmfp->bytes + mfp->stat.st_pagesize - 1) /
		    mfp->stat.st_pagesize);
	}
	if (FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
		mfp->no_backing_file = 1;
	if (FLD_ISSET(dbmfp->config_flags, DB_MPOOL_UNLINK))
		mfp->unlink_on_close = 1;

	if (LF_ISSET(DB_DURABLE_UNKNOWN | DB_RDONLY))
		F_SET(mfp, MP_DURABLE_UNKNOWN);
	if (LF_ISSET(DB_DIRECT))
		F_SET(mfp, MP_DIRECT);
	if (LF_ISSET(DB_EXTENT))
		F_SET(mfp, MP_EXTENT);
	if (LF_ISSET(DB_TXN_NOT_DURABLE))
		F_SET(mfp, MP_NOT_DURABLE);
	F_SET(mfp, MP_CAN_MMAP);

	/*
	 * An in-memory database with no name is a temp file.  Named
	 * in-memory databases get an artificially  bumped reference
	 * count so they don't disappear on close; they need a remove
	 * to make them disappear.
	 */
	if (path == NULL)
		F_SET(mfp, MP_TEMP);
	else if (FLD_ISSET(dbmfp->config_flags, DB_MPOOL_NOFILE))
		mfp->mpf_cnt++;

	/* Copy the file identification string into shared memory. */
	if (F_ISSET(dbmfp, MP_FILEID_SET)) {
		if ((ret = __memp_alloc(dbmp, dbmp->reginfo,
		    NULL, DB_FILE_ID_LEN, &mfp->fileid_off, &p)) != 0)
			goto err;
		memcpy(p, dbmfp->fileid, DB_FILE_ID_LEN);
	}

	/* Copy the file path into shared memory. */
	if (path != NULL) {
		if ((ret = __memp_alloc(dbmp, dbmp->reginfo,
		    NULL, strlen(path) + 1, &mfp->path_off, &p)) != 0)
			goto err;
		memcpy(p, path, strlen(path) + 1);
	}

	/* Copy the page cookie into shared memory. */
	if (dbmfp->pgcookie == NULL || dbmfp->pgcookie->size == 0) {
		mfp->pgcookie_len = 0;
		mfp->pgcookie_off = 0;
	} else {
		if ((ret = __memp_alloc(dbmp, dbmp->reginfo,
		    NULL, dbmfp->pgcookie->size,
		    &mfp->pgcookie_off, &p)) != 0)
			goto err;
		memcpy(p,
		     dbmfp->pgcookie->data, dbmfp->pgcookie->size);
		mfp->pgcookie_len = dbmfp->pgcookie->size;
	}

	if ((ret = __mutex_alloc(env,
	    MTX_MPOOLFILE_HANDLE, 0, &mfp->mutex)) != 0)
		goto err;
	*retmfp = mfp;

err:	return (ret);
}

/*
 * memp_fclose_pp --
 *	DB_MPOOLFILE->close pre/post processing.
 *
 * PUBLIC: int __memp_fclose_pp __P((DB_MPOOLFILE *, u_int32_t));
 */
int
__memp_fclose_pp(dbmfp, flags)
	DB_MPOOLFILE *dbmfp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbmfp->env;

	/*
	 * Validate arguments, but as a handle destructor, we can't fail.
	 */
	if (flags != 0)
		(void)__db_ferr(env, "DB_MPOOLFILE->close", 0);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_fclose(dbmfp, 0)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_fclose --
 *	DB_MPOOLFILE->close.
 *
 * PUBLIC: int __memp_fclose __P((DB_MPOOLFILE *, u_int32_t));
 */
int
__memp_fclose(dbmfp, flags)
	DB_MPOOLFILE *dbmfp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	ENV *env;
	MPOOLFILE *mfp;
	char *rpath;
	u_int32_t ref;
	int deleted, ret, t_ret;

	env = dbmfp->env;
	dbmp = env->mp_handle;
	ret = 0;

	/*
	 * Remove the DB_MPOOLFILE from the process' list.
	 *
	 * It's possible the underlying mpool cache may never have been created.
	 * In that case, all we have is a structure, discard it.
	 *
	 * It's possible the DB_MPOOLFILE was never added to the DB_MPOOLFILE
	 * file list, check the MP_OPEN_CALLED flag to be sure.
	 */
	if (dbmp == NULL)
		goto done;

	MUTEX_LOCK(env, dbmp->mutex);

	DB_ASSERT(env, dbmfp->ref >= 1);
	if ((ref = --dbmfp->ref) == 0 && F_ISSET(dbmfp, MP_OPEN_CALLED))
		TAILQ_REMOVE(&dbmp->dbmfq, dbmfp, q);

	/*
	 * Decrement the file descriptor's ref count -- if we're the last ref,
	 * we'll discard the file descriptor.
	 */
	if (ref == 0 && dbmfp->fhp != NULL && --dbmfp->fhp->ref > 0)
		dbmfp->fhp = NULL;
	MUTEX_UNLOCK(env, dbmp->mutex);
	if (ref != 0)
		return (0);

	/* Complain if pinned blocks never returned. */
	if (dbmfp->pinref != 0) {
		__db_errx(env, "%s: close: %lu blocks left pinned",
		    __memp_fn(dbmfp), (u_long)dbmfp->pinref);
		ret = __env_panic(env, DB_RUNRECOVERY);
	}

	/* Discard any mmap information. */
	if (dbmfp->addr != NULL &&
	    (ret = __os_unmapfile(env, dbmfp->addr, dbmfp->len)) != 0)
		__db_err(env, ret, "%s", __memp_fn(dbmfp));

	/*
	 * Close the file and discard the descriptor structure; temporary
	 * files may not yet have been created.
	 */
	if (dbmfp->fhp != NULL) {
		if ((t_ret =
		    __mutex_free(env, &dbmfp->fhp->mtx_fh)) != 0 && ret == 0)
			ret = t_ret;
		if ((t_ret = __os_closehandle(env, dbmfp->fhp)) != 0) {
			__db_err(env, t_ret, "%s", __memp_fn(dbmfp));
			if (ret == 0)
				ret = t_ret;
		}
		dbmfp->fhp = NULL;
	}

	/*
	 * Discard our reference on the underlying MPOOLFILE, and close it
	 * if it's no longer useful to anyone.  It possible the open of the
	 * file never happened or wasn't successful, in which case, mpf will
	 * be NULL and MP_OPEN_CALLED will not be set.
	 */
	mfp = dbmfp->mfp;
	DB_ASSERT(env,
	    (F_ISSET(dbmfp, MP_OPEN_CALLED) && mfp != NULL) ||
	    (!F_ISSET(dbmfp, MP_OPEN_CALLED) && mfp == NULL));
	if (!F_ISSET(dbmfp, MP_OPEN_CALLED))
		goto done;

	/*
	 * If it's a temp file, all outstanding references belong to unflushed
	 * buffers.  (A temp file can only be referenced by one DB_MPOOLFILE).
	 * We don't care about preserving any of those buffers, so mark the
	 * MPOOLFILE as dead so that even the dirty ones just get discarded
	 * when we try to flush them.
	 */
	deleted = 0;
	if (!LF_ISSET(DB_MPOOL_NOLOCK))
		MUTEX_LOCK(env, mfp->mutex);
	if (F_ISSET(dbmfp, MP_MULTIVERSION))
		--mfp->multiversion;
	if (--mfp->mpf_cnt == 0 || LF_ISSET(DB_MPOOL_DISCARD)) {
		if (LF_ISSET(DB_MPOOL_DISCARD) ||
		    F_ISSET(mfp, MP_TEMP) || mfp->unlink_on_close) {
			mfp->deadfile = 1;
		}
		if (mfp->unlink_on_close) {
			if ((t_ret = __db_appname(dbmp->env, DB_APP_DATA,
			    R_ADDR(dbmp->reginfo, mfp->path_off), NULL,
			    &rpath)) != 0 && ret == 0)
				ret = t_ret;
			if (t_ret == 0) {
				if ((t_ret = __os_unlink(
				    dbmp->env, rpath, 0)) != 0 && ret == 0)
					ret = t_ret;
				__os_free(env, rpath);
			}
		}
		if (mfp->mpf_cnt == 0) {
			F_CLR(mfp, MP_NOT_DURABLE);
			F_SET(mfp, MP_DURABLE_UNKNOWN);
		}
		if (mfp->block_cnt == 0) {
			/*
			 * We should never discard this mp file if our caller
			 * is holding the lock on it.  See comment in
			 * __memp_sync_file.
			 */
			DB_ASSERT(env, !LF_ISSET(DB_MPOOL_NOLOCK));
			if ((t_ret =
			    __memp_mf_discard(dbmp, mfp)) != 0 && ret == 0)
				ret = t_ret;
			deleted = 1;
		}
	}
	if (!deleted && !LF_ISSET(DB_MPOOL_NOLOCK))
		MUTEX_UNLOCK(env, mfp->mutex);

done:	/* Discard the DB_MPOOLFILE structure. */
	if (dbmfp->pgcookie != NULL) {
		__os_free(env, dbmfp->pgcookie->data);
		__os_free(env, dbmfp->pgcookie);
	}
	__os_free(env, dbmfp);

	return (ret);
}

/*
 * __memp_mf_discard --
 *	Discard an MPOOLFILE.
 *
 * PUBLIC: int __memp_mf_discard __P((DB_MPOOL *, MPOOLFILE *));
 */
int
__memp_mf_discard(dbmp, mfp)
	DB_MPOOL *dbmp;
	MPOOLFILE *mfp;
{
	DB_MPOOL_HASH *hp;
	ENV *env;
#ifdef HAVE_STATISTICS
	DB_MPOOL_STAT *sp;
#endif
	MPOOL *mp;
	int need_sync, ret, t_ret;

	env = dbmp->env;
	mp = dbmp->reginfo[0].primary;
	hp = R_ADDR(dbmp->reginfo, mp->ftab);
	hp += mfp->bucket;
	ret = 0;

	/*
	 * Expects caller to be holding the MPOOLFILE mutex.
	 *
	 * When discarding a file, we have to flush writes from it to disk.
	 * The scenario is that dirty buffers from this file need to be
	 * flushed to satisfy a future checkpoint, but when the checkpoint
	 * calls mpool sync, the sync code won't know anything about them.
	 * Ignore files not written, discarded, or only temporary.
	 */
	need_sync =
	   mfp->file_written && !mfp->deadfile && !F_ISSET(mfp, MP_TEMP);

	/*
	 * We have to release the MPOOLFILE mutex before acquiring the region
	 * mutex so we don't deadlock.  Make sure nobody ever looks at this
	 * structure again.
	 */
	mfp->deadfile = 1;

	/* Discard the mutex we're holding and return it too the pool. */
	MUTEX_UNLOCK(env, mfp->mutex);
	if ((t_ret = __mutex_free(env, &mfp->mutex)) != 0 && ret == 0)
		ret = t_ret;

	/* Lock the bucket and delete from the list of MPOOLFILEs. */
	MUTEX_LOCK(env, hp->mtx_hash);
	SH_TAILQ_REMOVE(&hp->hash_bucket, mfp, q, __mpoolfile);
	MUTEX_UNLOCK(env, hp->mtx_hash);

	/* Lock the region and collect stats and free the space. */
	MPOOL_SYSTEM_LOCK(env);
	if (need_sync &&
	    (t_ret = __memp_mf_sync(dbmp, mfp, 0)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_STATISTICS
	/* Copy the statistics into the region. */
	sp = &mp->stat;
	sp->st_cache_hit += mfp->stat.st_cache_hit;
	sp->st_cache_miss += mfp->stat.st_cache_miss;
	sp->st_map += mfp->stat.st_map;
	sp->st_page_create += mfp->stat.st_page_create;
	sp->st_page_in += mfp->stat.st_page_in;
	sp->st_page_out += mfp->stat.st_page_out;
#endif

	/* Free the space. */
	if (mfp->path_off != 0)
		__memp_free(&dbmp->reginfo[0],
		    R_ADDR(dbmp->reginfo, mfp->path_off));
	if (mfp->fileid_off != 0)
		__memp_free(&dbmp->reginfo[0],
		    R_ADDR(dbmp->reginfo, mfp->fileid_off));
	if (mfp->pgcookie_off != 0)
		__memp_free(&dbmp->reginfo[0],
		    R_ADDR(dbmp->reginfo, mfp->pgcookie_off));
	__memp_free(&dbmp->reginfo[0], mfp);

	MPOOL_SYSTEM_UNLOCK(env);

	return (ret);
}

/*
 * __memp_inmemlist --
 *	Return a list of the named in-memory databases.
 *
 * PUBLIC: int __memp_inmemlist __P((ENV *, char ***, int *));
 */
int
__memp_inmemlist(env, namesp, cntp)
	ENV *env;
	char ***namesp;
	int *cntp;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOL *mp;
	MPOOLFILE *mfp;
	int arraysz, cnt, i, ret;
	char **names;

	names = NULL;
	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;
	hp = R_ADDR(dbmp->reginfo, mp->ftab);

	arraysz = cnt = 0;
	for (i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(env, hp->mtx_hash);
		SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
			/* Skip dead files and temporary files. */
			if (mfp->deadfile || F_ISSET(mfp, MP_TEMP))
				continue;

			/* Skip entries that allow files. */
			if (!mfp->no_backing_file)
				continue;

			/* We found one. */
			if (cnt >= arraysz) {
				arraysz += 100;
				if ((ret = __os_realloc(env,
				    (u_int)arraysz * sizeof(names[0]),
				    &names)) != 0)
					goto nomem;
			}
			if ((ret = __os_strdup(env,
			    R_ADDR(dbmp->reginfo, mfp->path_off),
			    &names[cnt])) != 0)
				goto nomem;

			cnt++;
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}
	*namesp = names;
	*cntp = cnt;
	return (0);

nomem:	MUTEX_UNLOCK(env, hp->mtx_hash);
	if (names != NULL) {
		while (--cnt >= 0)
			__os_free(env, names[cnt]);
		__os_free(env, names);
	}

	/* Make sure we don't return any garbage. */
	*cntp = 0;
	*namesp = NULL;
	return (ret);
}
