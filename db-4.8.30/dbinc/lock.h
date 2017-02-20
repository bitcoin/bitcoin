/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_LOCK_H_
#define _DB_LOCK_H_

#if defined(__cplusplus)
extern "C" {
#endif

#define DB_LOCK_DEFAULT_N   1000    /* Default # of locks in region. */

/*
 * The locker id space is divided between the transaction manager and the lock
 * manager.  Lock IDs start at 1 and go to DB_LOCK_MAXID.  Txn IDs start at
 * DB_LOCK_MAXID + 1 and go up to TXN_MAXIMUM.
 */
#define DB_LOCK_INVALIDID   0
#define DB_LOCK_MAXID       0x7fffffff

/*
 * Out of band value for a lock.  Locks contain an offset into a lock region,
 * so we use an invalid region offset to indicate an invalid or unset lock.
 */
#define LOCK_INVALID        INVALID_ROFF
#define LOCK_ISSET(lock)    ((lock).off != LOCK_INVALID)
#define LOCK_INIT(lock)     ((lock).off = LOCK_INVALID)

/*
 * Macro to identify a write lock for the purpose of counting locks
 * for the NUMWRITES option to deadlock detection.
 */
#define IS_WRITELOCK(m) \
    ((m) == DB_LOCK_WRITE || (m) == DB_LOCK_WWRITE || \
        (m) == DB_LOCK_IWRITE || (m) == DB_LOCK_IWR)

/*
 * Macros to lock/unlock the lock region as a whole. Mostly used for
 * initialization.
 */
#define LOCK_REGION_LOCK(env)                       \
    MUTEX_LOCK(env, ((DB_LOCKREGION *)              \
        (env)->lk_handle->reginfo.primary)->mtx_region)
#define LOCK_REGION_UNLOCK(env)                     \
    MUTEX_UNLOCK(env, ((DB_LOCKREGION *)                \
        (env)->lk_handle->reginfo.primary)->mtx_region)

/*
 * DB_LOCKREGION --
 *  The lock shared region.
 */

typedef struct __db_lockregion {
    db_mutex_t  mtx_region; /* Region mutex. */

    u_int32_t   need_dd;    /* flag for deadlock detector */
    u_int32_t   detect;     /* run dd on every conflict */
    db_timespec next_timeout;   /* next time to expire a lock */
    db_mutex_t  mtx_dd;     /* mutex for lock object dd list. */
    db_mutex_t  mtx_lockers;    /* mutex for locker allocation. */
    SH_TAILQ_HEAD(__dobj) dd_objs;  /* objects with waiters */
                    /* free locker header */
    SH_TAILQ_HEAD(__flocker) free_lockers;
    SH_TAILQ_HEAD(__lkrs) lockers;  /* list of lockers */

    db_timeout_t    lk_timeout; /* timeout for locks. */
    db_timeout_t    tx_timeout; /* timeout for txns. */

    u_int32_t   locker_t_size;  /* size of locker hash table */
    u_int32_t   object_t_size;  /* size of object hash table */
    u_int32_t   part_t_size;    /* number of partitions */

    roff_t      conf_off;   /* offset of conflicts array */
    roff_t      obj_off;    /* offset of object hash table */
    roff_t      part_off;   /* offset of partition array */
    roff_t      stat_off;   /* offset to object hash stats */
    roff_t      locker_off; /* offset of locker hash table */

    u_int32_t   lock_id;    /* Current lock(er) id to allocate. */
    u_int32_t   cur_maxid;  /* Current max lock(er) id. */
    u_int32_t   nlockers;   /* Current number of lockers. */
    int     nmodes;     /* Number of modes in conflict table. */
    DB_LOCK_STAT    stat;       /* stats about locking. */
} DB_LOCKREGION;

/*
 * Since we will store DBTs in shared memory, we need the equivalent of a
 * DBT that will work in shared memory.
 */
typedef struct __sh_dbt {
    u_int32_t size;         /* Byte length. */
    roff_t    off;          /* Region offset. */
} SH_DBT;

#define SH_DBT_PTR(p)   ((void *)(((u_int8_t *)(p)) + (p)->off))

/*
 * Object structures;  these live in the object hash table.
 */
typedef struct __db_lockobj {
    u_int32_t   indx;       /* Hash index of this object. */
    u_int32_t   generation; /* Generation of this object. */
    SH_DBT  lockobj;        /* Identifies object locked. */
    SH_TAILQ_ENTRY links;       /* Links for free list or hash list. */
    SH_TAILQ_ENTRY dd_links;    /* Links for dd list. */
    SH_TAILQ_HEAD(__waitl) waiters; /* List of waiting locks. */
    SH_TAILQ_HEAD(__holdl) holders; /* List of held locks. */
                    /* Declare room in the object to hold
                     * typical DB lock structures so that
                     * we do not have to allocate them from
                     * shalloc at run-time. */
    u_int8_t objdata[sizeof(struct __db_ilock)];
} DB_LOCKOBJ;

/*
 * Locker structures; these live in the locker hash table.
 */
struct __db_locker {
    u_int32_t id;           /* Locker id. */

    pid_t pid;          /* Process owning locker ID */
    db_threadid_t tid;      /* Thread owning locker ID */

    u_int32_t dd_id;        /* Deadlock detector id. */

    u_int32_t nlocks;       /* Number of locks held. */
    u_int32_t nwrites;      /* Number of write locks held. */

    roff_t  master_locker;      /* Locker of master transaction. */
    roff_t  parent_locker;      /* Parent of this child. */
    SH_LIST_HEAD(_child) child_locker;  /* List of descendant txns;
                           only used in a "master"
                           txn. */
    SH_LIST_ENTRY child_link;   /* Links transactions in the family;
                       elements of the child_locker
                       list. */
    SH_TAILQ_ENTRY links;       /* Links for free and hash list. */
    SH_TAILQ_ENTRY ulinks;      /* Links in-use list. */
    SH_LIST_HEAD(_held) heldby; /* Locks held by this locker. */
    db_timespec lk_expire;  /* When current lock expires. */
    db_timespec tx_expire;  /* When this txn expires. */
    db_timeout_t    lk_timeout; /* How long do we let locks live. */

#define DB_LOCKER_DIRTY     0x0001
#define DB_LOCKER_INABORT   0x0002
#define DB_LOCKER_TIMEOUT   0x0004
    u_int32_t flags;
};

/*
 * Map a hash index into a partition.
 */
#define LOCK_PART(reg, ndx)  (ndx % (reg)->part_t_size)

/*
 * Structure that contains information about a lock table partition.
 */
typedef struct __db_lockpart{
    db_mutex_t  mtx_part;   /* mutex for partition*/
                    /* free lock header */
    SH_TAILQ_HEAD(__flock) free_locks;
                    /* free obj header */
    SH_TAILQ_HEAD(__fobj) free_objs;
#ifdef HAVE_STATISTICS
    DB_LOCK_PSTAT   part_stat;  /* Partition stats. */
#endif
} DB_LOCKPART;

#define FREE_LOCKS(lt, part)    ((lt)->part_array[part].free_locks)
#define FREE_OBJS(lt, part) ((lt)->part_array[part].free_objs)

/*
 * DB_LOCKTAB --
 *  The primary library lock data structure (i.e., the one referenced
 * by the environment, as opposed to the internal one laid out in the region.)
 */
struct __db_locktab {
    ENV     *env;       /* Environment. */
    REGINFO      reginfo;   /* Region information. */
    u_int8_t    *conflicts; /* Pointer to conflict matrix. */
    DB_LOCKPART *part_array;    /* Beginning of partition array. */
#ifdef HAVE_STATISTICS
    DB_LOCK_HSTAT   *obj_stat;  /* Object hash stats array. */
#endif
    DB_HASHTAB  *obj_tab;   /* Beginning of object hash table. */
    DB_HASHTAB  *locker_tab;    /* Beginning of locker hash table. */
};

/*
 * Test for conflicts.
 *
 * Cast HELD and WANTED to ints, they are usually db_lockmode_t enums.
 */
#define CONFLICTS(T, R, HELD, WANTED) \
    (T)->conflicts[((int)HELD) * (R)->nmodes + ((int)WANTED)]

#define OBJ_LINKS_VALID(L) ((L)->links.stqe_prev != -1)

struct __db_lock {
    /*
     * Wait on mutex to wait on lock.  You reference your own mutex with
     * ID 0 and others reference your mutex with ID 1.
     */
    db_mutex_t  mtx_lock;

    roff_t      holder;     /* Who holds this lock. */
    u_int32_t   gen;        /* Generation count. */
    SH_TAILQ_ENTRY  links;      /* Free or holder/waiter list. */
    SH_LIST_ENTRY   locker_links;   /* List of locks held by a locker. */
    u_int32_t   refcount;   /* Reference count the lock. */
    db_lockmode_t   mode;       /* What sort of lock. */
    roff_t      obj;        /* Relative offset of object struct. */
    u_int32_t   indx;       /* Hash index of this object. */
    db_status_t status;     /* Status of this lock. */
};

/*
 * Flag values for __lock_put_internal:
 * DB_LOCK_DOALL:     Unlock all references in this lock (instead of only 1).
 * DB_LOCK_FREE:      Free the lock (used in checklocker).
 * DB_LOCK_NOPROMOTE: Don't bother running promotion when releasing locks
 *            (used by __lock_put_internal).
 * DB_LOCK_UNLINK:    Remove from the locker links (used in checklocker).
 * Make sure that these do not conflict with the interface flags because
 * we pass some of those around.
 */
#define DB_LOCK_DOALL       0x010000
#define DB_LOCK_FREE        0x040000
#define DB_LOCK_NOPROMOTE   0x080000
#define DB_LOCK_UNLINK      0x100000
#define DB_LOCK_NOWAITERS   0x400000

/*
 * Macros to get/release different types of mutexes.
 */
/*
 * Operations on lock objects must be protected by a mutex, either on their
 * partition or on the lock region.  Lock structures associated with that
 * object are protected as well.  Each partition has a free list of objects
 * and lock structures protected by that mutex.  We want to avoid getting
 * multiple mutexes, particularly in __lock_vec, when there is only a
 * single partition.  If there is only one partition, then all the calls
 * to LOCK_SYSTEM_LOCK(UNLOCK) actually acquire(release) a lock system
 * wide mutex and MUTEX_LOCK(UNLOCK)_PARTITION are no-ops.  If the number
 * of partitions is greater than one, then LOCK_SYSTEM_LOCK(UNLOCK) is a
 * no-op, and MUTEX_LOCK(UNLOCK)_PARTITION acquire a mutex on a particular
 * partition of the lock table.
 */
#define LOCK_SYSTEM_LOCK(lt, reg) do {                  \
    if ((reg)->part_t_size == 1)                    \
        MUTEX_LOCK((lt)->env, (reg)->mtx_region);       \
} while (0)
#define LOCK_SYSTEM_UNLOCK(lt, reg) do {                \
    if ((reg)->part_t_size == 1)                    \
        MUTEX_UNLOCK((lt)->env, (reg)->mtx_region);     \
} while (0)
#define MUTEX_LOCK_PARTITION(lt, reg, p) do {               \
    if ((reg)->part_t_size != 1)                    \
        MUTEX_LOCK((lt)->env, (lt)->part_array[p].mtx_part);    \
} while (0)
#define MUTEX_UNLOCK_PARTITION(lt, reg, p) do {             \
    if ((reg)->part_t_size != 1)                    \
        MUTEX_UNLOCK((lt)->env, (lt)->part_array[p].mtx_part);  \
} while (0)

