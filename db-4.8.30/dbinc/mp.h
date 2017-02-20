/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_MP_H_
#define _DB_MP_H_

#include "dbinc/atomic.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct __bh;        typedef struct __bh BH;
struct __bh_frozen_p;   typedef struct __bh_frozen_p BH_FROZEN_PAGE;
struct __bh_frozen_a;   typedef struct __bh_frozen_a BH_FROZEN_ALLOC;
struct __db_mpool_hash; typedef struct __db_mpool_hash DB_MPOOL_HASH;
struct __db_mpreg;  typedef struct __db_mpreg DB_MPREG;
struct __mpool;     typedef struct __mpool MPOOL;

                /* We require at least 20KB of cache. */
#define DB_CACHESIZE_MIN    (20 * 1024)

/*
 * DB_MPOOLFILE initialization methods cannot be called after open is called,
 * other methods cannot be called before open is called
 */
#define MPF_ILLEGAL_AFTER_OPEN(dbmfp, name)             \
    if (F_ISSET(dbmfp, MP_OPEN_CALLED))             \
        return (__db_mi_open((dbmfp)->env, name, 1));
#define MPF_ILLEGAL_BEFORE_OPEN(dbmfp, name)                \
    if (!F_ISSET(dbmfp, MP_OPEN_CALLED))                \
        return (__db_mi_open((dbmfp)->env, name, 0));

/*
 * Cache flush operations, plus modifiers.
 */
#define DB_SYNC_ALLOC       0x0001  /* Flush for allocation. */
#define DB_SYNC_CACHE       0x0002  /* Flush entire cache. */
#define DB_SYNC_CHECKPOINT  0x0004  /* Checkpoint. */
#define DB_SYNC_FILE        0x0008  /* Flush file. */
#define DB_SYNC_INTERRUPT_OK    0x0010  /* Allow interrupt and return OK. */
#define DB_SYNC_QUEUE_EXTENT    0x0020  /* Flush a queue file with extents. */
#define DB_SYNC_SUPPRESS_WRITE  0x0040  /* Ignore max-write configuration. */
#define DB_SYNC_TRICKLE     0x0080  /* Trickle sync. */

/*
 * DB_MPOOL --
 *  Per-process memory pool structure.
 */
struct __db_mpool {
    /* These fields need to be protected for multi-threaded support. */
    db_mutex_t mutex;       /* Thread mutex. */

    /*
     * DB_MPREG structure for the DB pgin/pgout routines.
     *
     * Linked list of application-specified pgin/pgout routines.
     */
    DB_MPREG *pg_inout;
    LIST_HEAD(__db_mpregh, __db_mpreg) dbregq;

                    /* List of DB_MPOOLFILE's. */
    TAILQ_HEAD(__db_mpoolfileh, __db_mpoolfile) dbmfq;

    /*
     * The env and reginfo fields are not thread protected, as they are
     * initialized during mpool creation, and not modified again.
     */
    ENV    *env;        /* Enclosing environment. */
    REGINFO    *reginfo;        /* Underlying cache regions. */
};

/*
 * DB_MPREG --
 *  DB_MPOOL registry of pgin/pgout functions.
 */
struct __db_mpreg {
    LIST_ENTRY(__db_mpreg) q;   /* Linked list. */

    int32_t ftype;          /* File type. */
                    /* Pgin, pgout routines. */
    int (*pgin) __P((DB_ENV *, db_pgno_t, void *, DBT *));
    int (*pgout) __P((DB_ENV *, db_pgno_t, void *, DBT *));
};

/*
 * File hashing --
 *  We hash each file to hash bucket based on its fileid
 *  or, in the case of in memory files, its name.
 */

/* Number of file hash buckets, a small prime number */
#define MPOOL_FILE_BUCKETS  17

#define FHASH(id, len)  __ham_func5(NULL, id, (u_int32_t)(len))

#define FNBUCKET(id, len)                       \
    (FHASH(id, len) % MPOOL_FILE_BUCKETS)

