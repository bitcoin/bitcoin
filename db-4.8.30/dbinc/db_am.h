/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */
#ifndef _DB_AM_H_
#define _DB_AM_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Temporary for the patch release, define this bit here so it
 * does not renumber the other bits for DB->open.
 */
#define DB_NOERROR  0x10000000

struct __db_foreign_info; \
            typedef struct __db_foreign_info DB_FOREIGN_INFO;

/*
 * Keep track of information for foreign keys.  Used to maintain a linked list
 * of 'primary' DBs which reference this 'foreign' DB.
 */
struct __db_foreign_info {
    DB *dbp;
    u_int32_t flags;
    int (*callback) __P((DB *, const DBT *, DBT *, const DBT *, int *));

    /*
     * List entries for foreign key.
     *
     * !!!
     * Explicit representations of structures from queue.h.
     * LIST_ENTRY(__db) s_links;
     */
    struct {
        struct __db_foreign_info *le_next;
        struct __db_foreign_info **le_prev;
    } f_links;
};

/*
 * IS_ENV_AUTO_COMMIT --
 *  Auto-commit test for enviroment operations: DbEnv::{open,remove,rename}
 */
#define IS_ENV_AUTO_COMMIT(env, txn, flags)             \
    (LF_ISSET(DB_AUTO_COMMIT) || ((txn) == NULL &&          \
        F_ISSET((env)->dbenv, DB_ENV_AUTO_COMMIT) &&        \
        !LF_ISSET(DB_NO_AUTO_COMMIT)))

/*
 * IS_DB_AUTO_COMMIT --
 *  Auto-commit test for database operations.
 */
#define IS_DB_AUTO_COMMIT(dbp, txn)                 \
        ((txn) == NULL && F_ISSET((dbp), DB_AM_TXN))

/*
 * STRIP_AUTO_COMMIT --
 *  Releases after 4.3 no longer requires DB operations to specify the
 *  AUTO_COMMIT flag, but the API continues to allow it to be specified.
 */
#define STRIP_AUTO_COMMIT(f)    FLD_CLR((f), DB_AUTO_COMMIT)

/* DB recovery operation codes. */
#define DB_ADD_DUP  1
#define DB_REM_DUP  2
#define DB_ADD_BIG  3
#define DB_REM_BIG  4
#define DB_ADD_PAGE_COMPAT  5   /* Compatibility for 4.2 db_relink */
#define DB_REM_PAGE_COMPAT  6   /* Compatibility for 4.2 db_relink */
#define DB_APPEND_BIG   7

/*
 * Standard initialization and shutdown macros for all recovery functions.
 */
#define REC_INTRO(func, ip, do_cursor) do {             \
    argp = NULL;                            \
    dbc = NULL;                         \
    file_dbp = NULL;                        \
    COMPQUIET(mpf, NULL);   /* Not all recovery routines use mpf. */\
    if ((ret = func(env, &file_dbp,                 \
        (info != NULL) ? ((DB_TXNHEAD *)info)->td : NULL,       \
        dbtp->data, &argp)) != 0) {                 \
        if (ret == DB_DELETED) {                \
            ret = 0;                    \
            goto done;                  \
        }                           \
        goto out;                       \
    }                               \
    if (do_cursor) {                        \
        if ((ret =                      \
            __db_cursor(file_dbp, ip, NULL, &dbc, 0)) != 0) \
            goto out;                   \
        F_SET(dbc, DBC_RECOVER);                \
    }                               \
    mpf = file_dbp->mpf;                        \
} while (0)

#define REC_CLOSE {                         \
    int __t_ret;                            \
    if (argp != NULL)                       \
        __os_free(env, argp);                   \
    if (dbc != NULL &&                      \
        (__t_ret = __dbc_close(dbc)) != 0 && ret == 0)      \
        ret = __t_ret;                      \
    }                               \
    return (ret)

/*
 * No-op versions of the same macros.
 */