#define OBJECT_LOCK(lt, reg, obj, ndx) do {             \
    ndx = __lock_ohash(obj) % (reg)->object_t_size;         \
    MUTEX_LOCK_PARTITION(lt, reg, LOCK_PART(reg, ndx));     \
} while (0)

#define OBJECT_LOCK_NDX(lt, reg, ndx)                   \
    MUTEX_LOCK_PARTITION(lt, reg, LOCK_PART(reg, ndx));

#define OBJECT_UNLOCK(lt, reg, ndx)                 \
    MUTEX_UNLOCK_PARTITION(lt, reg, LOCK_PART(reg, ndx));

/*
 * Protect the object deadlock detector queue and the locker allocation
 * and active queues
 */
#define LOCK_DD(env, region)                        \
    MUTEX_LOCK(env, (region)->mtx_dd)
#define UNLOCK_DD(env, region)                      \
    MUTEX_UNLOCK(env, (region)->mtx_dd)
#define LOCK_LOCKERS(env, region)                   \
    MUTEX_LOCK(env, (region)->mtx_lockers)
#define UNLOCK_LOCKERS(env, region)                 \
    MUTEX_UNLOCK(env, (region)->mtx_lockers)

/*
 * __lock_locker_hash --
 *  Hash function for entering lockers into the locker hash table.
 *  Since these are simply 32-bit unsigned integers at the moment,
 *  just return the locker value.
 */
#define __lock_locker_hash(locker)  (locker)
#define LOCKER_HASH(lt, reg, locker, ndx)               \
    ndx = __lock_locker_hash(locker) % (reg)->locker_t_size;

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/lock_ext.h"
#endif /* !_DB_LOCK_H_ */
