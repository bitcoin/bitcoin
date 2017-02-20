/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_LOG_H_
#define _DB_LOG_H_

#include "dbinc/db_swap.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************
 * DBREG:
 *  The DB file register code keeps track of open files.  It's stored
 *  in the log subsystem's shared region, and so appears in the log.h
 *  header file, but is logically separate.
 *  The dbp may not be open if we are recovering the abort of a create.
 *******************************************************/
/*
 * The per-process table that maps log file-id's to DB structures.
 */
typedef struct __db_entry {
    DB  *dbp;           /* Open dbp for this file id. */
    int deleted;        /* File was not found during open. */
} DB_ENTRY;

/*
 * FNAME --
 *  File name and id.
 */
struct __fname {
    SH_TAILQ_ENTRY q;       /* File name queue. */

    pid_t     pid;          /* Process that owns this. */
    int32_t   id;           /* Logging file id. */
    int32_t   old_id;       /* Saved logging file id. */
    DBTYPE    s_type;       /* Saved DB type. */

    roff_t    fname_off;        /* File name offset. */
    roff_t    dname_off;        /* Database name offset. */
    db_pgno_t meta_pgno;        /* Page number of the meta page. */
    u_int8_t  ufid[DB_FILE_ID_LEN]; /* Unique file id. */

    u_int32_t create_txnid;     /*
                     * Txn ID of the DB create, stored so
                     * we can log it at register time.
                     */
    db_mutex_t mutex;       /* mutex from db handle. */
            /* number of txn referencing + 1 for the db handle. */
    u_int32_t txn_ref;

#define DB_FNAME_CLOSED     0x01    /* DBP was closed. */
#define DB_FNAME_DURABLE    0x02    /* File is durable. */
#define DB_FNAME_INMEM      0x04    /* File is in memory. */
#define DB_FNAME_NOTLOGGED  0x08    /* Log of close failed. */
#define DB_FNAME_RECOVER    0x10    /* File was opened by recovery code. */
#define DB_FNAME_RESTORED   0x20    /* File may be in restored txn. */
    u_int32_t flags;
};

/* File open/close register log record opcodes. */
#define DBREG_CHKPNT    1       /* Checkpoint: file name/id dump. */
#define DBREG_CLOSE 2       /* File close. */
#define DBREG_OPEN  3       /* File open. */
#define DBREG_PREOPEN   4       /* Open in mpool only. */
#define DBREG_RCLOSE    5       /* File close after recovery. */
#define DBREG_REOPEN    6       /* Open for in-memory database. */

/*******************************************************
 * LOG:
 *  The log subsystem information.
 *******************************************************/
struct __hdr;       typedef struct __hdr HDR;
struct __log;       typedef struct __log LOG;
struct __log_persist;   typedef struct __log_persist LOGP;

#define LFPREFIX    "log."      /* Log file name prefix. */
#define LFNAME      "log.%010d" /* Log file name template. */
#define LFNAME_V1   "log.%05d"  /* Log file name template, rev 1. */

#define LG_MAX_DEFAULT      (10 * MEGABYTE) /* 10 MB. */
#define LG_MAX_INMEM        (256 * 1024)    /* 256 KB. */
#define LG_BSIZE_INMEM      (1 * MEGABYTE)  /* 1 MB. */

/*
 * Allocate a few bytes under a power-of-two value.  BDB doesn't care if it's
 * a power-of-two or not, and requesting slightly under a power-of-two allows
 * stupid allocators to avoid wasting space.
 */
#define LG_BASE_REGION_SIZE (130000)    /* 128KB - 1072B */
#define LG_BSIZE_DEFAULT    (32000)     /* 32 KB - 768B */
#define LG_CURSOR_BUF_SIZE  (32000)     /* 32 KB - 768B */

/*
 * DB_LOG
 *  Per-process log structure.
 */
struct __db_log {
    /*
     * These fields need to be protected for multi-threaded support.
     */
    db_mutex_t mtx_dbreg;       /* Mutex for thread protection. */

    DB_ENTRY *dbentry;      /* Recovery file-id mapping. */
#define DB_GROW_SIZE    64
    int32_t dbentry_cnt;        /* Entries.  Grows by DB_GROW_SIZE. */

    /*
     * These fields are only accessed when the region lock is held, so
     * they do not have to be protected by the thread lock as well.
     */
    u_int32_t lfname;       /* Log file "name". */
    DB_FH    *lfhp;         /* Log file handle. */
    time_t    lf_timestamp;     /* Log file timestamp. */