#define REC_NOOP_INTRO(func) do {                   \
    argp = NULL;                            \
    if ((ret = func(env, dbtp->data, &argp)) != 0)      \
        return (ret);                       \
} while (0)
#define REC_NOOP_CLOSE                          \
    if (argp != NULL)                       \
        __os_free(env, argp);                   \
    return (ret)

/*
 * Macro for reading pages during recovery.  In most cases we
 * want to avoid an error if the page is not found during rollback.
 */
#define REC_FGET(mpf, ip, pgno, pagep, cont)                \
    if ((ret = __memp_fget(mpf,                 \
         &(pgno), ip, NULL, 0, pagep)) != 0) {          \
        if (ret != DB_PAGE_NOTFOUND) {              \
            ret = __db_pgerr(file_dbp, pgno, ret);      \
            goto out;                   \
        } else                          \
            goto cont;                  \
    }
#define REC_DIRTY(mpf, ip, priority, pagep)             \
    if ((ret = __memp_dirty(mpf,                    \
        pagep, ip, NULL, priority, DB_MPOOL_EDIT)) != 0) {      \
        ret = __db_pgerr(file_dbp, PGNO(*(pagep)), ret);    \
        goto out;                       \
    }

/*
 * Standard debugging macro for all recovery functions.
 */
#ifdef DEBUG_RECOVER
#define REC_PRINT(func)                         \
    (void)func(env, dbtp, lsnp, op, info);
#else
#define REC_PRINT(func)
#endif

/*
 * Actions to __db_lget
 */
#define LCK_ALWAYS      1   /* Lock even for off page dup cursors */
#define LCK_COUPLE      2   /* Lock Couple */
#define LCK_COUPLE_ALWAYS   3   /* Lock Couple even in txn. */
#define LCK_DOWNGRADE       4   /* Downgrade the lock. (internal) */
#define LCK_ROLLBACK        5   /* Lock even if in rollback */

/*
 * If doing transactions we have to hold the locks associated with a data item
 * from a page for the entire transaction.  However, we don't have to hold the
 * locks associated with walking the tree.  Distinguish between the two so that
 * we don't tie up the internal pages of the tree longer than necessary.
 */
#define __LPUT(dbc, lock)                       \
    __ENV_LPUT((dbc)->env, lock)

#define __ENV_LPUT(env, lock)                       \
    (LOCK_ISSET(lock) ? __lock_put(env, &(lock)) : 0)

/*
 * __TLPUT -- transactional lock put
 *  If the lock is valid then
 *     If we are not in a transaction put the lock.
 *     Else if the cursor is doing dirty reads and this was a read then
 *      put the lock.
 *     Else if the db is supporting dirty reads and this is a write then
 *      downgrade it.
 *  Else do nothing.
 */
#define __TLPUT(dbc, lock)                      \
    (LOCK_ISSET(lock) ? __db_lput(dbc, &(lock)) : 0)

/*
 * Check whether a database is a primary (that is, has associated secondaries).
 */
#define DB_IS_PRIMARY(dbp) (LIST_FIRST(&dbp->s_secondaries) != NULL)
/*
 * A database should be required to be readonly if it's been explicitly
 * specified as such or if we're a client in a replicated environment
 * and the user did not specify DB_TXN_NOT_DURABLE.
 */
#define DB_IS_READONLY(dbp)                     \
    (F_ISSET(dbp, DB_AM_RDONLY) ||                  \
    (IS_REP_CLIENT((dbp)->env) && !F_ISSET((dbp), DB_AM_NOT_DURABLE)))

#ifdef HAVE_COMPRESSION
/*
 * Check whether a database is compressed (btree only)
 */
#define DB_IS_COMPRESSED(dbp)                       \
    (((BTREE *)(dbp)->bt_internal)->bt_compress != NULL)
#endif

/*
 * We copy the key out if there's any chance the key in the database is not
 * the same as the user-specified key.  If there is a custom comparator we
 * return a key, as the user-specified key might be a partial key, containing
 * only the unique identifier.  [#13572] [#15770]
 *
 * The test for (flags != 0) is necessary for Db.{get,pget}, but it's not
 * legal to pass a non-zero flags value to Dbc.{get,pget}.
 *
 * We need to split out the hash component, since it is possible to build
 * without hash support enabled. Which would result in a null pointer access.
 */