/* Macros to lock/unlock the mpool region as a whole. */
#define MPOOL_SYSTEM_LOCK(env)                      \
    MUTEX_LOCK(env, ((MPOOL *)                  \
        (env)->mp_handle->reginfo[0].primary)->mtx_region)
#define MPOOL_SYSTEM_UNLOCK(env)                    \
    MUTEX_UNLOCK(env, ((MPOOL *)                    \
        (env)->mp_handle->reginfo[0].primary)->mtx_region)

/* Macros to lock/unlock a specific mpool region. */
#define MPOOL_REGION_LOCK(env, infop)                   \
    MUTEX_LOCK(env, ((MPOOL *)(infop)->primary)->mtx_region)
#define MPOOL_REGION_UNLOCK(env, infop)                 \
    MUTEX_UNLOCK(env, ((MPOOL *)(infop)->primary)->mtx_region)

/*
 * MPOOL --
 *  Shared memory pool region.
 */
struct __mpool {
    /*
     * The memory pool can be broken up into individual pieces/files.
     * There are two reasons for this: firstly, on Solaris you can allocate
     * only a little more than 2GB of memory in a contiguous chunk,
     * and I expect to see more systems with similar issues.  Secondly,
     * applications can add / remove pieces to dynamically resize the
     * cache.
     *
     * While this structure is duplicated in each piece of the cache,
     * the first of these pieces/files describes the entire pool, the
     * second only describe a piece of the cache.
     */
    db_mutex_t  mtx_region; /* Region mutex. */
    db_mutex_t  mtx_resize; /* Resizing mutex. */

    /*
     * The lsn field and list of underlying MPOOLFILEs are thread protected
     * by the region lock.
     */
    DB_LSN    lsn;          /* Maximum checkpoint LSN. */

    /* Configuration information: protected by the region lock. */
    u_int32_t max_nreg;     /* Maximum number of regions. */
    size_t    mp_mmapsize;      /* Maximum file size for mmap. */
    int       mp_maxopenfd;     /* Maximum open file descriptors. */
    int       mp_maxwrite;      /* Maximum buffers to write. */
    db_timeout_t mp_maxwrite_sleep; /* Sleep after writing max buffers. */

    /*
     * The number of regions and the total number of hash buckets across
     * all regions.
     * These fields are not protected by a mutex because we assume that we
     * can read a 32-bit value atomically.  They are only modified by cache
     * resizing which holds the mpool resizing mutex to ensure that
     * resizing is single-threaded.  See the comment in mp_resize.c for
     * more information.
     */
    u_int32_t nreg;         /* Number of underlying REGIONS. */
    u_int32_t nbuckets;     /* Total number of hash buckets. */

    /*
     * The regid field is protected by the resize mutex.
     */
    roff_t    regids;       /* Array of underlying REGION Ids. */

    roff_t    ftab;         /* Hash table of files. */

    /*
     * The following fields describe the per-cache portion of the region.
     *
     * The htab and htab_buckets fields are not thread protected as they
     * are initialized during mpool creation, and not modified again.
     *
     * The last_checked and lru_count fields are thread protected by
     * the region lock.
     */
    roff_t    htab;         /* Hash table offset. */
    u_int32_t htab_buckets;     /* Number of hash table entries. */
    u_int32_t last_checked;     /* Last bucket checked for free. */
    u_int32_t lru_count;        /* Counter for buffer LRU. */
    int32_t   lru_reset;        /* Hash bucket lru reset point. */

    /*
     * The stat fields are generally not thread protected, and cannot be
     * trusted.  Note that st_pages is an exception, and is always updated
     * inside a region lock (although it is sometimes read outside of the
     * region lock).
     */
    DB_MPOOL_STAT stat;     /* Per-cache mpool statistics. */

    /*
     * We track page puts so that we can decide when allocation is never
     * going to succeed.  We don't lock the field, all we care about is
     * if it changes.
     */
    u_int32_t  put_counter;     /* Count of page put calls. */

