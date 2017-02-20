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
#include "dbinc/db_am.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

#ifdef HAVE_STATISTICS
static void __memp_print_bh __P((ENV *,
		DB_MPOOL *, const char *, BH *, roff_t *));
static int  __memp_print_all __P((ENV *, u_int32_t));
static int  __memp_print_stats __P((ENV *, u_int32_t));
static int __memp_print_hash __P((ENV *,
		DB_MPOOL *, REGINFO *, roff_t *, u_int32_t));
static int  __memp_stat __P((ENV *,
		DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
static void __memp_stat_wait
		__P((ENV *, REGINFO *, MPOOL *, DB_MPOOL_STAT *, u_int32_t));
static int __memp_file_stats __P((ENV *,
		MPOOLFILE *, void *, u_int32_t *, u_int32_t));
static int __memp_count_files __P((ENV *,
		MPOOLFILE *, void *, u_int32_t *, u_int32_t));
static int __memp_get_files __P((ENV *,
		MPOOLFILE *, void *, u_int32_t *, u_int32_t));
static int __memp_print_files __P((ENV *,
		MPOOLFILE *, void *, u_int32_t *, u_int32_t));

/*
 * __memp_stat_pp --
 *	DB_ENV->memp_stat pre/post processing.
 *
 * PUBLIC: int __memp_stat_pp
 * PUBLIC:     __P((DB_ENV *, DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
 */
int
__memp_stat_pp(dbenv, gspp, fspp, flags)
	DB_ENV *dbenv;
	DB_MPOOL_STAT **gspp;
	DB_MPOOL_FSTAT ***fspp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->mp_handle, "DB_ENV->memp_stat", DB_INIT_MPOOL);

	if ((ret = __db_fchk(env,
	    "DB_ENV->memp_stat", flags, DB_STAT_CLEAR)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_stat(env, gspp, fspp, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

/*
 * __memp_stat --
 *	ENV->memp_stat
 */
static int
__memp_stat(env, gspp, fspp, flags)
	ENV *env;
	DB_MPOOL_STAT **gspp;
	DB_MPOOL_FSTAT ***fspp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_FSTAT **tfsp;
	DB_MPOOL_STAT *sp;
	MPOOL *c_mp, *mp;
	size_t len;
	int ret;
	u_int32_t i, st_bytes, st_gbytes, st_hash_buckets, st_pages;
	uintmax_t tmp_wait, tmp_nowait;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;

	/* Global statistics. */
	if (gspp != NULL) {
		*gspp = NULL;

		if ((ret = __os_umalloc(env, sizeof(**gspp), gspp)) != 0)
			return (ret);
		memset(*gspp, 0, sizeof(**gspp));
		sp = *gspp;

		/*
		 * Initialization and information that is not maintained on
		 * a per-cache basis.  Note that configuration information
		 * may be modified at any time, and so we have to lock.
		 */
		sp->st_gbytes = mp->stat.st_gbytes;
		sp->st_bytes = mp->stat.st_bytes;
		sp->st_pagesize = mp->stat.st_pagesize;
		sp->st_ncache = mp->nreg;
		sp->st_max_ncache = mp->max_nreg;
		sp->st_regsize = dbmp->reginfo[0].rp->size;
		sp->st_sync_interrupted = mp->stat.st_sync_interrupted;

		MPOOL_SYSTEM_LOCK(env);
		sp->st_mmapsize = mp->mp_mmapsize;
		sp->st_maxopenfd = mp->mp_maxopenfd;
		sp->st_maxwrite = mp->mp_maxwrite;
		sp->st_maxwrite_sleep = mp->mp_maxwrite_sleep;
		MPOOL_SYSTEM_UNLOCK(env);

		/* Walk the cache list and accumulate the global information. */
		for (i = 0; i < mp->nreg; ++i) {
			c_mp = dbmp->reginfo[i].primary;

			sp->st_map += c_mp->stat.st_map;
			sp->st_cache_hit += c_mp->stat.st_cache_hit;
			sp->st_cache_miss += c_mp->stat.st_cache_miss;
			sp->st_page_create += c_mp->stat.st_page_create;
			sp->st_page_in += c_mp->stat.st_page_in;
			sp->st_page_out += c_mp->stat.st_page_out;
			sp->st_ro_evict += c_mp->stat.st_ro_evict;
			sp->st_rw_evict += c_mp->stat.st_rw_evict;
			sp->st_page_trickle += c_mp->stat.st_page_trickle;
			sp->st_pages += c_mp->stat.st_pages;
			/*
			 * st_page_dirty	calculated by __memp_stat_hash
			 * st_page_clean	calculated here
			 */
			__memp_stat_hash(
			    &dbmp->reginfo[i], c_mp, &sp->st_page_dirty);
			sp->st_page_clean = sp->st_pages - sp->st_page_dirty;
			sp->st_hash_buckets += c_mp->stat.st_hash_buckets;
			sp->st_hash_searches += c_mp->stat.st_hash_searches;
			sp->st_hash_longest += c_mp->stat.st_hash_longest;
			sp->st_hash_examined += c_mp->stat.st_hash_examined;
			/*
			 * st_hash_nowait	calculated by __memp_stat_wait
			 * st_hash_wait
			 */
			__memp_stat_wait(
			    env, &dbmp->reginfo[i], c_mp, sp, flags);
			__mutex_set_wait_info(env,
			    c_mp->mtx_region, &tmp_wait, &tmp_nowait);
			sp->st_region_nowait += tmp_nowait;
			sp->st_region_wait += tmp_wait;
			sp->st_alloc += c_mp->stat.st_alloc;
			sp->st_alloc_buckets += c_mp->stat.st_alloc_buckets;
			if (sp->st_alloc_max_buckets <
			    c_mp->stat.st_alloc_max_buckets)
				sp->st_alloc_max_buckets =
				    c_mp->stat.st_alloc_max_buckets;
			sp->st_alloc_pages += c_mp->stat.st_alloc_pages;
			if (sp->st_alloc_max_pages <
			    c_mp->stat.st_alloc_max_pages)
				sp->st_alloc_max_pages =
				    c_mp->stat.st_alloc_max_pages;

			if (LF_ISSET(DB_STAT_CLEAR)) {
				if (!LF_ISSET(DB_STAT_SUBSYSTEM))
					__mutex_clear(env, c_mp->mtx_region);

				MPOOL_SYSTEM_LOCK(env);
				st_bytes = c_mp->stat.st_bytes;
				st_gbytes = c_mp->stat.st_gbytes;
				st_hash_buckets = c_mp->stat.st_hash_buckets;
				st_pages = c_mp->stat.st_pages;
				memset(&c_mp->stat, 0, sizeof(c_mp->stat));
				c_mp->stat.st_bytes = st_bytes;
				c_mp->stat.st_gbytes = st_gbytes;
				c_mp->stat.st_hash_buckets = st_hash_buckets;
				c_mp->stat.st_pages = st_pages;
				MPOOL_SYSTEM_UNLOCK(env);
			}
		}

		/*
		 * We have duplicate statistics fields in per-file structures
		 * and the cache.  The counters are only incremented in the
		 * per-file structures, except if a file is flushed from the
		 * mpool, at which time we copy its information into the cache
		 * statistics.  We added the cache information above, now we
		 * add the per-file information.
		 */
		if ((ret = __memp_walk_files(env, mp, __memp_file_stats,
		    sp, NULL, fspp == NULL ? LF_ISSET(DB_STAT_CLEAR) : 0)) != 0)
			return (ret);
	}

	/* Per-file statistics. */
	if (fspp != NULL) {
		*fspp = NULL;

		/* Count the MPOOLFILE structures. */
		i = 0;
		len = 0;
		if ((ret = __memp_walk_files(env,
		     mp, __memp_count_files, &len, &i, flags)) != 0)
			return (ret);

		if (i == 0)
			return (0);
		len += sizeof(DB_MPOOL_FSTAT *);	/* Trailing NULL */

		/* Allocate space */
		if ((ret = __os_umalloc(env, len, fspp)) != 0)
			return (ret);

		tfsp = *fspp;
		*tfsp = NULL;

		/*
		 * Files may have been opened since we counted, don't walk
		 * off the end of the allocated space.
		 */
		if ((ret = __memp_walk_files(env,
		    mp, __memp_get_files, &tfsp, &i, flags)) != 0)
			return (ret);

		*++tfsp = NULL;
	}

	return (0);
}

static int
__memp_file_stats(env, mfp, argp, countp, flags)
	ENV *env;
	MPOOLFILE *mfp;
	void *argp;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL_STAT *sp;
	u_int32_t pagesize;

	COMPQUIET(env, NULL);
	COMPQUIET(countp, NULL);

	sp = argp;

	sp->st_map += mfp->stat.st_map;
	sp->st_cache_hit += mfp->stat.st_cache_hit;
	sp->st_cache_miss += mfp->stat.st_cache_miss;
	sp->st_page_create += mfp->stat.st_page_create;
	sp->st_page_in += mfp->stat.st_page_in;
	sp->st_page_out += mfp->stat.st_page_out;
	if (LF_ISSET(DB_STAT_CLEAR)) {
		pagesize = mfp->stat.st_pagesize;
		memset(&mfp->stat, 0, sizeof(mfp->stat));
		mfp->stat.st_pagesize = pagesize;
	}
	return (0);
}

static int
__memp_count_files(env, mfp, argp, countp, flags)
	ENV *env;
	MPOOLFILE *mfp;
	void *argp;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	size_t len;

	COMPQUIET(flags, 0);
	dbmp = env->mp_handle;
	len = *(size_t *)argp;

	(*countp)++;
	len += sizeof(DB_MPOOL_FSTAT *) +
	    sizeof(DB_MPOOL_FSTAT) + strlen(__memp_fns(dbmp, mfp)) + 1;

	*(size_t *)argp = len;
	return (0);
}

/*
 * __memp_get_files --
 *	get file specific statistics
 *
 * Build each individual entry.  We assume that an array of pointers are
 * aligned correctly to be followed by an array of structures, which should
 * be safe (in this particular case, the first element of the structure
 * is a pointer, so we're doubly safe).  The array is followed by space
 * for the text file names.
 */
static int
__memp_get_files(env, mfp, argp, countp, flags)
	ENV *env;
	MPOOLFILE *mfp;
	void *argp;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_FSTAT **tfsp, *tstruct;
	char *name, *tname;
	size_t nlen;
	u_int32_t pagesize;

	if (*countp == 0)
		return (0);

	dbmp = env->mp_handle;
	tfsp = *(DB_MPOOL_FSTAT ***)argp;

	if (*tfsp == NULL) {
		/* Add 1 to count because we need to skip over the NULL. */
		tstruct = (DB_MPOOL_FSTAT *)(tfsp + *countp + 1);
		tname = (char *)(tstruct + *countp);
		*tfsp = tstruct;
	} else {
		tstruct = *tfsp + 1;
		tname = (*tfsp)->file_name + strlen((*tfsp)->file_name) + 1;
		*++tfsp = tstruct;
	}

	name = __memp_fns(dbmp, mfp);
	nlen = strlen(name) + 1;
	memcpy(tname, name, nlen);
	*tstruct = mfp->stat;
	tstruct->file_name = tname;

	*(DB_MPOOL_FSTAT ***)argp = tfsp;
	(*countp)--;

	if (LF_ISSET(DB_STAT_CLEAR)) {
		pagesize = mfp->stat.st_pagesize;
		memset(&mfp->stat, 0, sizeof(mfp->stat));
		mfp->stat.st_pagesize = pagesize;
	}
	return (0);
}

