/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_REGION_H_
#define _DB_REGION_H_

/*
 * The DB environment consists of some number of "regions", which are described
 * by the following four structures:
 *
 *  REGENV     -- shared information about the environment
 *  REGENV_REF -- file describing system memory version of REGENV
 *  REGION     -- shared information about a single region
 *  REGINFO    -- per-process information about a REGION
 *
 * There are three types of memory that hold regions:
 *  per-process heap (malloc)
 *  file mapped into memory (mmap, MapViewOfFile)
 *  system memory (shmget, CreateFileMapping)
 *
 * By default, regions are created in filesystem-backed shared memory.  They
 * can also be created in system shared memory (DB_SYSTEM_MEM), or, if private
 * to a process, in heap memory (DB_PRIVATE).
 *
 * Regions in the filesystem are named "__db.001", "__db.002" and so on.  If
 * we're not using a private environment allocated in heap, "__db.001" will
 * always exist, as we use it to synchronize on the regions, whether they are
 * in filesystem-backed memory or system memory.
 *
 * The file "__db.001" contains a REGENV structure and an array of REGION
 * structures.  Each REGION structures describes an underlying chunk of
 * shared memory.
 *
 *  __db.001
 *  +---------+
 *  |REGENV  |
 *  +---------+   +----------+
 *  |REGION   |-> | __db.002 |
 *  |     |   +----------+
 *  +---------+   +----------+
 *  |REGION   |-> | __db.003 |
 *  |     |   +----------+
 *  +---------+   +----------+
 *  |REGION   |-> | __db.004 |
 *  |     |   +----------+
 *  +---------+
 *
 * The tricky part about manipulating the regions is creating or joining the
 * database environment.  We have to be sure only a single thread of control
 * creates and/or recovers a database environment.  All other threads should
 * then join without seeing inconsistent data.
 *
 * We do this in two parts: first, we use the underlying O_EXCL flag to the
 * open system call to serialize creation of the __db.001 file.  The thread
 * of control creating that file then proceeds to create the remaining
 * regions in the environment, including the mutex region.  Once the mutex
 * region has been created, the creating thread of control fills in the
 * __db.001 file's magic number.  Other threads of control (the ones that
 * didn't create the __db.001 file), wait on the initialization of the
 * __db.001 file's magic number.  After it has been initialized, all threads
 * of control can proceed, using normal shared mutex locking procedures for
 * exclusion.
 *
 * REGIONs are not moved or removed during the life of the environment, and
 * so processes can have long-lived references to them.
 *
 * One of the REGION structures describes the environment region itself.
 *
 * The REGION array is not locked in any way.  It's an array so we don't have
 * to manipulate data structures after a crash -- on some systems, we have to
 * join and clean up the mutex region after application failure.  Using an
 * array means we don't have to worry about broken links or other nastiness
 * after the failure.
 *
 * All requests to create or join a region return a REGINFO structure, which
 * is held by the caller and used to open and subsequently close the reference
 * to the region.  The REGINFO structure contains the per-process information
 * that we need to access the region.
 *
 * The one remaining complication.  If the regions (including the environment
 * region) live in system memory, and the system memory isn't "named" somehow
 * in the filesystem name space, we need some way of finding it.  Do this by
 * by writing the REGENV_REF structure into the "__db.001" file.  When we find
 * a __db.001 file that is too small to be a real, on-disk environment, we use
 * the information it contains to redirect to the real "__db.001" file/memory.
 * This currently only happens when the REGENV file is in shared system memory.
 *
 * Although DB does not currently grow regions when they run out of memory, it
 * would be possible to do so.  To grow a region, allocate a new region of the
 * appropriate size, then copy the old region over it and insert the additional
 * memory into the already existing shalloc arena.  Region users must reset
 * their base addresses and any local pointers into the memory, of course.
 * This failed in historic versions of DB because the region mutexes lived in
 * the mapped memory, and when it was unmapped and remapped (or copied),
 * threads could lose track of it.  Also, some systems didn't support mutex
 * copying, e.g., from OSF1 V4.0:
 *
 *  The address of an msemaphore structure may be significant.  If the
 *  msemaphore structure contains any value copied from an msemaphore
 *  structure at a different address, the result is undefined.
 *
 * All mutexes are now maintained in a separate region which is never unmapped,
 * so growing regions should be possible.
 */