    /*
     * Cache flush operations take a long time...
     *
     * Some cache flush operations want to ignore the app's configured
     * max-write parameters (they are trying to quickly shut down an
     * environment, for example).  We can't specify that as an argument
     * to the cache region functions, because we may decide to ignore
     * the max-write configuration after the cache operation has begun.
     * If the variable suppress_maxwrite is set, ignore the application
     * max-write config.
     *
     * We may want to interrupt cache flush operations in high-availability
     * configurations.
     */
#define DB_MEMP_SUPPRESS_WRITE  0x01
#define DB_MEMP_SYNC_INTERRUPT  0x02
    u_int32_t config_flags;

    /* Free frozen buffer headers, protected by the region lock. */
    SH_TAILQ_HEAD(__free_frozen) free_frozen;

    /* Allocated blocks of frozen buffer headers. */
    SH_TAILQ_HEAD(__alloc_frozen) alloc_frozen;
};

/*
 * NREGION --
 *  Select a cache region given the bucket number.
 */
#define NREGION(mp, bucket)                     \
    ((bucket) / (mp)->htab_buckets)

/*
 * MP_HASH --
 *   We make the assumption that early pages of the file are more likely
 *   to be retrieved than the later pages, which means the top bits will
 *   be more interesting for hashing as they're less likely to collide.
 *   That said, as 512 8K pages represents a 4MB file, so only reasonably
 *   large files will have page numbers with any other than the bottom 9
 *   bits set.  We XOR in the MPOOL offset of the MPOOLFILE that backs the
 *   page, since that should also be unique for the page.  We don't want
 *   to do anything very fancy -- speed is more important to us than using
 *   good hashing.
 *
 *   Since moving to a dynamic hash, which boils down to using some of the
 *   least significant bits of the hash value, we no longer want to use a
 *   simple shift here, because it's likely with a bit shift that mf_offset
 *   will be ignored, and pages from different files end up in the same
 *   hash bucket.  Use a nearby prime instead.
 */
#define MP_HASH(mf_offset, pgno)                    \
    ((((pgno) << 8) ^ (pgno)) ^ (((u_int32_t) mf_offset) * 509))

/*
 * Inline the calculation of the mask, since we can't reliably store the mask
 * with the number of buckets in the region.
 *
 * This is equivalent to:
 *     mask = (1 << __db_log2(nbuckets)) - 1;
 */
#define MP_MASK(nbuckets, mask) do {                    \
    for (mask = 1; mask < (nbuckets); mask = (mask << 1) | 1)   \
        ;                           \
} while (0)

#define MP_HASH_BUCKET(hash, nbuckets, mask, bucket) do {       \
    (bucket) = (hash) & (mask);                 \
    if ((bucket) >= (nbuckets))                 \
        (bucket) &= ((mask) >> 1);              \
} while (0)

#define MP_BUCKET(mf_offset, pgno, nbuckets, bucket) do {       \
    u_int32_t __mask;                       \
    MP_MASK(nbuckets, __mask);                  \
    MP_HASH_BUCKET(MP_HASH(mf_offset, pgno), nbuckets,      \
        __mask, bucket);                        \
} while (0)

/*
 * MP_GET_REGION --
 *  Select the region for a given page.
 */
#define MP_GET_REGION(dbmfp, pgno, infopp, ret) do {            \
    DB_MPOOL *__t_dbmp;                     \
    MPOOL *__t_mp;                          \
                                    \
    __t_dbmp = dbmfp->env->mp_handle;               \
    __t_mp = __t_dbmp->reginfo[0].primary;              \
    if (__t_mp->max_nreg == 1) {                    \
        *(infopp) = &__t_dbmp->reginfo[0];          \
    } else                              \
        ret = __memp_get_bucket((dbmfp)->env,           \
            (dbmfp)->mfp, (pgno), (infopp), NULL, NULL);    \
} while (0)