    u_int8_t *bufp;         /* Region buffer. */

    /* These fields are not thread protected. */
    ENV  *env;          /* Environment */
    REGINFO   reginfo;      /* Region information. */

#define DBLOG_AUTOREMOVE    0x01    /* Autoremove log files. */
#define DBLOG_DIRECT        0x02    /* Do direct I/O on the log. */
#define DBLOG_DSYNC     0x04    /* Set OS_DSYNC on the log. */
#define DBLOG_FORCE_OPEN    0x08    /* Force the DB open even if it appears
                     * to be deleted. */
#define DBLOG_INMEMORY      0x10    /* Logging is in memory. */
#define DBLOG_OPENFILES     0x20    /* Prepared files need to be open. */
#define DBLOG_RECOVER       0x40    /* We are in recovery. */
#define DBLOG_ZERO      0x80    /* Zero fill the log. */
    u_int32_t flags;
};

/*
 * HDR --
 *  Log record header.
 */
struct __hdr {
    u_int32_t prev;         /* Previous offset. */
    u_int32_t len;          /* Current length. */
    u_int8_t  chksum[DB_MAC_KEY];   /* Current checksum. */
    u_int8_t  iv[DB_IV_BYTES];  /* IV */
    u_int32_t orig_size;        /* Original size of log record */
    /* !!! - 'size' is not written to log, must be last in hdr */
    size_t    size;         /* Size of header to use */
};

/*
 * LOG_HDR_SUM -- XOR in prev and len
 *  This helps avoids the race misreading the log while it
 * it is being updated.
 */
#define LOG_HDR_SUM(crypto, hdr, sum) do {              \
    if (crypto) {                           \
        ((u_int32_t *)sum)[0] ^= ((HDR *)hdr)->prev;        \
        ((u_int32_t *)sum)[1] ^= ((HDR *)hdr)->len;     \
    } else {                            \
        ((u_int32_t *)sum)[0] ^=                \
             ((HDR *)hdr)->prev ^ ((HDR *)hdr)->len;        \
    }                               \
} while (0)

/*
 * We use HDR internally, and then when we write out, we write out
 * prev, len, and then a 4-byte checksum if normal operation or
 * a crypto-checksum and IV and original size if running in crypto
 * mode.  We must store the original size in case we pad.  Set the
 * size when we set up the header.  We compute a DB_MAC_KEY size
 * checksum regardless, but we can safely just use the first 4 bytes.
 */
#define HDR_NORMAL_SZ   12
#define HDR_CRYPTO_SZ   12 + DB_MAC_KEY + DB_IV_BYTES

struct __log_persist {
    u_int32_t magic;        /* DB_LOGMAGIC */
    u_int32_t version;      /* DB_LOGVERSION */

    u_int32_t log_size;     /* Log file size. */
    u_int32_t notused;      /* Historically the log file mode. */
};

/* Macros to lock/unlock the log region as a whole. */
#define LOG_SYSTEM_LOCK(env)                        \
    MUTEX_LOCK(env, ((LOG *)                    \
        (env)->lg_handle->reginfo.primary)->mtx_region)
#define LOG_SYSTEM_UNLOCK(env)                      \
    MUTEX_UNLOCK(env, ((LOG *)                  \
        (env)->lg_handle->reginfo.primary)->mtx_region)

/*
 * LOG --
 *  Shared log region.  One of these is allocated in shared memory,
 *  and describes the log.
 */
struct __log {
    db_mutex_t mtx_region;      /* Region mutex. */

    db_mutex_t mtx_filelist;    /* Mutex guarding file name list. */

    LOGP    persist;        /* Persistent information. */

    SH_TAILQ_HEAD(__fq1) fq;    /* List of file names. */
    int32_t fid_max;        /* Max fid allocated. */
    roff_t  free_fid_stack;     /* Stack of free file ids. */
    u_int   free_fids;      /* Height of free fid stack. */
    u_int   free_fids_alloced;  /* N free fid slots allocated. */

    /*
     * The lsn LSN is the file offset that we're about to write and which
     * we will return to the user.
     */
    DB_LSN    lsn;          /* LSN at current file offset. */

    /*
     * The f_lsn LSN is the LSN (returned to the user) that "owns" the
     * first byte of the buffer.  If the record associated with the LSN
     * spans buffers, it may not reflect the physical file location of
     * the first byte of the buffer.
     */
    DB_LSN    f_lsn;        /* LSN of first byte in the buffer. */
    size_t    b_off;        /* Current offset in the buffer. */
    u_int32_t w_off;        /* Current write offset in the file. */
    u_int32_t len;          /* Length of the last record. */