#ifdef HAVE_HASH
#define DB_RETURNS_A_KEY_HASH(dbp)                  \
    ((HASH *)(dbp)->h_internal)->h_compare != NULL
#else
#define DB_RETURNS_A_KEY_HASH(dbp)  0
#endif
#define DB_RETURNS_A_KEY(dbp, flags)                    \
    (((flags) != 0 && (flags) != DB_GET_BOTH &&         \
        (flags) != DB_GET_BOTH_RANGE && (flags) != DB_SET) ||   \
        ((BTREE *)(dbp)->bt_internal)->bt_compare != __bam_defcmp ||\
        DB_RETURNS_A_KEY_HASH(dbp))

/*
 * For portability, primary keys that are record numbers are stored in
 * secondaries in the same byte order as the secondary database.  As a
 * consequence, we need to swap the byte order of these keys before attempting
 * to use them for lookups in the primary.  We also need to swap user-supplied
 * primary keys that are used in secondary lookups (for example, with the
 * DB_GET_BOTH flag on a secondary get).
 */
#include "dbinc/db_swap.h"

#define SWAP_IF_NEEDED(sdbp, pkey)                  \
    do {                                \
        if (((sdbp)->s_primary->type == DB_QUEUE ||     \
            (sdbp)->s_primary->type == DB_RECNO) &&     \
            F_ISSET((sdbp), DB_AM_SWAP))            \
            P_32_SWAP((pkey)->data);            \
    } while (0)

/*
 * Cursor adjustment:
 *  Return the first DB handle in the sorted ENV list of DB
 *  handles that has a matching file ID.
 */
#define FIND_FIRST_DB_MATCH(env, dbp, tdbp) do {            \
    for ((tdbp) = (dbp);                        \
        TAILQ_PREV((tdbp), __dblist, dblistlinks) != NULL &&    \
        TAILQ_PREV((tdbp),                      \
        __dblist, dblistlinks)->adj_fileid == (dbp)->adj_fileid;\
        (tdbp) = TAILQ_PREV((tdbp), __dblist, dblistlinks))     \
        ;                           \
} while (0)

/*
 * Macros used to implement a binary search algorithm. Shared between the
 * btree and hash implementations.
 */
#define DB_BINARY_SEARCH_FOR(base, limit, nument, adjust)       \
    for (base = 0, limit = (nument) / (db_indx_t)(adjust);      \
        (limit) != 0; (limit) >>= 1)

#define DB_BINARY_SEARCH_INCR(index, base, limit, adjust)       \
    index = (base) + (((limit) >> 1) * (adjust))

#define DB_BINARY_SEARCH_SHIFT_BASE(index, base, limit, adjust) do {    \
    base = (index) + (adjust);                  \
    --(limit);                          \
} while (0)

/*
 * Sequence macros, shared between sequence.c and seq_stat.c
 */
#define SEQ_IS_OPEN(seq)    ((seq)->seq_key.data != NULL)

#define SEQ_ILLEGAL_AFTER_OPEN(seq, name)               \
    if (SEQ_IS_OPEN(seq))                       \
        return (__db_mi_open((seq)->seq_dbp->env, name, 1));

#define SEQ_ILLEGAL_BEFORE_OPEN(seq, name)              \
    if (!SEQ_IS_OPEN(seq))                      \
        return (__db_mi_open((seq)->seq_dbp->env, name, 0));

/*
 * Flags to __db_chk_meta.
 */
#define DB_CHK_META 0x01    /* Checksum the meta page. */
#define DB_CHK_NOLSN    0x02    /* Don't check the LSN. */

#if defined(__cplusplus)
}
#endif

#include "dbinc/db_dispatch.h"
#include "dbinc_auto/db_auto.h"
#include "dbinc_auto/crdel_auto.h"
#include "dbinc_auto/db_ext.h"
#endif /* !_DB_AM_H_ */