/*
 * MP_GET_BUCKET --
 *  Select and lock the bucket for a given page.
 */
#define MP_GET_BUCKET(env, mfp, pgno, infopp, hp, bucket, ret) do { \
    DB_MPOOL *__t_dbmp;                     \
    MPOOL *__t_mp;                          \
    roff_t __t_mf_offset;                       \
                                    \
    __t_dbmp = (env)->mp_handle;                    \
    __t_mp = __t_dbmp->reginfo[0].primary;              \
    if (__t_mp->max_nreg == 1) {                    \
        *(infopp) = &__t_dbmp->reginfo[0];          \
        __t_mf_offset = R_OFFSET(*(infopp), (mfp));     \
        MP_BUCKET(__t_mf_offset,                \
            (pgno), __t_mp->nbuckets, bucket);      \
        (hp) = R_ADDR(*(infopp), __t_mp->htab);         \
        (hp) = &(hp)[bucket];               \
        MUTEX_READLOCK(env, (hp)->mtx_hash);            \
        ret = 0;                        \
    } else                              \
        ret = __memp_get_bucket((env),              \
            (mfp), (pgno), (infopp), &(hp), &(bucket));     \
} while (0)

struct __db_mpool_hash {
    db_mutex_t  mtx_hash;   /* Per-bucket mutex. */

    DB_HASHTAB  hash_bucket;    /* Head of bucket. */

    db_atomic_t hash_page_dirty;/* Count of dirty pages. */

#ifndef __TEST_DB_NO_STATISTICS
    u_int32_t   hash_io_wait;   /* Count of I/O waits. */
    u_int32_t   hash_frozen;    /* Count of frozen buffers. */
    u_int32_t   hash_thawed;    /* Count of thawed buffers. */
    u_int32_t   hash_frozen_freed;/* Count of freed frozen buffers. */
#endif

    DB_LSN      old_reader; /* Oldest snapshot reader (cached). */

    u_int32_t   flags;
};

/*
 * The base mpool priority is 1/4th of the name space, or just under 2^30.
 * When the LRU counter wraps, we shift everybody down to a base-relative
 * value.
 */
#define MPOOL_BASE_DECREMENT    (UINT32_MAX - (UINT32_MAX / 4))

/*
 * Mpool priorities from low to high.  Defined in terms of fractions of the
 * buffers in the pool.
 */
#define MPOOL_PRI_VERY_LOW  -1  /* Dead duck.  Check and set to 0. */
#define MPOOL_PRI_LOW       -2  /* Low. */
#define MPOOL_PRI_DEFAULT   0   /* No adjustment -- special case.*/
#define MPOOL_PRI_HIGH      10  /* With the dirty buffers. */
#define MPOOL_PRI_DIRTY     10  /* Dirty gets a 10% boost. */
#define MPOOL_PRI_VERY_HIGH 1   /* Add number of buffers in pool. */

/*
 * MPOOLFILE --
 *  Shared DB_MPOOLFILE information.
 */
struct __mpoolfile {
    db_mutex_t mutex;       /* MPOOLFILE mutex. */

    /* Protected by MPOOLFILE mutex. */
    u_int32_t mpf_cnt;      /* Ref count: DB_MPOOLFILEs. */
    u_int32_t block_cnt;        /* Ref count: blocks in cache. */
    db_pgno_t last_pgno;        /* Last page in the file. */
    db_pgno_t last_flushed_pgno;    /* Last page flushed to disk. */
    db_pgno_t orig_last_pgno;   /* Original last page in the file. */
    db_pgno_t maxpgno;      /* Maximum page number. */

    roff_t    path_off;     /* File name location. */

    /* Protected by hash bucket mutex. */
    SH_TAILQ_ENTRY q;       /* List of MPOOLFILEs */