    DB_LSN    active_lsn;       /* Oldest active LSN in the buffer. */
    size_t    a_off;        /* Offset in the buffer of first active
                       file. */

    /*
     * The s_lsn LSN is the last LSN that we know is on disk, not just
     * written, but synced.  This field is protected by the flush mutex
     * rather than by the region mutex.
     */
    db_mutex_t mtx_flush;       /* Mutex guarding flushing. */
    int    in_flush;        /* Log flush in progress. */
    DB_LSN     s_lsn;       /* LSN of the last sync. */

    DB_LOG_STAT stat;       /* Log statistics. */

    /*
     * This timestamp is updated anytime someone unlinks log
     * files.  This can happen when calling __log_vtruncate
     * or replication internal init when it unlinks log files.
     *
     * The timestamp is used so that other processes that might
     * have file handles to log files know to close/reopen them
     * so they're not potentially writing to now-removed files.
     */
    time_t     timestamp;       /* Log trunc timestamp. */

    /*
     * !!!
     * NOTE: the next group of fields are NOT protected by the log
     * region lock.  They are protected by REP->mtx_clientdb.  If you
     * need access to both, you must acquire REP->mtx_clientdb
     * before acquiring the log region lock.
     *
     * The waiting_lsn is used by the replication system.  It is the
     * first LSN that we are holding without putting in the log, because
     * we received one or more log records out of order.  Associated with
     * the waiting_lsn is the number of log records that we still have to
     * receive before we decide that we should request it again.
     *
     * The max_wait_lsn is used to control retransmission in the face
     * of dropped messages.  If we are requesting all records from the
     * current gap (i.e., chunk of the log that we are missing), then
     * the max_wait_lsn contains the first LSN that we are known to have
     * in the __db.rep.db.  If we requested only a single record, then
     * the max_wait_lsn has the LSN of that record we requested.
     */
    /* BEGIN fields protected by rep->mtx_clientdb. */
    DB_LSN    waiting_lsn;      /* First log record after a gap. */
    DB_LSN    verify_lsn;       /* LSN we are waiting to verify. */
    DB_LSN    prev_ckp;     /* LSN of ckp preceeding verify_lsn. */
    DB_LSN    max_wait_lsn;     /* Maximum LSN requested. */
    DB_LSN    max_perm_lsn;     /* Maximum PERMANENT LSN processed. */
    db_timespec max_lease_ts;   /* Maximum Lease timestamp seen. */
    db_timespec wait_ts;        /* Time to wait before requesting. */
    db_timespec rcvd_ts;        /* Initial received time to wait. */
    db_timespec last_ts;        /* Last time of insert in temp db. */
    /*
     * The ready_lsn is also used by the replication system.  It is the
     * next LSN we expect to receive.  It's normally equal to "lsn",
     * except at the beginning of a log file, at which point it's set
     * to the LSN of the first record of the new file (after the
     * header), rather than to 0.
     */
    DB_LSN    ready_lsn;
    /*
     * The bulk_buf is used by replication for bulk transfer.  While this
     * is protected by REP->mtx_clientdb, this doesn't contend with the
     * above fields because the above are used by clients and the bulk
     * fields below are used by a master.
     */
    roff_t    bulk_buf;     /* Bulk transfer buffer in region. */
    uintptr_t bulk_off;     /* Current offset into bulk buffer. */
    u_int32_t bulk_len;     /* Length of buffer. */
    u_int32_t bulk_flags;       /* Bulk buffer flags. */
    /* END fields protected by rep->mtx_clientdb. */

    /*
     * During initialization, the log system walks forward through the
     * last log file to find its end.  If it runs into a checkpoint
     * while it's doing so, it caches it here so that the transaction
     * system doesn't need to walk through the file again on its
     * initialization.
     */
    DB_LSN  cached_ckp_lsn;

    u_int32_t regionmax;        /* Configured size of the region. */

    roff_t    buffer_off;       /* Log buffer offset in the region. */
    u_int32_t buffer_size;      /* Log buffer size. */

    u_int32_t log_size;     /* Log file's size. */
    u_int32_t log_nsize;        /* Next log file's size. */

    int   filemode;     /* Log file permissions mode. */

    /*
     * DB_LOG_AUTOREMOVE and DB_LOG_INMEMORY: not protected by a mutex,
     * all we care about is if they're zero or non-zero.
     */
    int   db_log_autoremove;
    int   db_log_inmemory;

