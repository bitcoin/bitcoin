/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __ham_AUTO_H
#define __ham_AUTO_H
#define DB___ham_insdel 21
typedef struct ___ham_insdel_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    int32_t fileid;
    db_pgno_t   pgno;
    u_int32_t   ndx;
    DB_LSN  pagelsn;
    DBT key;
    DBT data;
} __ham_insdel_args;

#define DB___ham_newpage    22
typedef struct ___ham_newpage_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    u_int32_t   opcode;
    int32_t fileid;
    db_pgno_t   prev_pgno;
    DB_LSN  prevlsn;
    db_pgno_t   new_pgno;
    DB_LSN  pagelsn;
    db_pgno_t   next_pgno;
    DB_LSN  nextlsn;
} __ham_newpage_args;

#define DB___ham_splitdata  24
typedef struct ___ham_splitdata_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    u_int32_t   opcode;
    db_pgno_t   pgno;
    DBT pageimage;
    DB_LSN  pagelsn;
} __ham_splitdata_args;

#define DB___ham_replace    25
typedef struct ___ham_replace_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    u_int32_t   ndx;
    DB_LSN  pagelsn;
    int32_t off;
    DBT olditem;
    DBT newitem;
    u_int32_t   makedup;
} __ham_replace_args;

#define DB___ham_copypage   28
typedef struct ___ham_copypage_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  pagelsn;
    db_pgno_t   next_pgno;
    DB_LSN  nextlsn;
    db_pgno_t   nnext_pgno;
    DB_LSN  nnextlsn;
    DBT page;
} __ham_copypage_args;

#define DB___ham_metagroup_42   29
typedef struct ___ham_metagroup_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    u_int32_t   bucket;
    db_pgno_t   mmpgno;
    DB_LSN  mmetalsn;
    db_pgno_t   mpgno;
    DB_LSN  metalsn;
    db_pgno_t   pgno;
    DB_LSN  pagelsn;
    u_int32_t   newalloc;
} __ham_metagroup_42_args;

#define DB___ham_metagroup  29
typedef struct ___ham_metagroup_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    u_int32_t   bucket;
    db_pgno_t   mmpgno;
    DB_LSN  mmetalsn;
    db_pgno_t   mpgno;
    DB_LSN  metalsn;
    db_pgno_t   pgno;
    DB_LSN  pagelsn;
    u_int32_t   newalloc;
    db_pgno_t   last_pgno;
} __ham_metagroup_args;

#define DB___ham_groupalloc_42  32
typedef struct ___ham_groupalloc_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    DB_LSN  meta_lsn;
    db_pgno_t   start_pgno;
    u_int32_t   num;
    db_pgno_t   free;
} __ham_groupalloc_42_args;

#define DB___ham_groupalloc 32
typedef struct ___ham_groupalloc_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    DB_LSN  meta_lsn;
    db_pgno_t   start_pgno;
    u_int32_t   num;
    db_pgno_t   unused;
    db_pgno_t   last_pgno;
} __ham_groupalloc_args;

#define DB___ham_curadj 33
typedef struct ___ham_curadj_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    u_int32_t   indx;
    u_int32_t   len;
    u_int32_t   dup_off;
    int add;
    int is_dup;
    u_int32_t   order;
} __ham_curadj_args;

#define DB___ham_chgpg  34
typedef struct ___ham_chgpg_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_ham_mode mode;
    db_pgno_t   old_pgno;
    db_pgno_t   new_pgno;
    u_int32_t   old_indx;
    u_int32_t   new_indx;
} __ham_chgpg_args;

#endif