    /*
     * The following are used for file compaction processing.
     * They are only used when a thread is in the process
     * of trying to move free pages to the end of the file.
     * Other threads may look here when freeing a page.
     * Protected by a lock on the metapage.
     */
    u_int32_t free_ref;     /* Refcount to freelist. */
    u_int32_t free_cnt;     /* Count of free pages. */
    size_t    free_size;        /* Allocated size of free list. */
    roff_t    free_list;        /* Offset to free list. */

    /*
     * We normally don't lock the deadfile field when we read it since we
     * only care if the field is zero or non-zero.  We do lock on read when
     * searching for a matching MPOOLFILE -- see that code for more detail.
     */
    int32_t   deadfile;     /* Dirty pages can be discarded. */

    u_int32_t bucket;       /* hash bucket for this file. */

    /*
     * None of the following fields are thread protected.
     *
     * There are potential races with the ftype field because it's read
     * without holding a lock.  However, it has to be set before adding
     * any buffers to the cache that depend on it being set, so there
     * would need to be incorrect operation ordering to have a problem.
     */
    int32_t   ftype;        /* File type. */

    /*
     * There are potential races with the priority field because it's read
     * without holding a lock.  However, a collision is unlikely and if it
     * happens is of little consequence.
     */
    int32_t   priority;     /* Priority when unpinning buffer. */

    /*
     * There are potential races with the file_written field (many threads
     * may be writing blocks at the same time), and with no_backing_file
     * and unlink_on_close fields, as they may be set while other threads
     * are reading them.  However, we only care if the field value is zero
     * or non-zero, so don't lock the memory.
     *
     * !!!
     * Theoretically, a 64-bit architecture could put two of these fields
     * in a single memory operation and we could race.  I have never seen
     * an architecture where that's a problem, and I believe Java requires
     * that to never be the case.
     *
     * File_written is set whenever a buffer is marked dirty in the cache.
     * It can be cleared in some cases, after all dirty buffers have been
     * written AND the file has been flushed to disk.
     */
    int32_t   file_written;     /* File was written. */
    int32_t   no_backing_file;  /* Never open a backing file. */
    int32_t   unlink_on_close;  /* Unlink file on last close. */
    int32_t   multiversion;     /* Number of DB_MULTIVERSION handles. */

    /*
     * We do not protect the statistics in "stat" because of the cost of
     * the mutex in the get/put routines.  There is a chance that a count
     * will get lost.
     */
    DB_MPOOL_FSTAT stat;        /* Per-file mpool statistics. */

    /*
     * The remaining fields are initialized at open and never subsequently
     * modified.
     */
    int32_t   lsn_off;      /* Page's LSN offset. */
    u_int32_t clear_len;        /* Bytes to clear on page create. */

    roff_t    fileid_off;       /* File ID string location. */

    roff_t    pgcookie_len;     /* Pgin/pgout cookie length. */
    roff_t    pgcookie_off;     /* Pgin/pgout cookie location. */

    /*
     * The flags are initialized at open and never subsequently modified.
     */
#define MP_CAN_MMAP     0x001   /* If the file can be mmap'd. */
#define MP_DIRECT       0x002   /* No OS buffering. */
#define MP_DURABLE_UNKNOWN  0x004   /* We don't care about durability. */
#define MP_EXTENT       0x008   /* Extent file. */
#define MP_FAKE_DEADFILE    0x010   /* Deadfile field: fake flag. */
#define MP_FAKE_FILEWRITTEN 0x020   /* File_written field: fake flag. */
#define MP_FAKE_NB      0x040   /* No_backing_file field: fake flag. */
#define MP_FAKE_UOC     0x080   /* Unlink_on_close field: fake flag. */
#define MP_NOT_DURABLE      0x100   /* File is not durable. */
#define MP_TEMP         0x200   /* Backing file is a temporary. */
    u_int32_t  flags;
};

/*
 * Flags to __memp_bh_free.
 */
#define BH_FREE_FREEMEM     0x01
#define BH_FREE_REUSE       0x02
#define BH_FREE_UNLOCKED    0x04

/*
 * BH --
 *  Buffer header.
 */
