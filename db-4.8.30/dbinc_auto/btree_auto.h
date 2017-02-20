/* Do not edit: automatically built by gen_rec.awk. */

#ifndef __bam_AUTO_H
#define __bam_AUTO_H
#define DB___bam_split  62
typedef struct ___bam_split_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   left;
    DB_LSN  llsn;
    db_pgno_t   right;
    DB_LSN  rlsn;
    u_int32_t   indx;
    db_pgno_t   npgno;
    DB_LSN  nlsn;
    db_pgno_t   ppgno;
    DB_LSN  plsn;
    u_int32_t   pindx;
    DBT pg;
    DBT pentry;
    DBT rentry;
    u_int32_t   opflags;
} __bam_split_args;

#define DB___bam_split_42   62
typedef struct ___bam_split_42_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   left;
    DB_LSN  llsn;
    db_pgno_t   right;
    DB_LSN  rlsn;
    u_int32_t   indx;
    db_pgno_t   npgno;
    DB_LSN  nlsn;
    db_pgno_t   root_pgno;
    DBT pg;
    u_int32_t   opflags;
} __bam_split_42_args;

#define DB___bam_rsplit 63
typedef struct ___bam_rsplit_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DBT pgdbt;
    db_pgno_t   root_pgno;
    db_pgno_t   nrec;
    DBT rootent;
    DB_LSN  rootlsn;
} __bam_rsplit_args;

#define DB___bam_adj    55
typedef struct ___bam_adj_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    u_int32_t   indx;
    u_int32_t   indx_copy;
    u_int32_t   is_insert;
} __bam_adj_args;

#define DB___bam_cadjust    56
typedef struct ___bam_cadjust_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    u_int32_t   indx;
    int32_t adjust;
    u_int32_t   opflags;
} __bam_cadjust_args;

#define DB___bam_cdel   57
typedef struct ___bam_cdel_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    u_int32_t   indx;
} __bam_cdel_args;

#define DB___bam_repl   58
typedef struct ___bam_repl_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    u_int32_t   indx;
    u_int32_t   isdeleted;
    DBT orig;
    DBT repl;
    u_int32_t   prefix;
    u_int32_t   suffix;
} __bam_repl_args;

#define DB___bam_root   59
typedef struct ___bam_root_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   meta_pgno;
    db_pgno_t   root_pgno;
    DB_LSN  meta_lsn;
} __bam_root_args;

#define DB___bam_curadj 64
typedef struct ___bam_curadj_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_ca_mode  mode;
    db_pgno_t   from_pgno;
    db_pgno_t   to_pgno;
    db_pgno_t   left_pgno;
    u_int32_t   first_indx;
    u_int32_t   from_indx;
    u_int32_t   to_indx;
} __bam_curadj_args;

#define DB___bam_rcuradj    65
typedef struct ___bam_rcuradj_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    ca_recno_arg    mode;
    db_pgno_t   root;
    db_recno_t  recno;
    u_int32_t   order;
} __bam_rcuradj_args;

#define DB___bam_relink_43  147
typedef struct ___bam_relink_43_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    db_pgno_t   prev;
    DB_LSN  lsn_prev;
    db_pgno_t   next;
    DB_LSN  lsn_next;
} __bam_relink_43_args;

#define DB___bam_relink 147
typedef struct ___bam_relink_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    db_pgno_t   new_pgno;
    db_pgno_t   prev;
    DB_LSN  lsn_prev;
    db_pgno_t   next;
    DB_LSN  lsn_next;
} __bam_relink_args;

#define DB___bam_merge_44   148
typedef struct ___bam_merge_44_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    db_pgno_t   npgno;
    DB_LSN  nlsn;
    DBT hdr;
    DBT data;
    DBT ind;
} __bam_merge_44_args;

#define DB___bam_merge  148
typedef struct ___bam_merge_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    db_pgno_t   npgno;
    DB_LSN  nlsn;
    DBT hdr;
    DBT data;
    int32_t pg_copy;
} __bam_merge_args;

#define DB___bam_pgno   149
typedef struct ___bam_pgno_args {
    u_int32_t type;
    DB_TXN *txnp;
    DB_LSN prev_lsn;
    int32_t fileid;
    db_pgno_t   pgno;
    DB_LSN  lsn;
    u_int32_t   indx;
    db_pgno_t   opgno;
    db_pgno_t   npgno;
} __bam_pgno_args;

#endif
