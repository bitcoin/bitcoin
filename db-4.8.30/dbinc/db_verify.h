/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2009 Oracle.  All rights reserved.
 *
 * $Id$
 */

#ifndef _DB_VERIFY_H_
#define _DB_VERIFY_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Structures and macros for the storage and retrieval of all information
 * needed for inter-page verification of a database.
 */

/*
 * EPRINT is the macro for error printing.  Takes as an arg the arg set
 * for DB->err.
 */
#define EPRINT(x) do {                          \
    if (!LF_ISSET(DB_SALVAGE))                  \
        __db_errx x;                        \
} while (0)

/* Complain about a totally zeroed page where we don't expect one. */
#define ZEROPG_ERR_PRINT(dbenv, pgno, str) do {             \
    EPRINT(((dbenv), "Page %lu: %s is of inappropriate type %lu",   \
        (u_long)(pgno), str, (u_long)P_INVALID));           \
    EPRINT(((dbenv), "Page %lu: totally zeroed page",       \
        (u_long)(pgno)));                       \
} while (0)

/*
 * Note that 0 is, in general, a valid pgno, despite equaling PGNO_INVALID;
 * we have to test it separately where it's not appropriate.
 */
#define IS_VALID_PGNO(x)    ((x) <= vdp->last_pgno)

/*
 * VRFY_DBINFO is the fundamental structure;  it either represents the database
 * of subdatabases, or the sole database if there are no subdatabases.
 */
struct __vrfy_dbinfo {
    DB_THREAD_INFO *thread_info;
    /* Info about this database in particular. */
    DBTYPE      type;

    /* List of subdatabase meta pages, if any. */
    LIST_HEAD(__subdbs, __vrfy_childinfo) subdbs;

    /* File-global info--stores VRFY_PAGEINFOs for each page. */
    DB *pgdbp;

    /* Child database--stores VRFY_CHILDINFOs of each page. */
    DB *cdbp;

    /* Page info structures currently in use. */
    LIST_HEAD(__activepips, __vrfy_pageinfo) activepips;

    /*
     * DB we use to keep track of which pages are linked somehow
     * during verification.  0 is the default, "unseen";  1 is seen.
     */
    DB *pgset;

    /*
     * This is a database we use during salvaging to keep track of which
     * overflow and dup pages we need to come back to at the end and print
     * with key "UNKNOWN".  Pages which print with a good key get set
     * to SALVAGE_IGNORE;  others get set, as appropriate, to SALVAGE_LDUP,
     * SALVAGE_LRECNODUP, SALVAGE_OVERFLOW for normal db overflow pages,
     * and SALVAGE_BTREE, SALVAGE_LRECNO, and SALVAGE_HASH for subdb
     * pages.
     */
#define SALVAGE_INVALID     0
#define SALVAGE_IGNORE      1
#define SALVAGE_LDUP        2
#define SALVAGE_IBTREE      3
#define SALVAGE_OVERFLOW    4
#define SALVAGE_LBTREE      5
#define SALVAGE_HASH        6
#define SALVAGE_LRECNO      7
#define SALVAGE_LRECNODUP   8
    DB *salvage_pages;

    db_pgno_t   last_pgno;
    db_pgno_t   meta_last_pgno;
    db_pgno_t   pgs_remaining;  /* For dbp->db_feedback(). */

    /*
     * These are used during __bam_vrfy_subtree to keep track, while
     * walking up and down the Btree structure, of the prev- and next-page
     * chain of leaf pages and verify that it's intact.  Also, make sure
     * that this chain contains pages of only one type.
     */
    db_pgno_t   prev_pgno;
    db_pgno_t   next_pgno;
    u_int8_t    leaf_type;