struct __bh {
    db_mutex_t  mtx_buf;    /* Shared/Exclusive mutex */
    db_atomic_t ref;        /* Reference count. */
#define BH_REFCOUNT(bhp)    atomic_read(&(bhp)->ref)

#define BH_CALLPGIN 0x001       /* Convert the page before use. */
#define BH_DIRTY    0x002       /* Page is modified. */
#define BH_DIRTY_CREATE 0x004       /* Page is modified. */
#define BH_DISCARD  0x008       /* Page is useless. */
#define BH_EXCLUSIVE    0x010       /* Exclusive access acquired. */
#define BH_FREED    0x020       /* Page was freed. */
#define BH_FROZEN   0x040       /* Frozen buffer: allocate & re-read. */
#define BH_TRASH    0x080       /* Page is garbage. */
#define BH_THAWED   0x100       /* Page was thawed. */
    u_int16_t   flags;

    u_int32_t   priority;   /* Priority. */
    SH_TAILQ_ENTRY  hq;     /* MPOOL hash bucket queue. */

    db_pgno_t   pgno;       /* Underlying MPOOLFILE page number. */
    roff_t      mf_offset;  /* Associated MPOOLFILE offset. */
    u_int32_t   bucket;     /* Hash bucket containing header. */
    int     region;     /* Region containing header. */

    roff_t      td_off;     /* MVCC: creating TXN_DETAIL offset. */
    SH_CHAIN_ENTRY  vc;     /* MVCC: version chain. */
#ifdef DIAG_MVCC
    u_int16_t   align_off;  /* Alignment offset for diagnostics.*/
#endif

    /*
     * !!!
     * This array must be at least size_t aligned -- the DB access methods
     * put PAGE and other structures into it, and then access them directly.
     * (We guarantee size_t alignment to applications in the documentation,
     * too.)
     */
    u_int8_t   buf[1];      /* Variable length data. */
};

/*
 * BH_FROZEN_PAGE --
 *  Data used to find a frozen buffer header.
 */
struct __bh_frozen_p {
    BH header;
    db_pgno_t   spgno;      /* Page number in freezer file. */
};

/*
 * BH_FROZEN_ALLOC --
 *  Frozen buffer headers are allocated a page at a time in general.  This
 *  structure is allocated at the beginning of the page so that the
 *  allocation chunks can be tracked and freed (for private environments).
 */
struct __bh_frozen_a {
    SH_TAILQ_ENTRY links;
};

#define MULTIVERSION(dbp)   ((dbp)->mpf->mfp->multiversion)
#define IS_DIRTY(p)                         \
    (F_ISSET((BH *)((u_int8_t *)                    \
    (p) - SSZA(BH, buf)), BH_DIRTY|BH_EXCLUSIVE) == (BH_DIRTY|BH_EXCLUSIVE))

#define IS_VERSION(dbp, p)                      \
    (!F_ISSET(dbp->mpf->mfp, MP_CAN_MMAP) &&                \
    SH_CHAIN_HASPREV((BH *)((u_int8_t *)(p) - SSZA(BH, buf)), vc))

#define BH_OWNER(env, bhp)                      \
    ((TXN_DETAIL *)R_ADDR(&env->tx_handle->reginfo, bhp->td_off))

#define BH_OWNED_BY(env, bhp, txn)  ((txn) != NULL &&       \
    (bhp)->td_off != INVALID_ROFF &&                    \
    (txn)->td == BH_OWNER(env, bhp))

#define VISIBLE_LSN(env, bhp)                       \
    (&BH_OWNER(env, bhp)->visible_lsn)

/*
 * Make a copy of the buffer's visible LSN, one field at a time.  We rely on the
 * 32-bit operations being atomic.  The visible_lsn starts at MAX_LSN and is
 * set during commit or abort to the current LSN.
 *
 * If we race with a commit / abort, we may see either the file or the offset
 * still at UINT32_MAX, so vlsn is guaranteed to be in the future.  That's OK,
 * since we had to take the log region lock to allocate the read LSN so we were
 * never going to see this buffer anyway.
 */