#if defined(__cplusplus)
extern "C" {
#endif

#define DB_REGION_PREFIX    "__db"      /* DB file name prefix. */
#define DB_REGION_FMT       "__db.%03d" /* Region file name format. */
#define DB_REGION_ENV       "__db.001"  /* Primary environment name. */

#define INVALID_REGION_ID   0   /* Out-of-band region ID. */
#define REGION_ID_ENV       1   /* Primary environment ID. */

typedef enum {
    INVALID_REGION_TYPE=0,      /* Region type. */
    REGION_TYPE_ENV,
    REGION_TYPE_LOCK,
    REGION_TYPE_LOG,
    REGION_TYPE_MPOOL,
    REGION_TYPE_MUTEX,
    REGION_TYPE_TXN } reg_type_t;

#define INVALID_REGION_SEGID    -1  /* Segment IDs are either shmget(2) or
                     * Win16 segment identifiers.  They are
                     * both stored in a "long", and we need
                     * an out-of-band value.
                     */
/*
 * Nothing can live at region offset 0, because, in all cases, that's where
 * we store *something*.  Lots of code needs an out-of-band value for region
 * offsets, so we use 0.
 */
#define INVALID_ROFF        0

/* Reference describing system memory version of REGENV. */
typedef struct __db_reg_env_ref {
    roff_t     size;        /* Region size. */
    long       segid;       /* UNIX shmget ID, VxWorks ID. */
} REGENV_REF;

/* Per-environment region information. */
typedef struct __db_reg_env {
    /*
     * !!!
     * The magic, panic, version, envid and signature fields of the region
     * are fixed in size, the timestamp field is the first field which is
     * variable length.  These fields must never change in order, to
     * guarantee we can always read them, no matter what release we have.
     *
     * !!!
     * The magic and panic fields are NOT protected by any mutex, and for
     * this reason cannot be anything more complicated than zero/non-zero.
     */
    u_int32_t magic;        /* Valid region magic number. */
    u_int32_t panic;        /* Environment is dead. */

    u_int32_t majver;       /* Major DB version number. */
    u_int32_t minver;       /* Minor DB version number. */
    u_int32_t patchver;     /* Patch DB version number. */

    u_int32_t envid;        /* Unique environment ID. */

    u_int32_t signature;        /* Structure signatures. */

    time_t    timestamp;        /* Creation time. */

    u_int32_t init_flags;       /* Flags environment initialized with.*/

    /*
     * The mtx_regenv mutex protects the environment reference count and
     * memory allocation from the primary shared region (the crypto, thread
     * control block and replication implementations allocate memory from
     * the primary shared region).
     *
     * The rest of the fields are initialized at creation time, and don't
     * need mutex protection.  The flags, op_timestamp and rep_timestamp
     * fields are used by replication only and are protected by the
     * replication mutex.  The rep_timestamp is is not protected when it
     * is used in recovery as that is already single threaded.
     */
    db_mutex_t mtx_regenv;      /* Refcnt, region allocation mutex. */
    u_int32_t  refcnt;      /* References to the environment. */

    u_int32_t region_cnt;       /* Number of REGIONs. */
    roff_t    region_off;       /* Offset of region array */

    roff_t    cipher_off;       /* Offset of cipher area */

    roff_t    thread_off;       /* Offset of the thread area. */

    roff_t    rep_off;      /* Offset of the replication area. */
#define DB_REGENV_REPLOCKED 0x0001  /* Env locked for rep backup. */
    u_int32_t flags;        /* Shared environment flags. */
#define DB_REGENV_TIMEOUT   30  /* Backup timeout. */
    time_t    op_timestamp;     /* Timestamp for operations. */
    time_t    rep_timestamp;    /* Timestamp for rep db handles. */
    u_int32_t reg_panic;        /* DB_REGISTER triggered panic */
    uintmax_t unused;       /* The ALLOC_LAYOUT structure follows
                     * the REGENV structure in memory and
                     * contains uintmax_t fields.  Force
                     * proper alignment of that structure.
                     */
} REGENV;

/* Per-region shared region information. */
typedef struct __db_region {
    u_int32_t   id;     /* Region id. */
    reg_type_t  type;       /* Region type. */

    roff_t  size;           /* Region size in bytes. */

    roff_t  primary;        /* Primary data structure offset. */

    long    segid;          /* UNIX shmget(2), Win16 segment ID. */
} REGION;

/*
 * Per-process/per-attachment information about a single region.
 */
struct __db_reginfo_t {     /* __env_region_attach IN parameters. */
    ENV    *env;        /* Enclosing environment. */
    reg_type_t  type;       /* Region type. */
    u_int32_t   id;         /* Region id. */

                /* env_region_attach OUT parameters. */
    REGION     *rp;         /* Shared region. */

    char       *name;       /* Region file name. */

    void       *addr;       /* Region address. */
    void       *primary;        /* Primary data structure address. */

    size_t      max_alloc;      /* Maximum bytes allocated. */
    size_t      allocated;      /* Bytes allocated. */

    db_mutex_t  mtx_alloc;      /* number of mutex for allocation. */

#ifdef DB_WIN32
    HANDLE  wnt_handle;     /* Win/NT HANDLE. */
#endif

#define REGION_CREATE       0x01    /* Caller created region. */
#define REGION_CREATE_OK    0x02    /* Caller willing to create region. */
#define REGION_JOIN_OK      0x04    /* Caller is looking for a match. */
    u_int32_t   flags;
};

/*
 * R_ADDR   Return a per-process address for a shared region offset.
 * R_OFFSET Return a shared region offset for a per-process address.
 */
#define R_ADDR(reginfop, offset)                    \
    (F_ISSET((reginfop)->env, ENV_PRIVATE) ?            \
        (void *)(offset) :                      \
        (void *)((u_int8_t *)((reginfop)->addr) + (offset)))
#define R_OFFSET(reginfop, p)                       \
    (F_ISSET((reginfop)->env, ENV_PRIVATE) ?            \
        (roff_t)(p) :                       \
        (roff_t)((u_int8_t *)(p) - (u_int8_t *)(reginfop)->addr))

/*
 * PANIC_ISSET, PANIC_CHECK:
 *  Check to see if the DB environment is dead.
 */
#define PANIC_ISSET(env)                        \
    ((env) != NULL && (env)->reginfo != NULL &&         \
        ((REGENV *)(env)->reginfo->primary)->panic != 0 &&      \
        !F_ISSET((env)->dbenv, DB_ENV_NOPANIC))

#define PANIC_CHECK(env)                        \
    if (PANIC_ISSET(env))                       \
        return (__env_panic_msg(env));

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_REGION_H_ */