    /* Queue needs these to verify data pages in the first pass. */
    u_int32_t   re_pad;     /* Record pad character. */
    u_int32_t   re_len;     /* Record length. */
    u_int32_t   rec_page;
    u_int32_t   page_ext;
    u_int32_t       first_recno;
    u_int32_t       last_recno;
    int     nextents;
    db_pgno_t   *extents;

#define SALVAGE_PRINTABLE   0x01    /* Output printable chars literally. */
#define SALVAGE_PRINTHEADER 0x02    /* Print the unknown-key header. */
#define SALVAGE_PRINTFOOTER 0x04    /* Print the unknown-key footer. */
#define SALVAGE_HASSUBDBS   0x08    /* There are subdatabases to salvage. */
#define VRFY_LEAFCHAIN_BROKEN   0x10    /* Lost one or more Btree leaf pgs. */
#define VRFY_QMETA_SET      0x20    /* We've seen a QUEUE meta page and
                       set things up for it. */
    u_int32_t   flags;
}; /* VRFY_DBINFO */

/*
 * The amount of state information we need per-page is small enough that
 * it's not worth the trouble to define separate structures for each
 * possible type of page, and since we're doing verification with these we
 * have to be open to the possibility that page N will be of a completely
 * unexpected type anyway.  So we define one structure here with all the
 * info we need for inter-page verification.
 */
struct __vrfy_pageinfo {
    u_int8_t    type;
    u_int8_t    bt_level;
    u_int8_t    unused1;
    u_int8_t    unused2;
    db_pgno_t   pgno;
    db_pgno_t   prev_pgno;
    db_pgno_t   next_pgno;

    /* meta pages */
    db_pgno_t   root;
    db_pgno_t   free;       /* Free list head. */

    db_indx_t   entries;    /* Actual number of entries. */
    u_int16_t   unused;
    db_recno_t  rec_cnt;    /* Record count. */
    u_int32_t   re_pad;     /* Record pad character. */
    u_int32_t   re_len;     /* Record length. */
    u_int32_t   bt_minkey;
    u_int32_t   h_ffactor;
    u_int32_t   h_nelem;

    /* overflow pages */
    /*
     * Note that refcount is the refcount for an overflow page; pi_refcount
     * is this structure's own refcount!
     */
    u_int32_t   refcount;
    u_int32_t   olen;

#define VRFY_DUPS_UNSORTED  0x0001  /* Have to flag the negative! */
#define VRFY_HAS_CHKSUM     0x0002
#define VRFY_HAS_DUPS       0x0004
#define VRFY_HAS_DUPSORT    0x0008  /* Has the flag set. */
#define VRFY_HAS_PART_RANGE 0x0010  /* Has the flag set. */
#define VRFY_HAS_PART_CALLBACK  0x0020  /* Has the flag set. */
#define VRFY_HAS_RECNUMS    0x0040
#define VRFY_HAS_SUBDBS     0x0080
#define VRFY_INCOMPLETE     0x0100  /* Meta or item order checks incomp. */
#define VRFY_IS_ALLZEROES   0x0200  /* Hash page we haven't touched? */
#define VRFY_IS_FIXEDLEN    0x0400
#define VRFY_IS_RECNO       0x0800
#define VRFY_IS_RRECNO      0x1000
#define VRFY_OVFL_LEAFSEEN  0x2000
#define VRFY_HAS_COMPRESS   0x4000
    u_int32_t   flags;

    LIST_ENTRY(__vrfy_pageinfo) links;
    u_int32_t   pi_refcount;
}; /* VRFY_PAGEINFO */

struct __vrfy_childinfo {
    /* The following fields are set by the caller of __db_vrfy_childput. */
    db_pgno_t   pgno;

#define V_DUPLICATE 1       /* off-page dup metadata */
#define V_OVERFLOW  2       /* overflow page */
#define V_RECNO     3       /* btree internal or leaf page */
    u_int32_t   type;
    db_recno_t  nrecs;      /* record count on a btree subtree */
    u_int32_t   tlen;       /* ovfl. item total size */

    /* The following field is maintained by __db_vrfy_childput. */
    u_int32_t   refcnt;     /* # of times parent points to child. */

    LIST_ENTRY(__vrfy_childinfo) links;
}; /* VRFY_CHILDINFO */

#if defined(__cplusplus)
}
#endif
#endif /* !_DB_VERIFY_H_ */