    u_int32_t ncommit;      /* Number of txns waiting to commit. */
    DB_LSN    t_lsn;        /* LSN of first commit */
    SH_TAILQ_HEAD(__commit) commits;/* list of txns waiting to commit. */
    SH_TAILQ_HEAD(__free) free_commits;/* free list of commit structs. */

    /*
     * In-memory logs maintain a list of the start positions of all log
     * files currently active in the in-memory buffer.  This is to make the
     * lookup from LSN to log buffer offset efficient.
     */
    SH_TAILQ_HEAD(__logfile) logfiles;
    SH_TAILQ_HEAD(__free_logfile) free_logfiles;
};

/*
 * __db_commit structure --
 *  One of these is allocated for each transaction waiting to commit.
 */
struct __db_commit {
    db_mutex_t  mtx_txnwait;    /* Mutex for txn to wait on. */
    DB_LSN      lsn;        /* LSN of commit record. */
    SH_TAILQ_ENTRY  links;      /* Either on free or waiting list. */

#define DB_COMMIT_FLUSH     0x0001  /* Flush the log when you wake up. */
    u_int32_t   flags;
};

/*
 * Check for the proper progression of Log Sequence Numbers.
 * If we are rolling forward the LSN on the page must be greater
 * than or equal to the previous LSN in log record.
 * We ignore NOT LOGGED LSNs.  The user did an unlogged update.
 * We should eventually see a log record that matches and continue
 * forward.
 * A ZERO LSN implies a page that was allocated prior to the recovery
 * start point and then truncated later in the log.  An allocation of a
 * page after this page will extend the file, leaving a hole.  We want to
 * ignore this page until it is truncated again.
 *
 */

#define CHECK_LSN(e, redo, cmp, lsn, prev)              \
    if (DB_REDO(redo) && (cmp) < 0 &&               \
        ((!IS_NOT_LOGGED_LSN(*(lsn)) && !IS_ZERO_LSN(*(lsn))) ||    \
        IS_REP_CLIENT(e))) {                    \
        ret = __db_check_lsn(e, lsn, prev);         \
        goto out;                       \
    }
#define CHECK_ABORT(e, redo, cmp, lsn, prev)                \
    if (redo == DB_TXN_ABORT && (cmp) != 0 &&           \
        ((!IS_NOT_LOGGED_LSN(*(lsn)) && !IS_ZERO_LSN(*(lsn))) ||    \
        IS_REP_CLIENT(e))) {                    \
        ret = __db_check_lsn(e, lsn, prev);         \
        goto out;                       \
    }

/*
 * Helper for in-memory logs -- check whether an offset is in range
 * in a ring buffer (inclusive of start, exclusive of end).
 */
struct __db_filestart {
    u_int32_t   file;
    size_t      b_off;

    SH_TAILQ_ENTRY  links;      /* Either on free or waiting list. */
};

#define RINGBUF_LEN(lp, start, end)                 \
    ((start) < (end) ?                      \
        (end) - (start) : (lp)->buffer_size - ((start) - (end)))

/*
 * Internal macro to set pointer to the begin_lsn for generated
 * logging routines.  If begin_lsn is already set then do nothing.
 * Return a pointer to the last lsn too.
 */
#undef DB_SET_TXN_LSNP
#define DB_SET_TXN_LSNP(txn, blsnp, llsnp) do {             \
    DB_LSN *__lsnp;                         \
    TXN_DETAIL *__td;                       \
    __td = (txn)->td;                       \
    *(llsnp) = &__td->last_lsn;                 \
    while (__td->parent != INVALID_ROFF)                \
        __td = R_ADDR(&(txn)->mgrp->reginfo, __td->parent); \
    __lsnp = &__td->begin_lsn;                  \
    if (IS_ZERO_LSN(*__lsnp))                   \
        *(blsnp) = __lsnp;                  \
} while (0)

/*
 * These are used in __log_backup to determine which LSN in the
 * checkpoint record to compare and return.
 */
#define CKPLSN_CMP  0
#define LASTCKP_CMP 1

/*
 * Status codes indicating the validity of a log file examined by
 * __log_valid().
 */
typedef enum {
    DB_LV_INCOMPLETE,
    DB_LV_NONEXISTENT,
    DB_LV_NORMAL,
    DB_LV_OLD_READABLE,
    DB_LV_OLD_UNREADABLE
} logfile_validity;

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/dbreg_auto.h"
#include "dbinc_auto/dbreg_ext.h"
#include "dbinc_auto/log_ext.h"
#endif /* !_DB_LOG_H_ */