/*
 * __memp_stat_print_pp --
 *	ENV->memp_stat_print pre/post processing.
 *
 * PUBLIC: int __memp_stat_print_pp __P((DB_ENV *, u_int32_t));
 */
int
__memp_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	ENV *env;
	int ret;

	env = dbenv->env;

	ENV_REQUIRES_CONFIG(env,
	    env->mp_handle, "DB_ENV->memp_stat_print", DB_INIT_MPOOL);

#define	DB_STAT_MEMP_FLAGS						\
	(DB_STAT_ALL | DB_STAT_CLEAR | DB_STAT_MEMP_HASH)
	if ((ret = __db_fchk(env,
	    "DB_ENV->memp_stat_print", flags, DB_STAT_MEMP_FLAGS)) != 0)
		return (ret);

	ENV_ENTER(env, ip);
	REPLICATION_WRAP(env, (__memp_stat_print(env, flags)), 0, ret);
	ENV_LEAVE(env, ip);
	return (ret);
}

#define	FMAP_ENTRIES	200			/* Files we map. */

/*
 * __memp_stat_print --
 *	ENV->memp_stat_print method.
 *
 * PUBLIC: int  __memp_stat_print __P((ENV *, u_int32_t));
 */
int
__memp_stat_print(env, flags)
	ENV *env;
	u_int32_t flags;
{
	u_int32_t orig_flags;
	int ret;

	orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR | DB_STAT_SUBSYSTEM);
	if (flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __memp_print_stats(env,
		    LF_ISSET(DB_STAT_ALL) ? flags : orig_flags);
		if (flags == 0 || ret != 0)
			return (ret);
	}

	if (LF_ISSET(DB_STAT_ALL | DB_STAT_MEMP_HASH) &&
	    (ret = __memp_print_all(env, orig_flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __memp_print_stats --
 *	Display default mpool region statistics.
 */
static int
__memp_print_stats(env, flags)
	ENV *env;
	u_int32_t flags;
{
	DB_MPOOL_FSTAT **fsp, **tfsp;
	DB_MPOOL_STAT *gsp;
	int ret;

	if ((ret = __memp_stat(env, &gsp, &fsp, flags)) != 0)
		return (ret);

	if (LF_ISSET(DB_STAT_ALL))
		__db_msg(env, "Default cache region information:");
	__db_dlbytes(env, "Total cache size",
	    (u_long)gsp->st_gbytes, (u_long)0, (u_long)gsp->st_bytes);
	__db_dl(env, "Number of caches", (u_long)gsp->st_ncache);
	__db_dl(env, "Maximum number of caches", (u_long)gsp->st_max_ncache);
	__db_dlbytes(env, "Pool individual cache size",
	    (u_long)0, (u_long)0, (u_long)gsp->st_regsize);
	__db_dlbytes(env, "Maximum memory-mapped file size",
	    (u_long)0, (u_long)0, (u_long)gsp->st_mmapsize);
	STAT_LONG("Maximum open file descriptors", gsp->st_maxopenfd);
	STAT_LONG("Maximum sequential buffer writes", gsp->st_maxwrite);
	STAT_LONG("Sleep after writing maximum sequential buffers",
	    gsp->st_maxwrite_sleep);
	__db_dl(env,
	    "Requested pages mapped into the process' address space",
	    (u_long)gsp->st_map);
	__db_dl_pct(env, "Requested pages found in the cache",
	    (u_long)gsp->st_cache_hit, DB_PCT(
	    gsp->st_cache_hit, gsp->st_cache_hit + gsp->st_cache_miss), NULL);
	__db_dl(env, "Requested pages not found in the cache",
	    (u_long)gsp->st_cache_miss);
	__db_dl(env,
	    "Pages created in the cache", (u_long)gsp->st_page_create);
	__db_dl(env, "Pages read into the cache", (u_long)gsp->st_page_in);
	__db_dl(env, "Pages written from the cache to the backing file",
	    (u_long)gsp->st_page_out);
	__db_dl(env, "Clean pages forced from the cache",
	    (u_long)gsp->st_ro_evict);
	__db_dl(env, "Dirty pages forced from the cache",
	    (u_long)gsp->st_rw_evict);
	__db_dl(env, "Dirty pages written by trickle-sync thread",
	    (u_long)gsp->st_page_trickle);
	__db_dl(env, "Current total page count",
	    (u_long)gsp->st_pages);
	__db_dl(env, "Current clean page count",
	    (u_long)gsp->st_page_clean);
	__db_dl(env, "Current dirty page count",
	    (u_long)gsp->st_page_dirty);
	__db_dl(env, "Number of hash buckets used for page location",
	    (u_long)gsp->st_hash_buckets);
	__db_dl(env, "Assumed page size used",
	    (u_long)gsp->st_pagesize);
	__db_dl(env,
	    "Total number of times hash chains searched for a page",
	    (u_long)gsp->st_hash_searches);
	__db_dl(env, "The longest hash chain searched for a page",
	    (u_long)gsp->st_hash_longest);
	__db_dl(env,
	    "Total number of hash chain entries checked for page",
	    (u_long)gsp->st_hash_examined);
	__db_dl_pct(env,
	    "The number of hash bucket locks that required waiting",
	    (u_long)gsp->st_hash_wait, DB_PCT(
	    gsp->st_hash_wait, gsp->st_hash_wait + gsp->st_hash_nowait), NULL);
	__db_dl_pct(env,
    "The maximum number of times any hash bucket lock was waited for",
	    (u_long)gsp->st_hash_max_wait, DB_PCT(gsp->st_hash_max_wait,
	    gsp->st_hash_max_wait + gsp->st_hash_max_nowait), NULL);
	__db_dl_pct(env,
	    "The number of region locks that required waiting",
	    (u_long)gsp->st_region_wait, DB_PCT(gsp->st_region_wait,
	    gsp->st_region_wait + gsp->st_region_nowait), NULL);
	__db_dl(env, "The number of buffers frozen",
	    (u_long)gsp->st_mvcc_frozen);
	__db_dl(env, "The number of buffers thawed",
	    (u_long)gsp->st_mvcc_thawed);
	__db_dl(env, "The number of frozen buffers freed",
	    (u_long)gsp->st_mvcc_freed);
	__db_dl(env, "The number of page allocations", (u_long)gsp->st_alloc);
	__db_dl(env,
	    "The number of hash buckets examined during allocations",
	    (u_long)gsp->st_alloc_buckets);
	__db_dl(env,
	    "The maximum number of hash buckets examined for an allocation",
	    (u_long)gsp->st_alloc_max_buckets);
	__db_dl(env, "The number of pages examined during allocations",
	    (u_long)gsp->st_alloc_pages);
	__db_dl(env, "The max number of pages examined for an allocation",
	    (u_long)gsp->st_alloc_max_pages);
	__db_dl(env, "Threads waited on page I/O", (u_long)gsp->st_io_wait);
	__db_dl(env, "The number of times a sync is interrupted",
	    (u_long)gsp->st_sync_interrupted);

	for (tfsp = fsp; fsp != NULL && *tfsp != NULL; ++tfsp) {
		if (LF_ISSET(DB_STAT_ALL))
			__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Pool File: %s", (*tfsp)->file_name);
		__db_dl(env, "Page size", (u_long)(*tfsp)->st_pagesize);
		__db_dl(env,
		    "Requested pages mapped into the process' address space",
		    (u_long)(*tfsp)->st_map);
		__db_dl_pct(env, "Requested pages found in the cache",
		    (u_long)(*tfsp)->st_cache_hit, DB_PCT((*tfsp)->st_cache_hit,
		    (*tfsp)->st_cache_hit + (*tfsp)->st_cache_miss), NULL);
		__db_dl(env, "Requested pages not found in the cache",
		    (u_long)(*tfsp)->st_cache_miss);
		__db_dl(env, "Pages created in the cache",
		    (u_long)(*tfsp)->st_page_create);
		__db_dl(env, "Pages read into the cache",
		    (u_long)(*tfsp)->st_page_in);
		__db_dl(env,
		    "Pages written from the cache to the backing file",
		    (u_long)(*tfsp)->st_page_out);
	}

	__os_ufree(env, fsp);
	__os_ufree(env, gsp);
	return (0);
}

/*
 * __memp_print_all --
 *	Display debugging mpool region statistics.
 */
static int
__memp_print_all(env, flags)
	ENV *env;
	u_int32_t flags;
{
	static const FN cfn[] = {
		{ DB_MPOOL_NOFILE,	"DB_MPOOL_NOFILE" },
		{ DB_MPOOL_UNLINK,	"DB_MPOOL_UNLINK" },
		{ 0,			NULL }
	};
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	MPOOL *mp;
	roff_t fmap[FMAP_ENTRIES + 1];
	u_int32_t i, cnt;
	int ret;

	dbmp = env->mp_handle;
	mp = dbmp->reginfo[0].primary;
	ret = 0;

	MPOOL_SYSTEM_LOCK(env);

	__db_print_reginfo(env, dbmp->reginfo, "Mpool", flags);
	__db_msg(env, "%s", DB_GLOBAL(db_line));

	__db_msg(env, "MPOOL structure:");
	__mutex_print_debug_single(
	    env, "MPOOL region mutex", mp->mtx_region, flags);
	STAT_LSN("Maximum checkpoint LSN", &mp->lsn);
	STAT_ULONG("Hash table entries", mp->htab_buckets);
	STAT_ULONG("Hash table last-checked", mp->last_checked);
	STAT_ULONG("Hash table LRU count", mp->lru_count);
	STAT_ULONG("Put counter", mp->put_counter);

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_MPOOL handle information:");
	__mutex_print_debug_single(
	    env, "DB_MPOOL handle mutex", dbmp->mutex, flags);
	STAT_ULONG("Underlying cache regions", mp->nreg);

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "DB_MPOOLFILE structures:");
	for (cnt = 0, dbmfp = TAILQ_FIRST(&dbmp->dbmfq);
	    dbmfp != NULL; dbmfp = TAILQ_NEXT(dbmfp, q), ++cnt) {
		__db_msg(env, "File #%lu: %s: per-process, %s",
		    (u_long)cnt + 1, __memp_fn(dbmfp),
		    F_ISSET(dbmfp, MP_READONLY) ? "readonly" : "read/write");
		STAT_ULONG("Reference count", dbmfp->ref);
		STAT_ULONG("Pinned block reference count", dbmfp->ref);
		STAT_ULONG("Clear length", dbmfp->clear_len);
		__db_print_fileid(env, dbmfp->fileid, "\tID");
		STAT_ULONG("File type", dbmfp->ftype);
		STAT_ULONG("LSN offset", dbmfp->lsn_offset);
		STAT_ULONG("Max gbytes", dbmfp->gbytes);
		STAT_ULONG("Max bytes", dbmfp->bytes);
		STAT_ULONG("Cache priority", dbmfp->priority);
		STAT_POINTER("mmap address", dbmfp->addr);
		STAT_ULONG("mmap length", dbmfp->len);
		__db_prflags(env, NULL, dbmfp->flags, cfn, NULL, "\tFlags");
		__db_print_fh(env, "File handle", dbmfp->fhp, flags);
	}

	__db_msg(env, "%s", DB_GLOBAL(db_line));
	__db_msg(env, "MPOOLFILE structures:");
	cnt = 0;
	ret = __memp_walk_files(env, mp, __memp_print_files, fmap, &cnt, flags);
	MPOOL_SYSTEM_UNLOCK(env);
	if (ret != 0)
		return (ret);

	if (cnt < FMAP_ENTRIES)
		fmap[cnt] = INVALID_ROFF;
	else
		fmap[FMAP_ENTRIES] = INVALID_ROFF;

	/* Dump the individual caches. */
	for (i = 0; i < mp->nreg; ++i) {
		__db_msg(env, "%s", DB_GLOBAL(db_line));
		__db_msg(env, "Cache #%d:", i + 1);
		if (i > 0)
			__env_alloc_print(&dbmp->reginfo[i], flags);
		if ((ret = __memp_print_hash(
		    env, dbmp, &dbmp->reginfo[i], fmap, flags)) != 0)
			break;
	}

	return (ret);
}

static int
__memp_print_files(env, mfp, argp, countp, flags)
	ENV *env;
	MPOOLFILE *mfp;
	void *argp;
	u_int32_t *countp;
	u_int32_t flags;
{
	roff_t *fmap;
	DB_MPOOL *dbmp;
	u_int32_t mfp_flags;
	static const FN fn[] = {
		{ MP_CAN_MMAP,		"MP_CAN_MMAP" },
		{ MP_DIRECT,		"MP_DIRECT" },
		{ MP_EXTENT,		"MP_EXTENT" },
		{ MP_FAKE_DEADFILE,	"deadfile" },
		{ MP_FAKE_FILEWRITTEN,	"file written" },
		{ MP_FAKE_NB,		"no backing file" },
		{ MP_FAKE_UOC,		"unlink on close" },
		{ MP_NOT_DURABLE,	"not durable" },
		{ MP_TEMP,		"MP_TEMP" },
		{ 0,			NULL }
	};

	dbmp = env->mp_handle;
	fmap = argp;

	__db_msg(env, "File #%d: %s", *countp + 1, __memp_fns(dbmp, mfp));
	__mutex_print_debug_single(env, "Mutex", mfp->mutex, flags);

	MUTEX_LOCK(env, mfp->mutex);
	STAT_ULONG("Reference count", mfp->mpf_cnt);
	STAT_ULONG("Block count", mfp->block_cnt);
	STAT_ULONG("Last page number", mfp->last_pgno);
	STAT_ULONG("Original last page number", mfp->orig_last_pgno);
	STAT_ULONG("Maximum page number", mfp->maxpgno);
	STAT_LONG("Type", mfp->ftype);
	STAT_LONG("Priority", mfp->priority);
	STAT_LONG("Page's LSN offset", mfp->lsn_off);
	STAT_LONG("Page's clear length", mfp->clear_len);

	__db_print_fileid(env,
	    R_ADDR(dbmp->reginfo, mfp->fileid_off), "\tID");

	mfp_flags = 0;
	if (mfp->deadfile)
		FLD_SET(mfp_flags, MP_FAKE_DEADFILE);
	if (mfp->file_written)
		FLD_SET(mfp_flags, MP_FAKE_FILEWRITTEN);
	if (mfp->no_backing_file)
		FLD_SET(mfp_flags, MP_FAKE_NB);
	if (mfp->unlink_on_close)
		FLD_SET(mfp_flags, MP_FAKE_UOC);
	__db_prflags(env, NULL, mfp_flags, fn, NULL, "\tFlags");

	if (*countp < FMAP_ENTRIES)
		fmap[*countp] = R_OFFSET(dbmp->reginfo, mfp);
	(*countp)++;
	MUTEX_UNLOCK(env, mfp->mutex);
	return (0);
}

/*
 * __memp_print_hash --
 *	Display hash bucket statistics for a cache.
 */
static int
__memp_print_hash(env, dbmp, reginfo, fmap, flags)
	ENV *env;
	DB_MPOOL *dbmp;
	REGINFO *reginfo;
	roff_t *fmap;
	u_int32_t flags;
{
	BH *bhp, *vbhp;
	DB_MPOOL_HASH *hp;
	DB_MSGBUF mb;
	MPOOL *c_mp;
	u_int32_t bucket;

	c_mp = reginfo->primary;
	DB_MSGBUF_INIT(&mb);

	/* Display the hash table list of BH's. */
	__db_msg(env,
	    "BH hash table (%lu hash slots)", (u_long)c_mp->htab_buckets);
	__db_msg(env, "bucket #: priority, I/O wait, [mutex]");
	__db_msg(env, "\tpageno, file, ref, LSN, address, priority, flags");

	for (hp = R_ADDR(reginfo, c_mp->htab),
	    bucket = 0; bucket < c_mp->htab_buckets; ++hp, ++bucket) {
		MUTEX_READLOCK(env, hp->mtx_hash);
		if ((bhp = SH_TAILQ_FIRST(&hp->hash_bucket, __bh)) != NULL) {
			__db_msgadd(env, &mb,
			    "bucket %lu: %lu (%lu dirty)",
			    (u_long)bucket, (u_long)hp->hash_io_wait,
			    (u_long)atomic_read(&hp->hash_page_dirty));
			if (hp->hash_frozen != 0)
				__db_msgadd(env, &mb, "(MVCC %lu/%lu/%lu) ",
				    (u_long)hp->hash_frozen,
				    (u_long)hp->hash_thawed,
				    (u_long)hp->hash_frozen_freed);
			__mutex_print_debug_stats(
			    env, &mb, hp->mtx_hash, flags);
			DB_MSGBUF_FLUSH(env, &mb);
		}
		for (; bhp != NULL; bhp = SH_TAILQ_NEXT(bhp, hq, __bh)) {
			__memp_print_bh(env, dbmp, NULL, bhp, fmap);

			/* Print the version chain, if it exists. */
			for (vbhp = SH_CHAIN_PREV(bhp, vc, __bh);
			    vbhp != NULL;
			    vbhp = SH_CHAIN_PREV(vbhp, vc, __bh)) {
				__memp_print_bh(env, dbmp,
				    " next:\t", vbhp, fmap);
			}
		}
		MUTEX_UNLOCK(env, hp->mtx_hash);
	}

	return (0);
}

/*
 * __memp_print_bh --
 *	Display a BH structure.
 */
static void
__memp_print_bh(env, dbmp, prefix, bhp, fmap)
	ENV *env;
	DB_MPOOL *dbmp;
	const char *prefix;
	BH *bhp;
	roff_t *fmap;
{
	static const FN fn[] = {
		{ BH_CALLPGIN,		"callpgin" },
		{ BH_DIRTY,		"dirty" },
		{ BH_DIRTY_CREATE,	"created" },
		{ BH_DISCARD,		"discard" },
		{ BH_EXCLUSIVE,		"exclusive" },
		{ BH_FREED,		"freed" },
		{ BH_FROZEN,		"frozen" },
		{ BH_TRASH,		"trash" },
		{ BH_THAWED,		"thawed" },
		{ 0,			NULL }
	};
	DB_MSGBUF mb;
	int i;

	DB_MSGBUF_INIT(&mb);

	if (prefix != NULL)
		__db_msgadd(env, &mb, "%s", prefix);
	else
		__db_msgadd(env, &mb, "\t");

	for (i = 0; i < FMAP_ENTRIES; ++i)
		if (fmap[i] == INVALID_ROFF || fmap[i] == bhp->mf_offset)
			break;

	if (fmap[i] == INVALID_ROFF)
		__db_msgadd(env, &mb, "%5lu, %lu, ",
		    (u_long)bhp->pgno, (u_long)bhp->mf_offset);
	else
		__db_msgadd(
		    env, &mb, "%5lu, #%d, ", (u_long)bhp->pgno, i + 1);

	__db_msgadd(env, &mb, "%2lu, %lu/%lu", (u_long)atomic_read(&bhp->ref),
	    F_ISSET(bhp, BH_FROZEN) ? 0 : (u_long)LSN(bhp->buf).file,
	    F_ISSET(bhp, BH_FROZEN) ? 0 : (u_long)LSN(bhp->buf).offset);
	if (bhp->td_off != INVALID_ROFF)
		__db_msgadd(env, &mb, " (@%lu/%lu)",
		    (u_long)VISIBLE_LSN(env, bhp)->file,
		    (u_long)VISIBLE_LSN(env, bhp)->offset);
	__db_msgadd(env, &mb, ", %#08lx, %lu",
	    (u_long)R_OFFSET(dbmp->reginfo, bhp), (u_long)bhp->priority);
	__db_prflags(env, &mb, bhp->flags, fn, " (", ")");
	DB_MSGBUF_FLUSH(env, &mb);
}

/*
 * __memp_stat_wait --
 *	Total hash bucket wait stats into the region.
 */
static void
__memp_stat_wait(env, reginfo, mp, mstat, flags)
	ENV *env;
	REGINFO *reginfo;
	MPOOL *mp;
	DB_MPOOL_STAT *mstat;
	u_int32_t flags;
{
	DB_MPOOL_HASH *hp;
	u_int32_t i;
	uintmax_t tmp_nowait, tmp_wait;

	mstat->st_hash_max_wait = 0;
	hp = R_ADDR(reginfo, mp->htab);
	for (i = 0; i < mp->htab_buckets; i++, hp++) {
		__mutex_set_wait_info(
		    env, hp->mtx_hash, &tmp_wait, &tmp_nowait);
		mstat->st_hash_nowait += tmp_nowait;
		mstat->st_hash_wait += tmp_wait;
		if (tmp_wait > mstat->st_hash_max_wait) {
			mstat->st_hash_max_wait = tmp_wait;
			mstat->st_hash_max_nowait = tmp_nowait;
		}
		if (LF_ISSET(DB_STAT_CLEAR |
		    DB_STAT_SUBSYSTEM) == DB_STAT_CLEAR)
			__mutex_clear(env, hp->mtx_hash);

		mstat->st_io_wait += hp->hash_io_wait;
		mstat->st_mvcc_frozen += hp->hash_frozen;
		mstat->st_mvcc_thawed += hp->hash_thawed;
		mstat->st_mvcc_freed += hp->hash_frozen_freed;
		if (LF_ISSET(DB_STAT_CLEAR)) {
			hp->hash_io_wait = 0;
			hp->hash_frozen = 0;
			hp->hash_thawed = 0;
			hp->hash_frozen_freed = 0;
		}
	}
}

#else /* !HAVE_STATISTICS */

int
__memp_stat_pp(dbenv, gspp, fspp, flags)
	DB_ENV *dbenv;
	DB_MPOOL_STAT **gspp;
	DB_MPOOL_FSTAT ***fspp;
	u_int32_t flags;
{
	COMPQUIET(gspp, NULL);
	COMPQUIET(fspp, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}

int
__memp_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv->env));
}
#endif

/*
 * __memp_stat_hash --
 *	Total hash bucket stats (other than mutex wait) into the region.
 *
 * PUBLIC: void __memp_stat_hash __P((REGINFO *, MPOOL *, u_int32_t *));
 */
void
__memp_stat_hash(reginfo, mp, dirtyp)
	REGINFO *reginfo;
	MPOOL *mp;
	u_int32_t *dirtyp;
{
	DB_MPOOL_HASH *hp;
	u_int32_t dirty, i;

	hp = R_ADDR(reginfo, mp->htab);
	for (i = 0, dirty = 0; i < mp->htab_buckets; i++, hp++)
		dirty += (u_int32_t)atomic_read(&hp->hash_page_dirty);
	*dirtyp = dirty;
}