#define BH_VISIBLE(env, bhp, read_lsnp, vlsn)               \
    (bhp->td_off == INVALID_ROFF ||                 \
    ((vlsn).file = VISIBLE_LSN(env, bhp)->file,         \
    (vlsn).offset = VISIBLE_LSN(env, bhp)->offset,          \
    LOG_COMPARE((read_lsnp), &(vlsn)) >= 0))

#define BH_OBSOLETE(bhp, old_lsn, vlsn) (SH_CHAIN_HASNEXT(bhp, vc) ?    \
    BH_VISIBLE(env, SH_CHAIN_NEXTP(bhp, vc, __bh), &(old_lsn), vlsn) :\
    BH_VISIBLE(env, bhp, &(old_lsn), vlsn))

#define MVCC_SKIP_CURADJ(dbc, pgno) (dbc->txn != NULL &&        \
    F_ISSET(dbc->txn, TXN_SNAPSHOT) && MULTIVERSION(dbc->dbp) &&    \
    dbc->txn->td != NULL && __memp_skip_curadj(dbc, pgno))

#if defined(DIAG_MVCC) && defined(HAVE_MPROTECT)
#define VM_PAGESIZE 4096
#define MVCC_BHSIZE(mfp, sz) do {                   \
    sz += VM_PAGESIZE + sizeof(BH);                 \
    if (mfp->stat.st_pagesize < VM_PAGESIZE)            \
        sz += VM_PAGESIZE - mfp->stat.st_pagesize;      \
} while (0)

#define MVCC_BHALIGN(p) do {                        \
    BH *__bhp;                          \
    void *__orig = (p);                     \
    p = ALIGNP_INC(p, VM_PAGESIZE);                 \
    if ((u_int8_t *)p < (u_int8_t *)__orig + sizeof(BH))        \
        p = (u_int8_t *)p + VM_PAGESIZE;            \
    __bhp = (BH *)((u_int8_t *)p - SSZA(BH, buf));          \
    DB_ASSERT(env,                          \
        ((uintptr_t)__bhp->buf & (VM_PAGESIZE - 1)) == 0);      \
    DB_ASSERT(env,                          \
        (u_int8_t *)__bhp >= (u_int8_t *)__orig);           \
    DB_ASSERT(env, (u_int8_t *)p + mfp->stat.st_pagesize <      \
        (u_int8_t *)__orig + len);                  \
    __bhp->align_off =                      \
        (u_int16_t)((u_int8_t *)__bhp - (u_int8_t *)__orig);    \
    p = __bhp;                          \
} while (0)

#define MVCC_BHUNALIGN(bhp) do {                    \
    (bhp) = (BH *)((u_int8_t *)(bhp) - (bhp)->align_off);       \
} while (0)

#ifdef linux
#define MVCC_MPROTECT(buf, sz, mode) do {               \
    int __ret = mprotect((buf), (sz), (mode));          \
    DB_ASSERT(env, __ret == 0);                 \
} while (0)
#else
#define MVCC_MPROTECT(buf, sz, mode) do {               \
    if (!F_ISSET(env, ENV_PRIVATE | ENV_SYSTEM_MEM)) {      \
        int __ret = mprotect((buf), (sz), (mode));      \
        DB_ASSERT(env, __ret == 0);             \
    }                               \
} while (0)
#endif /* linux */

#else /* defined(DIAG_MVCC) && defined(HAVE_MPROTECT) */
#define MVCC_BHSIZE(mfp, sz) do {} while (0)
#define MVCC_BHALIGN(p) do {} while (0)
#define MVCC_BHUNALIGN(bhp) do {} while (0)
#define MVCC_MPROTECT(buf, size, mode) do {} while (0)
#endif

/*
 * Flags to __memp_ftruncate.
 */
#define MP_TRUNC_RECOVER    0x01

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/mp_ext.h"
#endif /* !_DB_MP_H_ */
