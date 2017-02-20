/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2009 Oracle.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
 *  Margo Seltzer.  All rights reserved.
 */
/*
 * Copyright (c) 1990, 1993, 1994
 *  The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id$
 */

#ifndef _DB_HASH_H_
#define _DB_HASH_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* Hash internal structure. */
typedef struct hash_t {
    db_pgno_t meta_pgno;    /* Page number of the meta data page. */
    u_int32_t h_ffactor;    /* Fill factor. */
    u_int32_t h_nelem;  /* Number of elements. */
                /* Hash and compare functions. */
    u_int32_t (*h_hash) __P((DB *, const void *, u_int32_t));
    int (*h_compare) __P((DB *, const DBT *, const DBT *));
} HASH;

/* Cursor structure definitions. */
typedef struct cursor_t {
    /* struct __dbc_internal */
    __DBC_INTERNAL

    /* Hash private part */

    /* Per-thread information */
    DB_LOCK hlock;          /* Metadata page lock. */
    HMETA *hdr;         /* Pointer to meta-data page. */
    PAGE *split_buf;        /* Temporary buffer for splits. */

    /* Hash cursor information */
    db_pgno_t   bucket;     /* Bucket we are traversing. */
    db_pgno_t   lbucket;    /* Bucket for which we are locked. */
    db_indx_t   dup_off;    /* Offset within a duplicate set. */
    db_indx_t   dup_len;    /* Length of current duplicate. */
    db_indx_t   dup_tlen;   /* Total length of duplicate entry. */
    u_int32_t   seek_size;  /* Number of bytes we need for add. */
    db_pgno_t   seek_found_page;/* Page on which we can insert. */
    db_indx_t   seek_found_indx;/* Insert position for item. */
    u_int32_t   order;      /* Relative order among deleted curs. */

#define H_CONTINUE  0x0001      /* Join--search strictly fwd for data */
#define H_DELETED   0x0002      /* Cursor item is deleted. */
#define H_DUPONLY   0x0004      /* Dups only; do not change key. */
#define H_EXPAND    0x0008      /* Table expanded. */
#define H_ISDUP     0x0010      /* Cursor is within duplicate set. */
#define H_NEXT_NODUP    0x0020      /* Get next non-dup entry. */
#define H_NOMORE    0x0040      /* No more entries in bucket. */
#define H_OK        0x0080      /* Request succeeded. */
    u_int32_t   flags;
} HASH_CURSOR;

/* Test string. */
#define CHARKEY         "%$sniglet^&"

/* Overflow management */
/*
 * The spares table indicates the page number at which each doubling begins.
 * From this page number we subtract the number of buckets already allocated
 * so that we can do a simple addition to calculate the page number here.
 */
#define BS_TO_PAGE(bucket, spares)      \
    ((bucket) + (spares)[__db_log2((bucket) + 1)])
#define BUCKET_TO_PAGE(I, B)    (BS_TO_PAGE((B), (I)->hdr->spares))

/* Constraints about much data goes on a page. */

#define MINFILL     4
#define ISBIG(I, N) (((N) > ((I)->hdr->dbmeta.pagesize / MINFILL)) ? 1 : 0)

/* Shorthands for accessing structure */
#define NDX_INVALID 0xFFFF
#define BUCKET_INVALID  0xFFFFFFFF

/* On page duplicates are stored as a string of size-data-size triples. */
#define DUP_SIZE(len)   ((len) + 2 * sizeof(db_indx_t))

/* Log messages types (these are subtypes within a record type) */
#define PAIR_KEYMASK        0x1
#define PAIR_DATAMASK       0x2
#define PAIR_DUPMASK        0x4
#define PAIR_MASK       0xf
#define PAIR_ISKEYBIG(N)    (N & PAIR_KEYMASK)
#define PAIR_ISDATABIG(N)   (N & PAIR_DATAMASK)
#define PAIR_ISDATADUP(N)   (N & PAIR_DUPMASK)
#define OPCODE_OF(N)    (N & ~PAIR_MASK)

#define PUTPAIR     0x20
#define DELPAIR     0x30
#define PUTOVFL     0x40
#define DELOVFL     0x50
#define HASH_UNUSED1    0x60
#define HASH_UNUSED2    0x70
#define SPLITOLD    0x80
#define SPLITNEW    0x90
#define SORTPAGE    0x100

/* Flags to control behavior of __ham_del_pair */
#define HAM_DEL_NO_CURSOR   0x01 /* Don't do any cursor adjustment */
#define HAM_DEL_NO_RECLAIM  0x02 /* Don't reclaim empty pages */
/* Just delete onpage items (even if they are references to off-page items). */
#define HAM_DEL_IGNORE_OFFPAGE  0x04

typedef enum {
    DB_HAM_CURADJ_DEL = 1,
    DB_HAM_CURADJ_ADD = 2,
    DB_HAM_CURADJ_ADDMOD = 3,
    DB_HAM_CURADJ_DELMOD = 4
} db_ham_curadj;

typedef enum {
    DB_HAM_CHGPG = 1,
    DB_HAM_DELFIRSTPG = 2,
    DB_HAM_DELMIDPG = 3,
    DB_HAM_DELLASTPG = 4,
    DB_HAM_DUP   = 5,
    DB_HAM_SPLIT = 6
} db_ham_mode;

#if defined(__cplusplus)
}
#endif

#include "dbinc_auto/hash_auto.h"
#include "dbinc_auto/hash_ext.h"
#include "dbinc/db_am.h"
#endif /* !_DB_HASH_H_ */
